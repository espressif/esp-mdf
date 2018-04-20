/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
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

#ifndef __MDF_BLUFI_NETWORK_CONFIG_H__
#define __MDF_BLUFI_NETWORK_CONFIG_H__

#include "mdf_common.h"
#include "mdf_network_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  initialize Bluetooth network configuration
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_blufi_init();

/**
 * @brief  de-initialize Bluetooth network configuration
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_blufi_deinit();

/**
 * @brief  turn on Bluetooth radio
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_blufi_gap_start_advertising();

/**
 * @brief esp_bt_controller_mem_release
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_blufi_mem_release();

/**
 * @brief  bluetooth memory is released
 *
 * @return
 *     - true: bluetooth memory has been released
 *     - false: bluetooth memory is not released
 */
bool mdf_blufi_mem_is_release();
/**
 * @brief  when a device is connected to a router, it cannot switch channels
 *
 * @return
 *     - true: connecting
 *     - false: has stopped connecting
 */
bool mdf_blufi_connecting_wifi();

#ifdef __cplusplus
}
#endif

#endif /**< __MDF_BLUFI_NETWORK_CONFIG_H__ */
