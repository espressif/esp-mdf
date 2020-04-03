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
#include "aliyun_answer.h"

static const char *TAG                          = "aliyun_answer";

typedef struct aliyun_answer {
    uint8_t                             resends_flag;
    uint8_t                             resends_num;
    aliyun_answer_status_t              status;
    uint32_t                            msg_id;
    int64_t                            timestamp;
} aliyun_answer_t;

static aliyun_answer_t g_aliyun_answer[ALIYUN_ANSWER_TYPE_MAX] = {0};
static SemaphoreHandle_t g_aliyun_answer_lock = NULL;

static mdf_err_t aliyun_answer_mutex_lock(void)
{
    if (g_aliyun_answer_lock == NULL) {
        g_aliyun_answer_lock = xSemaphoreCreateMutex();
        configASSERT(g_aliyun_answer_lock);
    }

    return xSemaphoreTake(g_aliyun_answer_lock, 10000 / portTICK_RATE_MS) != pdPASS ? MDF_FAIL : MDF_OK;
}

static mdf_err_t aliyun_answer_mutex_unlock(void)
{
    if (g_aliyun_answer_lock == NULL) {
        g_aliyun_answer_lock = xSemaphoreCreateMutex();
        configASSERT(g_aliyun_answer_lock);
    }

    return xSemaphoreGive(g_aliyun_answer_lock) != pdPASS ? MDF_FAIL : MDF_OK;
}


mdf_err_t aliyun_answer_set_success(aliyun_answer_type_t type, uint32_t msg_id)
{
    mdf_err_t ret = MDF_FAIL;
    ret = aliyun_answer_mutex_lock();
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_answer_mutex_lock error");
    g_aliyun_answer[type].msg_id = msg_id;
    g_aliyun_answer[type].status = ALIYUN_ANSWER_STATUS_SUCC;
    g_aliyun_answer[type].timestamp = aliyun_platform_get_utc_ms();
    ret = aliyun_answer_mutex_unlock();
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_answer_mutex_unlock error");
    return MDF_OK;
}

mdf_err_t aliyun_answer_set_wait(aliyun_answer_type_t type, uint32_t msg_id)
{
    mdf_err_t ret = MDF_FAIL;
    ret = aliyun_answer_mutex_lock();
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_answer_mutex_lock error");

    if (!g_aliyun_answer[type].resends_flag) {
        g_aliyun_answer[type].resends_num = CONFIG_ALIYUN_ANSWER_DEFAULT_RESENDS_NUM;
    }

    g_aliyun_answer[type].resends_flag = false;
    g_aliyun_answer[type].msg_id = msg_id;
    g_aliyun_answer[type].status = ALIYUN_ANSWER_STATUS_WAIT;
    g_aliyun_answer[type].timestamp = aliyun_platform_get_utc_ms() + CONFIG_ALIYUN_ANSWER_DEFAULT_TIMEOUT_MS;
    ret = aliyun_answer_mutex_unlock();
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_answer_mutex_unlock error");
    return MDF_OK;
}

mdf_err_t aliyun_answer_check_timeout(aliyun_answer_type_t type, uint32_t msg_id)
{
    mdf_err_t ret = MDF_FAIL;
    ret = aliyun_answer_mutex_lock();
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_answer_mutex_lock error");

    if (g_aliyun_answer[type].status ==  ALIYUN_ANSWER_STATUS_WAIT) {
        if (g_aliyun_answer[type].timestamp >= aliyun_platform_get_utc_ms()) {
            ret = MDF_OK;
        } else {
            g_aliyun_answer[type].status = ALIYUN_ANSWER_STATUS_TIMEOUT;
            MDF_LOGI("aliyun_platform_get_utc_ms:%lld", aliyun_platform_get_utc_ms());
            ret = MDF_FAIL;
        }
    } else {
        ret = MDF_FAIL;
    }

    aliyun_answer_mutex_unlock();
    return ret;
}

mdf_err_t aliyun_answer_check_resends(aliyun_answer_type_t type)
{
    mdf_err_t ret = MDF_FAIL;
    ret = aliyun_answer_mutex_lock();
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_answer_mutex_lock error");

    if (g_aliyun_answer[type].status ==  ALIYUN_ANSWER_STATUS_TIMEOUT  ||
            (g_aliyun_answer[type].status ==  ALIYUN_ANSWER_STATUS_WAIT &&
             g_aliyun_answer[type].timestamp <= aliyun_platform_get_utc_ms())) {
        if (g_aliyun_answer[type].resends_num > 0) {
            g_aliyun_answer[type].status = ALIYUN_ANSWER_STATUS_WAIT;
            g_aliyun_answer[type].timestamp = aliyun_platform_get_utc_ms() + CONFIG_ALIYUN_ANSWER_DEFAULT_TIMEOUT_MS;
            g_aliyun_answer[type].resends_num --;
            ret = MDF_FAIL;
        } else {
            g_aliyun_answer[type].status = ALIYUN_ANSWER_STATUS_FAIL;
            ret = MDF_OK;
        }
    } else {
        ret = MDF_OK;
    }

    aliyun_answer_mutex_unlock();
    return ret;
}

mdf_err_t aliyun_answer_set_resends_flag(aliyun_answer_type_t type)
{
    mdf_err_t ret = MDF_FAIL;
    ret = aliyun_answer_mutex_lock();
    g_aliyun_answer[type].resends_flag = true;
    ret = aliyun_answer_mutex_unlock();
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_answer_mutex_unlock error");
    return MDF_OK;
}

mdf_err_t aliyun_answer_update_utc_ms(int64_t utc_ms)
{
    mdf_err_t ret = MDF_FAIL;
    ret = aliyun_answer_mutex_lock();

    for (int i = 0; i <  ALIYUN_ANSWER_TYPE_MAX; i++) {
        g_aliyun_answer[i].timestamp += utc_ms;
    }

    ret = aliyun_answer_mutex_unlock();
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_answer_mutex_unlock error");
    return MDF_OK;
}
