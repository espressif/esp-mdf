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
#include "light_driver.h"
#include "mdf_info_store.h"

enum light_channel {
    CHANNEL_ID_R = 0,
    CHANNEL_ID_G,
    CHANNEL_ID_B,
    CHANNEL_ID_CT,
    CHANNEL_ID_BR,
};

enum light_mode {
    MODE_RGB = 1,
    MODE_HSV,
    MODE_CTB,
    MODE_CWB,
};

#define LIGHT_FULL_DUTY              ((1 << LEDC_TIMER_13_BIT) - 1)
#define MDF_LIGHT_STORE_KEY         "light_status"

static const char *TAG               = "light_driver";
static light_status_t g_light_status = {0};
static light_handle_t g_light_handle = NULL;

esp_err_t light_init(gpio_num_t red_gpio, gpio_num_t green_gpio, gpio_num_t blue_gpio,
                     gpio_num_t color_temperature_gpio, gpio_num_t brightness_gpio)
{
    g_light_handle = iot_light_create(LEDC_TIMER_0, LEDC_HIGH_SPEED_MODE, 1000, 5, LEDC_TIMER_13_BIT);

    iot_light_channel_regist(g_light_handle, CHANNEL_ID_R, red_gpio, LEDC_CHANNEL_0);
    iot_light_channel_regist(g_light_handle, CHANNEL_ID_G, green_gpio, LEDC_CHANNEL_1);
    iot_light_channel_regist(g_light_handle, CHANNEL_ID_B, blue_gpio, LEDC_CHANNEL_2);
    iot_light_channel_regist(g_light_handle, CHANNEL_ID_CT, brightness_gpio, LEDC_CHANNEL_3);
    iot_light_channel_regist(g_light_handle, CHANNEL_ID_BR, color_temperature_gpio, LEDC_CHANNEL_4);

    if (mdf_info_load(MDF_LIGHT_STORE_KEY, &g_light_status, sizeof(light_status_t)) < 0) {
        memset(&g_light_status, 0, sizeof(light_status_t));
        g_light_status.mode              = MODE_HSV;
        g_light_status.on                = 1;
        g_light_status.hue               = 360;
        g_light_status.saturation        = 0;
        g_light_status.value             = 100;
        g_light_status.color_temperature = 0;
        g_light_status.brightness        = 30;
    }

    MDF_LOGD("status: %d, mode: %d",
             g_light_status.on, g_light_status.mode);
    MDF_LOGD("hue: %d, saturation: %d, value: %d",
             g_light_status.hue, g_light_status.saturation, g_light_status.value);
    MDF_LOGD("brightness: %d, color_temperature: %d",
             g_light_status.brightness, g_light_status.color_temperature);

    return ESP_OK;
}

esp_err_t light_deinit()
{
    return iot_light_delete(g_light_handle);
}

esp_err_t light_set_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    esp_err_t ret = 0;

    ret = iot_light_duty_write(g_light_handle, CHANNEL_ID_R, red * LIGHT_FULL_DUTY / 255, LIGHT_SET_DUTY_DIRECTLY);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_duty_write, ret: %d", ret);

    ret = iot_light_duty_write(g_light_handle, CHANNEL_ID_G, green * LIGHT_FULL_DUTY / 255, LIGHT_SET_DUTY_DIRECTLY);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_duty_write, ret: %d", ret);

    ret = iot_light_duty_write(g_light_handle, CHANNEL_ID_B, blue * LIGHT_FULL_DUTY / 255, LIGHT_SET_DUTY_DIRECTLY);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_duty_write, ret: %d", ret);

    ret = iot_light_duty_write(g_light_handle, CHANNEL_ID_CT, 0, LIGHT_SET_DUTY_DIRECTLY);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_duty_write, ret: %d", ret);

    ret = iot_light_duty_write(g_light_handle, CHANNEL_ID_BR, 0, LIGHT_SET_DUTY_DIRECTLY);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_duty_write, ret: %d", ret);

    return ESP_OK;
}

esp_err_t light_set_cwb(uint8_t cool_white, uint8_t warm_white, uint8_t brightness)
{
    esp_err_t ret = ESP_OK;
    uint8_t red    = cool_white;
    uint8_t green  = warm_white;
    uint8_t blue   = brightness;

    ret = light_set_rgb(red, green, blue);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "light_set_rgb, ret: %d", ret);
    MDF_LOGV("red: %d, green: %d, blue: %d", red, green, blue);

    g_light_status.mode       = MODE_CWB;
    g_light_status.on         = 1;
    g_light_status.cool_white = cool_white;
    g_light_status.warm_white = warm_white;
    g_light_status.brightness = brightness;

    ret = mdf_info_save(MDF_LIGHT_STORE_KEY, &g_light_status, sizeof(light_status_t));
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_save, ret: %d", ret);

    return ESP_OK;
}

esp_err_t light_get_cwb(uint8_t *cool_white, uint8_t *warm_white, uint8_t *brightness)
{
    MDF_PARAM_CHECK(cool_white);
    MDF_PARAM_CHECK(warm_white);
    MDF_PARAM_CHECK(brightness);

    *cool_white = g_light_status.cool_white;
    *warm_white = g_light_status.warm_white;
    *brightness = g_light_status.brightness;

    return ESP_OK;
}

int light_get_cool_white()
{
    return g_light_status.cool_white;
}

int light_get_warm_white()
{
    return g_light_status.warm_white;
}

static esp_err_t light_hsv2rgb(uint16_t hue, uint8_t saturation, uint8_t value,
                               uint8_t *red, uint8_t *green, uint8_t *blue)
{
    uint16_t hi = (hue / 60) % 6;
    uint16_t F = 100 * hue / 60 - 100 * hi;
    uint16_t P = value * (100 - saturation) / 100;
    uint16_t Q = value * (10000 - F * saturation) / 10000;
    uint16_t T = value * (10000 - saturation * (100 - F)) / 10000;

    switch (hi) {
        case 0:
            *red   = value;
            *green = T;
            *blue  = P;
            break;

        case 1:
            *red   = Q;
            *green = value;
            *blue  = P;
            break;

        case 2:
            *red   = P;
            *green = value;
            *blue  = T;
            break;

        case 3:
            *red   = P;
            *green = Q;
            *blue  = value;
            break;

        case 4:
            *red   = T;
            *green = P;
            *blue  = value;
            break;

        case 5:
            *red   = value;
            *green = P;
            *blue  = Q;
            break;

        default:
            return ESP_FAIL;
    }

    *red   = *red * 255 / 100;
    *green = *green * 255 / 100;
    *blue  = *blue * 255 / 100;

    return ESP_OK;
}

esp_err_t light_set_hsv(uint16_t hue, uint8_t saturation, uint8_t value)
{
    MDF_PARAM_CHECK(hue <= 360);
    MDF_PARAM_CHECK(saturation <= 100);
    MDF_PARAM_CHECK(value <= 100);

    esp_err_t ret = ESP_OK;
    uint8_t red    = 0;
    uint8_t green  = 0;
    uint8_t blue   = 0;

    ret = light_hsv2rgb(hue, saturation, value, &red, &green, &blue);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "light_hsv2rgb, ret: %d", ret);

    ret = light_set_rgb(red, green, blue);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "light_set_rgb, ret: %d", ret);
    MDF_LOGV("red: %d, green: %d, blue: %d", red, green, blue);

    g_light_status.mode       = MODE_HSV;
    g_light_status.on         = 1;
    g_light_status.hue        = hue;
    g_light_status.saturation = saturation;
    g_light_status.value      = value;

    ret = mdf_info_save(MDF_LIGHT_STORE_KEY, &g_light_status, sizeof(light_status_t));
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_save, ret: %d", ret);

    return ESP_OK;
}

esp_err_t light_set_hue(uint16_t hue)
{
    return light_set_hsv(hue, g_light_status.saturation, g_light_status.value);
}

esp_err_t light_set_saturation(uint8_t saturation)
{
    return light_set_hsv(g_light_status.hue, saturation, g_light_status.value);
}

esp_err_t light_set_value(uint8_t value)
{
    return light_set_hsv(g_light_status.hue, g_light_status.saturation, value);
}

esp_err_t light_get_hsv(uint16_t *hue, uint8_t *saturation, uint8_t *value)
{
    MDF_PARAM_CHECK(hue);
    MDF_PARAM_CHECK(saturation);
    MDF_PARAM_CHECK(value);

    *hue        = g_light_status.hue;
    *saturation = g_light_status.saturation;
    *value      = g_light_status.value;

    return ESP_OK;
}

int light_get_hue()
{
    return g_light_status.hue;
}

int light_get_saturation()
{
    return g_light_status.saturation;
}

int light_get_value()
{
    return g_light_status.value;
}

esp_err_t light_set_ctb(uint8_t color_temperature, uint8_t brightness)
{
    MDF_PARAM_CHECK(brightness <= 100);
    MDF_PARAM_CHECK(color_temperature <= 100);

    esp_err_t ret = ESP_OK;
    uint8_t ct_tmp = color_temperature * brightness / 100;
    uint8_t br_tmp = (100 - color_temperature) * brightness / 100;
    ct_tmp         = ct_tmp < 15 ? ct_tmp : 14 + ct_tmp * 86 / 100;
    br_tmp         = br_tmp < 15 ? br_tmp : 14 + br_tmp * 86 / 100;

    ret = iot_light_duty_write(g_light_handle, CHANNEL_ID_BR,
                               br_tmp * LIGHT_FULL_DUTY / 100, LIGHT_SET_DUTY_DIRECTLY);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_duty_write, ret: %d", ret);

    ret = iot_light_duty_write(g_light_handle, CHANNEL_ID_CT,
                               ct_tmp * LIGHT_FULL_DUTY / 100, LIGHT_SET_DUTY_DIRECTLY);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_duty_write, ret: %d", ret);

    ret = iot_light_duty_write(g_light_handle, CHANNEL_ID_R, 0, LIGHT_SET_DUTY_DIRECTLY);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_duty_write, ret: %d", ret);

    ret = iot_light_duty_write(g_light_handle, CHANNEL_ID_G, 0, LIGHT_SET_DUTY_DIRECTLY);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_duty_write, ret: %d", ret);

    ret = iot_light_duty_write(g_light_handle, CHANNEL_ID_B, 0, LIGHT_SET_DUTY_DIRECTLY);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_duty_write, ret: %d", ret);

    g_light_status.mode              = MODE_CTB;
    g_light_status.on                = 1;
    g_light_status.brightness        = brightness;
    g_light_status.color_temperature = color_temperature;

    ret = mdf_info_save(MDF_LIGHT_STORE_KEY, &g_light_status, sizeof(light_status_t));
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_save, ret: %d", ret);

    return ESP_OK;
}

esp_err_t light_set_color_temperature(uint8_t color_temperature)
{
    return light_set_ctb(color_temperature, g_light_status.brightness);
}

esp_err_t light_set_brightness(uint8_t brightness)
{
    return light_set_ctb(g_light_status.color_temperature, brightness);
}

esp_err_t light_get_ctb(uint8_t *color_temperature, uint8_t *brightness)
{
    MDF_PARAM_CHECK(color_temperature);
    MDF_PARAM_CHECK(brightness);

    *brightness        = g_light_status.brightness;
    *color_temperature = g_light_status.color_temperature;

    return ESP_OK;
}

int light_get_color_temperature()
{
    return g_light_status.color_temperature;
}

int light_get_brightness()
{
    return g_light_status.brightness;
}

esp_err_t light_set_switch(uint8_t on)
{
    MDF_PARAM_CHECK(on <= 1);

    esp_err_t ret    = ESP_OK;
    g_light_status.on = on;

    if (on == 0) {
        ret = iot_light_duty_write(g_light_handle, CHANNEL_ID_R, 0, LIGHT_SET_DUTY_DIRECTLY);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_duty_write, ret: %d", ret);

        ret = iot_light_duty_write(g_light_handle, CHANNEL_ID_G, 0, LIGHT_SET_DUTY_DIRECTLY);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_duty_write, ret: %d", ret);

        ret = iot_light_duty_write(g_light_handle, CHANNEL_ID_B, 0, LIGHT_SET_DUTY_DIRECTLY);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_duty_write, ret: %d", ret);

        ret = iot_light_duty_write(g_light_handle, CHANNEL_ID_BR, 0, LIGHT_SET_DUTY_DIRECTLY);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_duty_write, ret: %d", ret);

        ret = iot_light_duty_write(g_light_handle, CHANNEL_ID_CT, 0, LIGHT_SET_DUTY_DIRECTLY);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_duty_write, ret: %d", ret);

        ret = mdf_info_save(MDF_LIGHT_STORE_KEY, &g_light_status, sizeof(light_status_t));
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_save, ret: %d", ret);
        return ESP_OK;
    }

    if (on == 1) {
        switch (g_light_status.mode) {
            case MODE_HSV:
                g_light_status.value = (g_light_status.value) ? g_light_status.value : 100;
                ret = light_set_hsv(g_light_status.hue, g_light_status.saturation, g_light_status.value);
                MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "light_set_hsv, ret: %d", ret);
                break;

            case MODE_CTB:
                g_light_status.brightness = (g_light_status.brightness) ? g_light_status.brightness : 100;
                ret = light_set_ctb(g_light_status.brightness, g_light_status.color_temperature);
                MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "light_set_ctb, ret: %d", ret);
                break;

            case MODE_CWB:
                g_light_status.brightness = (g_light_status.brightness) ? g_light_status.brightness : 100;
                ret = light_set_cwb(g_light_status.cool_white, g_light_status.warm_white, g_light_status.brightness);
                MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "light_set_cwb, ret: %d", ret);
                break;

            default:
                ret = light_set_brightness(100);
                MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "light_set_ctb, ret: %d", ret);

                ret = light_set_value(100);
                MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "light_set_ctb, ret: %d", ret);
                break;
        }
    }

    return ESP_OK;
}

int light_get_switch()
{
    return g_light_status.on;
}

esp_err_t light_set(const light_status_t *light_status)
{
    MDF_PARAM_CHECK(light_status);

    esp_err_t ret = ESP_OK;

    switch (light_status->mode) {
        case MODE_HSV:
            ret = light_set_hsv(light_status->hue, light_status->saturation, light_status->value);
            MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "light_set_hsv, ret: %d", ret);
            break;

        case MODE_CTB:
            ret = light_set_ctb(light_status->brightness, light_status->color_temperature);
            MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "light_set_ctb, ret: %d", ret);
            break;

        case MODE_CWB:
            ret = light_set_cwb(light_status->cool_white, light_status->warm_white, light_status->brightness);
            MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "light_set_cwb, ret: %d", ret);
            break;

        default:
            break;
    }

    return ESP_OK;
}

esp_err_t light_get(light_status_t *light_status)
{
    MDF_PARAM_CHECK(light_status);

    memcpy(light_status, &g_light_status, sizeof(light_status_t));

    return ESP_OK;
}

esp_err_t light_breath_set(int period_ms)
{
    esp_err_t ret = ESP_OK;

    ret = iot_light_breath_write(g_light_handle, CHANNEL_ID_R, period_ms);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_breath_write, ret: %d", ret);

    ret = iot_light_breath_write(g_light_handle, CHANNEL_ID_G, period_ms);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_breath_write, ret: %d", ret);

    ret = iot_light_breath_write(g_light_handle, CHANNEL_ID_B, period_ms);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_breath_write, ret: %d", ret);

    return ESP_OK;
}

static light_status_t g_light_blink_status = {0};
static TimerHandle_t g_blink_timer         = NULL;

static void light_blink_timeout_cb(void *timer)
{
    esp_err_t ret = 0;

    if (g_light_blink_status.on == 0) {
        ret = light_set_rgb(0, 0, 0);
        MDF_ERROR_CHECK(ret < 0, ; , "light_set_rgb, ret: %d", ret);
        g_light_blink_status.on = 1;
    } else {
        ret = light_set_rgb(g_light_blink_status.red, g_light_blink_status.green,
                            g_light_blink_status.blue);
        MDF_ERROR_CHECK(ret < 0, ; , "light_set_rgb, ret: %d", ret);

        g_light_blink_status.on = 0;
    }
}

esp_err_t light_blink_start(uint8_t red, uint8_t green, uint8_t blue, int freq)
{
    uint16_t period = 1000 / freq;

    g_light_blink_status.red   = red;
    g_light_blink_status.green = green;
    g_light_blink_status.blue  = blue;

    if (g_blink_timer) {
        if (!xTimerStop(g_blink_timer, portMAX_DELAY)) {
            MDF_LOGW("xTimerStop timer: %p", g_blink_timer);
        }

        if (!xTimerDelete(g_blink_timer, 0)) {
            MDF_LOGW("xTimerDelete timer: %p", g_blink_timer);
        }
    }

    g_blink_timer = xTimerCreate("light_timer", period / 2 / portTICK_RATE_MS,
                                 true, NULL, light_blink_timeout_cb);
    xTimerStart(g_blink_timer, 0);

    return ESP_OK;
}

esp_err_t light_blink_stop()
{
    if (!g_blink_timer) {
        return ESP_OK;
    }

    esp_err_t ret = ESP_OK;

    if (!xTimerStop(g_blink_timer, portMAX_DELAY)) {
        MDF_LOGW("xTimerStop timer: %p", g_blink_timer);
    }

    if (!xTimerDelete(g_blink_timer, 0)) {
        MDF_LOGW("xTimerDelete timer: %p", g_blink_timer);
    }

    g_blink_timer = NULL;

    ret = light_set(&g_light_status);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "light_set, ret: %d", ret);

    return ESP_OK;
}

esp_err_t light_blink_set(uint8_t red, uint8_t green, uint8_t blue, int freq, int blink_ms)
{
    MDF_PARAM_CHECK(freq < 1000);

    esp_err_t ret  = ESP_OK;
    uint16_t period = 1000 / freq;
    uint16_t count  = blink_ms / period;

    if (g_blink_timer) {
        if (!xTimerStop(g_blink_timer, portMAX_DELAY)) {
            MDF_LOGW("xTimerStop timer: %p", g_blink_timer);
        }

        if (!xTimerDelete(g_blink_timer, 0)) {
            MDF_LOGW("xTimerDelete timer: %p", g_blink_timer);
        }

        g_blink_timer = NULL;
    }

    for (int i = 0; i < count; i++) {
        ret = light_set_rgb(0, 0, 0);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "light_set_rgb, ret: %d", ret);
        vTaskDelay(period / 2 / portTICK_RATE_MS);

        ret = light_set_rgb(red, green, blue);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "light_set_rgb, ret: %d", ret);
        vTaskDelay(period / 2 / portTICK_RATE_MS);
    }

    ret = light_set(&g_light_status);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "light_set, ret: %d", ret);

    return ESP_OK;
}
