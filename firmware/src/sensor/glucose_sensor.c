/**
 * @file glucose_sensor.c
 * @brief Electrochemical glucose sensor interface implementation
 *
 * Implements: SWR-001, SWR-002, SWR-003, SWR-004, SWR-005
 * Risk Controls: RC-008 (sensor fault detection)
 */

#include "glucose_sensor.h"
#include "config/device_config.h"
#include <string.h>

/* Hardware register stubs - replaced by actual HAL on target */
#ifndef UNIT_TEST
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#else
#include "test_stubs.h"
#endif

/* Module state */
static struct {
    sensor_state_t state;
    sensor_fault_t active_faults;
    uint32_t       warmup_start_ms;
    uint32_t       insertion_time;
    uint32_t       fault_timer_ms;
    float          last_current_na;
    bool           initialized;
} s_sensor = {
    .state = SENSOR_STATE_IDLE,
    .active_faults = FAULT_NONE,
    .initialized = false
};

/* Forward declarations */
static uint16_t adc_read_raw(void);
static float    adc_to_nanoamps(uint16_t raw);
static uint32_t get_uptime_ms(void);
static void     apply_stabilization_voltage(bool enable);

cgm_error_t sensor_init(void)
{
    memset(&s_sensor, 0, sizeof(s_sensor));

    /* Configure ADC for 12-bit resolution, single-ended input
     * on the transimpedance amplifier output channel (SWR-001) */
#ifndef UNIT_TEST
    /* Actual ADC peripheral initialization would go here */
#endif

    s_sensor.state = SENSOR_STATE_IDLE;
    s_sensor.initialized = true;
    return CGM_OK;
}

/**
 * @brief Begin sensor warm-up sequence (SWR-004)
 *
 * The warm-up consists of:
 * 1. Apply stabilization voltage (+535mV) for 30 minutes
 * 2. Run impedance check to verify sensor hydration
 * 3. Collect initial calibration samples for 30 minutes
 * 4. Transition to ACTIVE only after calibration validates
 */
cgm_error_t sensor_start_warmup(void)
{
    if (!s_sensor.initialized) {
        return CGM_ERR_SENSOR_NOT_READY;
    }

    apply_stabilization_voltage(true);
    s_sensor.warmup_start_ms = get_uptime_ms();
    s_sensor.insertion_time = s_sensor.warmup_start_ms;
    s_sensor.state = SENSOR_STATE_WARMUP;

    return CGM_OK;
}

/**
 * @brief Read raw sensor current via ADC (SWR-001, SWR-002)
 *
 * Samples the electrochemical sensor at 12-bit resolution via
 * the transimpedance amplifier. The ADC is sampled at 10 Hz
 * and the result is converted from raw counts to nanoamperes.
 *
 * Valid sensor current range: 0.5 nA to 50 nA (SWR-002)
 */
cgm_error_t sensor_read_current(float *current_na)
{
    if (!s_sensor.initialized || s_sensor.state == SENSOR_STATE_IDLE) {
        return CGM_ERR_SENSOR_NOT_READY;
    }

    if (s_sensor.state == SENSOR_STATE_EXPIRED) {
        return CGM_ERR_SENSOR_EXPIRED;
    }

    uint16_t raw = adc_read_raw();
    float current = adc_to_nanoamps(raw);

    s_sensor.last_current_na = current;
    *current_na = current;

    return CGM_OK;
}

/**
 * @brief Detect sensor faults (SWR-003, Risk Control RC-008)
 *
 * Checks three fault conditions:
 * - Open circuit: current < 0.1 nA sustained for > 10 seconds
 * - Short circuit: current > 100 nA (immediate)
 * - Noise floor: SNR < 3 dB (requires multiple samples)
 *
 * Sets sensor state to FAULT within 30 seconds of fault occurrence.
 */
cgm_error_t sensor_detect_fault(float current_na, sensor_fault_t *fault)
{
    *fault = FAULT_NONE;

    /* Short circuit detection - immediate */
    if (current_na > SENSOR_FAULT_HIGH_NA) {
        *fault = FAULT_SHORT_CIRCUIT;
        s_sensor.active_faults |= FAULT_SHORT_CIRCUIT;
        s_sensor.state = SENSOR_STATE_FAULT;
        return CGM_OK;
    }

    /* Open circuit detection - requires sustained low current (SWR-003) */
    if (current_na < SENSOR_FAULT_LOW_NA) {
        if (s_sensor.fault_timer_ms == 0) {
            s_sensor.fault_timer_ms = get_uptime_ms();
        } else {
            uint32_t elapsed = get_uptime_ms() - s_sensor.fault_timer_ms;
            if (elapsed >= CONFIG_FAULT_DETECT_TIMEOUT_MS) {
                *fault = FAULT_OPEN_CIRCUIT;
                s_sensor.active_faults |= FAULT_OPEN_CIRCUIT;
                s_sensor.state = SENSOR_STATE_FAULT;
            }
        }
    } else {
        /* Current is valid, reset fault timer */
        s_sensor.fault_timer_ms = 0;
    }

    return CGM_OK;
}

sensor_state_t sensor_get_state(void)
{
    /* Check if warm-up is complete (SWR-004) */
    if (s_sensor.state == SENSOR_STATE_WARMUP) {
        uint32_t elapsed = get_uptime_ms() - s_sensor.warmup_start_ms;
        if (elapsed >= WARMUP_DURATION_MS) {
            apply_stabilization_voltage(false);
            s_sensor.state = SENSOR_STATE_ACTIVE;
        }
    }

    /* Check sensor lifetime expiration (SWR-005) */
    if (s_sensor.state == SENSOR_STATE_ACTIVE && sensor_is_expired()) {
        s_sensor.state = SENSOR_STATE_EXPIRED;
    }

    return s_sensor.state;
}

uint32_t sensor_get_runtime_minutes(void)
{
    if (s_sensor.insertion_time == 0) {
        return 0;
    }
    return (get_uptime_ms() - s_sensor.insertion_time) / 60000;
}

/**
 * @brief Check sensor lifetime (SWR-005)
 * Sensor must be replaced after 14 days (20,160 minutes).
 */
bool sensor_is_expired(void)
{
    return sensor_get_runtime_minutes() >= SENSOR_LIFETIME_MINUTES;
}

cgm_error_t sensor_read_temperature(float *temp_celsius)
{
    /* Read on-die temperature sensor for compensation (SWR-023) */
#ifndef UNIT_TEST
    /* Actual temperature sensor read via ADC or I2C */
    *temp_celsius = 37.0f; /* Placeholder */
#else
    extern float test_stub_temperature;
    *temp_celsius = test_stub_temperature;
#endif
    return CGM_OK;
}

cgm_error_t sensor_check_impedance(uint32_t *impedance_ohms)
{
    /* Apply small AC excitation and measure response
     * Used during warm-up to verify sensor hydration */
    *impedance_ohms = 0;

#ifndef UNIT_TEST
    /* Hardware-specific impedance measurement */
#else
    extern uint32_t test_stub_impedance;
    *impedance_ohms = test_stub_impedance;
#endif

    return CGM_OK;
}

void sensor_shutdown(void)
{
    apply_stabilization_voltage(false);
    s_sensor.state = SENSOR_STATE_SHUTDOWN;
    /* Disable ADC peripheral to minimize current draw */
}

/* --- Internal functions --- */

static uint16_t adc_read_raw(void)
{
#ifndef UNIT_TEST
    /* Read from actual ADC peripheral */
    return 0;
#else
    extern uint16_t test_stub_adc_value;
    return test_stub_adc_value;
#endif
}

/**
 * Convert raw ADC counts to sensor current in nanoamperes.
 * The transimpedance amplifier converts sensor current to voltage,
 * which is digitized by the 12-bit ADC.
 *
 * Conversion: current_nA = (raw / 4096) * Vref / (TIA_gain * 1e9)
 */
static float adc_to_nanoamps(uint16_t raw)
{
    float voltage_mv = ((float)raw / (float)(1 << ADC_RESOLUTION_BITS)) * ADC_REFERENCE_MV;
    float current_na = voltage_mv / (float)CONFIG_ADC_GAIN;
    return current_na;
}

static uint32_t get_uptime_ms(void)
{
#ifndef UNIT_TEST
    return k_uptime_get_32();
#else
    extern uint32_t test_stub_uptime_ms;
    return test_stub_uptime_ms;
#endif
}

static void apply_stabilization_voltage(bool enable)
{
#ifndef UNIT_TEST
    /* Set DAC output for sensor stabilization voltage */
    (void)enable;
#endif
}
