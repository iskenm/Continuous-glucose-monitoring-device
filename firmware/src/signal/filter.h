/**
 * @file filter.h
 * @brief Digital Butterworth low-pass filter
 *
 * 4th-order IIR Butterworth filter with 0.1 Hz cutoff for noise removal
 * from raw sensor current readings.
 *
 * Implements: SWR-010
 */

#ifndef FILTER_H
#define FILTER_H

/**
 * @brief Initialize the Butterworth filter.
 * Resets all internal state (delay lines) to zero.
 */
void filter_init(void);

/**
 * @brief Apply one sample through the 4th-order Butterworth filter.
 * @param[in] input  Raw (or partially processed) sensor current sample.
 * @return Filtered output value.
 */
float filter_apply(float input);

/**
 * @brief Reset filter state without full reinitialization.
 */
void filter_reset(void);

#endif /* FILTER_H */
