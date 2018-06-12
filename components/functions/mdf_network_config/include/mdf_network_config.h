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

#ifndef __MDF_NETWORK_CONFIG_H__
#define __MDF_NETWORK_CONFIG_H__

#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_MDF_USE_DEFAULT_NETWORK_CONFIG
#define MDF_USE_DEFAULT_NETWORK_CONFIG
#endif /**< CONFIG_USE_DEFAULT_NETWORK_CONFIG */

#ifndef CONFIG_MDF_DEFAULT_ROUTER_SSID
#define CONFIG_MDF_DEFAULT_ROUTER_SSID "router_ssid"
#endif /**< CONFIG_MDF_DEFAULT_ROUTER_SSID */
#define MDF_DEFAULT_ROUTER_SSID CONFIG_MDF_DEFAULT_ROUTER_SSID

#ifndef CONFIG_MDF_DEFAULT_ROUTER_PASSWD
#define CONFIG_MDF_DEFAULT_ROUTER_PASSWD "router_password"
#endif /**< CONFIG_MDF_DEFAULT_ROUTER_PASSWD */
#define MDF_DEFAULT_ROUTER_PASSWD CONFIG_MDF_DEFAULT_ROUTER_PASSWD

#ifdef MDF_USE_DEFAULT_NETWORK_CONFIG
static const uint8_t MDF_DEFAULT_ID[6] = {0x19, 0xfe, 0x34, 0x32, 0x12, 0x1d};
#endif

#ifndef CONFIG_MDF_AUTO_NETWORK_TIMEROUT
#define CONFIG_MDF_AUTO_NETWORK_TIMEROUT 30
#endif /**< CONFIG_MDF_AUTO_NETWORK_TIMEROUT */
#define MDF_AUTO_NETWORK_TIMEROUT CONFIG_MDF_AUTO_NETWORK_TIMEROUT

#ifdef CONFIG_MDF_USE_NETWORK_WHITELIST
#define MDF_USE_NETWORK_WHITELIST
#endif /**< CONFIG_MDF_USE_NETWORK_WHITELIST */

#ifndef CONFIG_MDF_NETWORK_WHITELIST_MAX_NUM
#define CONFIG_MDF_NETWORK_WHITELIST_MAX_NUM 200
#endif /**< CONFIG_MDF_NETWORK_WHITELIST_MAX_NUM */
#define MDF_NETWORK_WHITELIST_MAX_NUM CONFIG_MDF_NETWORK_WHITELIST_MAX_NUM

#ifndef CONFIG_MDF_NETWORK_NETWORK_ADDR_SIZE
#define CONFIG_MDF_NETWORK_NETWORK_ADDR_SIZE 3
#endif /**< CONFIG_MDF_NETWORK_NETWORK_ADDR_SIZE */
#define MDF_NETWORK_NETWORK_ADDR_SIZE CONFIG_MDF_NETWORK_NETWORK_ADDR_SIZE

#define MDF_SSID_LEN                   32
#define MDF_PASSWD_LEN                 32
#define MDF_TOKEN_LEN                  (40 + 1)
#define MDF_NETWORK_CONFIG_KEY         "mdf_network"

/**
 * @brief network configuration information
 */
typedef struct {
    char ssid[MDF_SSID_LEN];        /**< ssid */
    char password[MDF_PASSWD_LEN];  /**< password */
    uint8_t bssid[6];               /**< bssid */
    uint8_t channel;                /**< channel, the mesh network on */
    uint8_t mesh_id[6];             /**< mesh network identification */
    uint8_t token[MDF_TOKEN_LEN];   /**< For certification */
} network_config_t;

/**
 * @brief  get the network configuration information
 *
 * @param  network_config point to the configuration information
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_network_get_config(network_config_t *network_config);

/**
 * @brief  set the network configuration information
 *
 * @param  network_config point to the configuration information
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_network_set_config(const network_config_t *network_config);

/**
 * @brief  erase the network configuration information
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_network_clear_config(void);

/**
 * @brief  send configuration network information
 *
 * @param  network_config point to the configuration information
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_network_send_config(const network_config_t *network_config);

/**
 * @brief  enable automated network configuration
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_network_enable_auto(uint32_t timeout);

/**
 * @brief  enable configuration via Bluetooth network
 *
 * @param  network_config point to the configuration information
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_network_enable_blufi();

/**
 * @brief  add devices waiting for network configuration to the whitelist
 *
 * @param  device_addr device address
 * @param  num         number of devices
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_network_add_whitelist(const uint8_t device_addr[6], uint32_t num);

/**
 * @brief  delete whitelist
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_network_delete_whitelist();

#ifdef __cplusplus
}
#endif

#endif /**< __MDF_NETWORK_CONFIG_H__ */

