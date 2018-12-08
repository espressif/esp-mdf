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

#include "touchpad.h"
#include "mdf_device_handle.h"

static const char *TAG = "touchpad_main";

static esp_err_t touchpad_event_loop_cb(mdf_event_loop_t event, void *ctx)
{
    esp_err_t ret = ESP_OK;

    switch (event) {
        case MDF_EVENT_PARENT_CONNECTED:
            rgb_led_blink_stop();
            rgb_led_set(COLOR_LED_B, 100); /**< blue blink */
            break;

        case MDF_EVENT_NETWORK_START_CONFIG:
            MDF_LOGI("***********************************");
            MDF_LOGI("*      ENTER CONFIG NETWORK       *");
            MDF_LOGI("***********************************");
            rgb_led_blink_start(COLOR_LED_Y, 500);/**< blue blink */

            break;

        case MDF_EVENT_CONNECT_ROUTER:
        case MDF_EVENT_NETWORK_ORGANIZE:
            MDF_LOGI("***********************************");
            MDF_LOGI("NETWORK MODE: %d", event);
            MDF_LOGI("***********************************");
            rgb_led_blink_start(COLOR_LED_B, 500);/**< blue blink */
            break;

        case MDF_EVENT_NETWORK_FAIL:
            MDF_LOGI("***********************************");
            MDF_LOGI("*    ENTER CONFIG NETWORK FAIL    *");
            MDF_LOGI("***********************************");
            rgb_led_blink_start(COLOR_LED_R, 500);/**< red blink */
            break;

        case MDF_EVENT_UPGRADE_START:
            MDF_LOGI("***********************************");
            MDF_LOGI("*      ENTER OTA UPGRADE START    *");
            MDF_LOGI("***********************************");
            rgb_led_blink_start(COLOR_LED_G, 500); /**< green blink */
            break;

        case MDF_EVENT_UPGRADE_SUCCESS:
            MDF_LOGI("***********************************");
            MDF_LOGI("*    ENTER OTA UPGRADE SUCCESS    *");
            MDF_LOGI("***********************************");
            rgb_led_blink_stop();
            break;

        case MDF_EVENT_UPGRADE_FAIL:
            MDF_LOGI("***********************************");
            MDF_LOGI("*      ENTER OTA_UPGRADE FAIL     *");
            MDF_LOGI("***********************************");
            rgb_led_blink_stop();
            rgb_led_set(COLOR_LED_R, 100);
            break;

        case MDF_EVENT_SYSTEM_RESET:
            MDF_LOGI("***********************************");
            MDF_LOGI("*       RESTORE FACTORY RESET     *");
            MDF_LOGI("***********************************");
            ret = mdf_info_erase(MDF_SPACE_NAME);
            MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "erase mdf config");
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
            rgb_led_set(COLOR_LED_R, 100);
            break;

        default:
            break;
    }

    return ESP_OK;
}

static void touchpad_deep_sleep_start(void *timer)
{
    touchpad_calibrate();

    ESP_ERROR_CHECK(esp_sleep_enable_touchpad_wakeup());
    MDF_LOGI("enabling touch pad wakeup");

    xTimerStop(timer, portMAX_DELAY);
    xTimerDelete(timer, portMAX_DELAY);

    ch450_clear();
    MDF_LOGI("esp deep sleep start");
    esp_deep_sleep_start();
}

void app_main()
{
    esp_err_t ret = ESP_OK;
    rgb_led_init();
    ch450_dev_init();

    /**< wake up from deepsleep mode */
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TOUCHPAD) {
        int tp_num = esp_sleep_get_touchpad_wakeup_status();
        MDF_LOGI("wake up from touch on pad %d", tp_num);
        rgb_led_set(COLOR_LED_G, 100);

        ret = touchpad_set_value(tp_num);
        MDF_ERROR_CHECK(ret < 0, ; , "touchpad_set_value, ret: %d", ret);
    }

    ESP_ERROR_CHECK(mdf_device_init_config(12, "touch_pad", "v0.0.1"));
    ESP_ERROR_CHECK(mdf_device_add_characteristics(TOUCH_BUTTON_CID_R, "touch_button_0", PERMS_READ_TRIGGER, 0, 1, 1));
    ESP_ERROR_CHECK(mdf_device_add_characteristics(TOUCH_BUTTON_CID_G, "touch_button_1", PERMS_READ_TRIGGER, 0, 1, 1));
    ESP_ERROR_CHECK(mdf_device_add_characteristics(TOUCH_BUTTON_CID_B, "touch_button_2", PERMS_READ_TRIGGER, 0, 1, 1));
    ESP_ERROR_CHECK(mdf_device_add_characteristics(TOUCH_SLIDE_CID, "touch_slide", PERMS_READ_TRIGGER, 0, 100, 1));
    ESP_ERROR_CHECK(mdf_device_init_handle(touchpad_event_loop_cb, (void *)touchpad_get_value, NULL));
    ESP_ERROR_CHECK(mdf_trigger_init());

    touchpad_init();
    MDF_LOGI("touchpad_init success.");

    if (mdf_get_running_mode() & MODE_ESPNOW) {
        rgb_led_set(COLOR_LED_B, 100);
    } else {
        rgb_led_set(COLOR_LED_G, 100);
        TimerHandle_t deep_sleep_timer = xTimerCreate("deep_sleep_active_time",
                                         CONFIG_TOUCHPAD_DEEPSLEEP_DELAY_TIME / portTICK_RATE_MS,
                                         false, NULL, touchpad_deep_sleep_start);
        xTimerStart(deep_sleep_timer, 0);
        deepsleep_timer_pass(deep_sleep_timer);
    }
}
