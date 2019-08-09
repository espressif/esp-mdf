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

#ifndef __MDF_COMMON_H__
#define __MDF_COMMON_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "errno.h"

#include "rom/rtc.h"
#include "rom/crc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_partition.h"
#include "esp_event_loop.h"
#include "esp_http_client.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "driver/i2c.h"
#include "sys/param.h"
#include "driver/gpio.h"

#include "mdf_err.h"
#include "mdf_mem.h"
#include "mdf_event_loop.h"
#include "mdf_info_store.h"

#define MCOMMON_ESPRESSIF_ID        (0x02E5) /**< Espressif Incorporated */

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

/**
 * Get mdf version
 *
 * @return constant string from MDF_VER
 */
#define mdf_get_version()    MDF_VER

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __MDF_COMMON_H__ */
