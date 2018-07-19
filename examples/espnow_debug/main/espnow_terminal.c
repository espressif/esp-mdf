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

#include "esp_wifi.h"
#include "mdns.h"
#include "tcpip_adapter.h"

#include "mdf_common.h"
#include "espnow_lcd.h"
#include "espnow_sdcard.h"
#include "espnow_console.h"

static const char *TAG = "espnow_term";

#define WIFI_CONNECTED_BIT     BIT0

static bool g_is_wifi_connected              = false;
static EventGroupHandle_t g_wifi_event_group = NULL;

bool is_wifi_connected(void)
{
    return g_is_wifi_connected;
}

static void connect_to_default_ap(void)
{
#ifdef CONFIG_AUTO_CONNECT_TO_DEFAULT_AP
    wifi_config_t wifi_config = {
        .sta = {
            .ssid     = CONFIG_DEFAULT_ROUTER_SSID,
            .password = CONFIG_DEFAULT_ROUTER_PASSWD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());
#endif
}

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    MDF_LOGI("event->event_id: %d", event->event_id);
    system_event_sta_disconnected_t *disconnected = NULL;

    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            MDF_LOGI("SYSTEM_EVENT_STA_START");
            connect_to_default_ap();
            break;

        case SYSTEM_EVENT_STA_CONNECTED:
            MDF_LOGI("SYSTEM_EVENT_STA_CONNECTED");
            break;

        case SYSTEM_EVENT_STA_GOT_IP:
            MDF_LOGI("SYSTEM_EVENT_STA_GOT_IP");
            g_is_wifi_connected = true;
            xEventGroupSetBits(g_wifi_event_group, WIFI_CONNECTED_BIT);
            break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
            g_is_wifi_connected = false;
            disconnected = &event->event_info.disconnected;
            MDF_LOGW("SYSTEM_EVENT_STA_DISCONNECTED, reason: %d, free_heap: %d",
                     disconnected->reason, esp_get_free_heap_size());

            /**< This is a workaround as ESP32 WiFi libs don't currently
               auto-reassociate. */
            esp_wifi_connect();
            xEventGroupClearBits(g_wifi_event_group, WIFI_CONNECTED_BIT);
            break;

        default:
            break;
    }

    mdns_handle_system_event(ctx, event);
    return ESP_OK;
}

static esp_err_t espnow_wifi_init(void)
{
    if (!g_wifi_event_group) {
        g_wifi_event_group = xEventGroupCreate();
    }

    tcpip_adapter_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));/**< in case device in powe save mode */
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    return ESP_OK;
}

static void espnow_mdns_task(void *param)
{
    esp_err_t ret              = ESP_OK;
    char *mesh_info            = mdf_calloc(1, 64);
    mdns_result_t *mdns_result = NULL;

    while (1) {
        xEventGroupWaitBits(g_wifi_event_group, WIFI_CONNECTED_BIT,
                            false, true, portMAX_DELAY);

        ret = mdns_query_ptr("_mesh-http", "_tcp", 5000, 3, &mdns_result);
        MDF_ERROR_CONTINUE(ret || !mdns_result, "mdns_query_ptr, ret: %d, mdns_result: %p", ret, mdns_result);
        MDF_ERROR_CONTINUE(!mdns_result->txt, "mdns_result->txt is NULL");

        MDF_LOGI("hostname: %s, instance: %s, addr: "IPSTR", port: %u",
                 (mdns_result->hostname) ? mdns_result->hostname : "null",
                 (mdns_result->instance_name) ? mdns_result->instance_name : "null",
                 IP2STR(&mdns_result->addr->addr.u_addr.ip4),
                 mdns_result->port);
        MDF_LOGI("txt key: %s, value: %s, free heap: %d",
                 mdns_result->txt->key, mdns_result->txt->value, esp_get_free_heap_size());

#ifdef CONFIG_SHOW_IN_LCD
        sprintf(mesh_info, "%s=%s", mdns_result->txt->key, mdns_result->txt->value);
        espnow_lcd_info_update((uint8_t *)&mdns_result->addr->addr.u_addr.ip4, mesh_info);
#endif /**< CONFIG_SHOW_IN_LCD */

        mdns_query_results_free(mdns_result);

        vTaskDelay(10 * 1000 / portTICK_RATE_MS);
    }

    mdf_free(mesh_info);
    vTaskDelete(NULL);
}

extern esp_err_t espnow_handle_init(void);
void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());

#ifdef CONFIG_SHOW_IN_LCD
    espnow_lcd_init();
#endif /**< CONFIG_SHOW_IN_LCD */

#ifdef CONFIG_SAVE_TO_SDCARD
    esp_err_t ret = espnow_sd_init();
    MDF_ERROR_CHECK(ret < 0, ; , "espnow_sd_init fail, ret:%d", ret);
#endif /**< CONFIG_SAVE_TO_SDCARD */

    /**< init mdns before wifi init */
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set("espnow_term_hostname"));

    ESP_ERROR_CHECK(espnow_wifi_init());
    ESP_ERROR_CHECK(espnow_handle_init());

    espnow_console_init();

    /* disable all logs except console log, espnow_term warn log and log received from DUT via espnow. */
    esp_log_level_set("*", ESP_LOG_NONE);
    esp_log_level_set("espnow_term", ESP_LOG_WARN);
    esp_log_level_set("espnow_recv_handle", ESP_LOG_WARN);
    esp_log_level_set("espnow_cmd", ESP_LOG_INFO);
    esp_log_level_set("dut_recv", ESP_LOG_INFO);
    esp_log_level_set("dut_core_dump", ESP_LOG_INFO);
    esp_log_level_set("espnow_sdcard", ESP_LOG_INFO);

    /**< start mdns task to find mesh network */
    xTaskCreate(espnow_mdns_task, "espnow_mdns_task", 1024 * 3, NULL, 5, NULL);
}
