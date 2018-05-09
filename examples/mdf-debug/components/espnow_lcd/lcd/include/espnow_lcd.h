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

#ifndef __ESPNOW_LCD_H__
#define __ESPNOW_LCD_H__

#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

typedef enum espnow_debug_data_type {
    ESPNOW_NET_INFO = 0, /**< network info, request from mdns service */
    ESPNOW_NET_ANA,      /**< device info, analysis from espnow log */

    ESPNOW_MDF_MAX,
} espnow_data_type_t;

/**< query from mDNS service */
typedef enum espnow_net_info {
    ESPNOW_NET_INFO_ID = 0,
    ESPNOW_NET_INFO_MAX_LAYER,
    ESPNOW_NET_INFO_MAX_CONN,
    ESPNOW_NET_INFO_DEV_NUM,
    ESPNOW_NET_INFO_ROOT_MAC,
    ESPNOW_NET_INFO_ROOT_IP,

    ESPNOW_NET_INFO_MAX,
} espnow_net_info_t;

/**< analysis type enumerration */
typedef enum espnow_net_analysis {
    ESPNOW_NET_ANA_RUNTIME = 0,
    ESPNOW_NET_ANA_DEV_NUM,
    ESPNOW_NET_ANA_DUMP_NUM,
    ESPNOW_NET_ANA_REBOOT_NUM,
    ESPNOW_NET_ANA_ERR,
    ESPNOW_NET_ANA_WARN,

    ESPNOW_NET_ANA_MAX,
} espnow_net_analysis_t;

/**
 * @brief  update devices' running status
 *
 * @param  type analysis type
 * @param  data content after log analyzing
 * @param  len  content len
 *
 * @return
 *     - ESP_OK
 *     - others : fail
 */
esp_err_t espnow_lcd_ana_write(uint8_t type, const uint8_t *data, uint8_t len);

/**
 * @brief  update network information
 *
 * @param  addr IP address of root
 * @param  text txt field query from mDNS service
 *
 * @return
 *     - ESP_OK
 *     - others : fail
 */
esp_err_t espnow_lcd_info_update(const uint8_t *addr, const char *text);

/**
 * @brief init lcd of espnow terminal board
 */
void espnow_lcd_init(void);


#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif /**< __ESPNOW_LCD_H__ */
