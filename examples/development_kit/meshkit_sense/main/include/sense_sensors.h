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

#ifndef __SENSE_SENSOR_H__
#define __SENSE_SENSOR_H__

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

/**
 * @brief  init sensors in meshkit_sense board.
 *         The sensors include hts221(temperature&humidity sensor) and bh1750(luminance sensor).
 */
void sensor_init();

/**
 * @brief  deinit sensors in meshkit_sense board.
 *         The sensors include hts221(temperature&humidity sensor) and bh1750(luminance sensor).
 */
void sensor_deinit();
/**
 * @brief  power on sensors.
 *         The power of sensors was controlled by GPIO_NUM_27.
 */
void sensor_power_on();

/**
 * @brief  Init GPIO as digital GPIO
 */
void rtc_reset_gpio_degital();

/**
 * @brief  power on sensors in deepsleep mode.
 */
void rtc_sensor_power_on();

/**
 * @brief  Set i2c IO(SDA and SCL) as RTC GPIO.
 *         The i2c io should set to RTC GPIO befor the board entering deepsleep mode.
 */
void rtc_sensor_io_init();

/**
 * @brief   get luminance value of bh1750 sensor
 *
 * @param lum_value pointer to luminance value
 */
void bh1750_get_luminance(float *lum_value);

/**
 * @brief   get temperature value of hts221 sensor
 *
 * @attention The temperature value obtained should be devided by 10 to get the actual value.
 *
 * @param temp_value pointer to temperature value
 */
void hts221_get_temperature(int16_t *temp_value);

/**
 * @brief   get humidity value of hts221 sensor
 *
 * @attention The humidity value obtained should be devided by 10 to get the actual value.
 *
 * @param hum_value pointer to humidity value
 */
void hts221_get_humidity(int16_t *hum_value);

/**
 * @brief show ulp date when wake up from ulp stub
 */
void show_ulp_value();

/**
 * @brief   set the luminance threadhold of bh1750
 *
 * @param lum_min_value low threadhold of luminance
 * @param lum_max_value upper threadhold of luminance
 * @param lum_step      smallest change value of luminance
 */
void bh1750_set_luminance_threshold(int16_t lum_min_value, int16_t lum_max_value, uint16_t lum_step);

/**
 * @brief   set the temperature threadhold of hts221
 *
 * @param temp_min_value low threadhold of temperature
 * @param temp_max_value upper threadhold of temperature
 * @param temp_step      smallest change value of temperature
 */
void hts221_set_temperature_threshold(int16_t temp_min_value, int16_t temp_max_value, uint16_t temp_step);

/**
 * @brief   set the humidity threadhold of hts221
 *
 * @param hum_min_value low threadhold of humidity
 * @param hum_max_value upper threadhold of humidity
 * @param hum_step      smallest change value of humidity
 */
void hts221_set_humidity_threshold(int16_t hum_min_value, int16_t hum_max_value, uint16_t hum_step);

#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif /**< __SENSE_SENSOR_H__ */
