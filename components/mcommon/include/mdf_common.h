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
