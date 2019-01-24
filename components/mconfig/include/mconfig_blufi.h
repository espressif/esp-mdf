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

#define MCONFIG_BLUFI_NAME_SIZE                  (11) /**< Contains the ending character */
#define MCONFIG_BLUFI_CUSTOM_SIZE                (31) /**< BLE broadcast data packets have a valid length of up to 31 bytes */

#define MDF_EVENT_MCONFIG_BLUFI_STARTED          (MDF_EVENT_MCONFIG_BASE + 0x200)
#define MDF_EVENT_MCONFIG_BLUFI_STOPED           (MDF_EVENT_MCONFIG_BASE + 0x201)
#define MDF_EVENT_MCONFIG_BLUFI_CONNECTED        (MDF_EVENT_MCONFIG_BASE + 0x202)
#define MDF_EVENT_MCONFIG_BLUFI_DISCONNECTED     (MDF_EVENT_MCONFIG_BASE + 0x203)
#define MDF_EVENT_MCONFIG_BLUFI_STA_CONNECTED    (MDF_EVENT_MCONFIG_BASE + 0x204)
#define MDF_EVENT_MCONFIG_BLUFI_STA_DISCONNECTED (MDF_EVENT_MCONFIG_BASE + 0x205)
#define MDF_EVENT_MCONFIG_BLUFI_FINISH           (MDF_EVENT_MCONFIG_BASE + 0x206)

/**
 * @brief Bluetooth configuration network related configuration
 */
typedef struct {
    char name[MCONFIG_BLUFI_NAME_SIZE];             /**< Local device & peripheral name */
    uint16_t company_id;                            /**< Company Identifiers (https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers) */
    uint16_t tid;                                   /**< Type of device */
    uint8_t custom_size;                            /**< Custom data size */
    uint8_t custom_data[MCONFIG_BLUFI_CUSTOM_SIZE]; /**< Placed in a Bluetooth broadcast package */
    bool only_beacon;                               /**< Send only beacon does not support connection */
} mconfig_blufi_config_t;

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

#ifdef __cplusplus
}
#endif

#endif /**< __MCONFIG_BLUFI_H__ */
