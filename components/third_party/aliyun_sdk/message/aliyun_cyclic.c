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


#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "aliyun_platform.h"
#include "aliyun_cyclic.h"

static const char *TAG                          = "aliyun_cyclic";

typedef struct aliyun_cyclic {
    uint32_t                            cyclic_num;  //
    uint32_t                            delay_ms;    // 发现间隔
    int64_t                            timestamp;   // 时间戳
} aliyun_cyclic_t;

static aliyun_cyclic_t g_aliyun_cyclic[ALIYUN_CYCLIC_TYPE_MAX] = {0};
static SemaphoreHandle_t g_aliyun_cyclic_lock = NULL;

static mdf_err_t aliyun_cyclic_mutex_lock(void)
{
    if (g_aliyun_cyclic_lock == NULL) {
        g_aliyun_cyclic_lock = xSemaphoreCreateMutex();
        configASSERT(g_aliyun_cyclic_lock);
    }

    return xSemaphoreTake(g_aliyun_cyclic_lock, 10000 / portTICK_RATE_MS) != pdPASS ? MDF_FAIL : MDF_OK;
}

static mdf_err_t aliyun_cyclic_mutex_unlock(void)
{
    if (g_aliyun_cyclic_lock == NULL) {
        g_aliyun_cyclic_lock = xSemaphoreCreateMutex();
        configASSERT(g_aliyun_cyclic_lock);
    }

    return xSemaphoreGive(g_aliyun_cyclic_lock) != pdPASS ? MDF_FAIL : MDF_OK;
}

mdf_err_t aliyun_cyclic_config(aliyun_cyclic_type_t type, uint32_t cyclic_num, uint32_t delay_ms)
{
    mdf_err_t ret = MDF_FAIL;
    ret = aliyun_cyclic_mutex_lock();
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_cyclic_mutex_lock error");
    g_aliyun_cyclic[type].cyclic_num = cyclic_num;
    g_aliyun_cyclic[type].delay_ms = delay_ms;
    g_aliyun_cyclic[type].timestamp = aliyun_platform_get_utc_ms() + delay_ms;
    ret = aliyun_cyclic_mutex_unlock();
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_cyclic_mutex_unlock error");
    return MDF_OK;
}

mdf_err_t aliyun_cyclic_check_type(aliyun_cyclic_type_t type)
{
    mdf_err_t ret = MDF_FAIL;
    ret = aliyun_cyclic_mutex_lock();
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_cyclic_mutex_lock error");

    if (g_aliyun_cyclic[type].cyclic_num > 0) {
        if (g_aliyun_cyclic[type].timestamp <  aliyun_platform_get_utc_ms()) {
            g_aliyun_cyclic[type].timestamp += g_aliyun_cyclic[type].delay_ms;
            g_aliyun_cyclic[type].cyclic_num --;
            aliyun_cyclic_mutex_unlock();
            return MDF_OK;
        }
    }

    aliyun_cyclic_mutex_unlock();
    return MDF_FAIL;
}
