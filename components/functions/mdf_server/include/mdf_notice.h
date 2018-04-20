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

#ifndef __MDF_NOTICE_H__
#define __MDF_NOTICE_H__

#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

#ifndef CONFIG_MDF_UDP_BROADCAST_PORT
#define CONFIG_MDF_UDP_BROADCAST_PORT 3232
#endif /**< CONFIG_MDF_UDP_BROADCAST_PORT */
#define MDF_UDP_BROADCAST_PORT CONFIG_MDF_UDP_BROADCAST_PORT

#ifdef CONFIG_MDF_USE_UDP_SERVER
#define MDF_USE_UDP_SERVER
#endif /**< CONFIG_MDF_USE_UDP_SERVER */

#ifndef CONFIG_MDF_UDP_SERVER_PORT
#define CONFIG_MDF_UDP_SERVER_PORT    1025
#endif /**< CONFIG_MDF_UDP_SERVER_PORT */
#define MDF_UDP_SERVER_PORT CONFIG_MDF_UDP_SERVER_PORT

#define SERVER_NOTICE_QUEUE_NUM       30

/**
 * @brief Type of data notification within the LAN (local area network)
 */
typedef enum {
    NOTICE_CONNECTED = 1,           /**< new device added */
    NOTICE_DISCONNECTED,            /**< device remove */
    NOTICE_ROUTING_TABLE_CHANGE,    /**< routing table is changed */
    NOTICE_STATUS,                  /**< change of equipment status */
    NOTICE_SNIFFER,                 /**< sniffer notice */
    NOTICE_OTAFINISH,               /**< update completed */
} notice_type_t;

/**
 * @brief  send a notification event
 *
 * @param  notice type of notification
 * @param  addr   device address
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_notice_udp_send(notice_type_t notice, const uint8_t *addr);

/**
 * @brief notice initialization mdns
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_notice_mdns_init(void);

/**
 * @brief notice de-initialization mdns
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_notice_mdns_deinit(void);

/**
 * @brief notice initialization udp
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_notice_udp_init(void);

/**
 * @brief notice de-initialization udp
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_notice_udp_deinit(void);

/**
 * @brief udp client handle
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_notice_udp_client(void);

/**
 * @brief udp server handle
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_notice_udp_server(void);

#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif /**< __MDF_NOTICE_H__ */
