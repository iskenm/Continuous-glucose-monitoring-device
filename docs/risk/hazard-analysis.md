# Hazard Analysis

Document ID: HA-CGM3000-001 | Rev 1.4

| ID | Hazard | Cause | Severity | Probability | Risk Level | Mitigation (Risk Control) | Residual Risk |
|----|--------|-------|----------|-------------|------------|---------------------------|---------------|
| HAZ-001 | Falsely low glucose reading | Calibration drift, sensor degradation | 5 - Critical | 3 - Occasional | High | RC-001: Range clamping; RC-002: MARD validation; RC-009: Calibration validity check | Medium |
| HAZ-002 | Falsely high glucose reading | Interference substance (acetaminophen), sensor fault | 4 - Serious | 3 - Occasional | High | RC-001: Range clamping; RC-008: Sensor fault detection | Medium |
| HAZ-003 | Missed hypoglycemia alert | Signal processing delay, BLE disconnection | 5 - Critical | 2 - Remote | High | RC-003: Consecutive reading threshold; RC-006: Signal loss alert | Low |
| HAZ-004 | Missed hyperglycemia alert | Alert snooze misconfiguration, software defect | 3 - Moderate | 2 - Remote | Medium | RC-004: High glucose alert with 3-reading confirmation | Low |
| HAZ-005 | Excessive insulin dose from false high reading | Sensor fault not detected, calibration error | 5 - Critical | 2 - Remote | High | RC-008: Sensor fault detection; Labeling: "Confirm with fingerstick before treatment" | Low |
| HAZ-006 | Loss of monitoring during sleep | Battery depletion, BLE disconnect | 4 - Serious | 3 - Occasional | High | RC-007: Low battery alert 24h before depletion; RC-006: Signal loss alert | Medium |
| HAZ-007 | Data breach of glucose readings | BLE interception, weak encryption | 2 - Minor | 2 - Remote | Low | RC-010: AES-128-CCM encryption; BLE Secure Connections pairing | Low |
| HAZ-008 | Sensor-site infection | N/A (not software-related) | 3 - Moderate | 2 - Remote | Medium | Labeling: Insertion site care instructions | Low |
| HAZ-009 | Allergic reaction to adhesive | N/A (not software-related) | 3 - Moderate | 2 - Remote | Medium | Labeling: Material disclosure; biocompatibility testing | Low |
| HAZ-010 | Device reset during active monitoring | Watchdog timeout, firmware crash | 4 - Serious | 2 - Remote | Medium | RC-011: Watchdog recovery with state restore from flash | Low |

## Risk Control Measures

| ID | Description | Implementation | Verification |
|----|-------------|----------------|-------------|
| RC-001 | Glucose range clamping to [40, 400] mg/dL | `signal_processing.c:clamp_glucose_value()` | SWR-VER-014 |
| RC-002 | MARD accuracy validation via calibration | `calibration.c:apply_calibration()` | SWR-VER-012 |
| RC-003 | Low glucose alert requires 2 consecutive readings below threshold | `alert_manager.c:evaluate_low_glucose()` | SWR-VER-030 |
| RC-004 | High glucose alert requires 3 consecutive readings above threshold | `alert_manager.c:evaluate_high_glucose()` | SWR-VER-031 |
| RC-005 | Rapid fall detection with 3-reading confirmation | `alert_manager.c:evaluate_rate_alerts()` | SWR-VER-032 |
| RC-006 | Signal loss alert after 20 minutes without valid reading | `alert_manager.c:evaluate_signal_loss()` | SWR-VER-014 |
| RC-007 | Low battery alert at 24h remaining | `power_manager.c:check_battery_level()` | SWR-VER-061 |
| RC-008 | Sensor fault detection (open/short/noise) | `glucose_sensor.c:detect_sensor_fault()` | SWR-VER-003 |
| RC-009 | Calibration reference value rejection if >40% deviation | `calibration.c:validate_reference()` | SWR-VER-022 |
| RC-010 | AES-128-CCM encryption on BLE | `ble_manager.c:configure_security()` | SWR-VER-042 |
| RC-011 | Watchdog with flash state recovery | `main.c:watchdog_recovery()` | SWR-VER-070 |
