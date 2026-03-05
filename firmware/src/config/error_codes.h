/**
 * @file error_codes.h
 * @brief System-wide error codes for the CGM-3000 firmware
 */

#ifndef ERROR_CODES_H
#define ERROR_CODES_H

typedef enum {
    CGM_OK                      = 0,

    /* Sensor errors (0x0100 - 0x01FF) */
    CGM_ERR_SENSOR_NOT_READY    = 0x0100,
    CGM_ERR_SENSOR_OPEN_CIRCUIT = 0x0101,
    CGM_ERR_SENSOR_SHORT        = 0x0102,
    CGM_ERR_SENSOR_NOISE        = 0x0103,
    CGM_ERR_SENSOR_EXPIRED      = 0x0104,
    CGM_ERR_SENSOR_WARMUP       = 0x0105,

    /* Calibration errors (0x0200 - 0x02FF) */
    CGM_ERR_CAL_INVALID         = 0x0200,
    CGM_ERR_CAL_CRC_FAIL        = 0x0201,
    CGM_ERR_CAL_REFERENCE_OOR   = 0x0202,  /* Reference out of range */
    CGM_ERR_CAL_DEVIATION       = 0x0203,  /* Reference/sensor deviation > 40% */
    CGM_ERR_CAL_FLASH_WRITE     = 0x0204,

    /* Signal processing errors (0x0300 - 0x03FF) */
    CGM_ERR_SIGNAL_OVERFLOW     = 0x0300,
    CGM_ERR_SIGNAL_UNDERFLOW    = 0x0301,
    CGM_ERR_SIGNAL_INSUFFICIENT = 0x0302,  /* Not enough samples */

    /* BLE errors (0x0400 - 0x04FF) */
    CGM_ERR_BLE_INIT_FAIL       = 0x0400,
    CGM_ERR_BLE_ADV_FAIL        = 0x0401,
    CGM_ERR_BLE_CONN_FAIL       = 0x0402,
    CGM_ERR_BLE_NOTIFY_FAIL     = 0x0403,
    CGM_ERR_BLE_SECURITY_FAIL   = 0x0404,

    /* Storage errors (0x0500 - 0x05FF) */
    CGM_ERR_FLASH_WRITE_FAIL    = 0x0500,
    CGM_ERR_FLASH_READ_FAIL     = 0x0501,
    CGM_ERR_FLASH_ERASE_FAIL    = 0x0502,
    CGM_ERR_FLASH_FULL          = 0x0503,
    CGM_ERR_FLASH_CRC_FAIL      = 0x0504,

    /* Power errors (0x0600 - 0x06FF) */
    CGM_ERR_BATTERY_LOW         = 0x0600,
    CGM_ERR_BATTERY_CRITICAL    = 0x0601,

    /* Self-test errors (0x0700 - 0x07FF) */
    CGM_ERR_POST_RAM            = 0x0700,
    CGM_ERR_POST_FLASH          = 0x0701,
    CGM_ERR_POST_ADC            = 0x0702,
    CGM_ERR_POST_BLE            = 0x0703,
    CGM_ERR_POST_SENSOR         = 0x0704
} cgm_error_t;

#endif /* ERROR_CODES_H */
