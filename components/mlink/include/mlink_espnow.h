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

#ifndef __MLINK_ESPNOW_H__
#define __MLINK_ESPNOW_H__

#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

#define MLINK_ESPNOW_PAYLOAD_LEN  (226)

/**
 * @brief Mlink espnow configuration
 */
typedef struct {
    uint8_t channel;         /**< The channel on which the device is located */
    uint8_t parent_bssid[6]; /**< Forwarding device address */
} mlink_espnow_config_t;

/**
 * @brief Send a packet
 *
 * @param  addrs_list The address of the final destination of the packet
 * @param  addrs_num  Number of destination addresses
 * @param  data       Pointer to a sending espnow packet
 * @param  size       The length of the data
 * @param  wait_ticks wait time if a packet isn't immediately available(0:no wait, portMAX_DELAY:wait forever)
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 *    - MDF_ERR_INVALID_ARG
 */
mdf_err_t mlink_espnow_write(const uint8_t *addrs_list, size_t addrs_num, const void *data,
                             size_t size, TickType_t wait_ticks);

/**
 * @brief Receive a packet
 *
 * @param  addrs_list The address of the final source of the packet
 * @param  addrs_num  Number of source addresses
 * @param  data       Pointer to a sending espnow packet
 * @param  size       The length of the data
 * @param  wait_ticks wait time if a packet isn't immediately available(0:no wait, portMAX_DELAY:wait forever)
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 *    - MDF_ERR_INVALID_ARG
 */
mdf_err_t mlink_espnow_read(uint8_t **addrs_list, size_t *addrs_num, uint8_t **data,
                            size_t *size, TickType_t wait_ticks);

/**
 * @brief Initialize the use of ESP-NOW
 *
 * @param  config Mlink espnow initialization configuration.
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 *    - MDF_ERR_INVALID_ARG
 */
mdf_err_t mlink_espnow_init(mlink_espnow_config_t *config);

#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif /**< __MLINK_ESPNOW_H__ */
