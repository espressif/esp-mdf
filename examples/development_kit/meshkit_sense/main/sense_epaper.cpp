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

#include "mdf_common.h"

#include "iot_controllable_obj.h"
#include "epaper.h"
#include "epaper_fonts.h"
#include "ulp_sensor.h"

#include "sense_epaper.h"
#include "sense_sensors.h"

/**< Epaper pin definition */
#define EPAPER_BUSY_PIN        GPIO_NUM_5
#define EPAPER_RST_PIN         GPIO_NUM_18
#define EPAPER_DC_PIN          GPIO_NUM_19
#define EPAPER_CS_PIN          GPIO_NUM_21
#define EPAPER_SCK_PIN         GPIO_NUM_22
#define EPAPER_MOSI_PIN        GPIO_NUM_23
#define EPAPER_MISO_PIN        GPIO_NUM_37

/**< Epaper display resolution */
#define EPAPER_EPD_WIDTH       176
#define EPAPER_EPD_HEIGHT      264

/**< Epaper power control IO */
#define EPAPER_POWER_CNTL_IO   GPIO_NUM_14

static const char *TAG            = "sense_epaper";
static CPowerCtrl *g_epaper_power = NULL;
static epaper_handle_t g_epaper   = NULL;

void epaper_power_on()
{
    if (!g_epaper_power) {
        g_epaper_power = new CPowerCtrl(EPAPER_POWER_CNTL_IO);
    }
    g_epaper_power->on();
}

void epaper_gpio_init()
{
    epaper_conf_t epaper_conf;
    epaper_conf.busy_pin  = EPAPER_BUSY_PIN;
    epaper_conf.cs_pin    = EPAPER_CS_PIN;
    epaper_conf.dc_pin    = EPAPER_DC_PIN;
    epaper_conf.miso_pin  = EPAPER_MISO_PIN;
    epaper_conf.mosi_pin  = EPAPER_MOSI_PIN;
    epaper_conf.reset_pin = EPAPER_RST_PIN;
    epaper_conf.sck_pin   = EPAPER_SCK_PIN;

    gpio_config_t epaper_gpio_config;
    epaper_gpio_config.pin_bit_mask = (1 << epaper_conf.busy_pin) | (1 << epaper_conf.miso_pin);
    epaper_gpio_config.mode         = GPIO_MODE_INPUT;
    epaper_gpio_config.intr_type    = GPIO_INTR_DISABLE;
    epaper_gpio_config.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpio_config(&epaper_gpio_config);

    epaper_gpio_config.pin_bit_mask = (1 << epaper_conf.cs_pin)   | (1 << epaper_conf.dc_pin)   |
                                      (1 << epaper_conf.mosi_pin) | (1 << epaper_conf.reset_pin) |
                                      (1 << epaper_conf.sck_pin);
    epaper_gpio_config.mode         = GPIO_MODE_OUTPUT;
    gpio_config(&epaper_gpio_config);

    gpio_set_level((gpio_num_t)epaper_conf.cs_pin, 0);
    gpio_set_level((gpio_num_t)epaper_conf.dc_pin, 0);
    gpio_set_level((gpio_num_t)epaper_conf.mosi_pin, 0);
    gpio_set_level((gpio_num_t)epaper_conf.reset_pin, 0);
    gpio_set_level((gpio_num_t)epaper_conf.sck_pin, 0);

    MDF_LOGD("epaper_gpio_init success");
}

void epaper_show_status(float hum, float temp, float lum)
{
    char hum_str[6]  = {0};
    char temp_str[6] = {0};
    char lum_str[6]  = {0};

    iot_epaper_clean_paint(g_epaper, UNCOLORED);

    iot_epaper_draw_string(g_epaper, 190, 0, "@espressif", &epaper_font_12, COLORED);
    iot_epaper_draw_string(g_epaper, 15, 40, "Luminance", &epaper_font_16, COLORED);
    iot_epaper_draw_string(g_epaper, 15, 90, "Humidity", &epaper_font_16, COLORED);
    iot_epaper_draw_string(g_epaper, 15, 140, "Temperature", &epaper_font_16, COLORED);

    sprintf(hum_str, "%2.1f", hum);
    sprintf(temp_str, "%2.1f", temp);
    sprintf(lum_str, "%2.1f", lum);

    /**< dispaly string, use font16 */
    iot_epaper_draw_string(g_epaper, 180, 40, lum_str, &epaper_font_16, COLORED);
    iot_epaper_draw_string(g_epaper, 180, 90, hum_str, &epaper_font_16, COLORED);
    iot_epaper_draw_string(g_epaper, 180, 140, temp_str, &epaper_font_16, COLORED);

    iot_epaper_draw_horizontal_line(g_epaper, 10, 70, 240, COLORED);
    iot_epaper_draw_horizontal_line(g_epaper, 10, 120, 240, COLORED);
    iot_epaper_draw_vertical_line(g_epaper, 150, 20, 150, COLORED);
    iot_epaper_draw_rectangle(g_epaper, 10, 20, 250, 170, COLORED);

    /**< dispaly the frame buffer */
    iot_epaper_display_frame(g_epaper, NULL);
    MDF_LOGD("epaper_show_status, hum: %f, temp: %f, lum: %f", hum, temp, lum);
}

static void epaper_show_task(void *arg)
{
    int16_t temp = 0;
    int16_t hum  = 0;
    float lum    = 0;

    while (1) {
        bh1750_get_luminance(&lum);
        hts221_get_temperature(&temp);
        hts221_get_humidity(&hum);
        epaper_show_status((float)hum / 10, (float)temp / 10, lum);

        vTaskDelay(60 * 1000 / portTICK_RATE_MS);
    }
}

void epaper_show_deepsleep()
{
    iot_epaper_clean_paint(g_epaper, UNCOLORED);
    iot_epaper_draw_string(g_epaper, 190, 0, "@espressif", &epaper_font_12, COLORED);
    iot_epaper_draw_string(g_epaper, 90, 80, "Deepsleep", &epaper_font_16, COLORED);
    iot_epaper_display_frame(g_epaper, NULL);
    MDF_LOGD("epaper_show_deepsleep");
}

void epaper_init()
{
    epaper_conf_t epaper_conf;
    epaper_conf.busy_pin         = EPAPER_BUSY_PIN;
    epaper_conf.cs_pin           = EPAPER_CS_PIN;
    epaper_conf.dc_pin           = EPAPER_DC_PIN;
    epaper_conf.miso_pin         = EPAPER_MISO_PIN;
    epaper_conf.mosi_pin         = EPAPER_MOSI_PIN;
    epaper_conf.reset_pin        = EPAPER_RST_PIN;
    epaper_conf.sck_pin          = EPAPER_SCK_PIN;

    epaper_conf.rst_active_level = 0;
    epaper_conf.dc_lev_data      = 1;
    epaper_conf.dc_lev_cmd       = 0;

    epaper_conf.clk_freq_hz      = 20 * 1000 * 1000;
    epaper_conf.spi_host         = HSPI_HOST;

    epaper_conf.width            = EPAPER_EPD_WIDTH;
    epaper_conf.height           = EPAPER_EPD_HEIGHT;
    epaper_conf.color_inv        = 1;

    g_epaper = iot_epaper_create(NULL, &epaper_conf);
    iot_epaper_set_rotate(g_epaper, E_PAPER_ROTATE_270);

    xTaskCreate(epaper_show_task, "epaper_show_task", 1024 * 3, NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
    MDF_LOGD("epaper_init success");
}
