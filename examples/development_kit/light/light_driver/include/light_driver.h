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

/**
 * @brief The mode of the five-color light
 */
enum light_mode {
    MODE_NONE                = 0,
    MODE_RGB                 = 1,
    MODE_HSV                 = 2,
    MODE_CTB                 = 3,
    MODE_ON                  = 4,
    MODE_OFF                 = 5,
    MODE_HUE_INCREASE        = 4,
    MODE_HUE_DECREASE        = 5,
    MODE_WARM_INCREASE       = 6,
    MODE_WARM_DECREASE       = 7,
    MODE_BRIGHTNESS_INCREASE = 8,
    MODE_BRIGHTNESS_DECREASE = 9,
};

/**
 * @brief Light driven configuration
 */
typedef struct {
    gpio_num_t gpio_red;      /**< Red corresponds to GPIO */
    gpio_num_t gpio_green;    /**< Green corresponds to GPIO */
    gpio_num_t gpio_blue;     /**< Blue corresponds to GPIO */
    gpio_num_t gpio_cold;     /**< Cool corresponds to GPIO */
    gpio_num_t gpio_warm;     /**< Warm corresponds to GPIO */
    uint32_t fade_period_ms;  /**< The time from the current color to the next color */
    uint32_t blink_period_ms; /**< Period of flashing lights */
} light_driver_config_t;

/**
 * @brief  Light initialize
 *
 * @param  config [description]
 *
 * @return
 *      - MDF_OK
 *      - MDF_ERR_INVALID_ARG
 */
mdf_err_t light_driver_init(light_driver_config_t *config);

/**
 * @brief  Light deinitialize
 *
 * @return
 *      - MDF_OK
 *      - MDF_ERR_INVALID_ARG
 */
mdf_err_t light_driver_deinit();


/**
 * @brief Set the fade time of the light
 *
 * @param  fade_period_ms  The time from the current color to the next color
 * @param  blink_period_ms Light flashing frequency
 *
 * @return
 *      - MDF_OK
 *      - MDF_FAIL
 */
mdf_err_t light_driver_config(uint32_t fade_period_ms, uint32_t blink_period_ms);

/**@{*/
/**
 * @brief  Set the status of the light
 *
 *
 * @return
 *      - MDF_OK
 *      - MDF_ERR_INVALID_ARG
 */
mdf_err_t light_driver_set_hue(uint16_t hue);
mdf_err_t light_driver_set_saturation(uint8_t saturation);
mdf_err_t light_driver_set_value(uint8_t value);
mdf_err_t light_driver_set_color_temperature(uint8_t color_temperature);
mdf_err_t light_driver_set_brightness(uint8_t brightness);
mdf_err_t light_driver_set_hsv(uint16_t hue, uint8_t saturation, uint8_t value);
mdf_err_t light_driver_set_ctb(uint8_t color_temperature, uint8_t brightness);
mdf_err_t light_driver_set_switch(bool status);

/**@}*/

/**@{*/
/**
 * @brief  Set the status of the light
 */
uint16_t light_driver_get_hue();
uint8_t light_driver_get_saturation();
uint8_t light_driver_get_value();
mdf_err_t light_driver_get_hsv(uint16_t *hue, uint8_t *saturation, uint8_t *value);
uint8_t light_driver_get_color_temperature();
uint8_t light_driver_get_brightness();
mdf_err_t light_driver_get_ctb(uint8_t *color_temperature, uint8_t *brightness);
bool light_driver_get_switch();
uint8_t light_driver_get_mode();
/**@}*/

/**@{*/
/**
 * @brief  Used to indicate the operating mode, such as configuring the network mode, upgrading mode
 *
 * @note   The state of the light is not saved in nvs
 *
 * @return
 *      - MDF_OK
 *      - MDF_ERR_INVALID_ARG
 */
mdf_err_t light_driver_set_rgb(uint8_t red, uint8_t green, uint8_t blue);
mdf_err_t light_driver_breath_start(uint8_t red, uint8_t green, uint8_t blue);
mdf_err_t light_driver_breath_stop();
mdf_err_t light_driver_blink_start(uint8_t red, uint8_t green, uint8_t blue);
mdf_err_t light_driver_blink_stop();
/**@}*/

/**@{*/
/**
 * @brief  Color gradient
 *
 * @return
 *      - MDF_OK
 *      - MDF_ERR_INVALID_ARG
 */
mdf_err_t light_driver_fade_brightness(uint8_t brightness);
mdf_err_t light_driver_fade_hue(uint16_t hue);
mdf_err_t light_driver_fade_warm(uint8_t color_temperature);
mdf_err_t light_driver_fade_stop();
/**@}*/

#ifdef __cplusplus
}
#endif

#endif/**< __MDF_LIGHT_H__ */
