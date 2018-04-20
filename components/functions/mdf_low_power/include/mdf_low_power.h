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

#ifndef __MDF_ESPNOW_CONTROL_H__
#define __MDF_ESPNOW_CONTROL_H__

#include "mdf_wifi_mesh.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

typedef enum {
    POWER_ACTIVE     = 1 << 0,  /**< normal mode */
    POWER_DEEP_SLEEP = 1 << 1,  /**< deep sleep mode */
    TRANS_WIFI_MESH  = 1 << 4,  /**< data transmitted through mesh */
    TRANS_ESPNOW     = 1 << 5,  /**< data transmitted through espnow */
} mdf_running_mode_t;

/**
 * @brief  initialize the espnow control
 *
 * @param  gpio_num GPIO index of the pin that the button uses
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_low_power_init();

/**
 * @brief  gets and sets the espnow_control's parent node
 *
 * @param  parent_mac parent node mac address
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_low_power_get_parent(wifi_mesh_addr_t *parent_mac);

/**
 * @brief  send data through espnow
 *
 * @param  dest_addr the destination address of the packet
 * @param  data      pointer to the receiving mdf packet
 * @param  size      the size of the buffer
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_low_power_send(wifi_mesh_addr_t *dest_addr,
                             const void *data, size_t size);

/**
 * @brief  get the current device running mode
 *
 * @param  mode device running mode
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
mdf_running_mode_t mdf_get_running_mode();

/**
 * @brief  set the current device running mode
 *
 * @param  mode device running mode
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_set_running_mode(mdf_running_mode_t mode);

#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif /**< __MDF_ESPNOW_CONTROL_H__ */
