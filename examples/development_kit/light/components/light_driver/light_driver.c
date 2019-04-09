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
    CHANNEL_ID_WARM,
    CHANNEL_ID_COLD,
};

#define LIGHT_TIMER_BIT          (LEDC_TIMER_13_BIT)
#define LIGHT_FULL_DUTY          ((1 << LIGHT_TIMER_BIT) - 1)
#define LIGHT_STATUS_STORE_KEY   "light_status"
#define LIGHT_FADE_PERIOD_MAX_MS (3 * 1000)

static const char *TAG               = "light_driver";
static light_status_t g_light_status = {0};
static bool g_light_blink_flag       = false;
static TimerHandle_t g_fade_timer    = NULL;
static int g_fade_mode               = MODE_NONE;
static uint16_t g_fade_hue           = 0;

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
    iot_light_channel_regist(g_light_status.handle, CHANNEL_ID_WARM, config->gpio_cold, LEDC_CHANNEL_3);
    iot_light_channel_regist(g_light_status.handle, CHANNEL_ID_COLD, config->gpio_warm, LEDC_CHANNEL_4);

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

    ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_RED, red * LIGHT_FULL_DUTY / 255, 0);
    MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

    ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_GREEN, green * LIGHT_FULL_DUTY / 255, 0);
    MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

    ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_BLUE, blue * LIGHT_FULL_DUTY / 255, 0);
    MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

    ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_WARM, 0, 0);
    MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

    ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_COLD, 0, 0);
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

static void light_driver_rgb2hsv(uint16_t red, uint16_t green, uint16_t blue,
                                 uint16_t *h, uint8_t *s, uint8_t *v)
{
    double hue, saturation, value;
    double m_max = MAX(red, MAX(green, blue));
    double m_min = MIN(red, MIN(green, blue));
    double m_delta = m_max - m_min;

    value = m_max / 255.0;

    if (m_delta == 0) {
        hue = 0;
        saturation = 0;
    } else {
        saturation = m_delta / m_max;

        if (red == m_max) {
            hue = (green - blue) / m_delta;
        } else if (green == m_max) {
            hue = 2 + (blue - red) / m_delta;
        } else {
            hue = 4 + (red - green) / m_delta;
        }

        hue = hue * 60;

        if (hue < 0) {
            hue = hue + 360;
        }
    }

    *h = (int)(hue + 0.5);
    *s = (int)(saturation * 100 + 0.5);
    *v = (int)(value * 100 + 0.5);
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
        ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_WARM, 0, g_light_status.fade_period_ms);
        MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

        ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_COLD, 0, g_light_status.fade_period_ms);
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

    ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_COLD,
                                   cold_tmp * LIGHT_FULL_DUTY / 100, g_light_status.fade_period_ms);
    MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

    ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_WARM,
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

        ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_COLD, 0, g_light_status.fade_period_ms);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_fade_with_time, ret: %d", ret);

        ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_WARM, 0, g_light_status.fade_period_ms);
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
    mdf_err_t ret      = MDF_OK;
    g_light_blink_flag = true;

    ret = iot_light_blink_config(g_light_status.handle, CHANNEL_ID_RED,
                                 g_light_status.blink_period_ms);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_blink_config, ret: %d", ret);
    ret = iot_light_blink_config(g_light_status.handle, CHANNEL_ID_GREEN,
                                 g_light_status.blink_period_ms);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_blink_config, ret: %d", ret);
    ret = iot_light_blink_config(g_light_status.handle, CHANNEL_ID_BLUE,
                                 g_light_status.blink_period_ms);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_blink_config, ret: %d", ret);

    ret = iot_light_operate_start(g_light_status.handle, CHANNEL_ID_RED);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_operate_start, ret: %d", ret);
    ret = iot_light_operate_start(g_light_status.handle, CHANNEL_ID_GREEN);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_operate_start, ret: %d", ret);
    ret = iot_light_operate_start(g_light_status.handle, CHANNEL_ID_BLUE);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_operate_start, ret: %d", ret);

    return MDF_OK;
}

mdf_err_t light_driver_blink_stop()
{
    mdf_err_t ret = MDF_OK;

    if (g_light_blink_flag == false) {
        return MDF_OK;
    }

    ret = iot_light_operate_stop(g_light_status.handle, CHANNEL_ID_RED);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_operate_stop, ret: %d", ret);

    ret = iot_light_operate_stop(g_light_status.handle, CHANNEL_ID_GREEN);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_operate_stop, ret: %d", ret);

    ret = iot_light_operate_stop(g_light_status.handle, CHANNEL_ID_BLUE);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_operate_stop, ret: %d", ret);

    light_driver_set_switch(true);

    return MDF_OK;
}

mdf_err_t light_driver_breath_start(uint8_t red, uint8_t green, uint8_t blue)
{
    mdf_err_t ret = MDF_OK;

    ret = iot_light_breath_config(g_light_status.handle, CHANNEL_ID_RED,
                                  red * LIGHT_FULL_DUTY / 255, g_light_status.blink_period_ms);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_breath_config, ret: %d", ret);
    ret = iot_light_breath_config(g_light_status.handle, CHANNEL_ID_GREEN,
                                  green * LIGHT_FULL_DUTY / 255, g_light_status.blink_period_ms);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_breath_config, ret: %d", ret);
    ret = iot_light_breath_config(g_light_status.handle, CHANNEL_ID_BLUE,
                                  blue * LIGHT_FULL_DUTY / 255, g_light_status.blink_period_ms);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_breath_config, ret: %d", ret);

    ret = iot_light_operate_start(g_light_status.handle, CHANNEL_ID_RED);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_operate_start, ret: %d", ret);
    ret = iot_light_operate_start(g_light_status.handle, CHANNEL_ID_GREEN);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_operate_start, ret: %d", ret);
    ret = iot_light_operate_start(g_light_status.handle, CHANNEL_ID_BLUE);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_operate_start, ret: %d", ret);

    g_light_blink_flag = true;

    return MDF_OK;
}

mdf_err_t light_driver_breath_stop()
{
    return light_driver_blink_stop();
}

mdf_err_t light_driver_fade_brightness(uint8_t brightness)
{
    mdf_err_t ret = MDF_OK;
    g_fade_mode   = MODE_ON;
    uint32_t fade_period_ms = 0;

    if (g_light_status.mode == MODE_HSV) {
        uint8_t red   = 0;
        uint8_t green = 0;
        uint8_t blue  = 0;

        ret = light_driver_hsv2rgb(g_light_status.hue, g_light_status.saturation, g_light_status.value, &red, &green, &blue);
        MDF_ERROR_CHECK(ret < 0, ret, "light_driver_hsv2rgb, ret: %d", ret);

        if (brightness != 0) {
            uint8_t max_color       = MAX(MAX(red, green), blue);
            uint8_t change_value    = brightness * 255 / 100 - max_color;
            fade_period_ms = LIGHT_FADE_PERIOD_MAX_MS * change_value / 255;
        } else {
            fade_period_ms = LIGHT_FADE_PERIOD_MAX_MS * MAX(MAX(red, green), blue) / 255;
            red   = 0;
        }

        g_light_status.value = brightness;
        light_driver_hsv2rgb(g_light_status.hue, g_light_status.saturation, g_light_status.value, &red, &green, &blue);

        ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_RED, red * LIGHT_FULL_DUTY / 255, fade_period_ms);
        MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

        ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_GREEN, green * LIGHT_FULL_DUTY / 255, fade_period_ms);
        MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

        ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_BLUE, blue * LIGHT_FULL_DUTY / 255, fade_period_ms);
        MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

    } else if (g_light_status.mode == MODE_CTB) {
        uint8_t warm_tmp = 0;
        uint8_t cold_tmp = 0;
        fade_period_ms = LIGHT_FADE_PERIOD_MAX_MS * g_light_status.brightness / 100;

        if (brightness != 0) {
            uint8_t change_value = brightness - g_light_status.brightness;
            warm_tmp = g_light_status.color_temperature;
            cold_tmp = (brightness - g_light_status.color_temperature);
            fade_period_ms = LIGHT_FADE_PERIOD_MAX_MS * change_value / 100;
        }

        ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_COLD,
                                       cold_tmp * LIGHT_FULL_DUTY / 100, fade_period_ms);
        MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

        ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_WARM,
                                       warm_tmp * LIGHT_FULL_DUTY / 100, fade_period_ms);
        MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

        g_light_status.brightness = brightness;
    }

    ret = mdf_info_save(LIGHT_STATUS_STORE_KEY, &g_light_status, sizeof(light_status_t));
    MDF_ERROR_CHECK(ret < 0, ret, "mdf_info_save, ret: %d", ret);

    return MDF_OK;
}

static void light_fade_timer_stop()
{
    if (!g_fade_timer) {
        return ;
    }

    if (!xTimerStop(g_fade_timer, portMAX_DELAY)) {
        MDF_LOGW("xTimerStop timer: %p", g_fade_timer);
    }

    if (!xTimerDelete(g_fade_timer, portMAX_DELAY)) {
        MDF_LOGW("xTimerDelete timer: %p", g_fade_timer);
    }

    g_fade_timer = NULL;
}

static void light_fade_timer_cb(void *timer)
{
    uint8_t red   = 0;
    uint8_t green = 0;
    uint8_t blue  = 0;
    uint32_t fade_period_ms = LIGHT_FADE_PERIOD_MAX_MS * 2 / 6;
    int variety = (g_fade_hue > 180) ? 60 : -60;

    if (g_light_status.hue >= 360 || g_light_status.hue <= 0) {
        light_fade_timer_stop();
    }

    g_light_status.hue = g_light_status.hue >= 360 ? 360 : g_light_status.hue + variety;
    g_light_status.hue = g_light_status.hue <= 60 ? 0 : g_light_status.hue + variety;

    light_driver_hsv2rgb(g_light_status.hue, g_light_status.saturation, g_light_status.value, &red, &green, &blue);

    iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_RED, red * LIGHT_FULL_DUTY / 255, fade_period_ms);
    iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_GREEN, green * LIGHT_FULL_DUTY / 255, fade_period_ms);
    iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_BLUE, blue * LIGHT_FULL_DUTY / 255, fade_period_ms);
}

mdf_err_t light_driver_fade_hue(uint16_t hue)
{
    mdf_err_t ret = MDF_OK;
    g_fade_mode   = MODE_HSV;
    g_fade_hue    = hue;

    light_fade_timer_stop();

    if (g_light_status.mode != MODE_HSV) {
        ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_WARM, 0, 0);
        MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

        ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_COLD, 0, 0);
        MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);
    }

    g_light_status.mode     = MODE_HSV;
    g_light_status.value    = (g_light_status.value == 0) ? 100 : g_light_status.value;
    uint32_t fade_period_ms = LIGHT_FADE_PERIOD_MAX_MS * 2 / 6;

    light_fade_timer_cb(NULL);

    g_fade_timer = xTimerCreate("light_timer", fade_period_ms,
                                true, NULL, light_fade_timer_cb);
    xTimerStart(g_fade_timer, 0);

    return MDF_OK;
}

mdf_err_t light_driver_fade_warm(uint8_t color_temperature)
{
    mdf_err_t ret = MDF_OK;
    g_fade_mode   = MODE_CTB;

    if (g_light_status.mode != MODE_CTB) {
        ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_RED, 0, g_light_status.fade_period_ms);
        MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

        ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_GREEN, 0, g_light_status.fade_period_ms);
        MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

        ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_BLUE, 0, g_light_status.fade_period_ms);
        MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);
    }

    uint8_t warm_tmp =  color_temperature * g_light_status.brightness / 100;
    uint8_t cold_tmp = (100 - color_temperature) * g_light_status.brightness / 100;

    ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_COLD, cold_tmp * LIGHT_FULL_DUTY / 100, LIGHT_FADE_PERIOD_MAX_MS);
    MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

    ret = iot_light_fade_with_time(g_light_status.handle, CHANNEL_ID_WARM, warm_tmp * LIGHT_FULL_DUTY / 100, LIGHT_FADE_PERIOD_MAX_MS);
    MDF_ERROR_CHECK(ret < 0, ret, "iot_light_fade_with_time, ret: %d", ret);

    g_light_status.mode              = MODE_CTB;
    g_light_status.color_temperature = color_temperature;
    ret = mdf_info_save(LIGHT_STATUS_STORE_KEY, &g_light_status, sizeof(light_status_t));
    MDF_ERROR_CHECK(ret < 0, ret, "mdf_info_save, ret: %d", ret);

    return MDF_OK;
}

mdf_err_t light_driver_fade_stop()
{
    mdf_err_t ret = MDF_OK;

    light_fade_timer_stop();

    if (g_light_status.mode != MODE_CTB) {
        uint16_t hue       = 0;
        uint8_t saturation = 0;
        uint8_t value      = 0;

        ret = iot_light_operate_stop(g_light_status.handle, CHANNEL_ID_RED);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_operate_stop, ret: %d", ret);

        ret = iot_light_operate_stop(g_light_status.handle, CHANNEL_ID_GREEN);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_operate_stop, ret: %d", ret);

        ret = iot_light_operate_stop(g_light_status.handle, CHANNEL_ID_BLUE);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_operate_stop, ret: %d", ret);

        uint8_t red   = iot_light_duty_get(g_light_status.handle, CHANNEL_ID_RED) * 255 / LIGHT_FULL_DUTY;
        uint8_t green = iot_light_duty_get(g_light_status.handle, CHANNEL_ID_GREEN) * 255 / LIGHT_FULL_DUTY;
        uint8_t blue  = iot_light_duty_get(g_light_status.handle, CHANNEL_ID_BLUE) * 255 / LIGHT_FULL_DUTY;

        light_driver_rgb2hsv(red, green, blue, &hue, &saturation, &value);

        g_light_status.hue   = (g_fade_mode == MODE_HSV) ? hue : g_light_status.hue;
        g_light_status.value = (g_fade_mode == MODE_OFF || g_fade_mode == MODE_ON) ? value : g_light_status.value;
    } else {
        uint8_t color_temperature = 0;
        uint8_t brightness        = 0;

        ret = iot_light_operate_stop(g_light_status.handle, CHANNEL_ID_COLD);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_operate_stop, ret: %d", ret);

        ret = iot_light_operate_stop(g_light_status.handle, CHANNEL_ID_WARM);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "iot_light_operate_stop, ret: %d", ret);

        uint8_t warm_tmp = iot_light_duty_get(g_light_status.handle, CHANNEL_ID_WARM) * 100 / LIGHT_FULL_DUTY;
        uint8_t cold_tmp = iot_light_duty_get(g_light_status.handle, CHANNEL_ID_COLD) * 100 / LIGHT_FULL_DUTY;

        color_temperature = (!warm_tmp) ? 0 : 100 / (cold_tmp / warm_tmp + 1);
        brightness        = (!color_temperature) ? cold_tmp : warm_tmp * 100 / color_temperature;

        g_light_status.brightness        = (g_fade_mode == MODE_OFF || g_fade_mode == MODE_ON) ? brightness : g_light_status.brightness;
        g_light_status.color_temperature = (g_fade_mode == MODE_CTB) ? color_temperature : g_light_status.color_temperature;
    }

    ret = mdf_info_save(LIGHT_STATUS_STORE_KEY, &g_light_status, sizeof(light_status_t));
    MDF_ERROR_CHECK(ret < 0, ret, "mdf_info_save, ret: %d", ret);

    g_fade_mode = MODE_NONE;
    return MDF_OK;
}
