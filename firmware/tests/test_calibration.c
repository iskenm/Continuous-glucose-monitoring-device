/**
 * @file test_calibration.c
 * @brief Unit tests for the calibration module
 *
 * Verifies: SWR-012, SWR-020, SWR-021, SWR-022, SWR-023
 * Risk Controls: RC-002, RC-009
 */

#include "unity.h"
#include "sensor/calibration.h"
#include "storage/flash_storage.h"
#include "config/device_config.h"
#include "test_stubs.h"

void setUp(void)
{
    test_stubs_reset();
    flash_init();
    calibration_init();
}

void tearDown(void) {}

/**
 * @test SWR-VER-012: Glucose conversion accuracy
 * Verifies that calibration_apply correctly converts sensor current
 * to glucose concentration using the calibration equation.
 */
void test_glucose_conversion(void)
{
    /* Set known calibration: sensitivity=0.1 nA/(mg/dL), offset=1.0 nA */
    calibration_set_factory(0.1f, 1.0f, 0.0f);

    uint16_t glucose;
    /* current=11.0 nA -> glucose = (11.0 - 1.0) / 0.1 = 100 mg/dL */
    cgm_error_t err = calibration_apply(11.0f, 37.0f, &glucose);

    TEST_ASSERT_EQUAL(CGM_OK, err);
    TEST_ASSERT_EQUAL_UINT16(100, glucose);
}

/**
 * @test SWR-VER-012: Glucose conversion at measurement boundaries
 * Verifies correct conversion at the edges of the measurement range.
 */
void test_glucose_conversion_boundaries(void)
{
    calibration_set_factory(0.1f, 1.0f, 0.0f);

    uint16_t glucose;

    /* Low end: current=5.0 nA -> glucose = (5.0-1.0)/0.1 = 40 mg/dL */
    calibration_apply(5.0f, 37.0f, &glucose);
    TEST_ASSERT_EQUAL_UINT16(40, glucose);

    /* High end: current=41.0 nA -> glucose = (41.0-1.0)/0.1 = 400 mg/dL */
    calibration_apply(41.0f, 37.0f, &glucose);
    TEST_ASSERT_EQUAL_UINT16(400, glucose);
}

/**
 * @test SWR-VER-014: Range clamping below minimum
 * Verifies that glucose values below 40 mg/dL are clamped (RC-001).
 */
void test_glucose_clamp_low(void)
{
    calibration_set_factory(0.1f, 1.0f, 0.0f);

    uint16_t glucose;
    /* current=2.0 nA -> glucose = (2.0-1.0)/0.1 = 10 -> clamped to 40 */
    calibration_apply(2.0f, 37.0f, &glucose);
    TEST_ASSERT_EQUAL_UINT16(GLUCOSE_MIN_MGDL, glucose);
}

/**
 * @test SWR-VER-014: Range clamping above maximum
 * Verifies that glucose values above 400 mg/dL are clamped (RC-001).
 */
void test_glucose_clamp_high(void)
{
    calibration_set_factory(0.1f, 1.0f, 0.0f);

    uint16_t glucose;
    /* current=51.0 nA -> glucose = (51.0-1.0)/0.1 = 500 -> clamped to 400 */
    calibration_apply(51.0f, 37.0f, &glucose);
    TEST_ASSERT_EQUAL_UINT16(GLUCOSE_MAX_MGDL, glucose);
}

/**
 * @test SWR-VER-023: Temperature compensation
 * Verifies that temperature compensation adjusts the glucose reading
 * appropriately when body temperature deviates from 37°C.
 */
void test_temperature_compensation(void)
{
    calibration_set_factory(0.1f, 1.0f, 0.002f); /* temp_coeff = 0.2%/°C */

    uint16_t glucose_normal, glucose_warm;

    /* At normal body temperature (37°C) */
    calibration_apply(11.0f, 37.0f, &glucose_normal);

    /* At elevated temperature (39°C) - compensation should increase current */
    calibration_apply(11.0f, 39.0f, &glucose_warm);

    /* With positive temp_coeff, higher temp -> higher compensated current -> higher glucose */
    TEST_ASSERT_TRUE(glucose_warm > glucose_normal);
}

/**
 * @test SWR-VER-022: Calibration reference validation (RC-009)
 * Verifies that calibration references deviating >40% are rejected.
 */
void test_calibration_validity_accepted(void)
{
    /* Reference 100, sensor 90 -> 11% deviation -> should accept */
    cgm_error_t err = calibration_validate_reference(100, 90);
    TEST_ASSERT_EQUAL(CGM_OK, err);
}

void test_calibration_validity_rejected(void)
{
    /* Reference 200, sensor 100 -> 100% deviation -> should reject */
    cgm_error_t err = calibration_validate_reference(200, 100);
    TEST_ASSERT_EQUAL(CGM_ERR_CAL_DEVIATION, err);
}

/**
 * @test SWR-VER-021: In-vivo calibration update
 * Verifies that sensitivity is adjusted after a valid reference input.
 */
void test_invivo_calibration(void)
{
    calibration_set_factory(0.1f, 1.0f, 0.0f);

    const calibration_params_t *params_before = calibration_get_params();
    float sensitivity_before = params_before->sensitivity;

    /* Provide a reference that differs slightly from current sensor reading.
     * Sensor current 11.0 nA gives 100 mg/dL. Reference = 110 mg/dL.
     * Deviation = 10% (within 40% threshold). */
    cgm_error_t err = calibration_update(110, 11.0f);
    TEST_ASSERT_EQUAL(CGM_OK, err);

    const calibration_params_t *params_after = calibration_get_params();
    /* Sensitivity should have shifted toward the reference */
    TEST_ASSERT_TRUE(params_after->sensitivity != sensitivity_before);
}

/**
 * @test SWR-VER-020: Factory calibration storage integrity
 * Verifies that calibration data survives save/load cycle with CRC check.
 */
void test_factory_cal_storage(void)
{
    /* Set factory calibration */
    calibration_set_factory(0.15f, 2.0f, 0.003f);

    /* Re-initialize (loads from flash) */
    cgm_error_t err = calibration_init();
    TEST_ASSERT_EQUAL(CGM_OK, err);

    const calibration_params_t *params = calibration_get_params();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.15f, params->sensitivity);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, params->offset);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_glucose_conversion);
    RUN_TEST(test_glucose_conversion_boundaries);
    RUN_TEST(test_glucose_clamp_low);
    RUN_TEST(test_glucose_clamp_high);
    RUN_TEST(test_temperature_compensation);
    RUN_TEST(test_calibration_validity_accepted);
    RUN_TEST(test_calibration_validity_rejected);
    RUN_TEST(test_invivo_calibration);
    RUN_TEST(test_factory_cal_storage);

    return UNITY_END();
}
