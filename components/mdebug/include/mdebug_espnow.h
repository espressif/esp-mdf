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

#ifndef __MDF_ESPNOW_DEBUG_H__
#define __MDF_ESPNOW_DEBUG_H__

#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

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
    int16_t size;  /**< Size of data */
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
 * @brief  De-initialize the wireless debug receiver
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mdebug_espnow_deinit(void);

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
 * @param  src_addr  Destination address
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

#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif /**< __MDF_ESPNOW_DEBUG_H__ */
