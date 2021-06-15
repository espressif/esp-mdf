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
#include "sdkconfig.h"

#ifdef CONFIG_IDF_TARGET_ESP32C3
#include "esp32c3/rom/rtc.h"
#include "esp32c3/rom/crc.h"
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S2
#include "esp32s2/rom/rtc.h"
#include "esp32s2/rom/crc.h"
#endif

#ifdef CONFIG_IDF_TARGET_ESP32
#include "esp32/rom/rtc.h"
#include "esp32/rom/crc.h"
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_timer.h"
#include "esp_partition.h"
#include "esp_event.h"
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
/*
 *@brief Next templates makes gnu builtin: __builtin_types_compatible_p
 *       compatible with C++ compilation
 */
template<typename T, typename U>
struct is_same {
    static const int value = 0;
};

template<typename T>
struct is_same<T, T> {
    static const int value = 1;
};

template<typename T, typename U>
int cxx_is_compatible()
{
    return is_same<T, U>::value;
}

/**
 * @brief Macro adaptation for C++ compilation, using previous C++ templates for type checking
 */
#define builtin_types_compatible_p(data, type)  cxx_is_compatible<decltype(data),type>()
extern "C" {
#else
#define builtin_types_compatible_p(data, type)  __builtin_types_compatible_p(typeof(data), type)
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
