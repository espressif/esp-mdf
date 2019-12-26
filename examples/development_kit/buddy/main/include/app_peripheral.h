/* OLED screen Example

   For other examples please check:
   https://github.com/espressif/esp-iot-solution/tree/master/examples

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */
#ifndef _APP_PERIPHERAL_H_
#define _APP_PERIPHERAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/gpio.h"
#include "iot_ssd1306.h"
#include "ssd1306_fonts.h"
#include "attenuator.h"

#define MDF_EVENT_BUTTON_EVENT                  (MDF_EVENT_CUSTOM_BASE + 0x100)
#define MDF_EVENT_BUTTON_KEY_SHORT_RELEASE      (MDF_EVENT_BUTTON_EVENT + 0x01)
#define MDF_EVENT_BUTTON_KEY_MIDDLE_RELEASE     (MDF_EVENT_BUTTON_EVENT + 0x02)
#define MDF_EVENT_BUTTON_KEY_LONG_RELEASE       (MDF_EVENT_BUTTON_EVENT + 0x03)

#define SET_BUTTON_IO_NUM           32
#define PAGE_BUTTON_IO_NUM          0

enum {
    PAGE_STATUS = 0,
    PAGE_IPERF,
    PAGE_MAX,
};

enum {
    OLED_EVT_PAGE_NEXT,
    OLED_EVT_PAGE_PREV,
    OLED_EVT_SLEEP,
};

typedef struct {
    int type;
} oled_evt_t;

typedef struct oled_context {
    uint8_t current_page_num;
    TickType_t g_mesh_start_tick;
    TickType_t g_mesh_build_tick;
    bool iperf_running;
    float iperf_result;
} oled_context_t;

/**
 * @brief  SSD1306 show special page.
 */
void oled_show_page(int page, void *context);

/**
 * @brief  peripherals initilize.
 */
void peripherals_init();

/**
 * @brief  WS2812B show green color led.
 */
void ws2812b_green();

/**
 * @brief  WS2812B show blue color led.
 */
void ws2812b_blue();

/**
 * @brief  WS2812B show red color led.
 */
void ws2812b_red();

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

class COled: public CSsd1306
{
public:
    COled(CI2CBus *p_i2c_bus, uint8_t addr = SSD1306_I2C_ADDRESS) : CSsd1306(p_i2c_bus, addr) {};
    ~COled();
    esp_err_t show_status(float temprature, float humidity, void *context);
    esp_err_t show_iperf(void *context);
    void show_signs();
    void clean();
    void init();
};
#endif

#endif /* _APP_PERIPHERAL_H_ */
