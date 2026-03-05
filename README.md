# GlucoSense CGM-3000 Continuous Glucose Monitoring System

## Overview

The GlucoSense CGM-3000 is a Class II medical device providing continuous interstitial glucose monitoring for patients with Type 1 and Type 2 diabetes. The system consists of a subcutaneous electrochemical sensor, a wearable transmitter, and a companion mobile application.

## Intended Use

The GlucoSense CGM-3000 is intended for use by patients (age 18+) with diabetes mellitus for continuous monitoring of glucose levels in the interstitial fluid. The device provides real-time glucose readings, trend information, and configurable alerts for hypo/hyperglycemic events.

**This device is not intended to replace blood glucose monitoring for treatment decisions. All treatment decisions should be confirmed with a fingerstick blood glucose test.**

## Regulatory Classification

- **FDA Classification**: Class II Medical Device (21 CFR 862.1355)
- **EU Classification**: Class IIb (MDR 2017/745, Rule 11)
- **Software Safety Class**: IEC 62304 Class C
- **Risk Management**: ISO 14971:2019

## System Architecture

```
+------------------+        BLE 5.0        +------------------+
|   Sensor +       | <------------------> |   Mobile App     |
|   Transmitter    |                       |   (iOS/Android)  |
|   (Firmware)     |                       +--------+---------+
+------------------+                                |
                                                    | HTTPS/TLS 1.3
                                                    v
                                           +--------+---------+
                                           |   Cloud Backend  |
                                           |   (Data Storage) |
                                           +------------------+
```

## Repository Structure

```
├── docs/                    # Design control documentation
│   ├── requirements/        # System and software requirements
│   ├── architecture/        # Software architecture documents
│   ├── risk/                # Risk management artifacts
│   ├── verification/        # Verification and validation plans
│   └── soup/                # SOUP (Software of Unknown Provenance) list
├── firmware/                # Embedded firmware (ARM Cortex-M4)
│   ├── src/                 # Source code
│   │   ├── sensor/          # Glucose sensor interface
│   │   ├── signal/          # Signal processing pipeline
│   │   ├── ble/             # Bluetooth Low Energy stack
│   │   ├── storage/         # Flash memory management
│   │   ├── power/           # Power management
│   │   ├── alert/           # Alert and alarm system
│   │   └── config/          # Device configuration
│   ├── include/             # Shared header files
│   └── tests/               # Unit tests
├── tools/                   # Host-side utilities
│   ├── calibration_tool.py  # Factory calibration utility
│   └── data_analyzer.py     # Post-market data analysis
└── .github/workflows/       # CI/CD pipeline
```

## Building

### Prerequisites

- ARM GCC Toolchain (arm-none-eabi-gcc 12.x)
- CMake 3.20+
- Nordic nRF5 SDK 17.1.0
- Python 3.10+ (for host tools)

### Firmware Build

```bash
cd firmware
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../arm-toolchain.cmake ..
make -j$(nproc)
```

### Running Tests

```bash
cd firmware/build
cmake -DBUILD_TESTS=ON ..
make test
```

## Sensor Specifications

| Parameter              | Value                    |
|------------------------|--------------------------|
| Measurement Range      | 40 - 400 mg/dL          |
| Resolution             | 1 mg/dL                 |
| Accuracy (MARD)        | < 10%                   |
| Sample Rate            | 1 reading / 5 minutes   |
| Sensor Lifetime        | 14 days                 |
| Warm-up Period         | 1 hour                  |
| Operating Temperature  | 10°C - 45°C             |
| Storage Temperature    | 2°C - 30°C              |
| Wireless               | Bluetooth Low Energy 5.0|
| Battery Life           | 14 days (CR2032)        |

## Quality Management

This project follows:
- **IEC 62304:2015** - Medical device software lifecycle
- **ISO 14971:2019** - Risk management for medical devices
- **IEC 62366-1:2015** - Usability engineering
- **ISO 13485:2016** - Quality management systems

## License

Proprietary - GlucoSense Medical Devices, Inc. All rights reserved.
