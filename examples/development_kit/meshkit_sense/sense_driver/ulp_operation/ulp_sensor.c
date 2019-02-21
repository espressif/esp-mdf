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
#include "ulp_sensor.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/rtc.h"
#include "soc/soc.h"
#include "soc/sens_reg.h"
#include "driver/rtc_io.h"
#include "driver/adc.h"
#include "esp32/ulp.h"
#include "esp_log.h"
#include "ulp_ulp_operation.h"

static const char *TAG = "ulp_sensor";

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_ulp_operation_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_ulp_operation_bin_end");

/* This function is called once after power-on reset, to load ULP program into
 * RTC memory.
 */
void init_ulp_program()
{
    esp_err_t err = ulp_load_binary(0, ulp_main_bin_start,
                                    (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);
}

void set_ulp_read_interval(uint32_t time_ms)
{
    const uint32_t sleep_cycles = rtc_clk_slow_freq_get_hz() * time_ms / 1000;
    REG_WRITE(SENS_ULP_CP_SLEEP_CYC0_REG, sleep_cycles);
}

/* This function is called every time before going into deep sleep.
 * It starts the ULP program and resets measurement counter.
 */
void start_ulp_program()
{
    esp_err_t err = ulp_run((&ulp_entry - RTC_SLOW_MEM) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);
}

void print_ulp_raw_data()
{
    ESP_LOGI(TAG, "ulp_raw_hum_lsb:%x", (uint16_t)ulp_raw_hum_lsb);
    ESP_LOGI(TAG, "ulp_raw_hum_msb:%x", (uint16_t)ulp_raw_hum_msb);
    ESP_LOGI(TAG, "ulp_raw_hum_value:%d", (int16_t)((ulp_raw_hum_msb << 8) | ulp_raw_hum_lsb));
    ESP_LOGI(TAG, "ulp_last_raw_hum_value:%d", (int16_t)ulp_last_raw_hum_value);
    ESP_LOGI(TAG, "ulp_raw_temp_lsb:%x", (uint16_t)ulp_raw_temp_lsb);
    ESP_LOGI(TAG, "ulp_raw_temp_msb:%x", (uint16_t)ulp_raw_temp_msb);
    ESP_LOGI(TAG, "ulp_raw_temp_value:%d", (int16_t)((ulp_raw_temp_msb << 8) | ulp_raw_temp_lsb));
    ESP_LOGI(TAG, "ulp_last_raw_temp_value:%d", (int16_t)ulp_last_raw_temp_value);
    ESP_LOGI(TAG, "ulp_raw_lum_value:%d", (int16_t)ulp_raw_lum_value);
    ESP_LOGI(TAG, "ulp_last_raw_lum_value:%d", (int16_t)ulp_last_raw_lum_value);
}

void set_ulp_bh1750_luminance_threshold(bh1750_handle_t  bh1750, int16_t lum_min_value, int16_t lum_max_value, uint16_t lum_step)
{
    ulp_lum_min_value = lum_min_value * 1.2;
    ulp_lum_max_value = lum_max_value * 1.2;
    ulp_lum_step = lum_step * 1.2;

    float lum_value = 0;
    iot_bh1750_get_data(bh1750, &lum_value);
    ulp_last_raw_lum_value = lum_value * 1.2;
    ESP_LOGD(TAG, "ulp_lum_min_value:%d", (uint16_t)ulp_lum_min_value);
    ESP_LOGD(TAG, "ulp_lum_max_value:%d", (uint16_t)ulp_lum_max_value);
    ESP_LOGD(TAG, "ulp_lum_step:%d", (uint16_t)ulp_lum_step);
    ESP_LOGD(TAG, "ulp_last_raw_lum_value:%d", (uint16_t)ulp_last_raw_lum_value);
}

void set_ulp_hts221_humidity_threshold(hts221_handle_t hts221, int16_t hum_min_value, int16_t hum_max_value, uint16_t hum_step)
{
    int16_t h0_t0_out, h_t_out, h1_t0_out;
    int16_t h0_rh, h1_rh;
    uint8_t buffer[2];

    iot_hts221_read(hts221, HTS221_H0_RH_X2, 2, buffer);
    h0_rh = buffer[0] >> 1;
    h1_rh = buffer[1] >> 1;

    iot_hts221_read(hts221, HTS221_H0_T0_OUT_L, 2, buffer);
    h0_t0_out = (int16_t)(((uint16_t)buffer[1]) << 8) | (uint16_t)buffer[0];

    iot_hts221_read(hts221, HTS221_H1_T0_OUT_L, 2, buffer);
    h1_t0_out = (int16_t)(((uint16_t)buffer[1]) << 8) | (uint16_t)buffer[0];

    iot_hts221_read(hts221, HTS221_HR_OUT_L_REG, 2, buffer);
    h_t_out = (int16_t)(((uint16_t)buffer[1]) << 8) | (uint16_t)buffer[0];

    ulp_hum_min_value = (int16_t)((hum_min_value - h0_rh) * (h1_t0_out - h0_t0_out) / (h1_rh - h0_rh) + h0_t0_out);
    ulp_hum_max_value = (int16_t)((hum_max_value - h0_rh) * (h1_t0_out - h0_t0_out) / (h1_rh - h0_rh) + h0_t0_out);
    ulp_hum_step = (int16_t)(0 - (int16_t)(hum_step * (h1_t0_out - h0_t0_out) / (h1_rh - h0_rh)));
    ulp_last_raw_hum_value = (int16_t)h_t_out;

    ESP_LOGD(TAG, "h0_t0_out:%d", (int16_t)h0_t0_out);
    ESP_LOGD(TAG, "h1_t0_out:%d", (int16_t)h1_t0_out);
    ESP_LOGD(TAG, "ulp_hum_min_value:%d", (int16_t)ulp_hum_min_value);
    ESP_LOGD(TAG, "ulp_hum_max_value:%d", (int16_t)ulp_hum_max_value);
    ESP_LOGD(TAG, "ulp_hum_step:%d", (int16_t)ulp_hum_step);
    ESP_LOGD(TAG, "ulp_last_raw_hum_value:%d", (int16_t)ulp_last_raw_hum_value);
}

void set_ulp_hts221_temperature_threshold(hts221_handle_t hts221, int16_t temp_min_value, int16_t temp_max_value, uint16_t temp_step)
{
    int16_t t0_out, t1_out, t_out, t0_degc_x8_u16, t1_degc_x8_u16;
    int16_t t0_degc, t1_degc;
    uint8_t buffer[4], tmp_8;
    iot_hts221_read(hts221, HTS221_T0_DEGC_X8, 2, buffer);
    iot_hts221_read(hts221, HTS221_T0_T1_DEGC_H2, 1, &tmp_8);
    t0_degc_x8_u16 = (((uint16_t)(tmp_8 & 0x03)) << 8) | ((uint16_t)buffer[0]);
    t1_degc_x8_u16 = (((uint16_t)(tmp_8 & 0x0C)) << 6) | ((uint16_t)buffer[1]);
    t0_degc = t0_degc_x8_u16 >> 3;
    t1_degc = t1_degc_x8_u16 >> 3;

    iot_hts221_read(hts221, HTS221_T0_OUT_L, 4, buffer);
    t0_out = (((uint16_t)buffer[1]) << 8) | (uint16_t)buffer[0];
    t1_out = (((uint16_t)buffer[3]) << 8) | (uint16_t)buffer[2];

    iot_hts221_read(hts221, HTS221_TEMP_OUT_L_REG, 2, buffer);
    t_out = (((uint16_t)buffer[1]) << 8) | (uint16_t)buffer[0];

    ulp_temp_min_value = (int16_t)((temp_min_value - t0_degc) * (t1_out - t0_out) / (t1_degc - t0_degc) + t0_out);
    ulp_temp_max_value = (int16_t)((temp_max_value - t0_degc) * (t1_out - t0_out) / (t1_degc - t0_degc) + t0_out);
    ulp_temp_step = (int16_t)(temp_step * (t1_out - t0_out) / (t1_degc - t0_degc));
    ulp_last_raw_temp_value = (int16_t)t_out;

    ESP_LOGD(TAG, "t0_out:%d", (int16_t)t0_out);
    ESP_LOGD(TAG, "t1_out:%d", (int16_t)t1_out);
    ESP_LOGD(TAG, "ulp_temp_min_value:%d", (int16_t)ulp_temp_min_value);
    ESP_LOGD(TAG, "ulp_temp_max_value:%d", (int16_t)ulp_temp_max_value);
    ESP_LOGD(TAG, "ulp_temp_step:%d", (int16_t)ulp_temp_step);
    ESP_LOGD(TAG, "ulp_last_raw_temp_value:%d", (int16_t)ulp_last_raw_temp_value);
}

esp_err_t get_ulp_hts221_humidity(hts221_handle_t hts221, int16_t *humidity)
{
    int16_t h0_t0_out, h1_t0_out, h_t_out;
    int16_t h0_rh, h1_rh;
    uint8_t buffer[2];
    int32_t tmp_32;

    iot_hts221_read(hts221, HTS221_H0_RH_X2, 2, buffer);
    h0_rh = buffer[0] >> 1;
    h1_rh = buffer[1] >> 1;

    iot_hts221_read(hts221, HTS221_H0_T0_OUT_L, 2, buffer);
    h0_t0_out = (int16_t)(((uint16_t)buffer[1]) << 8) | (uint16_t)buffer[0];

    iot_hts221_read(hts221, HTS221_H1_T0_OUT_L, 2, buffer);
    h1_t0_out = (int16_t)(((uint16_t)buffer[1]) << 8) | (uint16_t)buffer[0];

    h_t_out = (ulp_raw_hum_msb << 8) | ulp_raw_hum_lsb;

    tmp_32 = ((int32_t)(h_t_out - h0_t0_out)) * ((int32_t)(h1_rh - h0_rh) * 10);
    *humidity = tmp_32 / (int32_t)(h1_t0_out - h0_t0_out)  + h0_rh * 10;

    if (*humidity > 1000) {
        *humidity = 1000;
    }

    return ESP_OK;
}

esp_err_t get_ulp_hts221_temperature(hts221_handle_t hts221, int16_t *temperature)
{
    int16_t t0_out, t1_out, t_out, t0_degc_x8_u16, t1_degc_x8_u16;
    int16_t t0_degc, t1_degc;
    uint8_t buffer[4], tmp_8;
    uint32_t tmp_32;
    iot_hts221_read(hts221, HTS221_T0_DEGC_X8, 2, buffer);
    iot_hts221_read(hts221, HTS221_T0_T1_DEGC_H2, 1, &tmp_8);
    t0_degc_x8_u16 = (((uint16_t)(tmp_8 & 0x03)) << 8) | ((uint16_t)buffer[0]);
    t1_degc_x8_u16 = (((uint16_t)(tmp_8 & 0x0C)) << 6) | ((uint16_t)buffer[1]);
    t0_degc = t0_degc_x8_u16 >> 3;
    t1_degc = t1_degc_x8_u16 >> 3;

    iot_hts221_read(hts221, HTS221_T0_OUT_L, 4, buffer);
    t0_out = (((uint16_t)buffer[1]) << 8) | (uint16_t)buffer[0];
    t1_out = (((uint16_t)buffer[3]) << 8) | (uint16_t)buffer[2];

    t_out = (ulp_raw_temp_msb << 8) | ulp_raw_temp_lsb;

    tmp_32 = ((uint32_t)(t_out - t0_out)) * ((uint32_t)(t1_degc - t0_degc) * 10);
    *temperature = tmp_32 / (t1_out - t0_out) + t0_degc * 10;

    return ESP_OK;
}

esp_err_t get_ulp_bh1750_luminance(int16_t *luminance)
{
    *luminance = ulp_raw_lum_value;
    return ESP_OK;
}
