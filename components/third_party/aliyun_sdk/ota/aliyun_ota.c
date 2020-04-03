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
#include "freertos/task.h"

#include "esp_http_client.h"

#include "mdf_mem.h"
#include "aliyun_ota.h"
#include "aliyun_list.h"
#include "aliyun_platform.h"

static const char *TAG  = "aliyun_ota";

static TaskHandle_t g_aliyun_ota_task_handle = NULL;
static aliyun_ota_info_t *g_ota_info = NULL;

static void aliyun_ota_task(void *arg)
{
    //aliyun_ota_info_t *ota_info = (aliyun_ota_info_t *)arg;
    assert(g_ota_info != NULL);

    mdf_err_t ret    = MDF_OK;
    uint8_t *data       = MDF_MALLOC(ALIYUN_OTA_PAYLOAD_MAXLEN);

    size_t total_size   = 0;
    int recv_size       = 0;

    esp_http_client_config_t config = {
        .url            = g_ota_info->url,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
    };

    MDF_LOGI("aliyun OTA starts in 3 seconds ");

    vTaskDelay(pdMS_TO_TICKS(3000));

    MDF_LOGI("aliyun OTA start");

    esp_http_client_handle_t client = esp_http_client_init(&config);
    MDF_ERROR_GOTO(!client, EXIT, "Initialise HTTP connection");

    ret = esp_http_client_open(client, 0);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "esp_http_client_open error, %d", ret);

    total_size = esp_http_client_fetch_headers(client);
    MDF_LOGD("total_size: %d", total_size);

    if (esp_http_client_is_chunked_response(client)) {
        total_size = g_ota_info->size;
    }

    ret = aliyun_platform_ota_start(g_ota_info->version, total_size);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "aliyun_ota_start error");

    for (ssize_t size = 0; recv_size < total_size; recv_size += size) {
        size = esp_http_client_read(client, (char *)data, ALIYUN_OTA_PAYLOAD_MAXLEN);
        MDF_ERROR_GOTO(size < 0, EXIT, "Read data from http stream error, size: %d", size);

        if (size > 0) {
            ret = aliyun_platform_ota_write(data, size);
            MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "Write firmware to flash, ret: %d, size: %d", ret, size);
        } else {
            MDF_LOGW("esp_http_client_read: ret: %d", size);
            goto EXIT;
        }
    }

    MDF_LOGD("version: %s, total_size: %d, recv_size: %d", g_ota_info->version, total_size, recv_size);

    ret = aliyun_platform_ota_stop();
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "aliyun_ota_stop error, %d", ret);

    MDF_LOGI("aliyun ota task exit");

EXIT:
    MDF_FREE(data);
    MDF_FREE(g_ota_info);
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    g_aliyun_ota_task_handle = NULL;
    vTaskDelete(NULL);
}

mdf_err_t aliyun_ota_init(const char *product_key, const char *device_name, aliyun_ota_info_t *ota_info)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);
    MDF_PARAM_CHECK(ota_info);
    MDF_LOGI("In aliyun_ota_init");

    mdf_err_t ret = MDF_OK;

    if (g_ota_info == NULL) {
        g_ota_info = MDF_MALLOC(sizeof(aliyun_ota_info_t));
        memcpy(g_ota_info, ota_info, sizeof(aliyun_ota_info_t));
        ret = aliyun_list_update_ota_type(product_key, device_name, ALIYUN_OTA_TYPE_START);
        MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_service_property_set Error");
    } else {
        MDF_ERROR_CHECK(strcmp(g_ota_info->version, ota_info->version), MDF_FAIL, "ota info version Error");
        MDF_ERROR_CHECK(strcmp(g_ota_info->sign_method, ota_info->sign_method), MDF_FAIL, "ota info sign_method Error");
        MDF_ERROR_CHECK(strcmp(g_ota_info->sign, ota_info->sign), MDF_FAIL, "ota info sign Error");

        ret = aliyun_list_update_ota_type(product_key, device_name, ALIYUN_OTA_TYPE_START);
        MDF_ERROR_CHECK(ret != MDF_OK, ret, "aliyun_parse_thing_service_property_set Error");
    }

    if (g_aliyun_ota_task_handle == NULL) {
        xTaskCreate(aliyun_ota_task, "aliyun_ota_task", 4 * 1024,
                    g_ota_info, CONFIG_ALIYUN_TASK_DEFAULT_PRIOTY, &g_aliyun_ota_task_handle);
    }

    return ret;
}
