/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2019 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __MLINK_BLE_H__
#define __MLINK_BLE_H__

#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif /*!< _cplusplus */

#ifndef CONFIG_BLE_GATEWAY_OUI
#define CONFIG_BLE_GATEWAY_OUI "MGW" /**< Used to filter other BLE broadcast packets, 3 bytes */
#endif  /**< CONFIG_BLE_GATEWAY_OUI */

/**
 * @brief BLE gateway configuration
 */
typedef struct {
    char name[28];           /**< Local device & peripheral name */
    uint16_t company_id;     /**< Company Identifiers (https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers) */
    uint8_t custom_size;     /**< Custom data size */
    uint8_t custom_data[31]; /**< Placed in a BLE broadcast package */
} mlink_ble_config_t;

/**
 * @brief Type of ble callback function
 */
typedef mdf_err_t (* mlink_ble_cb_t)(void *data, size_t size);

/**
 * @brief Initialize the function of ble
 *
 * @note 1. Using ble will take up 30kB of memory
 *       2. Using this feature requires Bluetooth to be turned on and the BLE stack is not released.
 */
mdf_err_t mlink_ble_init(const mlink_ble_config_t *config);

/**
 * @brief Initialize the function of ble
 *
 * @note The BLE stack space is not released.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t mlink_ble_deinit();

/**
 * @brief Modify the configuration of the BLE gateway
 *
 * @note config BLE gateway configuration
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t mlink_ble_set_config(const mlink_ble_config_t *config);

/**
 * @brief Get the configuration of the BLE gateway
 *
 * @note config BLE gateway configuration
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t mlink_ble_get_config(mlink_ble_config_t *config);

/**
 * @brief Proactively push data to the client
 *
 * @param  wait_ticks wait time if a packet isn't immediately available(0:no wait, portMAX_DELAY:wait forever)
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 *    - MDF_ERR_INVALID_ARG
 */
mdf_err_t mlink_ble_send(const void *data, ssize_t size);

/**
 * @brief Set the BLE sendback callback function
 *
 * @param read_cb BLE receive callback function
 * @param write_cb BLE send callback function
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 *    - MDF_ERR_INVALID_ARG
 */
mdf_err_t mlink_ble_set_cb(const mlink_ble_cb_t read_cb, const mlink_ble_cb_t write_cb);

/**
 * @brief Configure parameters related to BLE scanning
 *
 * @param interval Scan interval. This is defined as the time interval from
                   when the Controller started its last LE scan until it begins the subsequent LE scan.
 * @param window   Scan window. The duration of the LE scan
 * @param scan_cb  Scan result callback function
 *
* @return
 *    - MDF_OK
 *    - MDF_FAIL
 *    - MDF_ERR_INVALID_ARG
 */
mdf_err_t mlink_ble_set_scan(uint16_t interval, uint16_t window, mlink_ble_cb_t scan_cb);

#ifdef __cplusplus
}
#endif /*!< _cplusplus */

#endif /*!< __MLINK_BLE_H__ */
