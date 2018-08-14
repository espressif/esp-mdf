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
#include "iot_led.h"
#include "sense_led.h"
#include "mdf_common.h"

/**< Wifi and network status led */
#define WIFI_STATUS_LED_IO           GPIO_NUM_15
#define NETWORK_STATUS_LED_IO        GPIO_NUM_4

static CLED *g_wifi_led = NULL;
static CLED *g_network_led = NULL;
static const char *TAG = "sense_led";

void sense_state_led_io_hold()
{
    gpio_set_level((gpio_num_t)WIFI_STATUS_LED_IO, LED_DARK_HIGH);
    rtc_gpio_hold_en((gpio_num_t)WIFI_STATUS_LED_IO);
    gpio_set_level((gpio_num_t)NETWORK_STATUS_LED_IO, LED_DARK_LOW);
    rtc_gpio_hold_en((gpio_num_t)NETWORK_STATUS_LED_IO);
}

void sense_state_led_init()
{
    gpio_config_t led_io_config;
    led_io_config.pin_bit_mask = 1 << WIFI_STATUS_LED_IO | 1 << NETWORK_STATUS_LED_IO;
    led_io_config.mode = GPIO_MODE_OUTPUT;
    led_io_config.pull_up_en = GPIO_PULLUP_DISABLE;
    led_io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    led_io_config.intr_type = GPIO_INTR_DISABLE;

    gpio_config(&led_io_config);
    gpio_set_level((gpio_num_t)WIFI_STATUS_LED_IO, LED_DARK_HIGH);
    gpio_set_level((gpio_num_t)NETWORK_STATUS_LED_IO, LED_DARK_LOW);
    rtc_gpio_hold_dis((gpio_num_t)WIFI_STATUS_LED_IO);
    rtc_gpio_hold_dis((gpio_num_t)NETWORK_STATUS_LED_IO);

    if (!g_wifi_led) {
        g_wifi_led = new CLED(WIFI_STATUS_LED_IO, LED_DARK_HIGH);
        gpio_set_direction(WIFI_STATUS_LED_IO, GPIO_MODE_INPUT_OUTPUT);
        g_wifi_led->off();
    }

    if (!g_network_led) {
        g_network_led = new CLED(NETWORK_STATUS_LED_IO, LED_DARK_LOW);
        g_network_led->off();
    }
}

void sense_state_led_oprt(state_led_type_t led_type, state_led_oprt_t led_oprt)
{
    MDF_ASSERT(led_type == STATE_LED_WIFI || led_type == STATE_LED_NETWORK);
    MDF_ASSERT(led_oprt >= STATE_LED_OPRT_ON || led_oprt < STATE_LED_OPRT_MAX);

    CLED *state_led = NULL;

    if (led_type == STATE_LED_WIFI) {
        state_led = g_wifi_led;
    } else {
        state_led = g_network_led;
    }

    switch (led_oprt) {
        case STATE_LED_OPRT_ON:
            state_led->on();
            break;
        case STATE_LED_OPRT_OFF:
            state_led->off();
            break;
        case STATE_LED_OPRT_SLOW_BLINK:
            state_led->slow_blink();
            break;
        case STATE_LED_OPRT_QUICK_BLINK:
            state_led->quick_blink();
            break;
        default:
            MDF_LOGE("sense_state_led_oprt led_oprt: %d", led_oprt);
            return;
    }
}
