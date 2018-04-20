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

#include "light_driver.h"
#include "mdf_device_handle.h"

static const char *TAG = "light_bulb";

enum light_status_cid {
    STATUS_CID            = 0,
    HUE_CID               = 1,
    SATURATION_CID        = 2,
    VALUE_CID             = 3,
    COLOR_TEMPERATURE_CID = 4,
    BRIGHTNESS_CID        = 5,
};

static esp_err_t light_bulb_set_value(uint8_t cid, int value)
{
    switch (cid) {
        case STATUS_CID:
            light_set_switch(value);
            break;

        case HUE_CID:
            light_set_hue(value);
            break;

        case SATURATION_CID:
            light_set_saturation(value);
            break;

        case VALUE_CID:
            light_set_value(value);
            break;

        case COLOR_TEMPERATURE_CID:
            light_set_color_temperature(value);
            break;

        case BRIGHTNESS_CID:
            light_set_brightness(value);
            break;

        default:
            MDF_LOGE("no support cid: %d", cid);
            return ESP_FAIL;
    }

    MDF_LOGD("cid: %d, value: %d", cid, value);

    return ESP_OK;
}

static esp_err_t light_bulb_get_value(uint8_t cid, int *value)
{
    MDF_PARAM_CHECK(value);

    switch (cid) {
        case STATUS_CID:
            *value = light_get_switch();
            break;

        case HUE_CID:
            *value = light_get_hue();
            break;

        case SATURATION_CID:
            *value = light_get_saturation();
            break;

        case VALUE_CID:
            *value = light_get_value();
            break;

        case COLOR_TEMPERATURE_CID:
            *value = light_get_color_temperature();
            break;

        case BRIGHTNESS_CID:
            *value = light_get_brightness();
            break;

        default:
            MDF_LOGE("no support cid: %d", cid);
            return ESP_FAIL;
    }

    MDF_LOGD("cid: %d, value: %d", cid, *value);

    return ESP_OK;
}

/*************************** event ***************************/
static esp_err_t light_bulb_event_loop_cb(mdf_event_loop_t event, void *ctx)
{
    esp_err_t ret = ESP_OK;

    switch (event) {
        case MDF_EVENT_PARENT_CONNECTED:
            light_blink_stop();
            break;

        case MDF_EVENT_NETWORK_START_CONFIG:
            MDF_LOGI("***********************************");
            MDF_LOGI("*    ENTER CONFIG NETWORK MODE    *");
            MDF_LOGI("***********************************");
            light_blink_start(128, 128, 0, 1); /**< yellow blink */
            break;

        case MDF_EVENT_CONNECT_ROUTER:
            MDF_LOGI("***********************************");
            MDF_LOGI("*  START CONNECTING TO THE ROUTER *");
            MDF_LOGI("***********************************");
            light_blink_start(255, 128, 0, 1); /**< orange blink */
            break;

        case MDF_EVENT_NETWORK_ORGANIZE:
            MDF_LOGI("***********************************");
            MDF_LOGI("*        ORGANIZE NETWORKS        *");
            MDF_LOGI("***********************************");
            light_blink_start(0, 255, 0, 1); /**< green blink */
            break;

        case MDF_EVENT_NETWORK_FAIL:
            MDF_LOGI("***********************************");
            MDF_LOGI("*    ENTER CONFIG NETWORK FAIL    *");
            MDF_LOGI("***********************************");
            break;

        case MDF_EVENT_UPGRADE_START:
            MDF_LOGI("***********************************");
            MDF_LOGI("*      ENTER OTA UPGRADE START    *");
            MDF_LOGI("***********************************");
            light_blink_set(0, 0, 128, 2, 2000); /**< blue blink */
            break;

        case MDF_EVENT_UPGRADE_SUCCESS:
            MDF_LOGI("***********************************");
            MDF_LOGI("*    ENTER OTA UPGRADE SUCCESS    *");
            MDF_LOGI("***********************************");
            light_blink_set(0, 255, 0, 2, 2000); /**< green blink */
            break;

        case MDF_EVENT_UPGRADE_FAIL:
            MDF_LOGI("***********************************");
            MDF_LOGI("*      ENTER OTA_UPGRADE FAIL     *");
            MDF_LOGI("***********************************");
            light_blink_set(255, 0, 0, 2, 2000); /**< red blink */
            break;

        case MDF_EVENT_SYSTEM_RUNNING: {
            uint8_t on = light_get_switch();
            light_set_switch(on);
            break;
        }

        case MDF_EVENT_SYSTEM_RESET:
            MDF_LOGI("***********************************");
            MDF_LOGI("*       RESTORE FACTORY RESET     *");
            MDF_LOGI("***********************************");
            ret = mdf_info_erase(MDF_SPACE_NAME);
            MDF_ERROR_CHECK(ret != 0, ESP_FAIL, "erase mdf config");

            esp_restart();
            break;

        case MDF_EVENT_SYSTEM_REBOOT:
            MDF_LOGI("***********************************");
            MDF_LOGI("*       RESTORE SYSTEM REBOOT     *");
            MDF_LOGI("***********************************");
            esp_restart();
            break;

        case MDF_EVENT_SYSTEM_CRASH:
            MDF_LOGI("***********************************");
            MDF_LOGI("*        RESTORE SYSTEM CRASH     *");
            MDF_LOGI("***********************************");
            light_set_rgb(255, 0, 255); /**< pink */
            break;

        default:
            break;
    }

    return ESP_OK;
}

void app_main()
{
    ESP_ERROR_CHECK(light_init(GPIO_NUM_4, GPIO_NUM_16, GPIO_NUM_5, GPIO_NUM_19, GPIO_NUM_23));

    ESP_ERROR_CHECK(mdf_device_init_config(1, "light", "0.0.1"));

    ESP_ERROR_CHECK(mdf_device_add_characteristics(STATUS_CID, "on", PERMS_READ_WRITE_TRIGGER, 0, 1, 1));
    ESP_ERROR_CHECK(mdf_device_add_characteristics(HUE_CID, "hue", PERMS_READ_WRITE_TRIGGER, 0, 360, 1));
    ESP_ERROR_CHECK(mdf_device_add_characteristics(SATURATION_CID, "saturation", PERMS_READ_WRITE_TRIGGER, 0, 100, 1));
    ESP_ERROR_CHECK(mdf_device_add_characteristics(VALUE_CID, "value", PERMS_READ_WRITE_TRIGGER, 0, 100, 1));
    ESP_ERROR_CHECK(mdf_device_add_characteristics(COLOR_TEMPERATURE_CID, "color_temperature", PERMS_READ_WRITE_TRIGGER, 0, 100, 1));
    ESP_ERROR_CHECK(mdf_device_add_characteristics(BRIGHTNESS_CID, "brightness", PERMS_READ_WRITE_TRIGGER, 0, 100, 1));

    ESP_ERROR_CHECK(mdf_device_init_handle(light_bulb_event_loop_cb, light_bulb_get_value, light_bulb_set_value));
    ESP_ERROR_CHECK(mdf_trigger_init());
}
