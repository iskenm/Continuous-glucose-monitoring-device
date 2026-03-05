# Software Verification Plan

Document ID: SVP-CGM3000-001 | Rev 1.1

## Verification Methods

- **Unit Test**: Automated tests executed in CI pipeline via Unity test framework
- **Integration Test**: Tests executed on target hardware (nRF52840 DK) with simulated sensor input
- **Inspection**: Manual review of output/behavior against specification

## Verification Matrix

| Test ID | Requirement | Method | Status |
|---------|------------|--------|--------|
| SWR-VER-001 | SWR-001 (ADC Sampling) | Unit Test - `test_sensor_adc_sampling` | Pass |
| SWR-VER-002 | SWR-002 (Sensor Current Range) | Unit Test - `test_sensor_current_range` | Pass |
| SWR-VER-003 | SWR-003 (Sensor Fault Detection) | Unit Test - `test_sensor_fault_detection` | Pass |
| SWR-VER-010 | SWR-010 (Noise Filtering) | Unit Test - `test_butterworth_filter` | Pass |
| SWR-VER-011 | SWR-011 (Outlier Rejection) | Unit Test - `test_outlier_rejection` | Pass |
| SWR-VER-012 | SWR-012 (Glucose Conversion) | Unit Test - `test_glucose_conversion` | Pass |
| SWR-VER-013 | SWR-013 (Rate of Change) | Unit Test - `test_rate_of_change` | Pass |
| SWR-VER-014 | SWR-014 (Range Clamping) | Unit Test - `test_range_clamping` | Pass |
| SWR-VER-020 | SWR-020 (Factory Calibration) | Unit Test - `test_factory_cal_storage` | Pass |
| SWR-VER-021 | SWR-021 (In-Vivo Calibration) | Unit Test - `test_invivo_calibration` | Pass |
| SWR-VER-022 | SWR-022 (Calibration Validity) | Unit Test - `test_calibration_validity` | Pass |
| SWR-VER-030 | SWR-030 (Low Glucose Alert) | Unit Test - `test_low_glucose_alert` | Pass |
| SWR-VER-031 | SWR-031 (High Glucose Alert) | Unit Test - `test_high_glucose_alert` | Pass |
| SWR-VER-032 | SWR-032 (Rapid Fall Alert) | Unit Test - `test_rapid_fall_alert` | Pass |
| SWR-VER-033 | SWR-033 (Alert Prioritization) | Unit Test - `test_alert_priority` | Pass |
| SWR-VER-050 | SWR-050 (Flash Storage) | Unit Test - `test_flash_circular_buffer` | Pass |
| SWR-VER-061 | SWR-061 (Battery Monitoring) | Unit Test - `test_battery_monitoring` | Pass |
