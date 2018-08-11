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

#ifndef __TOUCHPAD_H__
#define __TOUCHPAD_H__

#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

/**< CID of touchpad example */
enum {
    TOUCH_BUTTON_CID_R = 0,
    TOUCH_BUTTON_CID_G = 1,
    TOUCH_BUTTON_CID_B = 2,
    TOUCH_SLIDE_CID = 3,
};

/**< RGB led color index */
enum {
    COLOR_LED_R = 1 << 0,
    COLOR_LED_G = 1 << 1,
    COLOR_LED_B = 1 << 2,
    COLOR_LED_Y = COLOR_LED_R | COLOR_LED_G,
};

/**
 * @brief  get value of touchpad with given cid
 *

 * @param  value pointer to cid value
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 */
esp_err_t touchpad_get_value(uint8_t cid, int *value);

/**
 * @brief  set value of touchpad with given touch pad number
 *
 * @param  tp_num touch pad number
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 */
esp_err_t touchpad_set_value(touch_pad_t tp_num);

/**
 * @brief init touchpad
 */
void touchpad_init(void);

/**
 * @brief calibrate all touch pad port to a medial value
 */
void touchpad_calibrate(void);

/**
 * @brief pass deepsleep delay timer to touchpad operation module
 *
 * @param  timer deepsleep delay timer handle
 */
void deepsleep_timer_pass(TimerHandle_t timer);

/**
 * @brief init rgb led
 */
void rgb_led_init();

/**
 * @brief  set value of touchpad with given touch pad number
 *
 * @param  color RGB led color
 * @param  bright_percent bright percent of RGB led(0 ~ 100)
 */
void rgb_led_set(uint8_t color, uint8_t bright_percent);

/**
 * @brief init rgb led
 *
 * @param  color RGB led color
 * @param  period_ms blink period of RGB led
 */
void rgb_led_blink_start(uint8_t color, uint32_t period_ms);

/**
 * @brief stop blink of RGB led
 */
void rgb_led_blink_stop();

/**
 * @brief init segment led ch450
 */
void ch450_dev_init();

/**
 * @brief clear segment led ch450
 */
void ch450_clear();

/**
 * @brief show positon value of linear slider in segment led ch450
 *
 * @param  pos positon value of linear slider
 */
void ch450_write_pos(uint8_t pos);

/**
 * @brief show touchpad button index in segment led ch450
 *
 * @param  button_index touchpad button index
 */
void ch450_write_button(uint8_t button_index);

#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif /**< __TOUCHPAD_H__ */
