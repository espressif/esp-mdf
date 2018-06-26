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

#ifndef __MDF_LIGHT_H__
#define __MDF_LIGHT_H__

#include "iot_light.h"
#include "mdf_common.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t mode;
    uint8_t on;
    uint16_t hue;
    uint8_t saturation;
    uint8_t value;
    uint8_t color_temperature;
    uint8_t brightness;
    uint8_t cool_white;
    uint8_t warm_white;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} light_status_t;

esp_err_t light_init(gpio_num_t red_gpio, gpio_num_t green_gpio, gpio_num_t blue_gpio,
                     gpio_num_t brightness_gpio, gpio_num_t color_temperature_gpio);

esp_err_t light_deinit();

esp_err_t light_set_rgb(uint8_t red, uint8_t green, uint8_t blue);

esp_err_t light_set_hsv(uint16_t hue, uint8_t saturation, uint8_t value);
esp_err_t light_set_hue(uint16_t hue);
esp_err_t light_set_saturation(uint8_t saturation);
esp_err_t light_set_value(uint8_t value);

esp_err_t light_get_hsv(uint16_t *hue, uint8_t *saturation, uint8_t *value);
int light_get_hue();
int light_get_saturation();
int light_get_value();

esp_err_t light_set_ctb(uint8_t color_temperature, uint8_t brightness);
esp_err_t light_set_color_temperature(uint8_t color_temperature);
esp_err_t light_set_brightness(uint8_t brightness);

esp_err_t light_get_ctb(uint8_t *color_temperature, uint8_t *brightness);
int light_get_color_temperature();
int light_get_brightness();

esp_err_t light_set_cwb(uint8_t cool_white, uint8_t warm_white, uint8_t brightness);

esp_err_t light_get_cwb(uint8_t *cool_white, uint8_t *warm_white, uint8_t *brightness);
int light_get_cool_white();
int light_get_warm_white();

esp_err_t light_set_switch(uint8_t status);
int light_get_switch();

esp_err_t light_set(const light_status_t *light_status);
esp_err_t light_get(light_status_t *light_status);

esp_err_t light_breath_set(int period_ms);
esp_err_t light_blink_start(uint8_t red, uint8_t green, uint8_t blue, int freq);
esp_err_t light_blink_stop();
esp_err_t light_blink_set(uint8_t red, uint8_t green, uint8_t blue, int freq, int blink_ms);

/************************** more friendly API ************************************/
#define light_black()        light_set_rgb(0,   0,   0)
#define light_white()        light_set_rgb(255, 255, 255)

#define light_red()          light_set_rgb(255, 0,   0)
#define light_orange()       light_set_rgb(255, 128, 0)
#define light_yellow()       light_set_rgb(255, 255, 0)
#define light_green()        light_set_rgb(0,   255, 0)
#define light_cyan()         light_set_rgb(0,   255, 255)
#define light_blue()         light_set_rgb(0,   0,   255)
#define light_purple()       light_set_rgb(128, 0,   255)

#ifdef __cplusplus
}
#endif

#endif/**< __MDF_LIGHT_H__ */
