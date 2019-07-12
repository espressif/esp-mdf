// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
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
#ifndef _IOT_ULP_SENSOR_H_
#define _IOT_ULP_SENSOR_H_

#include "iot_hts221.h"
#include "iot_bh1750.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief init ulp program.
 */
void init_ulp_program();

/**
 * @brief set interval time of ulp to read sensors' data
 *
 * @param time_ms interval time (millisecond)
 */
void set_ulp_read_interval(uint32_t time_ms);

/**
 * @brief start ulp program
 */
void start_ulp_program();

/**
 * @brief print the raw data of ulp.
 */
void print_ulp_raw_data();

/**
 * @brief set luminance threshold of sensor befor enter deepsleep mode.
 *
 * If the luminance value is less than the lower threshold or more than the upper threahold,
 * or the variation of luminance value is more than the lum_step, the ulp will cause wakeup.
 *
 * @param bh1750        handle of sensor bh1750
 * @param lum_min_value lower threshold value of luminance
 * @param lum_max_value upper threshold value of luminance
 * @param lum_step      minimum change of luminance to cause wakeup (Unimplemented).
 */
void set_ulp_bh1750_luminance_threshold(bh1750_handle_t  bh1750, int16_t lum_min_value, int16_t lum_max_value, uint16_t lum_step);

/**
 * @brief set temperature threshold of sensor befor enter deepsleep mode.
 *
 * If the temperature value is less than the lower threshold or more than the upper threahold,
 * or the variation of temperature value is more than the temp_step, the ulp will cause wakeup.
 *
 * @param hts221         handle of sensor hts221
 * @param temp_min_value lower threshold value of temperature
 * @param temp_max_value upper threshold value of temperature
 * @param temp_step      minimum change of temperature to cause wakeup (Unimplemented).
 */
void set_ulp_hts221_temperature_threshold(hts221_handle_t hts221, int16_t temp_min_value, int16_t temp_max_value, uint16_t temp_step);

/**
 * @brief set humidity threshold of sensor befor enter deepsleep mode.
 *
 * If the humidity value is less than the lower threshold or more than the upper threahold,
 * or the variation of humidity value is more than the hum_step, the ulp will cause wakeup.
 *
 * @param hts221        handle of sensor hts221
 * @param hum_min_value lower threshold value of humidity
 * @param hum_max_value upper threshold value of humidity
 * @param hum_step      minimum change of humidity to cause wakeup (Unimplemented).
 */
void set_ulp_hts221_humidity_threshold(hts221_handle_t hts221, int16_t hum_min_value, int16_t hum_max_value, uint16_t hum_step);

/**
 * @brief  get luminance value of sensor bh1750
 *
 * @param  luminance pointer to temperature value
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 */
esp_err_t get_ulp_bh1750_luminance(int16_t *luminance);

/**
 * @brief  get temperature value of sensor hts221
 *
 * @param  hts221      handle of sensor hts221
 * @param  temperature pointer to temperature value
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 */
esp_err_t get_ulp_hts221_temperature(hts221_handle_t hts221, int16_t *temperature);

/**
 * @brief  get humidity value of sensor hts221
 *
 * @param  hts221   handle of sensor hts221
 * @param  humidity pointer to humidity value
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 */
esp_err_t get_ulp_hts221_humidity(hts221_handle_t hts221, int16_t *humidity);

#ifdef __cplusplus
}
#endif

#endif
