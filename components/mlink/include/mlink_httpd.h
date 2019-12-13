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

#ifndef __MLINK_HTTPD_H__
#define __MLINK_HTTPD_H__

#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

/**
 * @brief Data request source
 */
typedef enum {
    MLINK_HTTPD_FROM_DEVICE = 1, /**< Request from within the ESP-WIFI-MESH network */
    MLINK_HTTPD_FROM_SERVER = 2, /**< Request from server */
} mlink_httpd_from_t;

/**
 *@brief Http body data format
 */
typedef enum {
    MLINK_HTTPD_FORMAT_NONE, /**< Invalid format */
    MLINK_HTTPD_FORMAT_HEX,  /**< data is a hex format */
    MLINK_HTTPD_FORMAT_JSON, /**< data is a json format */
    MLINK_HTTPD_FORMAT_HTML, /**< data is a html format */
} mlink_httpd_format_t;

/**
 * @brief Http server data type
 */
typedef struct {
    uint16_t sockfd   : 16; /**< File descriptor for tcp */
    uint8_t format    : 2;  /**< Http body data format */
    uint8_t from      : 2;  /**< Data request source */
    bool    resp      : 1;  /**< Whether to respond to request data */
    uint16_t received : 11; /**< Received */
} mlink_httpd_type_t;

/**
 * @brief Http server data
 */
typedef struct {
    mlink_httpd_type_t type; /**< Http server data type */
    bool group;              /**< Send a package as a group */
    size_t addrs_num;        /**< Number of addresses */
    uint8_t *addrs_list;     /**< List of addresses */
    size_t size;             /**< Length of data */
    char *data;              /**< Pointer of Data */
} mlink_httpd_t;

/**
 * @brief  Start http server
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
mdf_err_t mlink_httpd_start(void);

/**
 * @brief  Stop http server
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
mdf_err_t mlink_httpd_stop(void);

/**
 * @brief Receive data from http server
 *
 * @param  request    Request data
 * @param  wait_ticks Waiting timeout
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
mdf_err_t mlink_httpd_read(mlink_httpd_t **request, TickType_t wait_ticks);

/**
 * @brief Send data to http server
 *
 * @param  response   Response data
 * @param  wait_ticks Waiting timeout
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
mdf_err_t mlink_httpd_write(const mlink_httpd_t *response, TickType_t wait_ticks);

#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif /**< __MLINK_HTTPD_H__ */
