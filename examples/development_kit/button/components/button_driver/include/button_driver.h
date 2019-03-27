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

#ifndef __MDF_BUTTON_DRIVER_H__
#define __MDF_BUTTON_DRIVER_H__

#include "iot_button.h"
#include "mdf_common.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define MDF_EVENT_BUTTON_CHARGING_STOPED        (MDF_EVENT_CUSTOM_BASE + 0x01)
#define MDF_EVENT_BUTTON_CHARGING_COMPLETED     (MDF_EVENT_CUSTOM_BASE + 0x02)
#define MDF_EVENT_BUTTON_KEY_SHORT_PRESS        (MDF_EVENT_CUSTOM_BASE + 0x03)
#define MDF_EVENT_BUTTON_KEY_PUSH               (MDF_EVENT_CUSTOM_BASE + 0x04)
#define MDF_EVENT_BUTTON_KEY_RELEASE            (MDF_EVENT_CUSTOM_BASE + 0x05)
#define MDF_EVENT_BUTTON_KEY_LONG_PRESS_PUSH    (MDF_EVENT_CUSTOM_BASE + 0x06)
#define MDF_EVENT_BUTTON_KEY_LONG_PRESS_RELEASE (MDF_EVENT_CUSTOM_BASE + 0x07)
#define MDF_EVENT_BUTTON_KEY_EXCEPTION          (MDF_EVENT_CUSTOM_BASE + 0x08)

/**
 * @brief The state when the button is triggered
 */
typedef enum {
    BUTTON_KEY_NONE               = 0, /**< Button not triggered */
    BUTTON_KEY_SHORT_PRESS        = 1, /**< The button was short pressed */
    BUTTON_KEY_LONG_PRESS_PUSH    = 2, /**< The button was long pressed */
    BUTTON_KEY_LONG_PRESS_RELEASE = 3, /**< Press and hold the button to lift */
} button_key_status_t;

/**
 * @brief  Button initialize
 *
 * @return
 *      - MDF_OK
 *      - MDF_ERR_INVALID_ARG
 */
mdf_err_t button_driver_init();

/**
 * @brief  Button deinitialize
 *
 * @return
 *      - MDF_OK
 *      - MDF_ERR_INVALID_ARG
 */
mdf_err_t button_driver_deinit();

/**
 * @brief Whether to complete the battery charging
 *
 *
 * @return Button charging status
 */
bool button_battery_get_status();

/**
 * @brief The battery level of the button, this value is a percentage of the total battery
 * @return Battery power
 */
uint8_t button_battery_get_electricity();

/**
 *@brief Get the status record of the key
 *
 * @param  key_num Key sequence number
 * @return Key status
 */
button_key_status_t button_key_get_status(uint8_t key_num);

/**
 * @brief Reset key status record
 */
void button_key_reset_status();

/**
 * @brief Whether the usb interface is connected
 *
 * @return State of the usb interface
 */
bool button_usb_is_connected();

/**
 * @brief Set the color of the light
 *
 * @param  red   Red led brightness, the range is: 0 ~ 255
 * @param  green Green led brightness, the range is: 0 ~ 255
 * @param  blue  Blue led brightness, the range is: 0 ~ 255
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t button_driver_led_set_rgb(uint8_t red, uint8_t green, uint8_t blue);

/**
 * [button_driver_led_blink_start description]
 *
 * @param  red   Red led brightness, the range is: 0 ~ 255
 * @param  green Green led brightness, the range is: 0 ~ 255
 * @param  blue  Blue led brightness, the range is: 0 ~ 255
 * @param  period_ms breath period (uint: ms)
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t button_driver_led_blink_start(uint8_t red, uint8_t green, uint8_t blue, uint32_t period_ms);

/**
 * @brief Stop the blink of led
 *
 * @return
 *     - MDF_OK
 *     - MDF_FAIL
 */
mdf_err_t button_driver_led_blink_stop();

#ifdef __cplusplus
}
#endif

#endif/**< __MDF_BUTTON_DRIVER_H__ */
