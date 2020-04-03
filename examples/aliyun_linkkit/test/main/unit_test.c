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

static esp_err_t event_handler(void* ctx, system_event_t* event)
{
    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
        MDF_LOGI("wifi sta mode is running");
        MDF_ERROR_ASSERT(esp_wifi_connect());
        break;

    case SYSTEM_EVENT_STA_STOP:
        MDF_LOGI("wifi sta mode is stoped");
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
        MDF_LOGI("wifi sta got ip");
        MDF_LOGI("net address: %s", ip4addr_ntoa(&(event->event_info.got_ip.ip_info.ip)));
        MDF_LOGI("net mask: %s", ip4addr_ntoa(&(event->event_info.got_ip.ip_info.netmask)));
        MDF_LOGI("net gateway: %s", ip4addr_ntoa(&(event->event_info.got_ip.ip_info.gw)));
        xEventGroupSetBits(g_user_event, NETWORK_CONNECTED);
        break;

    case SYSTEM_EVENT_STA_LOST_IP:
        MDF_LOGI("wifi sta lost ip");
        xEventGroupClearBits(g_user_event, NETWORK_CONNECTED);
        break;

    default:
        break;
    }

    return ESP_OK;
}

static mdf_err_t wifi_init()
{
    mdf_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        MDF_ERROR_ASSERT(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    MDF_ERROR_ASSERT(ret);

    tcpip_adapter_init();
    MDF_ERROR_ASSERT(esp_event_loop_init(event_handler, NULL));
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