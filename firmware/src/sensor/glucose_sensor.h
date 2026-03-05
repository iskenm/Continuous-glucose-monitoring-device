/**
 * @file glucose_sensor.h
 * @brief Electrochemical glucose sensor interface
 *
 * Provides the hardware abstraction for the amperometric glucose sensor,
 * including ADC sampling, fault detection, and warm-up sequencing.
 *
 * Implements: SWR-001, SWR-002, SWR-003, SWR-004, SWR-005
 */

#ifndef GLUCOSE_SENSOR_H
#define GLUCOSE_SENSOR_H

#include "cgm_types.h"
#include "config/error_codes.h"

/**
 * @brief Initialize the sensor interface and ADC peripheral.
 * Must be called before any other sensor functions.
 * @return CGM_OK on success, CGM_ERR_SENSOR_NOT_READY on failure.
 */
cgm_error_t sensor_init(void);

/**
 * @brief Begin the sensor warm-up sequence (SWR-004).
 * Applies stabilization voltage, performs impedance check, collects initial
 * calibration samples. Non-blocking; call sensor_get_state() to monitor progress.
 * @return CGM_OK if warm-up started successfully.
 */
cgm_error_t sensor_start_warmup(void);

/**
 * @brief Read raw sensor current from the ADC.
 * Samples the transimpedance amplifier output and converts to nanoamperes.
 * @param[out] current_na  Raw sensor current in nanoamperes.
 * @return CGM_OK on valid reading, CGM_ERR_SENSOR_NOT_READY if sensor inactive.
 */
cgm_error_t sensor_read_current(float *current_na);

/**
 * @brief Perform sensor fault detection (SWR-003).
 * Checks for open circuit, short circuit, and excessive noise conditions.
 * @param[in]  current_na  Current sensor reading in nanoamperes.
 * @param[out] fault       Detected fault type (FAULT_NONE if healthy).
 * @return CGM_OK if check completed (fault may still be set).
 */
cgm_error_t sensor_detect_fault(float current_na, sensor_fault_t *fault);

/**
 * @brief Get the current sensor operational state.
 */
sensor_state_t sensor_get_state(void);

/**
 * @brief Get cumulative sensor runtime in minutes.
 * Used for sensor lifetime tracking (SWR-005).
 */
uint32_t sensor_get_runtime_minutes(void);

/**
 * @brief Check if sensor has exceeded its 14-day lifetime (SWR-005).
 * @return true if sensor is expired.
 */
bool sensor_is_expired(void);

/**
 * @brief Read the on-board temperature sensor.
 * Used for temperature compensation (SWR-023).
 * @param[out] temp_celsius  Temperature in degrees Celsius.
 */
cgm_error_t sensor_read_temperature(float *temp_celsius);

/**
 * @brief Perform sensor impedance check for hydration verification.
 * @param[out] impedance_ohms  Measured impedance in ohms.
 * @return CGM_OK if measurement completed.
 */
cgm_error_t sensor_check_impedance(uint32_t *impedance_ohms);

/**
 * @brief Shut down sensor interface and disable ADC.
 * Called during critical battery shutdown (SWR-062).
 */
void sensor_shutdown(void);

#endif /* GLUCOSE_SENSOR_H */
