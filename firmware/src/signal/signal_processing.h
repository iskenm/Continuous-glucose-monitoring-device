/**
 * @file signal_processing.h
 * @brief Glucose signal processing pipeline
 *
 * Implements the signal conditioning chain: raw ADC samples -> filtered signal
 * -> outlier rejection -> glucose conversion -> rate of change calculation.
 *
 * Implements: SWR-010, SWR-011, SWR-012, SWR-013, SWR-014
 */

#ifndef SIGNAL_PROCESSING_H
#define SIGNAL_PROCESSING_H

#include "cgm_types.h"
#include "config/error_codes.h"

/**
 * @brief Initialize the signal processing pipeline.
 * Resets filter state, clears sample buffers, and prepares for first reading.
 */
cgm_error_t signal_init(void);

/**
 * @brief Add a raw sensor current sample to the processing buffer.
 * Called at 10 Hz (SWR-001). Samples are accumulated and processed
 * when a full measurement interval (5 minutes) of data is available.
 * @param[in] current_na  Raw sensor current in nanoamperes.
 */
cgm_error_t signal_add_sample(float current_na);

/**
 * @brief Process accumulated samples and produce a glucose reading.
 * Runs the full pipeline: filter -> outlier rejection -> glucose conversion.
 * Should be called every 5 minutes (MEASUREMENT_INTERVAL_MS).
 * @param[in]  temperature_c  Current temperature for calibration compensation.
 * @param[out] reading        The computed glucose reading with trend.
 * @return CGM_OK on success, CGM_ERR_SIGNAL_INSUFFICIENT if not enough samples.
 */
cgm_error_t signal_process(float temperature_c, glucose_reading_t *reading);

/**
 * @brief Get the current glucose rate of change (SWR-013).
 * Uses linear regression over the last 15 minutes (3 readings).
 * @param[out] rate_mgdl_per_min  Rate in mg/dL per minute.
 */
cgm_error_t signal_get_rate_of_change(float *rate_mgdl_per_min);

/**
 * @brief Determine trend direction from rate of change (SYS-REQ-020).
 */
glucose_trend_t signal_classify_trend(float rate_mgdl_per_min);

/**
 * @brief Clamp glucose value to valid range [40, 400] (SWR-014).
 * @return Clamped value in mg/dL.
 */
uint16_t signal_clamp_glucose(int32_t raw_glucose);

/**
 * @brief Reset the signal processing pipeline.
 * Called after sensor replacement or system reset.
 */
void signal_reset(void);

#endif /* SIGNAL_PROCESSING_H */
