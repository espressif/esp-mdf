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

#ifndef __IOT_LIGHT_H__
#define __IOT_LIGHT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/ledc.h"

typedef void* light_handle_t;

#define LIGHT_MAX_CHANNEL_NUM   (5)

typedef enum {
    LIGHT_SET_DUTY_DIRECTLY = 0,    /*!< set duty directly */
    LIGHT_DUTY_FADE_1S,             /*!< set duty with fade in 1 second */
    LIGHT_DUTY_FADE_2S,             /*!< set duty with fade in 2 second */
    LIGHT_DUTY_FADE_3S,             /*!< set duty with fade in 3 second */
    LIGHT_DUTY_FADE_MAX,            /*!< user shouldn't use this */
} light_duty_mode_t;

typedef enum {
    LIGHT_CH_NUM_1 = 1,             /*!< Light channel number */
    LIGHT_CH_NUM_2 = 2,             /*!< Light channel number */
    LIGHT_CH_NUM_3 = 3,             /*!< Light channel number */
    LIGHT_CH_NUM_4 = 4,             /*!< Light channel number */
    LIGHT_CH_NUM_5 = 5,             /*!< Light channel number */
    LIGHT_CH_NUM_MAX,               /*!< user shouldn't use this */
} light_channel_num_t;

/**
  * @brief  light initialize
  *
  * @param  timer the ledc timer used by light
  * @param  speed_mode
  * @param  freq_hz frequency of timer
  * @param  channel_num decide how many channels the light contains
  * @param  timer_bit
  *
  * @return  the handle of light
  */
light_handle_t iot_light_create(ledc_timer_t timer, ledc_mode_t speed_mode,
                                uint32_t freq_hz, uint8_t channel_num, ledc_timer_bit_t timer_bit);

/**
  * @brief  add an output channel to light
  *
  * @param  light_handle
  * @param  channel_idx the id of channel (0 ~ channel_num-1)
  * @param  io_num
  * @param  channel the ledc channel you want to use
  *
  * @return
  *     - ESP_OK: succeed
  *     - others: fail
  */
esp_err_t iot_light_channel_regist(light_handle_t light_handle,
                                   uint8_t channel_idx, gpio_num_t io_num, ledc_channel_t channel);

/**
  * @brief  free the momery of light
  *
  * @param  light_handle
  *
  * @return
  *     - ESP_OK: succeed
  *     - others: fail
  */
esp_err_t iot_light_delete(light_handle_t light_handle);

/**
  * @brief  set light fade with time
  *
  * @param  light_handle
  * @param  channel_id
  * @param  duty
  * @param  fade_period_ms
  *
  * @return
  *     - ESP_OK: succeed
  *     - others: fail
  */
esp_err_t iot_light_fade_with_time(light_handle_t light_handle,
                                   uint8_t channel_id, uint32_t duty, uint32_t fade_period_ms);

/**
  * @brief  set breath config of a light channel, call `iot_light_breath_start` to start breath operation
  *
  * @param  light_handle
  * @param  channel_id
  * @param  duty
  * @param  breath_period
  *
  * @return
  *     - ESP_OK: succeed
  *     - others: fail
  */
esp_err_t iot_light_breath_config(light_handle_t light_handle,
                                  uint8_t channel_id, uint32_t duty, int breath_period);

/**
  * @brief  start breath operation, user need to set breath config befor call this API
  *
  * @param  light_handle
  * @param  channel_id
  *
  * @return
  *     - ESP_OK: succeed
  *     - others: fail
  */
esp_err_t iot_light_breath_start(light_handle_t light_handle, uint8_t channel_id);

/**
  * @brief  stop breath operation
  *
  * @param  light_handle
  * @param  channel_id
  *
  * @return
  *     - ESP_OK: succeed
  *     - others: fail
  */
esp_err_t iot_light_breath_stop(light_handle_t light_handle, uint8_t channel_id);

/**
  * @brief  set blink config of a light channel, call `iot_light_blink_start` to start blink operation
  *
  * @param  light_handle
  * @param  channel_id
  * @param  blink_period_ms
  *
  * @return
  *     - ESP_OK: succeed
  *     - others: fail
  */
esp_err_t iot_light_blink_config(light_handle_t light_handle, uint8_t channel_id, uint32_t blink_period_ms);

/**
  * @brief  start blink operation, user need to set blink config befor call this API
  *
  * @param  light_handle
  * @param  channel_id
  *
  * @return
  *     - ESP_OK: succeed
  *     - others: fail
  */
esp_err_t iot_light_blink_start(light_handle_t light_handle, uint8_t channel_id);

/**
  * @brief  stop blink operation
  *
  * @param  light_handle
  * @param  channel_id
  *
  * @return
  *     - ESP_OK: succeed
  *     - others: fail
  */
esp_err_t iot_light_blink_stop(light_handle_t light_handle, uint8_t channel_id);

#ifdef __cplusplus
}
#endif

#endif /**< __IOT_LIGHT_H__ */