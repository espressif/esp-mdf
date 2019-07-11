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

#include "driver/rtc_io.h"
#include "driver/i2c.h"

#include "iot_controllable_obj.h"
#include "iot_hts221.h"
#include "iot_bh1750.h"
#include "iot_i2c_bus.h"
#include "ulp_sensor.h"

#include "sense_sensors.h"
#include "sense_epaper.h"

#define SENSOR_POWER_CNTL_IO    GPIO_NUM_27
#define SENSOR_I2C_PORT         I2C_NUM_1
#define SENSOR_I2C_SCL_IO       GPIO_NUM_32
#define SENSOR_I2C_SDA_IO       GPIO_NUM_33

static i2c_bus_handle_t i2c_bus = NULL;
static hts221_handle_t hts221   = NULL;
static bh1750_handle_t bh1750   = NULL;
static CPowerCtrl *sensor_power = NULL;

void hts221_get_temperature(int16_t *temp_value)
{
    iot_hts221_get_temperature(hts221, temp_value);
}

void hts221_get_humidity(int16_t *hum_value)
{
    iot_hts221_get_humidity(hts221, hum_value);
}

void bh1750_get_luminance(float* lum_value)
{
    iot_bh1750_get_data(bh1750, lum_value);
}

void bh1750_set_luminance_threshold(int16_t lum_min_value, int16_t lum_max_value, uint16_t lum_step)
{
    set_ulp_bh1750_luminance_threshold(bh1750, lum_min_value, lum_max_value, lum_step);
}

void hts221_set_temperature_threshold(int16_t temp_min_value, int16_t temp_max_value, uint16_t temp_step)
{
    set_ulp_hts221_temperature_threshold(hts221, temp_min_value, temp_max_value, temp_step);
}

void hts221_set_humidity_threshold(int16_t hum_min_value, int16_t hum_max_value, uint16_t hum_step)
{
    set_ulp_hts221_humidity_threshold(hts221, hum_min_value, hum_max_value, hum_step);
}

void show_ulp_value()
{
    int16_t raw_hum, raw_temp, raw_lum;
    get_ulp_hts221_humidity(hts221, &raw_hum);
    get_ulp_hts221_temperature(hts221, &raw_temp);
    get_ulp_bh1750_luminance(&raw_lum);
    epaper_show_status((float)(raw_hum / 10), (float)(raw_temp / 10), (float)(raw_lum * 10 / 12));
}

void sensor_power_on()
{
    if (!sensor_power) {
        sensor_power = new CPowerCtrl(SENSOR_POWER_CNTL_IO);
    }
    sensor_power->on();
}

void sensor_init()
{
    i2c_config_t i2c_config;
    i2c_config.mode             = I2C_MODE_MASTER;
    i2c_config.sda_io_num       = SENSOR_I2C_SDA_IO;
    i2c_config.sda_pullup_en    = GPIO_PULLUP_ENABLE;
    i2c_config.scl_io_num       = SENSOR_I2C_SCL_IO;
    i2c_config.scl_pullup_en    = GPIO_PULLUP_ENABLE;
    i2c_config.master.clk_speed = 100000;
    i2c_bus = iot_i2c_bus_create((i2c_port_t)SENSOR_I2C_PORT, &i2c_config);
    hts221 = iot_hts221_create(i2c_bus, HTS221_I2C_ADDRESS);

    hts221_config_t hts221_config;
    iot_hts221_get_config(hts221, &hts221_config);
    hts221_config.avg_h         = HTS221_AVGH_32;
    hts221_config.avg_t         = HTS221_AVGT_16;
    hts221_config.odr           = HTS221_ODR_1HZ;
    hts221_config.bdu_status    = HTS221_DISABLE;
    hts221_config.heater_status = HTS221_DISABLE;
    iot_hts221_set_config(hts221, &hts221_config);
    iot_hts221_set_activate(hts221);

    bh1750 = iot_bh1750_create(i2c_bus, BH1750_I2C_ADDRESS_DEFAULT);
    iot_bh1750_power_on(bh1750);
    iot_bh1750_set_measure_mode(bh1750, BH1750_CONTINUE_4LX_RES);
}

void sensor_deinit()
{
    iot_hts221_delete(hts221, false);
    iot_bh1750_delete(bh1750, false);
    iot_i2c_bus_delete(i2c_bus);
    /* If pull up of I2C IO did not be disabled, the current in deepsleep mode will be greater */
    gpio_pullup_dis(SENSOR_I2C_SDA_IO);
    gpio_pullup_dis(SENSOR_I2C_SCL_IO);
}

void rtc_reset_gpio_degital()
{
    rtc_gpio_deinit(SENSOR_POWER_CNTL_IO);
    rtc_gpio_deinit(SENSOR_I2C_SCL_IO);
    rtc_gpio_deinit(SENSOR_I2C_SDA_IO);
}

void rtc_sensor_power_on()
{
    rtc_gpio_init(SENSOR_POWER_CNTL_IO);
    rtc_gpio_set_direction(SENSOR_POWER_CNTL_IO, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level(SENSOR_POWER_CNTL_IO, 0);
}

void rtc_sensor_io_init()
{
    rtc_gpio_init(SENSOR_I2C_SCL_IO);
    rtc_gpio_set_direction(SENSOR_I2C_SCL_IO, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_init(SENSOR_I2C_SDA_IO);
    rtc_gpio_set_direction(SENSOR_I2C_SDA_IO, RTC_GPIO_MODE_INPUT_ONLY);
}
