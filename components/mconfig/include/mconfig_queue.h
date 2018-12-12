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

#ifndef __MCONFIG_QUEUE_H__
#define __MCONFIG_QUEUE_H__

#include "mdf_common.h"
#include "mdf_event_loop.h"
#include "mwifi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief List of configured networks for each device
 */
typedef struct {
    uint8_t addr[MWIFI_ADDR_LEN]; /**< the address of the device */

#ifdef CONFIG_MCONFIG_WHITELIST_SECURITY_ENABLE
    /**< Used to verify the validity of the public key to prevent fake device attacks */
    uint8_t checksum[CONFIG_MCONFIG_WHITELIST_CHECKSUM_SIZE];
#endif
} mconfig_whitelist_t;

/**
 * @brief Network configuration information
 */
typedef struct {
    mwifi_config_t config;                 /**< Mwifi AP configuration */
    mwifi_init_config_t init_config;       /**< Mwifi initialization configuration, 174 Byte */
    uint8_t init_config_reserved[4];       /**< Reserve 4 bytes to prevent version incompatibility
                                                caused by mwifi_init_config_t changes */
    uint16_t whitelist_size;               /**< The size of the device's whitelist */
    mconfig_whitelist_t whitelist_data[0]; /**< Whitelist of devices */
} __attribute__((packed)) mconfig_data_t;

/**
 * @brief  Write data to the queue of mconfig
 *
 * @param  mconfig_data information that points to the configuration of the network
 * @param  wait_ticks   wait time if a packet isn't immediately available
 *
 * @return
 *     - MDF_OK
 *     - MDF_ERR_TIMEOUT
 */
mdf_err_t mconfig_queue_write(const mconfig_data_t *mconfig_data, TickType_t wait_ticks);

/**
 * @brief  READ data to the queue of mconfig
 *
 * @param  mconfig_data    mconfig_data is a secondary pointer and must be called after MDF_FREE is used
 * @param  wait_ticks wait time if a packet isn't immediately available
 *
 * @return
 *     - MDF_OK
 *     - MDF_ERR_TIMEOUT
 */
mdf_err_t mconfig_queue_read(mconfig_data_t **mconfig_data, TickType_t wait_ticks);

#ifdef __cplusplus
}
#endif

#endif /**< __MCONFIG_QUEUE_H__ */
