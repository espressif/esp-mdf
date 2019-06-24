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
#ifndef __MDF_ESPNOW_DEBUG_H__
#define __MDF_ESPNOW_DEBUG_H__

#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

/**
 * @brief Log sending configuration
 */
typedef struct {
    uint8_t dest_addr[6]; /**< Turn off log sending if all is zero */
} mdebug_log_config_t;

/**
 * @brief Type of data sent during wireless debugging
 */
typedef enum {
    MDEBUG_ESPNOW_COREDUMP = 1, /**< Core dump information */
    MDEBUG_ESPNOW_CONSOLE,      /**< Remotely call local terminal commands */
    MDEBUG_ESPNOW_LOG,          /**< Log information */
} mdebug_espnow_t;

/**
 * @brief Type of core dump data
 */
enum {
    MDEBUG_COREDUMP_BEGIN = 1, /**< Start transferring core dump data */
    MDEBUG_COREDUMP_DATA,      /**< Send core dump data */
    MDEBUG_COREDUMP_END,       /**< End core dump data transfer */
};

/**
 * @brief Core dump data structure
 */
typedef struct {
    uint8_t type;  /**< Type of packet */
    int16_t size;  /**< size of data */
    int16_t seq;   /**< Sequence */
    uint8_t data[230]; /**< data */
}  __attribute__((packed)) mdebug_coredump_packet_t;

/**
 * @brief  Initialize the wireless debug receiver
 *          - register espnow log redirect function
 *          - Create mdebug espnow send and mdebug log send task
 *          - Create log send queue
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mdebug_espnow_init(void);

/**
 * @brief  Send debug data with ESP-NOW
 *
 * @param  dest_addr  Destination address
 * @param  data       Point to send data buffer
 * @param  size       send data len
 * @param  type       Type of data
 * @param  wait_ticks wait time if a packet isn't immediately available
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
mdf_err_t mdebug_espnow_write(const uint8_t *dest_addr, const void *data, size_t size,
                              mdebug_espnow_t type, TickType_t wait_ticks);

/**
 * @brief  receive debug data with ESP-NOW
 *
 * @param  dest_addr  Destination address
 * @param  data       Point to send data buffer
 * @param  size       send data len
 * @param  type       Type of data
 * @param  wait_ticks wait time if a packet isn't immediately available
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
mdf_err_t mdebug_espnow_read(uint8_t *src_addr, void *data, size_t *size,
                             mdebug_espnow_t *type, TickType_t wait_ticks);


/**
 * @brief  Get the configuration of the log during wireless debugging
 *
 * @param  config The configuration of the log
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mdebug_log_get_config(mdebug_log_config_t *config);

/**
 * @brief  Set the configuration of the log during wireless debugging
 *
 * @param  config The configuration of the log
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mdebug_log_set_config(const mdebug_log_config_t *config);

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __MDF_ESPNOW_DEBUG_H__ */
