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
    MLINK_HTTPD_FROM_DEVICE = 1, /**< Request from within the ESP-MESH network */
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
    size_t addrs_num;        /**< Number of addresses */
    uint8_t *addrs_list;     /**< List of addresses */
    size_t size;             /**< Length of data */
    char *data;              /**< Pointer of Data */
} mlink_httpd_t;

/**
 * @brief  Start http server
 *
 * @return  [description]
 */
mdf_err_t mlink_httpd_start(void);

/**
 * @brief  Stop http server
 *
 * @return  [description]
 */
mdf_err_t mlink_httpd_stop(void);

/**
 * @brief Receive data from http server
 *
 * @param  request    Request data
 * @param  wait_ticks Waiting timeout
 *
 * @return            [description]
 */
mdf_err_t mlink_httpd_read(mlink_httpd_t **request, TickType_t wait_ticks);

/**
 * @brief Send data to http server
 *
 * @param  response   Response data
 * @param  wait_ticks Waiting timeout
 *
 * @return            [description]
 */
mdf_err_t mlink_httpd_write(const mlink_httpd_t *response, TickType_t wait_ticks);

#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif /**< __MLINK_HTTPD_H__ */
