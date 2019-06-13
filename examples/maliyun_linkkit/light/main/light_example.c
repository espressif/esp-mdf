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

#include "mdf_common.h"
#include "mwifi.h"
#include "mupgrade.h"

#include "mespnow.h"
#include "mconfig_blufi.h"
#include "mconfig_chain.h"

#include "light_driver.h"

#include "maliyun_linkkit_gateway.h"
#include "maliyun_linkkit_subdevice.h"

#include "util.h"

static const char *TAG              = "main";

#define MALIYUN_LINKKIT_NVS_CONFIG  "ap_config"
#define LIGHT_LOGIN_TIMEOUT_MS  60000
#define LINKKIT_LIGHT_MODE_COLOR 4
#define LINKKIT_LIGHT_TYPE_RGBCW 5

static light_driver_config_t driver_config = {
    .gpio_red        = CONFIG_LIGHT_GPIO_RED,
    .gpio_green      = CONFIG_LIGHT_GPIO_GREEN,
    .gpio_blue       = CONFIG_LIGHT_GPIO_BLUE,
    .gpio_cold       = CONFIG_LIGHT_GPIO_COLD,
    .gpio_warm       = CONFIG_LIGHT_GPIO_WARM,
    .fade_period_ms  = CONFIG_LIGHT_FADE_PERIOD_MS,
    .blink_period_ms = CONFIG_LIGHT_BLINK_PERIOD_MS,
};

static mdf_err_t maliyun_linkkit_login_cb(const int status, const char *product_key, const char *device_name)
{
    MDF_LOGI("maliyun linkkit login status:%d, product_key:%s, device_name:%s", status, product_key, device_name);
    return MDF_OK;
}

static mdf_err_t maliyun_linkkit_logout_cb(const int status, const char *product_key, const char *device_name)
{
    MDF_LOGI("maliyun linkkit logout status:%d, product_key:%s, device_name:%s", status, product_key, device_name);
    return MDF_OK;
}

static mdf_err_t maliyun_linkkit_gateway_status_cb(const char *status, size_t status_len)
{
    MDF_LOGI("maliyun linkkit connect status: %.*s", status_len, status);
    return MDF_OK;
}

static mdf_err_t maliyun_linkkit_propery_set_cb(char *request, int request_len)
{
    MDF_LOGI("maliyun_linkkit_propery_set_cb: %.*s", request_len, request);

    cJSON *pJson  = NULL;
    cJSON *pnext  = NULL;
    cJSON *psub   = NULL;

    pJson = cJSON_Parse(request);
    MDF_ERROR_CHECK(!pJson, ESP_FAIL, "cJSON_Parse");

    if (NULL != (pnext = cJSON_GetObjectItem(pJson, "LightSwitch"))) {
        light_driver_set_switch(pnext->valueint);
    } else if (NULL != (pnext = cJSON_GetObjectItem(pJson, "Brightness"))) {
        light_driver_set_brightness(pnext->valueint);
    } else if (NULL != (pnext = cJSON_GetObjectItem(pJson, "LightMode"))) {
        //
    } else if (NULL != (pnext = cJSON_GetObjectItem(pJson, "LightType"))) {
        //
    } else if (NULL != (pnext = cJSON_GetObjectItem(pJson, "ColorTemperature"))) {
        light_driver_set_color_temperature((pnext->valueint - 2000 )/50);
    } else if (NULL != (pnext = cJSON_GetObjectItem(pJson, "HSVColor"))) {
        psub = cJSON_GetObjectItem(pnext, "Hue");
        light_driver_set_hue(psub->valueint);
        psub = cJSON_GetObjectItem(pnext, "Saturation");
        light_driver_set_saturation(psub->valueint);
        psub = cJSON_GetObjectItem(pnext, "Value");
        light_driver_get_value(psub->valueint);
    } else if (NULL != (pnext = cJSON_GetObjectItem(pJson, "RGBColor"))) {
        psub = cJSON_GetObjectItem(pnext, "Red");
        uint8_t Red = psub->valueint;
        psub = cJSON_GetObjectItem(pnext, "Green");
        uint8_t Green = psub->valueint;
        psub = cJSON_GetObjectItem(pnext, "Blue");
        uint8_t Blue = psub->valueint;
        light_driver_set_rgb(Red, Green, Blue);
    } else {
        MDF_LOGI("propery set Key not found");
    }

    /**< TODO FREE()*/
    cJSON_Delete(pJson);
    return MDF_OK;
}

static mdf_err_t maliyun_linkkit_report_reply_cb(int msgid, int code, const char *reply,
        int reply_len)
{
    MDF_LOGI("maliyun linkkit report reply,  msgid:%d, code:%d", msgid, code);

    if (reply_len > 0) {
        MDF_LOGI("reply:%s,reply_len:%d", reply, reply_len);
    }

    return MDF_OK;
}

static mdf_err_t light_post_property_cloud(void)
{
    mdf_err_t ret = MDF_OK;
    cJSON *pJson  = NULL;
    char *info_data = NULL;

    pJson = cJSON_CreateObject();
    MDF_ERROR_GOTO(pJson == NULL, JSON_EXIT, "cJSON_CreateObject pJson Error");

    cJSON_AddBoolToObject(pJson, "LightSwitch", light_driver_get_switch());
    cJSON_AddNumberToObject(pJson, "Brightness", light_driver_get_brightness());
    cJSON_AddNumberToObject(pJson, "ColorTemperature", light_driver_get_color_temperature() * 50 + 2000);
    cJSON_AddNumberToObject(pJson, "LightMode", LINKKIT_LIGHT_MODE_COLOR);
    cJSON_AddNumberToObject(pJson, "LightType", LINKKIT_LIGHT_TYPE_RGBCW);

    cJSON      *item =  cJSON_CreateObject();
    MDF_ERROR_GOTO(item == NULL, JSON_EXIT, "cJSON_CreateObject item Error");

    cJSON_AddNumberToObject(item, "Hue", light_driver_get_hue());
    cJSON_AddNumberToObject(item, "Saturation", light_driver_get_saturation());
    cJSON_AddNumberToObject(item, "Value", light_driver_get_value());

    cJSON_AddItemToObject(pJson, "HSVColor", item);

    cJSON_AddNumberToObject(pJson, "layer", esp_mesh_get_layer());

    info_data = cJSON_PrintUnformatted(pJson);
    MDF_ERROR_GOTO(info_data == NULL, JSON_EXIT, "cJSON_PrintUnformatted Error");

    ret = maliyun_linkkit_report(MALIYUN_LINKKIT_MSG_POST_PROPERTY, (uint8_t *)info_data, strlen(info_data));
    MDF_LOGI("maliyun_linkkit_report buf:%s", info_data);

    MDF_FREE(info_data);
    cJSON_Delete(pJson);

    return ret;

JSON_EXIT:

    if (info_data != NULL) {
        MDF_FREE(info_data);
    }

    if (pJson != NULL) {
        cJSON_Delete(pJson);
    }

    return MDF_FAIL;

}


static void maliyun_light_publish_task(void *arg)
{
    MDF_LOGI("maliyun_light_publish_task is running");
    mdf_err_t ret = MDF_FAIL;
    uint32_t timeout_multiple = 0;
    for (;;) {
        while (!mwifi_is_connected() || !maliyun_linkkit_get_gateway_status() || maliyun_linkkit_get_login_status()) {
            vTaskDelay(500 / portTICK_RATE_MS);
        }

        MDF_LOGI("aliyun_linkkit_login, delay ...");
        ret = maliyun_linkkit_login(LIGHT_LOGIN_TIMEOUT_MS);

        if (ret != ESP_OK) {
            MDF_LOGI("aliyun_linkkit_login timeout");
            if(ret != ESP_ERR_TIMEOUT){
                vTaskDelay(LIGHT_LOGIN_TIMEOUT_MS * timeout_multiple ++);
            }
            if(timeout_multiple > 3 ){
                light_driver_set_rgb(0, 0, 255);
            }
            continue;
        }

        MDF_LOGI("aliyun_linkkit_login success");
        timeout_multiple = 0;
        ret = light_post_property_cloud();

        if (ret != ESP_OK) {
            MDF_LOGW("light_post_property_cloud error");
        }
    }

    MDF_LOGW("maliyun_light_publish_task is exit");
    vTaskDelete(NULL);
}

static mdf_err_t event_loop_cb(mdf_event_loop_t event, void *ctx)
{
    MDF_LOGI("event_loop_cb, event: 0x%x", event);

    mdf_err_t ret = ESP_OK;

    switch (event) {
        case MDF_EVENT_MWIFI_STARTED: {
            MDF_LOGI("MESH is started");
            break;
        }

        case MDF_EVENT_MWIFI_PARENT_CONNECTED: {
            MDF_LOGI("Parent is connected on station interface");
            ret = light_driver_breath_stop();
            MDF_ERROR_CHECK(ret != ESP_OK, ret, "light driver breath stop error !!");
            break;
        }

        case MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE: {
            if (esp_mesh_is_root()) {
                ret = maliyun_linkkit_gateway_refresh_subdevice();
                MDF_ERROR_CHECK(ret != ESP_OK, ret, "maliyun linkkit gateway refresh subdevice error !!");
            }

            break;
        }

        case MDF_EVENT_MWIFI_ROOT_GOT_IP: {
            MDF_LOGI("Root obtains the IP address");
            ret = maliyun_linkkit_gateway_init();
            MDF_ERROR_CHECK(ret != ESP_OK, ret, "maliyun linkkit gateway init error !!");
            break;
        }

        case MDF_EVENT_MWIFI_ROOT_LOST_IP: {
            MDF_LOGI("Root loses the IP address");
            ret = maliyun_linkkit_gateway_deinit();
            MDF_ERROR_CHECK(ret != ESP_OK, ret, "maliyun linkkit gateway deinit error !!");
            break;
        }

        case MDF_EVENT_MUPGRADE_STARTED: {
            MDF_LOGD("Enter upgrade mode");
            light_driver_breath_start(0, 0, 128); /**< blue blink */
            vTaskDelay(3000 / portTICK_RATE_MS);
            light_driver_breath_stop();
            break;
        }

        case MDF_EVENT_MUPGRADE_FINISH: {
            MDF_LOGD("Upgrade completed waiting for restart");
            light_driver_breath_start(0, 0, 255); /**< blue blink */
            break;
        }

        case MDF_EVENT_MWIFI_NETWORK_STATE: {
            MDF_LOGE("mesh event network state");
            ret = maliyun_linkkit_set_gateway_status(!((mesh_event_info_t *)ctx)->network_state.is_rootless);
            MDF_ERROR_CHECK(ret != ESP_OK, ret, "maliyun linkkit set gateway status error !!");
            break;
        }

        default:
            break;
    }

    return MDF_OK;
}

void app_main()
{

    mwifi_config_t ap_config            = {0x0};
    mwifi_init_config_t init_config     = MWIFI_INIT_CONFIG_DEFAULT();

    /**
     * @brief Set the log level for serial port printing.
     */
    //esp_log_level_set("*", ESP_LOG_DEBUG);
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    /**
     * @brief maliyun linkkit set the log level for serial port printing .
     */
    IOT_SetLogLevel(IOT_LOG_WARNING);

    /**
     * @brief Initialize wifi
     */
    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));
    MDF_ERROR_ASSERT(wifi_init());
    MDF_ERROR_ASSERT(mespnow_init());

    /**
     * @brief Initialize device
     */
    MDF_ERROR_ASSERT(light_driver_init(&driver_config));

    /**
     * @brief Continuous power off and restart more than three times to reset the device
     */
    if (restart_count_get() >= MALIYUN_LINKKIT_RESTART_COUNT_RESET) {
        MDF_LOGW("Erase information saved in flash");
        mdf_info_erase(MDF_SPACE_NAME);
    }

    /**
     * @brief Restart is exception
     */
    if (restart_is_exception()) {
        MDF_LOGE("system restart is exception !!! ");
        light_driver_breath_start(255, 0, 0); /**< red blink */
    }

    /**
     * @brief show system info
     */
    TimerHandle_t timer = xTimerCreate("show_system_info", 30000 / portTICK_RATE_MS,
                                       true, NULL, show_system_info_timercb);
    xTimerStart(timer, 0);

    /**
     * @brief mdf check network config
     */
    if (mdf_info_load(MALIYUN_LINKKIT_NVS_CONFIG, &ap_config, sizeof(mwifi_config_t)) == MDF_OK) {
        MDF_LOGW("mesh id:"MACSTR, MAC2STR(ap_config.mesh_id));
        MDF_ERROR_ASSERT(mwifi_init(&init_config));
        MDF_ERROR_ASSERT(mwifi_set_config(&ap_config));
        MDF_ERROR_ASSERT(mwifi_start());
        light_driver_set_switch(true);
    } else {
        MDF_LOGI("light awss config network !!!");

        char product_key[64] = {0};
        char product_secret[64] = {0};
        char device_name[64] = {0};

        HAL_GetProductKey(product_key);
        HAL_GetProductSecret(product_secret);
        HAL_GetDeviceName(device_name);

        MDF_LOGI("product_key:%s", product_key);
        MDF_LOGI("product_secret:%s", product_secret);
        MDF_LOGI("device_name:%s", device_name);

        light_driver_breath_start(255, 255, 0); /**< yellow blink */
        maliyun_linkkit_start_awss();
        mwifi_get_config(&ap_config);
        mdf_info_save(MALIYUN_LINKKIT_NVS_CONFIG, &ap_config, sizeof(mwifi_config_t));
        maliyun_linkkit_reset(60000);
    }

    maliyun_linkkit_subscribe_set(MALIYUN_LINKKIT_SUB_LOGIN, maliyun_linkkit_login_cb);
    maliyun_linkkit_subscribe_set(MALIYUN_LINKKIT_SUB_LOGOUT, maliyun_linkkit_logout_cb);
    maliyun_linkkit_subscribe_set(MALIYUN_LINKKIT_SUB_GATEWAY_STATUS, maliyun_linkkit_gateway_status_cb);
    maliyun_linkkit_subscribe_set(MALIYUN_LINKKIT_SUB_PROPERTY_SET, maliyun_linkkit_propery_set_cb);
    maliyun_linkkit_subscribe_set(MALIYUN_LINKKIT_SUB_REPORT_REPLY, maliyun_linkkit_report_reply_cb);
    xTaskCreate(&maliyun_light_publish_task, "light_publish_task", 4 * 1024, NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
    maliyun_linkkit_task_init();
}

