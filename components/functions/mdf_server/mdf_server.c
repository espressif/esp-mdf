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

#include "mdf_http.h"
#include "mdf_json.h"
#include "mdf_common.h"
#include "mdf_http_handle.h"
#include "mdf_notice.h"
#include "mdf_openssl.h"
#include "mdf_socket.h"

static const char *TAG                          = "mdf_server";
static bool g_server_conn_flag                  = false;
static TaskHandle_t g_http_response_task_handle = NULL;

bool mdf_server_conn_is_running()
{
    return esp_mesh_is_root() && mdf_wifi_mesh_is_connect() && g_server_conn_flag;
}

void mdf_http_request_task(void *arg)
{
    esp_err_t ret = ESP_OK;

    while (mdf_server_conn_is_running()) {
        ret = mdf_http_server_request();
        MDF_ERROR_CONTINUE(ret != ESP_OK, "mdf_http_request, ret: %d", ret);
    }

    vTaskDelete(NULL);
}

void mdf_http_response_task(void *arg)
{
    esp_err_t ret = ESP_OK;

    while (esp_mesh_is_root()) {
        ret = mdf_http_server_response();
        MDF_ERROR_CONTINUE(ret != ESP_OK, "mdf_http_response, ret: %d", ret);
    }

    g_http_response_task_handle = NULL;
    vTaskDelete(NULL);
}

void mdf_notice_udp_server_task(void *arg)
{
    esp_err_t ret = ESP_OK;

    while (mdf_server_conn_is_running()) {
        ret = mdf_notice_udp_server();
        MDF_ERROR_CONTINUE(ret != ESP_OK, "mdf_notice_udp_handle, ret: %d", ret);
    }

    vTaskDelete(NULL);
}

void mdf_notice_udp_client_task(void *arg)
{
    esp_err_t ret = ESP_OK;

    while (mdf_server_conn_is_running()) {
        ret = mdf_notice_udp_client();
        MDF_ERROR_CONTINUE(ret != ESP_OK, "mdf_notice_udp_handle, ret: %d", ret);
    }

    vTaskDelete(NULL);
}

static void mdf_server_init_task(void *arg)
{
    MDF_LOGD("mdf server connect create");

    ESP_ERROR_CHECK(mdf_http_server_init());

#ifdef CONFIG_MDF_USE_MDNS_SERVICE
    ESP_ERROR_CHECK(mdf_notice_mdns_init());
#endif

    ESP_ERROR_CHECK(mdf_notice_udp_init());

    xTaskCreate(mdf_http_request_task, "mdf_http_request", 1024 * 3, NULL,
                MDF_TASK_DEFAULT_PRIOTY - 3, NULL);

    if (!g_http_response_task_handle) {
        xTaskCreate(mdf_http_response_task, "mdf_http_response", 1024 * 3, NULL,
                    MDF_TASK_DEFAULT_PRIOTY, &g_http_response_task_handle);
    }

    xTaskCreate(mdf_notice_udp_client_task, "mdf_udp_client", 1024 * 3, NULL, MDF_TASK_DEFAULT_PRIOTY, NULL);

#ifdef MDF_USE_UDP_SERVER
    xTaskCreate(mdf_notice_udp_server_task, "mdf_udp_server", 1024 * 3, NULL, MDF_TASK_DEFAULT_PRIOTY, NULL);
#endif

    vTaskDelete(NULL);
}

esp_err_t mdf_server_init()
{
    if (esp_mesh_is_root() && mdf_wifi_mesh_is_connect() && !g_server_conn_flag) {
        xTaskCreate(mdf_server_init_task, "mdf_server_init_task", 4096, NULL,
                    MDF_TASK_DEFAULT_PRIOTY, NULL);
        g_server_conn_flag = true;
    }

    return ESP_OK;
}

esp_err_t mdf_server_deinit()
{
    if (g_server_conn_flag) {
        MDF_LOGD("mdf server connect delete");
        g_server_conn_flag = false;
        vTaskDelay(1000 / portTICK_RATE_MS);

        ESP_ERROR_CHECK(mdf_http_server_deinit());

#ifdef CONFIG_MDF_USE_MDNS_SERVICE
        ESP_ERROR_CHECK(mdf_notice_mdns_deinit());
#endif

        ESP_ERROR_CHECK(mdf_notice_udp_deinit());
    }

    return ESP_OK;
}
