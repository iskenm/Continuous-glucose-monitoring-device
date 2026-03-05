/**
 * @file flash_storage.h
 * @brief Non-volatile flash storage for glucose readings and calibration data
 *
 * Implements a circular buffer in external SPI flash for glucose readings
 * with wear leveling and power-loss-safe writes.
 *
 * Implements: SWR-050, SWR-051, SWR-052
 */

#ifndef FLASH_STORAGE_H
#define FLASH_STORAGE_H

#include "cgm_types.h"
#include "config/error_codes.h"

/**
 * @brief Initialize flash storage subsystem.
 * Scans flash to find the current write position in the circular buffer
 * and verifies calibration data integrity.
 */
cgm_error_t flash_init(void);

/**
 * @brief Store a glucose reading in the circular buffer (SWR-050).
 * Each entry is 8 bytes. The buffer holds 96 entries (8 hours).
 * Overwrites the oldest entry when full.
 */
cgm_error_t flash_store_reading(const glucose_reading_t *reading);

/**
 * @brief Read a glucose reading from the circular buffer.
 * @param[in]  index    Index relative to oldest stored reading (0 = oldest).
 * @param[out] reading  The retrieved reading.
 */
cgm_error_t flash_read_reading(uint16_t index, glucose_reading_t *reading);

/**
 * @brief Get the number of readings currently stored.
 */
uint16_t flash_get_reading_count(void);

/**
 * @brief Read calibration parameters from protected flash region (SWR-052).
 */
cgm_error_t flash_read_calibration(calibration_params_t *params);

/**
 * @brief Write calibration parameters to protected flash with CRC (SWR-052).
 */
cgm_error_t flash_write_calibration(const calibration_params_t *params);

/**
 * @brief Erase the reading storage area.
 * Called during factory reset or sensor replacement.
 */
cgm_error_t flash_erase_readings(void);

/**
 * @brief Save current device state for watchdog recovery (RC-011).
 */
cgm_error_t flash_save_device_state(const device_status_t *status);

/**
 * @brief Restore device state after watchdog reset.
 */
cgm_error_t flash_restore_device_state(device_status_t *status);

#endif /* FLASH_STORAGE_H */
