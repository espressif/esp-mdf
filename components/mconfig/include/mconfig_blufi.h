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

#ifndef __MCONFIG_BLUFI_H__
#define __MCONFIG_BLUFI_H__

#include "esp_bt.h"

#include "mdf_common.h"
#include "mconfig_queue.h"
#include "mconfig_chain.h"
#include "mdf_event_loop.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MCONFIG_BLUFI_NAME_SIZE                  (28) /**< Contains the ending character */
#define MCONFIG_BLUFI_CUSTOM_SIZE                (31) /**< BLE broadcast data packets have a valid length of up to 31 bytes */

#ifndef CONFIG_BLUFI_BROADCAST_OUI
#define CONFIG_BLUFI_BROADCAST_OUI               "MDF" /**< Used to filter other Bluetooth broadcast packets, 3 bytes */
#endif  /**< CONFIG_BLUFI_FILTER_OUI */

#define MDF_EVENT_MCONFIG_BLUFI_STARTED          (MDF_EVENT_MCONFIG_BASE + 0x200)
#define MDF_EVENT_MCONFIG_BLUFI_STOPED           (MDF_EVENT_MCONFIG_BASE + 0x201)
#define MDF_EVENT_MCONFIG_BLUFI_CONNECTED        (MDF_EVENT_MCONFIG_BASE + 0x202)
#define MDF_EVENT_MCONFIG_BLUFI_DISCONNECTED     (MDF_EVENT_MCONFIG_BASE + 0x203)
#define MDF_EVENT_MCONFIG_BLUFI_STA_CONNECTED    (MDF_EVENT_MCONFIG_BASE + 0x204)
#define MDF_EVENT_MCONFIG_BLUFI_STA_DISCONNECTED (MDF_EVENT_MCONFIG_BASE + 0x205)
#define MDF_EVENT_MCONFIG_BLUFI_FINISH           (MDF_EVENT_MCONFIG_BASE + 0x206)
#define MDF_EVENT_MCONFIG_BLUFI_RECV             (MDF_EVENT_MCONFIG_BASE + 0x207)

/**
 * @brief Bluetooth configuration network related configuration
 */
typedef struct {
    char name[MCONFIG_BLUFI_NAME_SIZE];             /**< Local device & peripheral name,
                                                         If the length of name is greater than 10 bytes, it will overwrite custom_data, and custom_data will not be available.*/
    uint16_t company_id;                            /**< Company Identifiers (https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers) */
    uint16_t tid;                                   /**< Type of device */
    uint8_t custom_size;                            /**< Custom data size */
    uint8_t custom_data[MCONFIG_BLUFI_CUSTOM_SIZE]; /**< Placed in a Bluetooth broadcast package */
    bool only_beacon;                               /**< Send only beacon does not support connection */
} mconfig_blufi_config_t;

/**
 * @brief Mconfig_blufi event callback parameters
 */
typedef struct {
    uint8_t *data; /**< Custom data value */
    size_t size;   /**< The length of custom data */
} mconfig_blufi_data_t;

/**
 * @brief  initialize Bluetooth network configuratxion
 *
 * @attention The BLE stack must be enabled first through menuconfig configuration.
 *
 * @return
 *     - MDF_OK
 *     - MDF_ERR_INVALID_ARG
 *     - MDF_FAIL
 */
mdf_err_t mconfig_blufi_init(const mconfig_blufi_config_t *config);

/**
 * @brief  de-initialize Bluetooth network configuration
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t mconfig_blufi_deinit(void);

/**
 * @brief  This function is called to custom data,
 *         send a custom request to the APP to verify the device
 *
 * @param  data  Custom data value
 * @param  size  The length of custom data
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 *     - MDF_ERR_INVALID_ARG
 */
mdf_err_t mconfig_blufi_send(uint8_t *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif /**< __MCONFIG_BLUFI_H__ */
