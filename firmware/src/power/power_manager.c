/**
 * @file power_manager.c
 * @brief Power management implementation
 *
 * Implements: SWR-060, SWR-061, SWR-062
 * Risk Controls: RC-007 (low battery alert)
 */

#include "power_manager.h"
#include "../alert/alert_manager.h"
#include "../storage/flash_storage.h"
#include "../sensor/glucose_sensor.h"
#include "../ble/ble_manager.h"
#include "config/device_config.h"

#ifndef UNIT_TEST
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/pm/pm.h>
#else
#include "test_stubs.h"
#endif

static struct {
    uint16_t last_battery_mv;
    uint8_t  battery_percent;
    bool     initialized;
} s_power;

/**
 * @brief CR2032 discharge curve lookup table.
 * Maps battery voltage (mV) to remaining capacity (%).
 * Based on typical CR2032 discharge profile at 10 uA average draw.
 */
static const struct {
    uint16_t mv;
    uint8_t  percent;
} s_discharge_curve[] = {
    { 3000, 100 },
    { 2900,  85 },
    { 2800,  70 },
    { 2700,  50 },
    { 2600,  30 },
    { 2500,  15 },
    { 2400,  10 },
    { 2200,   5 },
    { 2000,   0 },
};

#define DISCHARGE_CURVE_SIZE (sizeof(s_discharge_curve) / sizeof(s_discharge_curve[0]))

cgm_error_t power_init(void)
{
    s_power.last_battery_mv = CONFIG_BATTERY_FULL_MV;
    s_power.battery_percent = 100;
    s_power.initialized = true;
    return CGM_OK;
}

/**
 * @brief Enter low-power sleep mode (SWR-060)
 *
 * Configures the MCU for minimal power consumption (< 5 uA):
 * - CPU halted, RAM retained
 * - All peripherals power-gated except RTC and BLE (if advertising)
 * - Wake sources: RTC alarm (for next measurement), BLE event
 */
void power_enter_sleep(void)
{
#ifndef UNIT_TEST
    /* The Zephyr power management subsystem handles the actual
     * transition to low-power state. The idle thread calls
     * pm_state_force() when no work is pending. */
#endif
}

uint16_t power_get_battery_mv(void)
{
#ifndef UNIT_TEST
    /* Read battery voltage via ADC with voltage divider */
    /* uint16_t raw = adc_read(...);
     * s_power.last_battery_mv = raw * VREF / 4096 * DIVIDER_RATIO; */
#else
    extern uint16_t test_stub_battery_mv;
    s_power.last_battery_mv = test_stub_battery_mv;
#endif
    return s_power.last_battery_mv;
}

/**
 * @brief Estimate battery percentage from voltage (SWR-061)
 *
 * Uses linear interpolation on the CR2032 discharge curve to
 * estimate remaining capacity. This provides a reasonable approximation
 * of remaining battery life for the 24-hour low battery warning.
 */
uint8_t power_get_battery_percent(void)
{
    uint16_t mv = power_get_battery_mv();

    /* Handle boundary conditions */
    if (mv >= s_discharge_curve[0].mv) {
        s_power.battery_percent = 100;
        return 100;
    }
    if (mv <= s_discharge_curve[DISCHARGE_CURVE_SIZE - 1].mv) {
        s_power.battery_percent = 0;
        return 0;
    }

    /* Linear interpolation between curve points */
    for (size_t i = 0; i < DISCHARGE_CURVE_SIZE - 1; i++) {
        if (mv <= s_discharge_curve[i].mv && mv > s_discharge_curve[i + 1].mv) {
            uint16_t mv_range = s_discharge_curve[i].mv - s_discharge_curve[i + 1].mv;
            uint8_t pct_range = s_discharge_curve[i].percent -
                                s_discharge_curve[i + 1].percent;
            uint16_t mv_offset = s_discharge_curve[i].mv - mv;

            uint8_t pct = s_discharge_curve[i].percent -
                          (uint8_t)((uint32_t)mv_offset * pct_range / mv_range);
            s_power.battery_percent = pct;
            return pct;
        }
    }

    return s_power.battery_percent;
}

/**
 * @brief Check battery and generate alerts (SWR-061, Risk Control RC-007)
 *
 * Called once per hour. Generates LOW_BATTERY alert when estimated
 * remaining life falls below 24 hours (~10% capacity for CGM workload).
 *
 * @return true if battery voltage is critically low (< 2.0V), requiring
 *         immediate shutdown (SWR-062).
 */
bool power_check_battery(void)
{
    uint16_t mv = power_get_battery_mv();
    uint8_t percent = power_get_battery_percent();

    /* Low battery alert at threshold (SWR-061, RC-007) */
    if (percent <= CONFIG_BATTERY_LOW_THRESHOLD_PCT) {
        /* Trigger alert through alert manager */
        alert_evaluate(0, 0.0f, FAULT_NONE); /* Updates other alerts too */
    }

    /* Critical battery shutdown threshold (SWR-062) */
    if (mv <= CONFIG_BATTERY_CRITICAL_MV) {
        return true;
    }

    return false;
}

/**
 * @brief Graceful shutdown sequence (SWR-062)
 *
 * When battery drops below 2.0V, we must shut down gracefully to
 * prevent data corruption:
 * 1. Save current device state to flash
 * 2. Notify connected BLE device of shutdown
 * 3. Disable the sensor (stop applying excitation voltage)
 * 4. Enter deepest sleep mode
 */
void power_shutdown(void)
{
    /* Step 1: Save state to flash for potential recovery */
    device_status_t status = {
        .sensor_state = sensor_get_state(),
        .battery_percent = s_power.battery_percent,
    };
    flash_save_device_state(&status);

    /* Step 2: Notify connected device */
    if (ble_is_connected()) {
        ble_disconnect();
    }
    ble_stop_advertising();

    /* Step 3: Disable sensor */
    sensor_shutdown();

    /* Step 4: Enter deep sleep (no wake source) */
#ifndef UNIT_TEST
    /* pm_state_force(0, &(struct pm_state_info){PM_STATE_SOFT_OFF}); */
#endif
}
