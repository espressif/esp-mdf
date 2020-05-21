// Copyright 2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __MLINK_SNIFFER_H__
#define __MLINK_SNIFFER_H__

#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif /*!< _cplusplus */

/**
 * @breif Type of wireless packet
 */
typedef enum {
    MLINK_SNIFFER_NONE     = 0,
    MLINK_SNIFFER_WIFI     = 1 << 0,
    MLINK_SNIFFER_BLE      = 1 << 1,
    MLINK_SNIFFER_WIFI_BLE = MLINK_SNIFFER_WIFI | MLINK_SNIFFER_BLE,
} sniffer_type_t;

/**
 * @breif The type of each field within the packet
 */
enum sniffer_data_type {
    MLINK_SNIFFER_DATA_NONE         = 0,
    MLINK_SNIFFER_DATA_RSSI         = 1,
    MLINK_SNIFFER_DATA_ADDR         = 2,
    MLINK_SNIFFER_DATA_TIMESTAMP    = 3,
    MLINK_SNIFFER_DATA_NAME         = 4,
    MLINK_SNIFFER_DATA_CHANNEL      = 5,
    MLINK_SNIFFER_DATA_MANUFACTURER = 6,
};

/**
 * @breif Packet format
 */
typedef struct {
    uint8_t size;    /**< Total length of data, size = type + data*/
    uint8_t type;    /**< Type of wireless packet */
    uint8_t data[0]; /**< The contents of the packet, where the fields are in ltv format */
} __attribute__((packed)) mlink_sniffer_data_t;

/**
 * @brief Sniffer configuration
 */
typedef struct {
    sniffer_type_t enable_type; /**< Enable wireless packet type for listening */
    size_t buffer_num;          /**< Buffer space size */
    uint8_t notice_percentage;  /**< When the proportion of the buffer space is exceeded, send a UDP notification APP to get */
    bool esp_filter;            /**< Filter the espressif module to eliminate its own interference */
    uint16_t ble_scan_interval; /**< BLE scan interval */
    uint16_t ble_scan_window;   /**< BLE time per scan */
} mlink_sniffer_config_t;

/**
 * @brief Start Wi-Fi scanning
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t mlink_sniffer_wifi_start();

/**
 * @brief Stop Wi-Fi scanning
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t mlink_sniffer_wifi_stop();

#ifdef CONFIG_IDF_TARGET_ESP32
/**
 * @brief Start BLE scanning
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t mlink_sniffer_ble_start();

/**
 * @brief Stop BLE scanning
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t mlink_sniffer_ble_stop();

#endif

/**
 * @brief Set the configuration of the sniffer
 *
 * @param config The configuration of the sniffer
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t mlink_sniffer_set_config(const mlink_sniffer_config_t *config);

/**
 * @brief Get the configuration of the sniffer
 *
 * @param config The configuration of the sniffer
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t mlink_sniffer_get_config(mlink_sniffer_config_t *config);

/**
 * @brief
 *
 * @param data In order to reduce the length of the data and to use the binary ltv format for later expansion
 *             The data format is: [ size | type | data ]
 *                Size: sizeof(type) + sizeof(data)
 *                Type: MLINK_SNIFFER_WIFI or MLINK_SNIFFER_BLE
 *                Data: < len | data_type | data >
 *                      Len: sizeof(data_type) + sizeof(data)
 *                      Type: MLINK_SNIFFER_DATA_RSSI
 *                            MLINK_SNIFFER_DATA_ADDR
 *                            MLINK_SNIFFER_DATA_TIMESTAMP,
 *                            MLINK_SNIFFER_DATA_NAME,
 *                            MLINK_SNIFFER_DATA_CHANNEL,
 *                            MLINK_SNIFFER_DATA_MANUFACTURER
 * @param size The size of data
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t mlink_sniffer_data(uint8_t **data, size_t *size);

/**
 * @brief Initialize sniffer
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t mlink_sniffer_init();

/**
 * @brief De-initialize sniffer
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t mlink_sniffer_deinit();

#ifdef __cplusplus
}
#endif /*!< _cplusplus */

#endif /*!< __MLINK_SNIFFER_H__ */
