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
#include "espnow_lcd.h"
#include "mdf_common.h"

#include "rom/ets_sys.h"
#include "driver/spi_master.h"
#include "iot_lcd.h"
#include "Adafruit_GFX.h"

#define PROGMEM
#include "FreeMono9pt7b.h"
#include "FreeSansOblique9pt7b.h"


/*!< max characters in one line of lcd */
#define LCD_STR_LEN 30

static const char *TAG = "espnow_lcd";

static CEspLcd* g_lcd_obj = NULL;

static esp_err_t espnow_lcd_info_write(uint8_t type, const uint8_t *data, uint8_t len)
{
    MDF_ERROR_CHECK(type >= ESPNOW_NET_INFO_MAX, ESP_FAIL, "type: %d", type);

    char content[LCD_STR_LEN] = { 0 };
    const uint16_t str_x      = 10; /*!< Start position X of content, set 10 */
    const uint16_t str_y      = 45; /*!< Start position Y of first line content, set 45 */

    switch (type) {
    case ESPNOW_NET_INFO_ID:
        MDF_ERROR_CHECK(len != 6, ESP_FAIL, "ESPNOW_NET_INFO_ID len: %d", len);
        sprintf(content, "ID: %02x%02x%02x%02x%02x%02x", MAC2STR(data));
        break;

    case ESPNOW_NET_INFO_ROOT_MAC:
        MDF_ERROR_CHECK(len != 6, ESP_FAIL, "ESPNOW_NET_INFO_ROOT_MAC len: %d", len);
        sprintf(content, "Mac: %02x%02x%02x%02x%02x%02x", MAC2STR(data));
        break;

    case ESPNOW_NET_INFO_ROOT_IP:
        MDF_ERROR_CHECK(len != 4, ESP_FAIL, "ESPNOW_NET_INFO_ROOT_IP len: %d", len);
        sprintf(content, "IP: " IPSTR, data[0], data[1], data[2], data[3]);
        break;

    case ESPNOW_NET_INFO_MAX_LAYER:
        sprintf(content, "Max Layer: %d", data[0]);
        break;

    case ESPNOW_NET_INFO_MAX_CONN:
        sprintf(content, "Max Conn: %d", data[0]);
        break;

    case ESPNOW_NET_INFO_DEV_NUM:
        sprintf(content, "Dev Num: %d", data[0]);
        break;

    default:
        break;
    }

    g_lcd_obj->setTextColor(COLOR_WHITE, COLOR_BLACK);
    g_lcd_obj->fillRect(1, str_y - 12 + type * 20, 238, 16, COLOR_BLACK);
    g_lcd_obj->drawString(content, str_x, str_y + type * 20);

    return ESP_OK;
}


esp_err_t espnow_lcd_ana_write(uint8_t type, const uint8_t *data, uint8_t len)
{
    MDF_ERROR_CHECK(type >= ESPNOW_NET_ANA_MAX, ESP_FAIL, "type err: %d", type);

    char content[LCD_STR_LEN] = { 0 };
    const uint16_t str_x      = 10;
    const uint16_t str_y      = 205;

    /*!< it depends on the funciton to maintain */
    static uint32_t s_err_count      = 0;
    static uint32_t s_warn_count     = 0;
    static uint32_t s_reboot_count   = 0;
    static uint32_t s_coredump_count = 0;

    switch (type) {
    case ESPNOW_NET_ANA_WARN:
        g_lcd_obj->setTextColor(COLOR_YELLOW, COLOR_BLACK);
        sprintf(content, "Warn Log: %d", s_warn_count);
        s_warn_count++;
        break;

    case ESPNOW_NET_ANA_ERR:
        g_lcd_obj->setTextColor(COLOR_RED, COLOR_BLACK);
        sprintf(content, "Err Log: %d", s_err_count);
        s_err_count++;
        break;

    case ESPNOW_NET_ANA_REBOOT_NUM:
        g_lcd_obj->setTextColor(COLOR_WHITE, COLOR_BLACK);
        sprintf(content, "Reboot: %d", s_reboot_count);
        s_reboot_count++;
        break;

    case ESPNOW_NET_ANA_DUMP_NUM:
        g_lcd_obj->setTextColor(COLOR_WHITE, COLOR_BLACK);
        sprintf(content, "Coredump: %d", s_coredump_count);
        s_coredump_count++;
        break;

    case ESPNOW_NET_ANA_DEV_NUM:
        g_lcd_obj->setTextColor(COLOR_WHITE, COLOR_BLACK);
        sprintf(content, "Rece Dev: %d", data[0]);
        break;

    case ESPNOW_NET_ANA_RUNTIME:
        g_lcd_obj->setTextColor(COLOR_WHITE, COLOR_BLACK);
        sprintf(content, "Running Time: %ds", *((uint32_t *)data) / 1000);
        break;

    default:
        break;
    }

    g_lcd_obj->fillRect(1, str_y - 12 + type * 20, 238, 16, COLOR_BLACK);
    g_lcd_obj->drawString(content, str_x, str_y + type * 20);

    return ESP_OK;
}

esp_err_t espnow_lcd_info_update(const uint8_t *addr, const char *text)
{
    MDF_PARAM_CHECK(addr);
    MDF_PARAM_CHECK(text);

    esp_err_t ret       = ESP_OK;
    char *index         = NULL;
    uint8_t index_data  = 0;
    uint8_t mac_data[7] = { 0 }; /*!< last byte is zero */

    ret = espnow_lcd_info_write(ESPNOW_NET_INFO_ROOT_IP, addr, 4);
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "espnow_lcd_info_write ret: %d", ret);

    index = strstr(text, "mesh_id");
    if (index) {
        str2mac(index + strlen("mesh_id="), mac_data);
        espnow_lcd_info_write(ESPNOW_NET_INFO_ID, mac_data, 6);
    }

    index = strstr(text, "mac");
    if (index) {
        str2mac(index + strlen("mac="), mac_data);
        espnow_lcd_info_write(ESPNOW_NET_INFO_ROOT_MAC, mac_data, 6);
    }

    index = strstr(text, "max_layer");
    if (index) {
        index_data = (index + strlen("max_layer="))[0] - '0';
        espnow_lcd_info_write(ESPNOW_NET_INFO_MAX_LAYER, &index_data, 1);
    }

    index = strstr(text, "max_conn");
    if (index) {
        index_data = (index + strlen("max_conn="))[0] - '0';
        espnow_lcd_info_write(ESPNOW_NET_INFO_MAX_CONN, &index_data, 1);
    }

    index = strstr(text, "dev_num");
    if (index) {
        index_data = (index + strlen("dev_num="))[0] - '0';
        espnow_lcd_info_write(ESPNOW_NET_INFO_DEV_NUM, &index_data, 1);
    }

    return ESP_OK;
}

void espnow_lcd_init(void)
{
    /*Initialize LCD*/
    lcd_conf_t lcd_pins = {
        .lcd_model    = LCD_MOD_AUTO_DET,
        .pin_num_miso = GPIO_NUM_25,
        .pin_num_mosi = GPIO_NUM_23,
        .pin_num_clk  = GPIO_NUM_19,
        .pin_num_cs   = GPIO_NUM_22,
        .pin_num_dc   = GPIO_NUM_21,
        .pin_num_rst  = GPIO_NUM_18,
        .pin_num_bckl = GPIO_NUM_5,
        .clk_freq     = 40 * 1000 * 1000,
        .rst_active_level = 0,
        .bckl_active_level = 0,
        .spi_host = HSPI_HOST,
        .init_spi_bus = true,
    };

    if (g_lcd_obj == NULL) {
        g_lcd_obj = new CEspLcd(&lcd_pins);
    }

    /*Welcome screen*/
    g_lcd_obj->setRotation(2);
    g_lcd_obj->setTextSize(1);

    g_lcd_obj->fillScreen(COLOR_BLACK);
    g_lcd_obj->drawRect(0, 0, 240, 320, COLOR_ESP_BKGD);
    g_lcd_obj->drawLine(0, 159, 239, 159, COLOR_ESP_BKGD);

    g_lcd_obj->setTextColor(COLOR_GREEN, COLOR_BLACK);
    g_lcd_obj->setFont(&FreeSansOblique9pt7b);
    g_lcd_obj->drawString("MDF INFO", 67, 20);
    g_lcd_obj->drawString("MDF ANALYSIS", 48, 180);

    /*!< initial espnow analysis display */
    uint8_t zero_num = 0;
    g_lcd_obj->setFont(&FreeMono9pt7b);
    espnow_lcd_ana_write(ESPNOW_NET_ANA_RUNTIME, &zero_num, 1);
    espnow_lcd_ana_write(ESPNOW_NET_ANA_DEV_NUM, &zero_num, 1);
    espnow_lcd_ana_write(ESPNOW_NET_ANA_DUMP_NUM, &zero_num, 1);
    espnow_lcd_ana_write(ESPNOW_NET_ANA_REBOOT_NUM, &zero_num, 1);
    espnow_lcd_ana_write(ESPNOW_NET_ANA_ERR, &zero_num, 1);
    espnow_lcd_ana_write(ESPNOW_NET_ANA_WARN, &zero_num, 1);

    MDF_LOGI("espnow_lcd_init success");
}
