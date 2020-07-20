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

#include "esp32/rom/uart.h"

#include "mdf_common.h"
#include "mwifi.h"
#include "mespnow.h"
#include "mdebug.h"

#include "sdcard.h"
#include "debug_recv.h"
#include "debug_cmd.h"

static const char *TAG = "main";

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == SYSTEM_EVENT_STA_DISCONNECTED) {
        MDF_LOGW("ESP32 station disconnected from AP");

        if (!mwifi_is_started()) {
            esp_wifi_connect();
        }

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        MDF_LOGI("ip: "IPSTR, IP2STR(&event->ip_info.ip));
    } else {
        MDF_LOGI("event_base: %s, event_id: %d", event_base, event_id);
    }
}

/**
 * @brief Initialize wifi.
 */
static mdf_err_t wifi_init()
{
    mdf_err_t ret          = nvs_flash_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        MDF_ERROR_ASSERT(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    MDF_ERROR_ASSERT(ret);

    MDF_ERROR_ASSERT(esp_netif_init());
    MDF_ERROR_ASSERT(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    MDF_ERROR_ASSERT(esp_wifi_init(&cfg));
    MDF_ERROR_ASSERT(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    MDF_ERROR_ASSERT(esp_wifi_set_mode(WIFI_MODE_STA));
    MDF_ERROR_ASSERT(esp_wifi_set_ps(WIFI_PS_NONE));
    MDF_ERROR_ASSERT(esp_wifi_start());

    return MDF_OK;
}

static mdf_err_t event_loop_cb(mdf_event_loop_t event, void *ctx)
{
    return MDF_OK;
}

void app_main()
{
    mwifi_init_config_t init_config = MWIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t wifi_config = {0x0};
    mdebug_log_config_t log_config = {
        .log_uart_enable = true,
        .log_espnow_enable = true,
    };

    /**
     * @brief Set the log level for serial port printing.
     */
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mesh", ESP_LOG_ERROR);

    /**
     * @brief   1.Initialize event loop, receive event
     *          2.Initialize wifi with station mode
     *          3.Initialize espnow(ESP-NOW is a kind of connectionless WiFi communication protocol)
     *          4.Initialize ESP-WIFI-MESH network
     */
    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));
    MDF_ERROR_ASSERT(wifi_init());
    MDF_ERROR_ASSERT(mespnow_init());
    MDF_ERROR_ASSERT(mwifi_init(&init_config));
    MDF_ERROR_ASSERT(mdebug_log_set_config(&log_config));

    /**
     * @brief Add debug function, you can use serial command and wireless debugging.
     *      1. Initialize console module
     */
    MDF_ERROR_ASSERT(mdebug_console_init());

    /** Initializing SD card via SDMMC peripheral */
    sdcard_init();

    /** Register some commands */
    if (sdcard_is_mount()) {
        register_sdcard();
        register_wifi_sniffer();
    }

    register_wifi_config();
    register_wifi_scan();
    register_log();
    register_coredump();
    register_command();

    /** Create esp-now receive task to listen esp-now data from other device */
    MDF_ERROR_ASSERT(debug_recv_init());

    MDF_LOGI(" ==================================================");
    MDF_LOGI(" |          Steps to wireless debugging           |");
    MDF_LOGI(" |                                                |");
    MDF_LOGI(" |  1. Print 'help' to gain overview of commands  |");
    MDF_LOGI(" |     mdf> help                                  |");
    MDF_LOGI(" |  2. Configure Wi-Fi                            |");
    MDF_LOGI(" |     mdf> wifi_config -c <channel> or wifi_config -s <ssid> -p <password> |");
    MDF_LOGI(" |  3. Add devices to monitor                     |");
    MDF_LOGI(" |     mdf> log -a <xx:xx:xx:xx:xx:xx>            |");
    MDF_LOGI(" |  4. Use other commands to get the status       |");
    MDF_LOGI(" |                                                |");
    MDF_LOGI(" ==================================================\n");

    if (mdf_info_load("wifi_config", &wifi_config, sizeof(wifi_config_t)) == MDF_OK) {
        if (strlen((char *)wifi_config.sta.ssid)) {
            ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
            ESP_ERROR_CHECK(esp_wifi_connect());
        } else {
            ESP_ERROR_CHECK(esp_wifi_set_channel(wifi_config.sta.channel, WIFI_SECOND_CHAN_NONE));
            MDF_LOGI("Set primary/secondary channel of ESP32, channel: %d", wifi_config.sta.channel);
        }
    } else {
        MDF_LOGW("Wifi is not configured, please set it first");
    }
}
