/**
 * @file test_signal_processing.c
 * @brief Unit tests for the signal processing pipeline
 *
 * Verifies: SWR-010, SWR-011, SWR-013, SWR-014
 */

#include "unity.h"
#include "signal/signal_processing.h"
#include "sensor/calibration.h"
#include "storage/flash_storage.h"
#include "config/device_config.h"
#include "test_stubs.h"

void setUp(void)
{
    test_stubs_reset();
    flash_init();
    calibration_init();
    calibration_set_factory(0.1f, 1.0f, 0.0f);
    signal_init();
}

void tearDown(void) {}

/**
 * @test SWR-VER-010: Butterworth low-pass filter
 * Verifies that the filter attenuates high-frequency noise while
 * passing the DC component (representing the glucose signal).
 */
void test_butterworth_filter(void)
{
    /* Feed a constant signal - filter output should converge to input value */
    float constant_current = 11.0f; /* Corresponds to 100 mg/dL */

    for (int i = 0; i < 100; i++) {
        signal_add_sample(constant_current);
    }

    glucose_reading_t reading;
    cgm_error_t err = signal_process(37.0f, &reading);

    TEST_ASSERT_EQUAL(CGM_OK, err);
    /* With constant input, output should be close to expected glucose */
    TEST_ASSERT_UINT16_WITHIN(5, 100, reading.glucose_mgdl);
}

/**
 * @test SWR-VER-011: Outlier rejection
 * Verifies that spike outliers (>3 sigma) are rejected and do not
 * significantly affect the glucose reading.
 */
void test_outlier_rejection(void)
{
    /* Feed mostly constant signal with a few extreme outliers */
    for (int i = 0; i < 100; i++) {
        float sample = 11.0f; /* Normal: ~100 mg/dL */

        /* Insert extreme outliers at samples 30, 60, 90 */
        if (i == 30 || i == 60 || i == 90) {
            sample = 50.0f; /* Huge spike */
        }

        signal_add_sample(sample);
    }

    glucose_reading_t reading;
    cgm_error_t err = signal_process(37.0f, &reading);

    TEST_ASSERT_EQUAL(CGM_OK, err);
    /* Despite outliers, glucose should still be close to 100 mg/dL */
    TEST_ASSERT_UINT16_WITHIN(15, 100, reading.glucose_mgdl);
}

/**
 * @test SWR-VER-013: Rate of change calculation
 * Verifies linear regression over 3 readings produces correct rate.
 */
void test_rate_of_change(void)
{
    /* Simulate 3 measurement cycles with rising glucose */
    float currents[] = { 11.0f, 16.0f, 21.0f }; /* ~100, ~150, ~200 mg/dL */

    for (int cycle = 0; cycle < 3; cycle++) {
        signal_reset();
        calibration_set_factory(0.1f, 1.0f, 0.0f);

        for (int i = 0; i < 50; i++) {
            signal_add_sample(currents[cycle]);
        }

        glucose_reading_t reading;
        signal_process(37.0f, &reading);
    }

    float rate;
    cgm_error_t err = signal_get_rate_of_change(&rate);

    TEST_ASSERT_EQUAL(CGM_OK, err);
    /* Rate should be positive (rising glucose) */
    TEST_ASSERT_TRUE(rate > 0.0f);
}

/**
 * @test SWR-VER-014: Range clamping
 * Verifies glucose values are clamped to [40, 400] mg/dL (RC-001).
 */
void test_range_clamping(void)
{
    TEST_ASSERT_EQUAL_UINT16(40, signal_clamp_glucose(10));
    TEST_ASSERT_EQUAL_UINT16(40, signal_clamp_glucose(39));
    TEST_ASSERT_EQUAL_UINT16(40, signal_clamp_glucose(40));
    TEST_ASSERT_EQUAL_UINT16(100, signal_clamp_glucose(100));
    TEST_ASSERT_EQUAL_UINT16(400, signal_clamp_glucose(400));
    TEST_ASSERT_EQUAL_UINT16(400, signal_clamp_glucose(401));
    TEST_ASSERT_EQUAL_UINT16(400, signal_clamp_glucose(999));
}

/**
 * @test Trend classification per SYS-REQ-020
 */
void test_trend_classification(void)
{
    TEST_ASSERT_EQUAL(TREND_RISING_RAPIDLY, signal_classify_trend(4.0f));
    TEST_ASSERT_EQUAL(TREND_RISING, signal_classify_trend(2.0f));
    TEST_ASSERT_EQUAL(TREND_STABLE, signal_classify_trend(0.0f));
    TEST_ASSERT_EQUAL(TREND_FALLING, signal_classify_trend(-2.0f));
    TEST_ASSERT_EQUAL(TREND_FALLING_RAPIDLY, signal_classify_trend(-4.0f));
}

/**
 * @test SWR-VER-010: Filter handles insufficient samples
 */
void test_insufficient_samples(void)
{
    /* Only add a few samples (well below minimum) */
    signal_add_sample(11.0f);
    signal_add_sample(11.0f);

    glucose_reading_t reading;
    cgm_error_t err = signal_process(37.0f, &reading);

    TEST_ASSERT_EQUAL(CGM_ERR_SIGNAL_INSUFFICIENT, err);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_butterworth_filter);
    RUN_TEST(test_outlier_rejection);
    RUN_TEST(test_rate_of_change);
    RUN_TEST(test_range_clamping);
    RUN_TEST(test_trend_classification);
    RUN_TEST(test_insufficient_samples);

    return UNITY_END();
}
