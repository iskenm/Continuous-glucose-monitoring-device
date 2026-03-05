/**
 * @file signal_processing.c
 * @brief Glucose signal processing pipeline implementation
 *
 * Implements: SWR-010, SWR-011, SWR-012, SWR-013, SWR-014
 * Risk Controls: RC-001 (range clamping), RC-002 (accuracy via calibration)
 */

#include "signal_processing.h"
#include "filter.h"
#include "../sensor/calibration.h"
#include "config/device_config.h"
#include <string.h>
#include <math.h>

/* Sample buffer for one measurement interval */
#define SAMPLE_BUFFER_SIZE  SAMPLES_PER_MEASUREMENT

/* History buffer for rate-of-change calculation */
#define GLUCOSE_HISTORY_SIZE  CONFIG_RATE_CALC_POINTS

/* Module state */
static struct {
    float    sample_buffer[SAMPLE_BUFFER_SIZE];
    uint16_t sample_count;
    uint16_t glucose_history[GLUCOSE_HISTORY_SIZE];
    uint8_t  history_count;
    uint8_t  history_index;
    float    last_rate;
    bool     initialized;
} s_signal;

/* Forward declarations */
static float compute_median(float *data, uint16_t count);
static float compute_stddev(float *data, uint16_t count, float median);
static void  reject_outliers(float *data, uint16_t count, float median, float stddev);
static float compute_rate_regression(void);

cgm_error_t signal_init(void)
{
    memset(&s_signal, 0, sizeof(s_signal));
    s_signal.initialized = true;

    /* Initialize the Butterworth filter (SWR-010) */
    filter_init();

    return CGM_OK;
}

cgm_error_t signal_add_sample(float current_na)
{
    if (!s_signal.initialized) {
        return CGM_ERR_SIGNAL_INSUFFICIENT;
    }

    if (s_signal.sample_count >= SAMPLE_BUFFER_SIZE) {
        /* Buffer full - should have been processed already */
        return CGM_ERR_SIGNAL_OVERFLOW;
    }

    /* Apply Butterworth low-pass filter to each sample (SWR-010) */
    float filtered = filter_apply(current_na);
    s_signal.sample_buffer[s_signal.sample_count++] = filtered;

    return CGM_OK;
}

/**
 * @brief Process accumulated samples into a glucose reading.
 *
 * Pipeline:
 * 1. Compute median of filtered samples
 * 2. Reject outliers > 3 sigma from median (SWR-011)
 * 3. Average remaining samples
 * 4. Apply calibration for glucose conversion (SWR-012)
 * 5. Clamp to valid range (SWR-014)
 * 6. Calculate rate of change (SWR-013)
 * 7. Classify trend direction (SYS-REQ-020)
 */
cgm_error_t signal_process(float temperature_c, glucose_reading_t *reading)
{
    if (!s_signal.initialized || s_signal.sample_count < 10) {
        return CGM_ERR_SIGNAL_INSUFFICIENT;
    }

    /* Step 1: Compute rolling median for outlier detection */
    float median = compute_median(s_signal.sample_buffer, s_signal.sample_count);
    float stddev = compute_stddev(s_signal.sample_buffer, s_signal.sample_count, median);

    /* Step 2: Reject outliers beyond 3 sigma (SWR-011) */
    reject_outliers(s_signal.sample_buffer, s_signal.sample_count, median, stddev);

    /* Step 3: Compute mean of remaining (non-rejected) samples */
    float sum = 0.0f;
    uint16_t valid_count = 0;
    for (uint16_t i = 0; i < s_signal.sample_count; i++) {
        if (!isnan(s_signal.sample_buffer[i])) {
            sum += s_signal.sample_buffer[i];
            valid_count++;
        }
    }

    if (valid_count == 0) {
        return CGM_ERR_SIGNAL_INSUFFICIENT;
    }

    float avg_current = sum / (float)valid_count;

    /* Step 4: Convert to glucose via calibration (SWR-012) */
    uint16_t glucose_mgdl;
    cgm_error_t err = calibration_apply(avg_current, temperature_c, &glucose_mgdl);
    if (err != CGM_OK) {
        return err;
    }

    /* Step 5: Range clamping (SWR-014, Risk Control RC-001) */
    glucose_mgdl = signal_clamp_glucose((int32_t)glucose_mgdl);

    /* Step 6: Update glucose history for rate calculation */
    s_signal.glucose_history[s_signal.history_index] = glucose_mgdl;
    s_signal.history_index = (s_signal.history_index + 1) % GLUCOSE_HISTORY_SIZE;
    if (s_signal.history_count < GLUCOSE_HISTORY_SIZE) {
        s_signal.history_count++;
    }

    /* Step 7: Calculate rate of change (SWR-013) */
    s_signal.last_rate = compute_rate_regression();
    glucose_trend_t trend = signal_classify_trend(s_signal.last_rate);

    /* Build the output reading */
    reading->glucose_mgdl = glucose_mgdl;
    reading->trend = (uint8_t)trend;
    reading->status_flags = 0x01; /* Bit 0 = valid */

    /* Reset sample buffer for next interval */
    s_signal.sample_count = 0;

    return CGM_OK;
}

cgm_error_t signal_get_rate_of_change(float *rate_mgdl_per_min)
{
    if (s_signal.history_count < 2) {
        return CGM_ERR_SIGNAL_INSUFFICIENT;
    }
    *rate_mgdl_per_min = s_signal.last_rate;
    return CGM_OK;
}

/**
 * @brief Classify glucose trend from rate of change (SYS-REQ-020)
 *
 * Categories:
 *   > +3  mg/dL/min  -> RISING_RAPIDLY
 *   +1 to +3         -> RISING
 *   -1 to +1         -> STABLE
 *   -3 to -1         -> FALLING
 *   < -3             -> FALLING_RAPIDLY
 */
glucose_trend_t signal_classify_trend(float rate_mgdl_per_min)
{
    if (rate_mgdl_per_min > 3.0f) {
        return TREND_RISING_RAPIDLY;
    } else if (rate_mgdl_per_min > 1.0f) {
        return TREND_RISING;
    } else if (rate_mgdl_per_min >= -1.0f) {
        return TREND_STABLE;
    } else if (rate_mgdl_per_min >= -3.0f) {
        return TREND_FALLING;
    } else {
        return TREND_FALLING_RAPIDLY;
    }
}

/**
 * @brief Clamp glucose to valid measurement range (SWR-014, RC-001)
 * Values outside [40, 400] mg/dL are clamped to boundary values.
 */
uint16_t signal_clamp_glucose(int32_t raw_glucose)
{
    if (raw_glucose < GLUCOSE_MIN_MGDL) {
        return GLUCOSE_MIN_MGDL;
    }
    if (raw_glucose > GLUCOSE_MAX_MGDL) {
        return GLUCOSE_MAX_MGDL;
    }
    return (uint16_t)raw_glucose;
}

void signal_reset(void)
{
    memset(&s_signal, 0, sizeof(s_signal));
    s_signal.initialized = true;
    filter_init();
}

/* --- Internal helper functions --- */

/**
 * @brief Simple median calculation via partial sort.
 * For production, a more efficient algorithm would be used, but the
 * sample count per interval is bounded so O(n^2) is acceptable.
 */
static float compute_median(float *data, uint16_t count)
{
    /* Copy to temp buffer to avoid modifying original */
    float temp[SAMPLE_BUFFER_SIZE];
    memcpy(temp, data, count * sizeof(float));

    /* Partial insertion sort to find median */
    for (uint16_t i = 1; i < count; i++) {
        float key = temp[i];
        int j = i - 1;
        while (j >= 0 && temp[j] > key) {
            temp[j + 1] = temp[j];
            j--;
        }
        temp[j + 1] = key;
    }

    if (count % 2 == 0) {
        return (temp[count / 2 - 1] + temp[count / 2]) / 2.0f;
    }
    return temp[count / 2];
}

static float compute_stddev(float *data, uint16_t count, float median)
{
    float sum_sq = 0.0f;
    for (uint16_t i = 0; i < count; i++) {
        float diff = data[i] - median;
        sum_sq += diff * diff;
    }
    return sqrtf(sum_sq / (float)count);
}

/**
 * @brief Reject outliers beyond 3 sigma from median (SWR-011)
 * Rejected samples are replaced with NaN and excluded from averaging.
 */
static void reject_outliers(float *data, uint16_t count, float median, float stddev)
{
    float threshold = CONFIG_OUTLIER_SIGMA_THRESHOLD * stddev;

    for (uint16_t i = 0; i < count; i++) {
        if (fabsf(data[i] - median) > threshold) {
            data[i] = NAN; /* Mark as rejected */
        }
    }
}

/**
 * @brief Linear regression for glucose rate of change (SWR-013)
 *
 * Fits a line to the most recent 3 glucose readings (15 minutes)
 * and returns the slope in mg/dL/min.
 *
 * Uses the standard least-squares formula:
 *   slope = (n * sum(x*y) - sum(x) * sum(y)) / (n * sum(x^2) - sum(x)^2)
 * where x is time in minutes and y is glucose in mg/dL.
 */
static float compute_rate_regression(void)
{
    if (s_signal.history_count < 2) {
        return 0.0f;
    }

    uint8_t n = s_signal.history_count;
    float sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;

    for (uint8_t i = 0; i < n; i++) {
        /* Each reading is 5 minutes apart */
        float x = (float)i * 5.0f;
        uint8_t idx = (s_signal.history_index + GLUCOSE_HISTORY_SIZE - n + i) %
                      GLUCOSE_HISTORY_SIZE;
        float y = (float)s_signal.glucose_history[idx];

        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_x2 += x * x;
    }

    float denominator = (float)n * sum_x2 - sum_x * sum_x;
    if (fabsf(denominator) < 1e-6f) {
        return 0.0f;
    }

    return ((float)n * sum_xy - sum_x * sum_y) / denominator;
}
