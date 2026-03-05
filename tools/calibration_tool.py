#!/usr/bin/env python3
"""
Factory Calibration Tool for GlucoSense CGM-3000

Programs device-specific calibration parameters (sensitivity, offset,
temperature coefficient) into the transmitter's protected flash memory
during manufacturing.

Usage:
    python calibration_tool.py --port /dev/ttyACM0 --sensitivity 0.12 \
        --offset 1.5 --temp-coeff 0.002

The tool communicates with the transmitter via a UART debug interface
that is only accessible during manufacturing (disabled in production firmware).

Implements manufacturing support for: SWR-020 (factory calibration storage)
"""

import argparse
import struct
import sys
import time
from dataclasses import dataclass
from typing import Optional

# CRC-32 (IEEE 802.3) matching the firmware implementation
import binascii


@dataclass
class CalibrationParams:
    """Mirrors the firmware calibration_params_t structure."""
    sensitivity: float      # nA per mg/dL
    offset: float           # Baseline current offset (nA)
    temp_coefficient: float # Temperature correction coefficient
    timestamp: int          # Epoch seconds when calibration was performed

    # Valid ranges per device specifications
    SENSITIVITY_MIN = 0.05
    SENSITIVITY_MAX = 0.30
    OFFSET_MIN = 0.5
    OFFSET_MAX = 5.0
    TEMP_COEFF_MIN = 0.0005
    TEMP_COEFF_MAX = 0.01

    def validate(self) -> list[str]:
        """Validate calibration parameters against manufacturing limits."""
        errors = []
        if not (self.SENSITIVITY_MIN <= self.sensitivity <= self.SENSITIVITY_MAX):
            errors.append(
                f"Sensitivity {self.sensitivity} out of range "
                f"[{self.SENSITIVITY_MIN}, {self.SENSITIVITY_MAX}]"
            )
        if not (self.OFFSET_MIN <= self.offset <= self.OFFSET_MAX):
            errors.append(
                f"Offset {self.offset} out of range "
                f"[{self.OFFSET_MIN}, {self.OFFSET_MAX}]"
            )
        if not (self.TEMP_COEFF_MIN <= self.temp_coefficient <= self.TEMP_COEFF_MAX):
            errors.append(
                f"Temp coefficient {self.temp_coefficient} out of range "
                f"[{self.TEMP_COEFF_MIN}, {self.TEMP_COEFF_MAX}]"
            )
        return errors

    def serialize(self) -> bytes:
        """Pack parameters into binary format matching firmware struct layout."""
        # struct calibration_params_t: float, float, float, uint32_t, uint32_t
        data = struct.pack('<fffI', self.sensitivity, self.offset,
                           self.temp_coefficient, self.timestamp)
        crc = binascii.crc32(data) & 0xFFFFFFFF
        return data + struct.pack('<I', crc)


class TransmitterInterface:
    """UART interface to the transmitter's manufacturing debug port."""

    BAUD_RATE = 115200
    CMD_WRITE_CAL = 0x01
    CMD_READ_CAL = 0x02
    CMD_VERIFY_CAL = 0x03
    CMD_READ_SENSOR = 0x04
    RESPONSE_OK = 0x00
    RESPONSE_ERROR = 0xFF

    def __init__(self, port: str):
        self.port = port
        self._serial = None

    def connect(self) -> bool:
        """Open serial connection to the transmitter."""
        try:
            import serial
            self._serial = serial.Serial(
                self.port, self.BAUD_RATE, timeout=5
            )
            time.sleep(0.5)  # Wait for device to initialize
            return True
        except Exception as e:
            print(f"Error connecting to {self.port}: {e}")
            return False

    def disconnect(self):
        """Close the serial connection."""
        if self._serial and self._serial.is_open:
            self._serial.close()

    def write_calibration(self, params: CalibrationParams) -> bool:
        """Write calibration parameters to the transmitter's flash."""
        if not self._serial:
            return False

        payload = params.serialize()
        packet = bytes([self.CMD_WRITE_CAL, len(payload)]) + payload

        self._serial.write(packet)
        response = self._serial.read(1)

        if not response or response[0] != self.RESPONSE_OK:
            print("Error: Device rejected calibration write")
            return False

        return True

    def verify_calibration(self, expected: CalibrationParams) -> bool:
        """Read back calibration and verify against expected values."""
        if not self._serial:
            return False

        self._serial.write(bytes([self.CMD_READ_CAL, 0]))
        # Expected response: status byte + calibration_params_t
        response = self._serial.read(1 + 20)  # 1 status + 5 fields

        if not response or response[0] != self.RESPONSE_OK:
            print("Error: Could not read back calibration")
            return False

        # Unpack and compare
        data = response[1:]
        sensitivity, offset, temp_coeff, timestamp, crc = struct.unpack(
            '<fffII', data
        )

        tolerance = 0.001
        if (abs(sensitivity - expected.sensitivity) > tolerance or
                abs(offset - expected.offset) > tolerance or
                abs(temp_coeff - expected.temp_coefficient) > tolerance):
            print("Error: Readback values do not match written values")
            return False

        # Verify CRC
        expected_crc = binascii.crc32(data[:16]) & 0xFFFFFFFF
        if crc != expected_crc:
            print(f"Error: CRC mismatch (got {crc:#x}, expected {expected_crc:#x})")
            return False

        return True

    def read_sensor_current(self) -> Optional[float]:
        """Read the current sensor output for calibration verification."""
        if not self._serial:
            return None

        self._serial.write(bytes([self.CMD_READ_SENSOR, 0]))
        response = self._serial.read(5)  # status + float

        if not response or response[0] != self.RESPONSE_OK:
            return None

        current_na = struct.unpack('<f', response[1:5])[0]
        return current_na


def run_calibration(port: str, sensitivity: float, offset: float,
                    temp_coeff: float) -> bool:
    """Execute the full factory calibration sequence."""

    params = CalibrationParams(
        sensitivity=sensitivity,
        offset=offset,
        temp_coefficient=temp_coeff,
        timestamp=int(time.time())
    )

    # Step 1: Validate parameters
    errors = params.validate()
    if errors:
        print("Calibration parameter validation failed:")
        for error in errors:
            print(f"  - {error}")
        return False

    print(f"Calibration parameters:")
    print(f"  Sensitivity:  {params.sensitivity:.4f} nA/(mg/dL)")
    print(f"  Offset:       {params.offset:.4f} nA")
    print(f"  Temp Coeff:   {params.temp_coefficient:.6f}")
    print()

    # Step 2: Connect to transmitter
    interface = TransmitterInterface(port)
    if not interface.connect():
        return False

    try:
        # Step 3: Write calibration
        print("Writing calibration to device flash...")
        if not interface.write_calibration(params):
            return False

        # Step 4: Verify write (read back and compare)
        print("Verifying calibration readback...")
        if not interface.verify_calibration(params):
            return False

        # Step 5: Read sensor to verify basic operation
        print("Reading sensor current for verification...")
        current = interface.read_sensor_current()
        if current is not None:
            print(f"  Sensor current: {current:.2f} nA")
            estimated_glucose = (current - offset) / sensitivity
            print(f"  Estimated glucose: {estimated_glucose:.0f} mg/dL")
        else:
            print("  Warning: Could not read sensor (may not be inserted)")

        print()
        print("Calibration completed successfully.")
        return True

    finally:
        interface.disconnect()


def main():
    parser = argparse.ArgumentParser(
        description='GlucoSense CGM-3000 Factory Calibration Tool'
    )
    parser.add_argument('--port', required=True,
                        help='Serial port (e.g., /dev/ttyACM0, COM3)')
    parser.add_argument('--sensitivity', type=float, required=True,
                        help='Sensor sensitivity in nA/(mg/dL)')
    parser.add_argument('--offset', type=float, required=True,
                        help='Baseline current offset in nA')
    parser.add_argument('--temp-coeff', type=float, default=0.002,
                        help='Temperature coefficient (default: 0.002)')

    args = parser.parse_args()

    success = run_calibration(
        port=args.port,
        sensitivity=args.sensitivity,
        offset=args.offset,
        temp_coeff=args.temp_coeff
    )

    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
