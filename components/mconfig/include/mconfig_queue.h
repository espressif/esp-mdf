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
    union {
        mwifi_init_config_t init_config;   /**< Mwifi initialization configuration, Used only during debugging */
        uint8_t custom[32 + CONFIG_MCONFIG_CUSTOM_EXTERN_LEN]; /**< Custom data for specific applications, such as: uuid, token, username, etc */
    };
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
