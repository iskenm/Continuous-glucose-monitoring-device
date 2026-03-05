/**
 * @file power_manager.h
 * @brief Power management and battery monitoring
 *
 * Controls sleep/wake transitions, battery level monitoring, and
 * graceful shutdown on critical battery.
 *
 * Implements: SWR-060, SWR-061, SWR-062
 */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include "cgm_types.h"
#include "config/error_codes.h"

/**
 * @brief Initialize power management subsystem.
 * Configures low-power sleep mode and battery monitoring ADC.
 */
cgm_error_t power_init(void);

/**
 * @brief Enter low-power sleep mode (SWR-060).
 * Target current draw: < 5 uA. Wakes on next RTOS timer event.
 */
void power_enter_sleep(void);

/**
 * @brief Read current battery voltage in millivolts.
 */
uint16_t power_get_battery_mv(void);

/**
 * @brief Get estimated battery level as percentage (SWR-061).
 * Uses a CR2032 discharge curve model.
 */
uint8_t power_get_battery_percent(void);

/**
 * @brief Check battery level and generate alerts if needed (SWR-061).
 * Generates LOW_BATTERY alert when < 24 hours remaining.
 * @return true if battery level is critical (< 2.0V).
 */
bool power_check_battery(void);

/**
 * @brief Perform graceful shutdown (SWR-062).
 * Saves state to flash, notifies connected device, disables sensor,
 * then enters deepest sleep mode.
 */
void power_shutdown(void);

#endif /* POWER_MANAGER_H */
