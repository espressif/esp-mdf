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

#include "driver/rtc_io.h"
#include "driver/adc.h"
#include "ulp_sensor.h"

#include "iot_hts221.h"
#include "iot_bh1750.h"
#include "iot_led.h"
#include "iot_button.h"

#include "sense_epaper.h"
#include "sense_led.h"
#include "sense_sensors.h"
#include "mdf_device_handle.h"

static const char *TAG = "sense_main";

/**< GPIO number to wake up */
#define GPIO_WAKE_UP_IO              GPIO_NUM_34
#define GPIO_WAKE_UP_LEVEL           (BUTTON_ACTIVE_LOW)

/**< Switch running mode button */
#define SWITCH_MODE_BUTTON_IO        GPIO_NUM_34
#define BUTTON_ACTIVE_LEVEL          (BUTTON_ACTIVE_LOW)

#define ULP_READ_INTERVAL            CONFIG_ULP_READ_INTERVAL
#define LUMINANCE_THRESHOLD_LOW      CONFIG_LUMINANCE_THRESHOLD_LOW
#define LUMINANCE_THRESHOLD_UPPER    CONFIG_LUMINANCE_THRESHOLD_UPPER
#ifdef CONFIG_ENABLE_TIMER_WAKEUP
#define TIMER_INTERVAL_DEEPSLEEP     CONFIG_TIMER_INTERVAL_DEEPSLEEP
#endif
#define SENSE_DEEPSLEEP_DELAY_TIME   CONFIG_SENSE_DEEPSLEEP_DELAY_TIME

enum sensor_status_cid {
    CID_LUM = 0,
    CID_HUM = 1,
    CID_TEMP = 2,
};

static esp_err_t sensor_get_value(uint8_t cid, int *value)
{
    MDF_PARAM_CHECK(value);
    int16_t temp_value = 0;
    int16_t hum_value = 0;
    float lum_value = 0;

    switch (cid) {
        case CID_TEMP:
            hts221_get_temperature(&temp_value);
            *value = (int)(temp_value / 10);
            break;

        case CID_HUM:
            hts221_get_humidity(&hum_value);
            *value = (int)(hum_value / 10);
            break;

        case CID_LUM:
            bh1750_get_luminance(&lum_value);
            *value = (int)lum_value;
            break;

        default:
            MDF_LOGE("no support cid: %d", cid);
            return ESP_FAIL;
    }

    return ESP_OK;
}

/*************************** event ***************************/
static esp_err_t sensor_event_loop_cb(mdf_event_loop_t event, void *ctx)
{
    esp_err_t ret = ESP_OK;

    switch (event) {
        case MDF_EVENT_PARENT_CONNECTED:
            sense_state_led_oprt(STATE_LED_NETWORK, STATE_LED_OPRT_ON);
            break;

        case MDF_EVENT_NETWORK_START_CONFIG:
            MDF_LOGI("***********************************");
            MDF_LOGI("*    ENTER CONFIG NETWORK MODE    *");
            MDF_LOGI("***********************************");
            sense_state_led_oprt(STATE_LED_NETWORK, STATE_LED_OPRT_SLOW_BLINK);
            break;

        case MDF_EVENT_CONNECT_ROUTER:
            MDF_LOGI("***********************************");
            MDF_LOGI("*  START CONNECTING TO THE ROUTER *");
            MDF_LOGI("***********************************");
            sense_state_led_oprt(STATE_LED_NETWORK, STATE_LED_OPRT_QUICK_BLINK);
            break;

        case MDF_EVENT_NETWORK_ORGANIZE:
            MDF_LOGI("***********************************");
            MDF_LOGI("*        ORGANIZE NETWORKS        *");
            MDF_LOGI("***********************************");
            sense_state_led_oprt(STATE_LED_NETWORK, STATE_LED_OPRT_QUICK_BLINK);
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
            break;

        case MDF_EVENT_UPGRADE_SUCCESS:
            MDF_LOGI("***********************************");
            MDF_LOGI("*    ENTER OTA UPGRADE SUCCESS    *");
            MDF_LOGI("***********************************");
            break;

        case MDF_EVENT_UPGRADE_FAIL:
            MDF_LOGI("***********************************");
            MDF_LOGI("*      ENTER OTA_UPGRADE FAIL     *");
            MDF_LOGI("***********************************");
            break;

        case MDF_EVENT_SYSTEM_RUNNING:
            MDF_LOGI("***********************************");
            MDF_LOGI("*      SYSTEM RUNNING NORMALLY    *");
            MDF_LOGI("***********************************");
            sense_state_led_oprt(STATE_LED_NETWORK, STATE_LED_OPRT_ON);
            break;

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
            break;

        default:
            break;
    }

    return ESP_OK;
}

static void sense_deepsleep_set()
{
#ifdef CONFIG_ENABLE_ULP_WAKEUP
    bh1750_set_luminance_threshold(LUMINANCE_THRESHOLD_LOW, LUMINANCE_THRESHOLD_UPPER, 100);

    /**< The i2c io should set to RTC GPIO befor the board entering deepsleep mode. */
    rtc_sensor_power_on();
    vTaskDelay(100 / portTICK_RATE_MS);
    rtc_sensor_io_init();

    /**< init_ulp_program */
    set_ulp_read_interval(ULP_READ_INTERVAL);
    start_ulp_program();
    MDF_LOGI("esp_sleep_enable_ulp_wakeup");
    ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());
#endif /**< CONFIG_ENABLE_ULP_WAKEUP */

#ifdef CONFIG_ENABLE_TIMER_WAKEUP
    MDF_LOGI("esp_sleep_enable_timer_wakeup, internal: %d ms", TIMER_INTERVAL_DEEPSLEEP);
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(TIMER_INTERVAL_DEEPSLEEP * 1000));
#endif

#ifdef CONFIG_ENABLE_EXT1_WAKEUP
    MDF_LOGI("esp_sleep_enable_ext1_wakeup, GPIO: %d", GPIO_WAKE_UP_IO);
    ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup((uint64_t)(((uint64_t)1) << GPIO_WAKE_UP_IO), GPIO_WAKE_UP_LEVEL));
#endif

}

static void sense_mode_switch_task(void *arg)
{
    esp_err_t ret = ESP_OK;

    do {
        ret = mdf_set_running_mode((mdf_running_mode_t)(POWER_DEEP_SLEEP | TRANS_ESPNOW));
        MDF_ERROR_BREAK(ret < 0, "mdf_set_running_mode: %d, ret: %d", (POWER_DEEP_SLEEP | TRANS_ESPNOW), ret);

        MDF_LOGI("mdf_set_running_mode, mode: %d", POWER_DEEP_SLEEP | TRANS_ESPNOW);
        MDF_LOGI("esp deep sleep start");

        sense_deepsleep_set();
        esp_deep_sleep_start();
    } while (0);

    vTaskDelete(NULL);
}

static void sense_deepsleep_timer_cb(void *timer)
{
    sense_deepsleep_set();
    esp_deep_sleep_start();
}

static void button_tap_cb(void *arg)
{
    xTaskCreate(sense_mode_switch_task, "sense_mode_switch_task", 1024 * 3, NULL, MDF_TASK_DEFAULT_PRIOTY, NULL);
}

static void trigger_operation_task(void *arg)
{
    int16_t temp = 0;
    int16_t hum = 0;
    float lum = 0;

    while (1) {
        hts221_get_temperature(&temp);
        hts221_get_humidity(&hum);
        bh1750_get_luminance(&lum);

        MDF_LOGI("luminance: %f, humidity: %f, temperature: %f", lum, hum / 10.0, temp / 10.0);

        if (lum < LUMINANCE_THRESHOLD_LOW || lum > LUMINANCE_THRESHOLD_UPPER) {
            ESP_ERROR_CHECK(mdf_trigger_isr());
        }

        vTaskDelay(500 / portTICK_RATE_MS);
    }

    vTaskDelete(NULL);
}

static void sense_device_init()
{
    /**< initialize i2c bus and devices */
    sensor_power_on();
    vTaskDelay(100 / portTICK_RATE_MS);
    sensor_init();

    /**< initialize status led */
    sense_state_led_init();

#ifdef CONFIG_USE_EPAPER_DISPLAY
    epaper_gpio_init();
    epaper_power_on();
    vTaskDelay(100 / portTICK_RATE_MS);
    epaper_init();
    show_ulp_value();
#endif /**< CONFIG_USE_EPAPER_DISPLAY */

}

static void sense_wakeup_handle()
{
    esp_err_t ret = ESP_OK;
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    /**< reset to digital GPIO to read sensor's data */
    rtc_reset_gpio_degital();

    if (cause != ESP_SLEEP_WAKEUP_ULP) {
        init_ulp_program();
    }

    switch (cause) {
        case ESP_SLEEP_WAKEUP_ULP:
            MDF_LOGI("ESP_SLEEP_WAKEUP_ULP");
            print_ulp_raw_data();
            break;

        case ESP_SLEEP_WAKEUP_EXT1:
            MDF_LOGI("ESP_SLEEP_WAKEUP_EXT1: %d, Switch to mesh & active mode", GPIO_WAKE_UP_IO);
            ret = mdf_set_running_mode((mdf_running_mode_t)(POWER_ACTIVE | TRANS_WIFI_MESH));
            MDF_ERROR_CHECK(ret < 0, ; , "mdf_set_running_mode mode: %d, ret: %d",
                            (POWER_ACTIVE | TRANS_WIFI_MESH), ret);
            break;

        case ESP_SLEEP_WAKEUP_TIMER:
            MDF_LOGI("ESP_SLEEP_WAKEUP_TIMER");
            break;

        default:
            MDF_LOGI("not a deep sleep reset");
    }
}

void app_main()
{
    mdf_running_mode_t mode;
    TimerHandle_t deepsleep_timer;

    sense_wakeup_handle();
    sense_device_init();

    ESP_ERROR_CHECK(mdf_device_init_config(22, "sensor", "0.0.1"));
    ESP_ERROR_CHECK(mdf_device_add_characteristics(CID_TEMP, "temperature", PERMS_READ_TRIGGER, 0, 120, 1));
    ESP_ERROR_CHECK(mdf_device_add_characteristics(CID_HUM, "humidity", PERMS_READ_TRIGGER, 0, 100, 1));
    ESP_ERROR_CHECK(mdf_device_add_characteristics(CID_LUM, "luminance", PERMS_READ_TRIGGER, 0, 1200, 1));
    ESP_ERROR_CHECK(mdf_device_init_handle(sensor_event_loop_cb, (const void *)sensor_get_value, NULL));
    ESP_ERROR_CHECK(mdf_trigger_init());

    mode = mdf_get_running_mode();
    MDF_LOGI("mdf_get_running_mode: %d", mode);

    if (mode & POWER_DEEP_SLEEP) {
#ifdef CONFIG_USE_EPAPER_DISPLAY
        epaper_show_deepsleep();
#endif
        deepsleep_timer = xTimerCreate("deep_sleep_maintain_time",
                                       SENSE_DEEPSLEEP_DELAY_TIME / portTICK_RATE_MS,
                                       false, NULL, sense_deepsleep_timer_cb);
        xTimerStart(deepsleep_timer, portMAX_DELAY);
    } else {
        button_handle_t btn_handle = iot_button_create(SWITCH_MODE_BUTTON_IO, BUTTON_ACTIVE_LEVEL);
        iot_button_set_evt_cb(btn_handle, BUTTON_CB_TAP, button_tap_cb, NULL);
    }

    xTaskCreate(trigger_operation_task, "trigger_operation_task", 1024 * 2, NULL, MDF_TASK_DEFAULT_PRIOTY, NULL);
}
