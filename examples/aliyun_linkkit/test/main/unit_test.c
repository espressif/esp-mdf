#include <stdio.h>

#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "mdf_common.h"
#include "unity.h"

#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASSWORD CONFIG_WIFI_PSWD

static const char TAG[] = "UNIT_TEST";

static EventGroupHandle_t g_user_event;
#define NETWORK_CONNECTED BIT0

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                MDF_LOGI("wifi sta mode is running");
                MDF_ERROR_ASSERT(esp_wifi_connect());
                break;

            case WIFI_EVENT_STA_STOP:
                MDF_LOGI("wifi sta mode is stoped");
                break;

            default:
                MDF_LOGD("WIFI_EVENT (%d)", event_id);
                break;
        }

    } else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP: {
                ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
                MDF_LOGI("wifi sta got ip");
                MDF_LOGI("net address: " IPSTR, IP2STR(&(event->ip_info.ip)));
                MDF_LOGI("net mask: " IPSTR, IP2STR(&(event->ip_info.netmask)));
                MDF_LOGI("net gateway: " IPSTR, IP2STR(&(event->ip_info.gw)));
                xEventGroupSetBits(g_user_event, NETWORK_CONNECTED);
                break;
            }

            case IP_EVENT_STA_LOST_IP:
                MDF_LOGI("wifi sta lost ip");
                xEventGroupClearBits(g_user_event, NETWORK_CONNECTED);
                break;

            default:
                MDF_LOGD("IP_EVENT (%d)", event_id);
                break;

        }

    } else {
        MDF_LOGE("Unsupportted event base(%s)", event_base);
    }
}

static mdf_err_t wifi_init()
{
    mdf_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        MDF_ERROR_ASSERT(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    MDF_ERROR_ASSERT(ret);

    MDF_ERROR_ASSERT(esp_netif_init());
    MDF_ERROR_ASSERT(esp_event_loop_create_default());
    MDF_ERROR_ASSERT(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    MDF_ERROR_ASSERT(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    if (!esp_netif_create_default_wifi_sta()) {
        MDF_LOGE("Create default wifi sta netif failed");
        return MDF_FAIL;
    }

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    MDF_ERROR_ASSERT(esp_wifi_init(&init_cfg));
    wifi_config_t cfg = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        }
    };
    MDF_ERROR_ASSERT(esp_wifi_set_mode(WIFI_MODE_STA));
    MDF_ERROR_ASSERT(esp_wifi_set_config(ESP_IF_WIFI_STA, &cfg));
    MDF_ERROR_ASSERT(esp_wifi_start());
    MDF_LOGI("connect to wifi(%s,%s)", cfg.sta.ssid, cfg.sta.password);
    return MDF_OK;
}

void app_main()
{
    g_user_event = xEventGroupCreate();
    ESP_ERROR_CHECK(wifi_init());
    EventBits_t bits = xEventGroupWaitBits(g_user_event, NETWORK_CONNECTED, pdFALSE, pdFALSE, pdMS_TO_TICKS(10 * 1000));

    if ((bits & NETWORK_CONNECTED) != NETWORK_CONNECTED) {
        MDF_LOGE("Can not connected Wi-Fi");
        assert(false);
    }

    printf("Running aliyun sdk unit test\n");
    UNITY_BEGIN();
    unity_run_all_tests();
    UNITY_END();
}