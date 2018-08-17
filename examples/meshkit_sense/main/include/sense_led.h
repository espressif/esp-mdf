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

#ifndef __SENSE_LED_H__
#define __SENSE_LED_H__

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

typedef enum {
    STATE_LED_WIFI    = 0,
    STATE_LED_NETWORK = 1,
    STATE_LED_MAX     = 2,
} state_led_type_t;

typedef enum {
    STATE_LED_OPRT_ON          = 0, /**< Turn on led */
    STATE_LED_OPRT_OFF         = 1, /**< Turn off led */
    STATE_LED_OPRT_SLOW_BLINK  = 2, /**< Start slow blink */
    STATE_LED_OPRT_QUICK_BLINK = 3, /**< Start quick blink */
    STATE_LED_OPRT_MAX         = 4, /**< Invalid operation */
} state_led_oprt_t;

/**
 * @brief  init state led in meshkit_sense board, include wifi led and network led.
 */
void sense_state_led_init();

/**
 * @brief  operate state led with given led type and operation.
 *
 * @param  led_type  type of state led
 * @param  led_oprt  operation of state led
 */
void sense_state_led_oprt(state_led_type_t led_type, state_led_oprt_t led_oprt);

/**
 * @brief hold the electrical level of state led pin in deep-sleep mode
 */
void sense_state_led_io_hold();

#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif /**< __SENSE_LED_H__ */
