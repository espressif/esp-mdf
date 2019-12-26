/* OLED screen Example

   For other examples please check:
   https://github.com/espressif/esp-iot-solution/tree/master/examples

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "driver/rmt.h"

#include "mdf_common.h"
#include "mwifi.h"

#include "iot_ssd1306.h"
#include "iot_hts221.h"
#include "iot_button.h"
#include "esp_log.h"
#include "ws2812b.h"
#include "app_peripheral.h"

// Sensor
#define SENSOR_I2C_PORT          ((i2c_port_t) 1)
#define SENSOR_SCL_IO            23
#define SENSOR_SDA_IO            18

#define OLED_SHOW_LEFT_TOUCH        9
#define OLED_SHOW_RIGHT_TOUCH       8
#define TOUCHPAD_THRES_PERCENT      950
#define TOUCHPAD_FILTER_INTERVAL    10

#define BUTTON_ACTIVE_LEVEL         0

static const char* TAG = "Peripheral";

static CI2CBus *i2c_bus_sens   = NULL;
static CHts221 *hts221         = NULL;
static COled *oled             = NULL;

void COled::init()
{

}

void COled::clean()
{
    clear_screen(0);
}

esp_err_t COled::show_iperf(void * context)
{
    clean();
    
    draw_string(20, 0, (const uint8_t *) "iPerf Test", 16, 1);

    // if (((oled_context_t *)context)->iperf_server) {
    if (esp_mesh_is_root()) {
        draw_string(40, 16, (const uint8_t *) "Server", 16, 1);
    } else {
        draw_string(40, 16, (const uint8_t *) "Client", 16, 1);
    }

    if (((oled_context_t *)context)->iperf_running) {
        draw_string(110, 0, (const uint8_t *) "R", 16, 1);
    } else {
        draw_string(110, 0, (const uint8_t *) "S", 16, 1);
    }

    /*< iperf_ >*/
    char iperf_str[6];
    if (((oled_context_t *)context)->iperf_result / 10 > 1) {
        sprintf(iperf_str, "%04.1f", ((oled_context_t *)context)->iperf_result);
        iperf_str[4] = '\0';

        draw_3216char(10, 32, iperf_str[0]);
        draw_3216char(26, 32, iperf_str[1]);
        draw_char(42, 48, iperf_str[2], 16, 1);
        draw_3216char(58, 32, iperf_str[3]);
    } else {
        sprintf(iperf_str, "%04.2f", ((oled_context_t *)context)->iperf_result);
        iperf_str[4] = '\0';

        draw_3216char(10, 32, iperf_str[0]);
        draw_char(26, 48, iperf_str[1], 16, 1);
        draw_3216char(42, 32, iperf_str[2]);
        draw_3216char(58, 32, iperf_str[3]);
    }

    draw_string(78, 48, (const uint8_t *) "Mbit/s", 16, 1);

    return refresh_gram();
}

esp_err_t COled::show_status(float temprature, float humidity, void * context)
{
    clean();
    /*< temprature >*/
    char tempraturestr[6];
    sprintf(tempraturestr, "%4.1f", temprature);
    tempraturestr[4] = '\0';

    draw_string(0, 0, (const uint8_t *) "TEM", 16, 1);
    draw_1616char(25, 0, tempraturestr[0]);
    draw_1616char(41, 0, tempraturestr[1]);

    /*< humidity >*/
    char humiditystr[6];
    sprintf(humiditystr, "%4.1f", humidity);
    humiditystr[4] = '\0';

    draw_string(64, 0, (const uint8_t *) "HUM", 16, 1);
    draw_1616char(89, 0, humiditystr[0]);
    draw_1616char(105, 0, humiditystr[1]);

    if (esp_mesh_is_root()) {
        draw_string(0, 16, (const uint8_t *)"ROOT", 16, 1);
        sprintf(humiditystr, "%02d", esp_mesh_get_total_node_num());
        draw_3216char(0, 32, humiditystr[0]);
        draw_3216char(16, 32, humiditystr[1]);
    } else {
        draw_string(0, 16, (const uint8_t *) "NODE", 16, 1);
        sprintf(humiditystr, "%02d", esp_mesh_get_layer());
        if (mwifi_is_connected()) {
            draw_3216char(0, 32, humiditystr[0]);
            draw_3216char(16, 32, humiditystr[1]);
        } else {
            draw_string(0, 40, (const uint8_t *) "dis", 16, 1);
        }
    }
    draw_string(90, 24, (const uint8_t *)"RSSI", 16, 1);
    sprintf(humiditystr, "%-4d", mwifi_get_parent_rssi());
    draw_string(90, 40, (const uint8_t *)humiditystr, 16, 1);

    draw_string(40, 16, (const uint8_t *) "Time", 16, 1);
    sprintf(humiditystr, "%03d", ((oled_context_t *)context)->g_mesh_build_tick * portTICK_PERIOD_MS / 1000);
    draw_3216char(40, 32, humiditystr[0]);
    draw_3216char(56, 32, humiditystr[1]);
    draw_3216char(71, 32, humiditystr[2]);

    return refresh_gram();
}

void oled_show_page(int page, void * context)
{
    static int page_prev = PAGE_MAX;
    if (page_prev == page) {

    } else {
        page_prev = page;
        oled->clean();
    }
    switch (page % PAGE_MAX) {
        case PAGE_IPERF:
            oled->show_iperf(context);
            break;
        case PAGE_STATUS:
            oled->show_status(hts221->read_temperature(), hts221->read_humidity(), context);
            break;
        default:
            oled->show_status(hts221->read_temperature(), hts221->read_humidity(), context);
            break;
    }
}

void button_tap_cb(void* arg)
{
    ESP_LOGI(TAG, "tap cb(%d), heap: %d\n", *(uint32_t *)arg, esp_get_free_heap_size());
    mdf_event_loop_send(MDF_EVENT_BUTTON_KEY_SHORT_RELEASE, (void *)arg);
}

void button_press_2s_cb(void* arg)
{
    ESP_LOGI(TAG, "press 2s(%d), heap: %d\n", *(uint32_t *)arg, esp_get_free_heap_size());
    mdf_event_loop_send(MDF_EVENT_BUTTON_KEY_MIDDLE_RELEASE, (void *)arg);
}

void button_press_5s_cb(void* arg)
{
    ESP_LOGI(TAG, "press 5s(%d), heap: %d\n", *(uint32_t *)arg, esp_get_free_heap_size());
    mdf_event_loop_send(MDF_EVENT_BUTTON_KEY_LONG_RELEASE, (void *)arg);
}

void button_init()
{
    /* set button */
    static uint32_t gpio_num_32 = SET_BUTTON_IO_NUM;
    button_handle_t btn_handle_32 = iot_button_create((gpio_num_t)SET_BUTTON_IO_NUM, (button_active_t)BUTTON_ACTIVE_LEVEL);
    iot_button_set_evt_cb(btn_handle_32, BUTTON_CB_TAP, button_tap_cb, &gpio_num_32);

    iot_button_add_on_release_cb(btn_handle_32, 2, button_press_2s_cb, &gpio_num_32);
    iot_button_add_on_release_cb(btn_handle_32, 5, button_press_5s_cb, &gpio_num_32);

    /* page button */
    static uint32_t gpio_num_0 = PAGE_BUTTON_IO_NUM;
    button_handle_t btn_handle_0 = iot_button_create((gpio_num_t)PAGE_BUTTON_IO_NUM, (button_active_t)BUTTON_ACTIVE_LEVEL);
    iot_button_set_evt_cb(btn_handle_0, BUTTON_CB_TAP, button_tap_cb, &gpio_num_0);

    iot_button_add_on_release_cb(btn_handle_0, 2, button_press_2s_cb, &gpio_num_0);
    iot_button_add_on_release_cb(btn_handle_0, 5, button_press_5s_cb, &gpio_num_0);
}

void peripherals_init()
{
    i2c_bus_sens = new CI2CBus((i2c_port_t) SENSOR_I2C_PORT, (gpio_num_t) SENSOR_SCL_IO, (gpio_num_t) SENSOR_SDA_IO);
    
    hts221 = new CHts221(i2c_bus_sens);
    oled = new COled(i2c_bus_sens);

    oled->init();

    ws2812b_init(RMT_CHANNEL_0, GPIO_NUM_25, 1);
    ws_rgb_t rgb[1] = {0x00, 0x00, 0xff};
    ws2812b_set_leds(rgb, 1);

    attenuator_init();

    button_init();
}

void ws2812b_green()
{
    ws_rgb_t rgb[1] = {0x0f, 0x00, 0x00};
    ws2812b_set_leds(rgb, 1);
}

void ws2812b_blue()
{
    ws_rgb_t rgb[1] = {0x00, 0x0f, 0x00};
    ws2812b_set_leds(rgb, 1);
}

void ws2812b_red()
{
    ws_rgb_t rgb[1] = {0x00, 0x00, 0x0f};
    ws2812b_set_leds(rgb, 1);
}
