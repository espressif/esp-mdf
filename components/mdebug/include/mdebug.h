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
#ifndef __MDF_DEBUG_H__
#define __MDF_DEBUG_H__

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

#include "mdf_common.h"
#include "mdebug_console.h"
#include "mdebug_espnow.h"
#include "mdebug_flash.h"
#include "mdebug_log.h"

/**
 *@brief Configuration mdebug print enable, whether the output information according to the client needs to know.
 *       please assign CONFIG_MDEBUG_PRINTF_ENABLE a value.
 *
 * @note CONFIG_MDEBUG_PRINTF_ENABLE = 1  enable
 *       CONFIG_MDEBUG_PRINTF_ENABLE = 0  disable
 */
// #define CONFIG_MDEBUG_PRINTF_ENABLE
#ifdef CONFIG_MDEBUG_PRINTF_ENABLE
#define MDEBUG_PRINTF(fmt, ...) printf("D [%s, %d]: " fmt, TAG, __LINE__, __VA_ARGS__)
#else
#define MDEBUG_PRINTF(fmt, ...)
#endif

#define MDEBUG_ADDR_IS_EMPTY(addr) (((addr)[0] | (addr)[1] | (addr)[2] | (addr)[3] | (addr)[4] | (addr)[5]) == 0x0)

#define MDF_EVENT_MDEBUG_FLASH_FULL (MDF_EVENT_MDEBUG_BASE + 0x00)

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __MDF_DEBUG_H__ */
