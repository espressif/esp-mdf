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

/**
 * @brief The state of the five-color light
 */
typedef struct {
    uint8_t mode;
    uint8_t on;
    uint16_t hue;
    uint8_t saturation;
    uint8_t value;
    uint8_t color_temperature;
    uint8_t brightness;
    light_handle_t handle;
    uint32_t fade_period_ms;
    uint32_t blink_period_ms;
} light_status_t;

/**
 * @brief The channel of the five-color light
 */
enum light_channel {
    CHANNEL_ID_RED = 0,
    CHANNEL_ID_GREEN,
    CHANNEL_ID_BLUE,
    CHANNEL_ID_COLD,
    CHANNEL_ID_WARM,
};

/**
 * @brief The mode of the five-color light
 */
enum light_mode {
    MODE_RGB = 1,
    MODE_HSV,
    MODE_CTB,
};

#define LIGHT_TIMER_BIT        (LEDC_TIMER_13_BIT)
#define LIGHT_FULL_DUTY        ((1 << LIGHT_TIMER_BIT) - 1)
#define LIGHT_STATUS_STORE_KEY "light_status"

static const char *TAG               = "light_driver";
static light_status_t g_light_status = {0};
static bool g_light_breath_flag      = false;

mdf_err_t light_driver_init(light_driver_config_t *config)
{
    MDF_PARAM_CHECK(config);

    if (mdf_info_load(LIGHT_STATUS_STORE_KEY, &g_light_status, sizeof(light_status_t)) != MDF_OK) {
        memset(&g_light_status, 0, sizeof(light_status_t));
        g_light_status.mode              = MODE_HSV;
        g_light_status.on                = 1;
        g_light_status.hue               = 360;
        g_light_status.saturation        = 0;
        g_light_status.value             = 100;
        g_light_status.color_temperature = 0;
        g_light_status.brightness        = 30;
    }

    g_light_status.handle = iot_light_create(LEDC_TIMER_0, LEDC_HIGH_SPEED_MODE, 1000, LIGHT_MAX_CHANNEL_NUM, LIGHT_TIMER_BIT);
    MDF_ERROR_CHECK(g_light_status.handle == NULL, MDF_FAIL, "light initialize");

    g_light_status.fade_period_ms  = config->fade_period_ms;
    g_light_status.blink_period_ms = config->blink_period_ms;

    iot_light_channel_regist(g_light_status.handle, CHANNEL_ID_RED, config->gpio_red, LEDC_CHANNEL_0);
    iot_light_channel_regist(g_light_status.handle, CHANNEL_ID_GREEN, config->gpio_green, LEDC_CHANNEL_1);
    iot_light_channel_regist(g_light_status.handle, CHANNEL_ID_BLUE, config->gpio_blue, LEDC_CHANNEL_2);
    iot_light_channel_regist(g_light_status.handle, CHANNEL_ID_COLD, config->gpio_cold, LEDC_CHANNEL_3);
    iot_light_channel_regist(g_light_status.handle, CHANNEL_ID_WARM, config->gpio_warm, LEDC_CHANNEL_4);

    MDF_LOGD("hue: %d, saturation: %d, value: %d",
             g_light_status.hue, g_light_status.saturation, g_light_status.value);
    MDF_LOGD("brightness: %d, color_temperature: %d",
             g_light_status.brightness, g_light_status.color_temperature);

    return MDF_OK;
}

mdf_err_t light_driver_deinit()
{
    mdf_err_t ret = MDF_OK;

    if (g_light_status.handle) {
        ret = iot_light_delete(g_light_status.handle);
    }

    return ret;
}

mdf_err_t light_driver_config(uint32_t fade_period_ms, uint32_t blink_period_ms)
{
    g_light_status.fade_period_ms = fade_period_ms;
    g_light_status.blink_period_ms = blink_period_ms;

    return MDF_OK;
}

mdf_err_t light_driver_set_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    mdf_err_t ret = 0;

    ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_RED, red * LIGHT_FULL_DUTY / 255, g_light_status.fade_period_ms);
    MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

    ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_GREEN, green * LIGHT_FULL_DUTY / 255, g_light_status.fade_period_ms);
    MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

    ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_BLUE, blue * LIGHT_FULL_DUTY / 255, g_light_status.fade_period_ms);
    MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

    ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_COLD, 0, g_light_status.fade_period_ms);
    MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

    ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_WARM, 0, g_light_status.fade_period_ms);
    MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

    return MDF_OK;
}

static mdf_err_t light_driver_hsv2rgb(uint16_t hue, uint8_t saturation, uint8_t value,
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
            return MDF_FAIL;
    }

    *red   = *red * 255 / 100;
    *green = *green * 255 / 100;
    *blue  = *blue * 255 / 100;

    return MDF_OK;
}

mdf_err_t light_driver_set_hsv(uint16_t hue, uint8_t saturation, uint8_t value)
{
    MDF_PARAM_CHECK(hue <= 360);
    MDF_PARAM_CHECK(saturation <= 100);
    MDF_PARAM_CHECK(value <= 100);

    mdf_err_t ret = MDF_OK;
    uint8_t red   = 0;
    uint8_t green = 0;
    uint8_t blue  = 0;

    ret = light_driver_hsv2rgb(hue, saturation, value, &red, &green, &blue);
    MDF_ERROR_CHECK(ret < 0, ret, "light_driver_hsv2rgb, ret: %d", ret);

    MDF_LOGV("red: %d, green: %d, blue: %d", red, green, blue);

    ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_RED, red * LIGHT_FULL_DUTY / 255, g_light_status.fade_period_ms);
    MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

    ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_GREEN, green * LIGHT_FULL_DUTY / 255, g_light_status.fade_period_ms);
    MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

    ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_BLUE, blue * LIGHT_FULL_DUTY / 255, g_light_status.fade_period_ms);
    MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

    if (g_light_status.mode != MODE_HSV) {
        ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_COLD, 0, g_light_status.fade_period_ms);
        MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

        ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_WARM, 0, g_light_status.fade_period_ms);
        MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);
    }

    g_light_status.mode       = MODE_HSV;
    g_light_status.on         = 1;
    g_light_status.hue        = hue;
    g_light_status.value      = value;
    g_light_status.saturation = saturation;

    ret = mdf_info_save(LIGHT_STATUS_STORE_KEY, &g_light_status, sizeof(light_status_t));
    MDF_ERROR_CHECK(ret < 0, ret, "mdf_info_save, ret: %d", ret);

    return MDF_OK;
}

mdf_err_t light_driver_set_hue(uint16_t hue)
{
    return light_driver_set_hsv(hue, g_light_status.saturation, g_light_status.value);
}

mdf_err_t light_driver_set_saturation(uint8_t saturation)
{
    return light_driver_set_hsv(g_light_status.hue, saturation, g_light_status.value);
}

mdf_err_t light_driver_set_value(uint8_t value)
{
    return light_driver_set_hsv(g_light_status.hue, g_light_status.saturation, value);
}

mdf_err_t light_driver_get_hsv(uint16_t *hue, uint8_t *saturation, uint8_t *value)
{
    MDF_PARAM_CHECK(hue);
    MDF_PARAM_CHECK(saturation);
    MDF_PARAM_CHECK(value);

    *hue        = g_light_status.hue;
    *saturation = g_light_status.saturation;
    *value      = g_light_status.value;

    return MDF_OK;
}

uint16_t light_driver_get_hue()
{
    return g_light_status.hue;
}

uint8_t light_driver_get_saturation()
{
    return g_light_status.saturation;
}

uint8_t light_driver_get_value()
{
    return g_light_status.value;
}

uint8_t light_driver_get_mode()
{
    return g_light_status.mode;
}

mdf_err_t light_driver_set_ctb(uint8_t color_temperature, uint8_t brightness)
{
    MDF_PARAM_CHECK(brightness <= 100);
    MDF_PARAM_CHECK(color_temperature <= 100);

    mdf_err_t ret = MDF_OK;
    uint8_t warm_tmp = color_temperature * brightness / 100;
    uint8_t cold_tmp = (100 - color_temperature) * brightness / 100;
    warm_tmp         = warm_tmp < 15 ? warm_tmp : 14 + warm_tmp * 86 / 100;
    cold_tmp         = cold_tmp < 15 ? cold_tmp : 14 + cold_tmp * 86 / 100;

    ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_WARM,
                                   cold_tmp * LIGHT_FULL_DUTY / 100, g_light_status.fade_period_ms);
    MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

    ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_COLD,
                                   warm_tmp * LIGHT_FULL_DUTY / 100, g_light_status.fade_period_ms);
    MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

    if (g_light_status.mode != MODE_CTB) {
        ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_RED, 0, g_light_status.fade_period_ms);
        MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

        ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_GREEN, 0, g_light_status.fade_period_ms);
        MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

        ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_BLUE, 0, g_light_status.fade_period_ms);
        MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);
    }

    g_light_status.mode              = MODE_CTB;
    g_light_status.on                = 1;
    g_light_status.brightness        = brightness;
    g_light_status.color_temperature = color_temperature;

    ret = mdf_info_save(LIGHT_STATUS_STORE_KEY, &g_light_status, sizeof(light_status_t));
    MDF_ERROR_CHECK(ret < 0, ret, "mdf_info_save, ret: %d", ret);

    return MDF_OK;
}

mdf_err_t light_driver_set_color_temperature(uint8_t color_temperature)
{
    return light_driver_set_ctb(color_temperature, g_light_status.brightness);
}

mdf_err_t light_driver_set_brightness(uint8_t brightness)
{
    return light_driver_set_ctb(g_light_status.color_temperature, brightness);
}

mdf_err_t light_driver_get_ctb(uint8_t *color_temperature, uint8_t *brightness)
{
    MDF_PARAM_CHECK(color_temperature);
    MDF_PARAM_CHECK(brightness);

    *brightness        = g_light_status.brightness;
    *color_temperature = g_light_status.color_temperature;

    return MDF_OK;
}

uint8_t light_driver_get_color_temperature()
{
    return g_light_status.color_temperature;
}

uint8_t light_driver_get_brightness()
{
    return g_light_status.brightness;
}

mdf_err_t light_driver_set_switch(bool on)
{
    esp_err_t ret     = MDF_OK;
    g_light_status.on = on;

    if (!g_light_status.on) {
        ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_RED, 0, g_light_status.fade_period_ms);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_fade_with_time, ret: %d", ret);

        ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_GREEN, 0, g_light_status.fade_period_ms);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_fade_with_time, ret: %d", ret);

        ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_BLUE, 0, g_light_status.fade_period_ms);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_fade_with_time, ret: %d", ret);

        ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_WARM, 0, g_light_status.fade_period_ms);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_fade_with_time, ret: %d", ret);

        ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_COLD, 0, g_light_status.fade_period_ms);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_fade_with_time, ret: %d", ret);

    } else {
        switch (g_light_status.mode) {
            case MODE_HSV:
                g_light_status.value = (g_light_status.value) ? g_light_status.value : 100;
                ret = light_driver_set_hsv(g_light_status.hue, g_light_status.saturation, g_light_status.value);
                MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "light_driver_set_hsv, ret: %d", ret);
                break;

            case MODE_CTB:
                g_light_status.brightness = (g_light_status.brightness) ? g_light_status.brightness : 100;
                ret = light_driver_set_ctb(g_light_status.color_temperature, g_light_status.brightness);
                MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "light_driver_set_ctb, ret: %d", ret);
                break;

            default:
                MDF_LOGW("This operation is not supported");
                break;
        }
    }

    ret = mdf_info_save(LIGHT_STATUS_STORE_KEY, &g_light_status, sizeof(light_status_t));
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_save, ret: %d", ret);

    return MDF_OK;
}

bool light_driver_get_switch()
{
    return g_light_status.on;
}

mdf_err_t light_driver_blink_start(uint8_t red, uint8_t green, uint8_t blue)
{
    mdf_err_t ret = MDF_OK;

    ret = iot_light_blink_config(g_light_status.handle, CHANNEL_ID_RED,
                                 red * LIGHT_FULL_DUTY / 255);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_blink_config, ret: %d", ret);
    ret = iot_light_blink_config(g_light_status.handle, CHANNEL_ID_GREEN,
                                 green * LIGHT_FULL_DUTY / 255);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_blink_config, ret: %d", ret);
    ret = iot_light_blink_config(g_light_status.handle, CHANNEL_ID_BLUE,
                                 blue * LIGHT_FULL_DUTY / 255);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_blink_config, ret: %d", ret);

    ret = iot_light_blink_start(g_light_status.handle, CHANNEL_ID_RED);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_blink_start, ret: %d", ret);
    ret = iot_light_blink_start(g_light_status.handle, CHANNEL_ID_GREEN);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_blink_start, ret: %d", ret);
    ret = iot_light_blink_start(g_light_status.handle, CHANNEL_ID_BLUE);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_blink_start, ret: %d", ret);

    return MDF_OK;
}

mdf_err_t light_driver_blink_stop()
{
    mdf_err_t ret = MDF_OK;

    ret = iot_light_blink_stop(g_light_status.handle, CHANNEL_ID_RED);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_blink_stop, ret: %d", ret);

    ret = iot_light_blink_stop(g_light_status.handle, CHANNEL_ID_GREEN);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_blink_stop, ret: %d", ret);

    ret = iot_light_blink_stop(g_light_status.handle, CHANNEL_ID_BLUE);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_blink_stop, ret: %d", ret);

    light_driver_set_switch(true);

    return MDF_OK;
}

mdf_err_t light_driver_breath_start(uint8_t red, uint8_t green, uint8_t blue)
{
    mdf_err_t ret = MDF_OK;
    g_light_breath_flag = true;

    ret = iot_light_breath_config(g_light_status.handle, CHANNEL_ID_RED,
                                  red * LIGHT_FULL_DUTY / 255, g_light_status.blink_period_ms);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_breath_config, ret: %d", ret);
    ret = iot_light_breath_config(g_light_status.handle, CHANNEL_ID_GREEN,
                                  green * LIGHT_FULL_DUTY / 255, g_light_status.blink_period_ms);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_breath_config, ret: %d", ret);
    ret = iot_light_breath_config(g_light_status.handle, CHANNEL_ID_BLUE,
                                  blue * LIGHT_FULL_DUTY / 255, g_light_status.blink_period_ms);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_breath_config, ret: %d", ret);

    ret = iot_light_breath_start(g_light_status.handle, CHANNEL_ID_RED);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_breath_start, ret: %d", ret);
    ret = iot_light_breath_start(g_light_status.handle, CHANNEL_ID_GREEN);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_breath_start, ret: %d", ret);
    ret = iot_light_breath_start(g_light_status.handle, CHANNEL_ID_BLUE);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_breath_start, ret: %d", ret);

    return MDF_OK;
}

mdf_err_t light_driver_breath_stop()
{
    mdf_err_t ret = MDF_OK;

    if (g_light_breath_flag == false) {
        return MDF_OK;
    }

    ret = iot_light_breath_stop(g_light_status.handle, CHANNEL_ID_RED);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_breath_stop, ret: %d", ret);

    ret = iot_light_breath_stop(g_light_status.handle, CHANNEL_ID_GREEN);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_breath_stop, ret: %d", ret);

    ret = iot_light_breath_stop(g_light_status.handle, CHANNEL_ID_BLUE);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_breath_stop, ret: %d", ret);

    g_light_breath_flag = false;

    light_driver_set_switch(true);
    light_driver_set_switch(true);

    return MDF_OK;
}
