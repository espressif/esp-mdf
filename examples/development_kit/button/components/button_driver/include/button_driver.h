// Copyright 2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
