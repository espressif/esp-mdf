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

#ifndef __MDF_BLUFI_NETWORK_CONFIG_H__
#define __MDF_BLUFI_NETWORK_CONFIG_H__

#include "mdf_common.h"
#include "mconfig_queue.h"
#include "mconfig_chain.h"
#include "mdf_event_loop.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_MCONFIG_BLUFI_COMPANY_ID
#define CONFIG_MCONFIG_BLUFI_COMPANY_ID (0x02E5)    /**< Espressif Incorporated */
#endif /**< CONFIG_MCONFIG_BLUFI_COMPANY_ID */
#define MCONFIG_BLUFI_COMPANY_ID CONFIG_MCONFIG_BLUFI_COMPANY_ID

#define MCONFIG_BLUFI_NAME_SIZE                  (11) /**< Contains the ending character */
#define MCONFIG_BLUFI_CUSTOM_SIZE                (31) /**< BLE broadcast data packets have a valid length of up to 31 bytes */

#define MDF_EVENT_MCONFIG_BLUFI_STARTED          (MDF_EVENT_MCONFIG_BASE + 0x200)
#define MDF_EVENT_MCONFIG_BLUFI_STOPED           (MDF_EVENT_MCONFIG_BASE + 0x201)
#define MDF_EVENT_MCONFIG_BLUFI_CONNECTED        (MDF_EVENT_MCONFIG_BASE + 0x202)
#define MDF_EVENT_MCONFIG_BLUFI_DISCONNECTED     (MDF_EVENT_MCONFIG_BASE + 0x203)
#define MDF_EVENT_MCONFIG_BLUFI_STA_CONNECTED    (MDF_EVENT_MCONFIG_BASE + 0x204)
#define MDF_EVENT_MCONFIG_BLUFI_STA_DISCONNECTED (MDF_EVENT_MCONFIG_BASE + 0x205)

/**
 * @brief Bluetooth configuration network related configuration
 */
typedef struct {
    char name[MCONFIG_BLUFI_NAME_SIZE];             /**< local device & peripheral name */
    uint16_t tid;                                   /**< Type of device */
    uint8_t custom_size;                            /**< Custom data size */
    uint8_t custom_data[MCONFIG_BLUFI_CUSTOM_SIZE]; /**< Placed in a Bluetooth broadcast package */
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

#endif /**< __MDF_BLUFI_NETWORK_CONFIG_H__ */
