/**
 * @file calibration.h
 * @brief Sensor calibration management
 *
 * Handles factory calibration storage, in-vivo recalibration, temperature
 * compensation, and calibration integrity verification.
 *
 * Implements: SWR-020, SWR-021, SWR-022, SWR-023
 */

#ifndef CALIBRATION_H
#define CALIBRATION_H

#include "cgm_types.h"
#include "config/error_codes.h"

/**
 * @brief Initialize calibration module.
 * Loads factory calibration from flash and verifies CRC integrity.
 * @return CGM_OK or CGM_ERR_CAL_CRC_FAIL if stored data is corrupt.
 */
cgm_error_t calibration_init(void);

/**
 * @brief Apply calibration to convert sensor current to glucose (SWR-012).
 * glucose = (current_na - offset) / sensitivity
 * @param[in]  current_na     Filtered sensor current in nanoamperes.
 * @param[in]  temperature_c  Current temperature for compensation (SWR-023).
 * @param[out] glucose_mgdl   Calculated glucose in mg/dL.
 */
cgm_error_t calibration_apply(float current_na, float temperature_c,
                               uint16_t *glucose_mgdl);

/**
 * @brief Process an in-vivo calibration reference value (SWR-021).
 * Adjusts sensitivity using weighted least-squares with exponential forgetting.
 * @param[in] reference_mgdl  Fingerstick glucose reference value.
 * @param[in] sensor_current  Current sensor reading in nanoamperes.
 * @return CGM_ERR_CAL_DEVIATION if reference differs > 40% from sensor (SWR-022).
 */
cgm_error_t calibration_update(uint16_t reference_mgdl, float sensor_current);

/**
 * @brief Validate a calibration reference value (SWR-022).
 * @return CGM_OK if reference is acceptable, CGM_ERR_CAL_DEVIATION otherwise.
 */
cgm_error_t calibration_validate_reference(uint16_t reference_mgdl,
                                            uint16_t current_sensor_mgdl);

/**
 * @brief Store current calibration parameters to flash with CRC (SWR-020).
 */
cgm_error_t calibration_save(void);

/**
 * @brief Get the current calibration parameters (read-only).
 */
const calibration_params_t *calibration_get_params(void);

/**
 * @brief Set factory calibration parameters.
 * Called during manufacturing via the calibration tool.
 */
cgm_error_t calibration_set_factory(float sensitivity, float offset,
                                     float temp_coefficient);

#endif /* CALIBRATION_H */
