/**
 * @file ble_manager.h
 * @brief BLE connection and security management
 *
 * Manages BLE advertising, connection lifecycle, security (pairing/bonding),
 * and power-efficient advertising intervals.
 *
 * Implements: SWR-042, SWR-044, SYS-REQ-031, SYS-REQ-033
 */

#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include "cgm_types.h"
#include "config/error_codes.h"

typedef void (*ble_connect_cb_t)(void);
typedef void (*ble_disconnect_cb_t)(void);

/**
 * @brief Initialize BLE stack and configure security.
 * Sets up LE Secure Connections with AES-128-CCM (SWR-042).
 */
cgm_error_t ble_init(void);

/**
 * @brief Start BLE advertising (SWR-044).
 * Begins at fast interval (1s), reduces to slow (2s) after 5 minutes.
 */
cgm_error_t ble_start_advertising(void);

/** @brief Stop BLE advertising. */
void ble_stop_advertising(void);

/** @brief Check if a device is currently connected. */
bool ble_is_connected(void);

/**
 * @brief Register connection state callbacks.
 */
void ble_register_callbacks(ble_connect_cb_t on_connect,
                            ble_disconnect_cb_t on_disconnect);

/**
 * @brief Disconnect the current connection gracefully.
 * Used during shutdown sequence.
 */
cgm_error_t ble_disconnect(void);

/** @brief Get the current BLE TX power in dBm. */
int8_t ble_get_tx_power(void);

#endif /* BLE_MANAGER_H */
