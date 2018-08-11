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

#include "iot_light.h"
#include "touchpad.h"

#define CHANNEL_R_IO   GPIO_NUM_18
#define CHANNEL_G_IO   GPIO_NUM_21
#define CHANNEL_B_IO   GPIO_NUM_19

static CLight *g_led = NULL;
static uint8_t g_bright_percent_r = 0;  /**< 0 ~ 100 */
static uint8_t g_bright_percent_g = 0;  /**< 0 ~ 100 */
static uint8_t g_bright_percent_b = 0;  /**< 0 ~ 100 */

void rgb_led_clear()
{
    g_led->blue.duty(0, LIGHT_SET_DUTY_DIRECTLY);
    g_led->green.duty(0, LIGHT_SET_DUTY_DIRECTLY);
    g_led->red.duty(0, LIGHT_SET_DUTY_DIRECTLY);
    g_bright_percent_r = 0;
    g_bright_percent_g = 0;
    g_bright_percent_b = 0;
}

void rgb_led_blink_stop()
{
    rgb_led_clear();
    g_led->blink_stop();
}

void rgb_led_blink_start(uint8_t color, uint32_t period_ms)
{
    rgb_led_clear();
    g_led->blink_start(color, period_ms);
}

void rgb_led_set(uint8_t color, uint8_t bright_percent)
{
    bright_percent = bright_percent > 100 ? 100 : bright_percent;
    rgb_led_clear();

    for (int i = 0; i < 3; i++) {
        if (color & COLOR_LED_R) {
            g_led->red.duty(g_led->get_full_duty() / 100 * bright_percent,
                              LIGHT_SET_DUTY_DIRECTLY);
            g_bright_percent_r = bright_percent;
        }

        if (color & COLOR_LED_G) {
            g_led->green.duty(g_led->get_full_duty() / 100 * bright_percent,
                              LIGHT_SET_DUTY_DIRECTLY);
            g_bright_percent_r = bright_percent;
        }

        if (color & COLOR_LED_B) {
            g_led->blue.duty(g_led->get_full_duty() / 100 * bright_percent,
                               LIGHT_SET_DUTY_DIRECTLY);
            g_bright_percent_b = bright_percent;
        }
    }
}

void rgb_led_get(uint8_t color, uint8_t *bright_percent)
{
    if (bright_percent == NULL) {
        return;
    }

    switch (color) {
        case COLOR_LED_R:
            *bright_percent = g_bright_percent_r;
            break;
        case COLOR_LED_G:
            *bright_percent = g_bright_percent_g;
            break;
        case COLOR_LED_B:
            *bright_percent = g_bright_percent_b;
            break;
    }
}

void rgb_led_init()
{
    if (g_led == NULL) {
        g_led = new CLight(LIGHT_CH_NUM_3);
    }
    g_led->red.init(CHANNEL_R_IO, LEDC_CHANNEL_0);
    g_led->green.init(CHANNEL_G_IO, LEDC_CHANNEL_1);
    g_led->blue.init(CHANNEL_B_IO, LEDC_CHANNEL_2);
}
