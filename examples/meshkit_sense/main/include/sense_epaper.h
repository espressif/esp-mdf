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

#ifndef __SENSE_EPAPER_H__
#define __SENSE_EPAPER_H__

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

/**
 * @brief  init GPIOs of epaper.
 */
void epaper_gpio_init();

/**
 * @brief  power on epaper.
 *         The power of epaper was controlled by GPIO_NUM_14.
 */
void epaper_power_on();

/**
 * @brief  init epaper.
 *
 * @attention This API could be called after epaper_gpio_init() and epaper_power_on().
 *
 */
void epaper_init();

/**
 * @brief  show deepsleep state in epaper.
 */
void epaper_show_deepsleep();

/**
 * @brief show states of sensors in epaper with given data
 *
 * @param hum  humidity value
 * @param temp temperature value
 * @param lum  luminance value
 */
void epaper_show_status(float hum, float temp, float lum);

#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif /**< __SENSE_EPAPER_H__ */
