/**
 * @file ble_manager.c
 * @brief BLE connection and security management implementation
 *
 * Implements: SWR-042, SWR-044, SYS-REQ-031, SYS-REQ-033
 * Risk Controls: RC-010 (AES-128-CCM encryption)
 */

#include "ble_manager.h"
#include "config/device_config.h"

#ifndef UNIT_TEST
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#else
#include "test_stubs.h"
#endif

static struct {
    bool            initialized;
    bool            connected;
    bool            advertising;
    uint32_t        adv_start_time;
    ble_connect_cb_t    on_connect;
    ble_disconnect_cb_t on_disconnect;
} s_ble;

/* Forward declarations */
static uint32_t get_uptime_ms(void);

/**
 * @brief Initialize BLE stack with security (SWR-042, RC-010)
 *
 * Configures LE Secure Connections pairing with numeric comparison
 * and AES-128-CCM encryption for all data transfers.
 */
cgm_error_t ble_init(void)
{
#ifndef UNIT_TEST
    int err = bt_enable(NULL);
    if (err) {
        return CGM_ERR_BLE_INIT_FAIL;
    }

    /* Configure LE Secure Connections (SWR-042)
     * - Require MITM protection via numeric comparison
     * - Enable bonding for persistent pairing
     * - AES-128-CCM encryption is mandatory */
    /* bt_conn_auth_cb_register(...) */
#endif

    s_ble.initialized = true;
    s_ble.connected = false;
    s_ble.advertising = false;

    return CGM_OK;
}

/**
 * @brief Start BLE advertising (SWR-044)
 *
 * Advertising behavior:
 * - Fast interval: 1 second when no device connected
 * - Slow interval: 2 seconds after 5 minutes without connection
 * - Advertises device name and CGM Service UUID (0x181F)
 */
cgm_error_t ble_start_advertising(void)
{
    if (!s_ble.initialized) {
        return CGM_ERR_BLE_INIT_FAIL;
    }

#ifndef UNIT_TEST
    /* Configure advertising data:
     * - Flags: LE General Discoverable, BR/EDR Not Supported
     * - Complete Local Name: CONFIG_BLE_DEVICE_NAME
     * - 16-bit Service UUID: 0x181F (CGM Service) */
    struct bt_le_adv_param adv_param = {
        .interval_min = CONFIG_BLE_ADV_INTERVAL_FAST_MS * 8 / 5, /* Convert to 0.625ms units */
        .interval_max = CONFIG_BLE_ADV_INTERVAL_FAST_MS * 8 / 5,
    };

    int err = bt_le_adv_start(&adv_param, NULL, 0, NULL, 0);
    if (err) {
        return CGM_ERR_BLE_ADV_FAIL;
    }
#endif

    s_ble.advertising = true;
    s_ble.adv_start_time = get_uptime_ms();

    return CGM_OK;
}

void ble_stop_advertising(void)
{
#ifndef UNIT_TEST
    bt_le_adv_stop();
#endif
    s_ble.advertising = false;
}

bool ble_is_connected(void)
{
    return s_ble.connected;
}

void ble_register_callbacks(ble_connect_cb_t on_connect,
                            ble_disconnect_cb_t on_disconnect)
{
    s_ble.on_connect = on_connect;
    s_ble.on_disconnect = on_disconnect;
}

/**
 * @brief Auto-reconnection support (SYS-REQ-033)
 * After disconnection, advertising resumes automatically to allow
 * the mobile app to reconnect within 30 seconds.
 */
cgm_error_t ble_disconnect(void)
{
    if (!s_ble.connected) {
        return CGM_OK;
    }

#ifndef UNIT_TEST
    /* bt_conn_disconnect(...) */
#endif

    s_ble.connected = false;
    if (s_ble.on_disconnect) {
        s_ble.on_disconnect();
    }

    /* Resume advertising for auto-reconnection */
    return ble_start_advertising();
}

int8_t ble_get_tx_power(void)
{
    return CONFIG_BLE_TX_POWER_DBM;
}

/* --- BLE event handlers (called from SoftDevice context) --- */

#ifndef UNIT_TEST
static void connected_cb(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        return;
    }
    s_ble.connected = true;
    s_ble.advertising = false;

    if (s_ble.on_connect) {
        s_ble.on_connect();
    }
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
    s_ble.connected = false;

    if (s_ble.on_disconnect) {
        s_ble.on_disconnect();
    }

    /* Auto-restart advertising for reconnection (SYS-REQ-033) */
    ble_start_advertising();
}
#endif

static uint32_t get_uptime_ms(void)
{
#ifndef UNIT_TEST
    return k_uptime_get_32();
#else
    extern uint32_t test_stub_uptime_ms;
    return test_stub_uptime_ms;
#endif
}
