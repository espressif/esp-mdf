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

#include "touchpad.h"
#include "iot_i2c_bus.h"
#include "iot_ch450.h"

static i2c_bus_handle_t i2c_bus = NULL;
static ch450_handle_t seg = NULL;

#define TOUCH_SLIDE_LED_NUM_0   3
#define TOUCH_SLIDE_LED_NUM_1   4
#define TOUCH_SLIDE_LED_NUM_2   5

#define I2C_MASTER_NUM     I2C_NUM_0
#define I2C_MASTER_SDA_IO  GPIO_NUM_22
#define I2C_MASTER_SCL_IO  GPIO_NUM_23
#define I2C_MASTER_FREQ_HZ 100000

/**< function declaration */
static void ch450_write_dig(int idx, int val);

void ch450_dev_init()
{
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_bus = iot_i2c_bus_create(I2C_MASTER_NUM, &conf);

    if (seg == NULL) {
        seg = iot_ch450_create(i2c_bus);
    }

    for (int i = 0; i < 6; i++) {
        ch450_write_dig(i, -1);
    }

    ch450_write_dig(TOUCH_SLIDE_LED_NUM_0, 0);
    ch450_write_dig(TOUCH_SLIDE_LED_NUM_1, 0);
    ch450_write_dig(TOUCH_SLIDE_LED_NUM_2, 0);
}

void ch450_clear()
{
    for (int i = 0; i < 6; i++) {
        ch450_write_dig(i, -1);
    }
}

void ch450_write_pos(uint8_t pos)
{
    ch450_write_dig(0, -1);
    ch450_write_dig(1, -1);
    ch450_write_dig(2, -1);
    ch450_write_dig(3, -1);
    ch450_write_dig(TOUCH_SLIDE_LED_NUM_0, pos % 10);
    ch450_write_dig(TOUCH_SLIDE_LED_NUM_1, pos % 100 / 10);
    ch450_write_dig(TOUCH_SLIDE_LED_NUM_2, pos / 100);
}

void ch450_write_button(uint8_t button_index)
{
    ch450_write_dig(0, -1);
    ch450_write_dig(1, -1);
    ch450_write_dig(2, -1);
    ch450_write_dig(3, -1);

    switch (button_index) {
        case 0:
            ch450_write_dig(TOUCH_SLIDE_LED_NUM_0, 0);
            ch450_write_dig(TOUCH_SLIDE_LED_NUM_1, 0);
            ch450_write_dig(TOUCH_SLIDE_LED_NUM_2, 1);
            break;

        case 1:
            ch450_write_dig(TOUCH_SLIDE_LED_NUM_0, 0);
            ch450_write_dig(TOUCH_SLIDE_LED_NUM_1, 1);
            ch450_write_dig(TOUCH_SLIDE_LED_NUM_2, 0);
            break;

        case 2:
            ch450_write_dig(TOUCH_SLIDE_LED_NUM_0, 1);
            ch450_write_dig(TOUCH_SLIDE_LED_NUM_1, 0);
            ch450_write_dig(TOUCH_SLIDE_LED_NUM_2, 0);
            break;
    }
}

static void ch450_write_dig_reverse(int idx, int num)
{
    uint8_t val = 0;

    switch (num) {
        case 1:
            val = 0x82;
            break;

        case 2:
            val = 0xcd;
            break;

        case 3:
            val = 0xc7;
            break;

        case 4:
            val = 0xa3;
            break;

        case 5:
            val = 0x67;
            break;

        case 6:
            val = 0x6f;
            break;

        case 7:
            val = 0xc2;
            break;

        case 8:
            val = 0xef;
            break;

        case 9:
            val = 0xe7;
            break;

        case 0:
            val = 0xee;
            break;

        default:
            val = 0;
            break;
    }

    ch450_cmd_t seg_cmd = CH450_SEG_2 + idx * 2;
    iot_ch450_write(seg, seg_cmd, val);
}

static void ch450_write_dig(int idx, int val)
{
    ch450_write_dig_reverse(idx, val);
}
