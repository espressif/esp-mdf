/*  Copyright (C) 2017  Florian Menne
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "string.h"
#include "stdio.h"

#include "ws2812b.h"

#ifndef CONFIG_WS2812B_INVERTED
#define CONFIG_WS2812B_INVERTED 1
#endif

#ifndef CONFIG_WS2812B_USE_PL9823
#define CONFIG_WS2812B_USE_PL9823 0
#endif

#if CONFIG_WS2812B_INVERTED == 0
#if CONFIG_WS2812B_USE_PL9823 == 0
static const rmt_item32_t wsLogicZero = {.level0 = 1, .duration0 = 32, .level1 = 0, .duration1 = 68};
static const rmt_item32_t wsLogicOne = {.level0 = 1, .duration0 = 64, .level1 = 0, .duration1 = 36};
#else
static const rmt_item32_t wsLogicZero = {.level0 = 1, .duration0 = 28, .level1 = 0, .duration1 = 109};
static const rmt_item32_t wsLogicOne = {.level0 = 1, .duration0 = 109, .level1 = 0, .duration1 = 28};
#endif

#else
#if CONFIG_WS2812B_USE_PL9823 == 0
static const rmt_item32_t wsLogicZero = {.level0 = 0, .duration0 = 32, .level1 = 1, .duration1 = 68};
static const rmt_item32_t wsLogicOne = {.level0 = 0, .duration0 = 64, .level1 = 1, .duration1 = 36};
#else
static const rmt_item32_t wsLogicZero = {.level0 = 0, .duration0 = 28, .level1 = 1, .duration1 = 109};
static const rmt_item32_t wsLogicOne = {.level0 = 0, .duration0 = 109, .level1 = 1, .duration1 = 28};
#endif

#endif

static rmt_channel_t channel;
static unsigned int size;
static rmt_item32_t *items;

void ws2812b_init(rmt_channel_t chan, gpio_num_t gpio, unsigned int psize)
{
    channel = chan;
    size = psize;
    items = NULL;

    if (!size) {
        printf("%s: %d Invalid size 0!\n", __FILE__, __LINE__);
        return;
    }

    if (NULL == (items = malloc(sizeof(rmt_item32_t) * size * 24))) {
        printf("%s: %d Unable to allocate space!\n", __FILE__, __LINE__);
        return;
    }

    rmt_config_t rmt_tx;
    memset(&rmt_tx, 0, sizeof(rmt_config_t));

    rmt_tx.channel = channel;
    rmt_tx.gpio_num = gpio;
    rmt_tx.mem_block_num = 1;
    rmt_tx.clk_div = 1;
    rmt_tx.tx_config.idle_output_en = 1;
#if CONFIG_WS2812B_INVERTED == 1
    rmt_tx.tx_config.idle_level = RMT_IDLE_LEVEL_HIGH;
#endif

    rmt_config(&rmt_tx);
    rmt_driver_install(rmt_tx.channel, 0, 0);
}

void ws2812b_set_leds(ws_rgb_t *data, unsigned int size)
{
    unsigned int itemCnt = 0;

    for (int i = 0; i < size; i++)
        for (int j = 0; j < 24; j++) {
            if (j < 8) {
#if CONFIG_WS2812B_USE_PL9823 == 0

                if (data[i].green & (1 << (7 - j))) {
                    items[itemCnt++] = wsLogicOne;
                }

#else

                if (data[i].red & (1 << (7 - j))) {
                    items[itemCnt++] = wsLogicOne;
                }

#endif
                else {
                    items[itemCnt++] = wsLogicZero;
                }
            }

            else if (j < 16) {
#if CONFIG_WS2812B_USE_PL9823 == 0

                if (data[i].red & (1 << (7 - (j % 8)))) {
                    items[itemCnt++] = wsLogicOne;
                }

#else

                if (data[i].green & (1 << (7 - (j % 8)))) {
                    items[itemCnt++] = wsLogicOne;
                }

#endif
                else {
                    items[itemCnt++] = wsLogicZero;
                }
            } else {
                if (data[i].blue & (1 << (7 - (j % 8)))) {
                    items[itemCnt++] = wsLogicOne;
                } else {
                    items[itemCnt++] = wsLogicZero;
                }
            }

        }

    rmt_write_items(channel, items, size * 24, false);
}

void ws2812b_deinit()
{
    rmt_driver_uninstall(channel);
    free(items);
}
