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

#ifndef __MDF_HTTP_SERVER_H__
#define __MDF_HTTP_SERVER_H__

#include "openssl/ssl.h"

#include "mdf_common.h"
#include "mdf_socket.h"
#include "mdf_network_config.h"
#include "mdf_wifi_mesh.h"
#include "mdf_server.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

#ifndef CONFIG_MDF_HTTP_SERVER_PORT
#define CONFIG_MDF_HTTP_SERVER_PORT 80
#endif /**< CONFIG_MDF_HTTP_SERVER_PORT */
#define MDF_HTTP_SERVER_PORT CONFIG_MDF_HTTP_SERVER_PORT

#ifndef CONFIG_MDF_HTTPS_SERVER_PORT
#define CONFIG_MDF_HTTPS_SERVER_PORT 443
#endif /**< CONFIG_MDF_HTTPS_SERVER_PORT */
#define MDF_HTTPS_SERVER_PORT CONFIG_MDF_HTTPS_SERVER_PORT

#ifndef CONFIG_MDF_CONNECT_NUM
#define CONFIG_MDF_CONNECT_NUM 8
#endif /**< CONFIG_MDF_CONNECT_NUM */
#define MDF_CONNECT_NUM CONFIG_MDF_CONNECT_NUM

#ifndef CONFIG_MDF_CONNECT_BUF_SIZE
#define CONFIG_MDF_CONNECT_BUF_SIZE 3072
#endif /**< CONFIG_MDF_CONNECT_BUF_SIZE */
#define MDF_CONNECT_BUF_SIZE CONFIG_MDF_CONNECT_BUF_SIZE

/**
 * @brief http chunk data type
 */
enum server_http_chunk {
    MDF_HTTP_CHUNK_NONE   = 0,  /**< connection is not http chunk */
    MDF_HTTP_CHUNK        = 1,  /**< the connection is http chunk */
    MDF_HTTP_CHUNK_HEADER = 1,  /**< head of http chunk */
    MDF_HTTP_CHUNK_BODY   = 2,  /**< the data section of the http chunk */
};

/**
 * @brief http local connection data structure
 */
typedef struct server_http_connect {
    sockfd_t sockfd;                    /**< file descriptor */
    SSL *ssl;                           /**< ssl file descriptor */

    wifi_mesh_data_type_t type;         /**< the type of data */
    wifi_mesh_addr_t src_addr;          /**< the address contains remote IP address and port(IPv4:PORT) */
    uint8_t dest_addrs_num;             /**< the number of target devices */
    wifi_mesh_addr_t *dest_addrs;       /**< the number of target devices */

    uint8_t chunk;                      /**< whether the link is chunk's */
    uint8_t chunk_num;                  /**< the number of chunks */

    char *http_head;                    /**< pointer to the header of the http data */
    ssize_t http_size;                  /**< the size of http data */
    char *http_body;                    /**< pointer to the header of the http data */
    size_t http_body_size;              /**< the size of the body of http data */

    uint16_t ota_packet_size;           /**< the size of ota data */

    struct server_http_connect *prior;  /**< the forward pointer of the doubly linked list */
    struct server_http_connect *next;   /**< the backward pointer of the doubly linked list */
} server_http_connect_t;

/**
 * @brief  initialize the LAN http
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_http_server_init();

/**
 * @brief  de-initialize the LAN http
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_http_server_deinit();

/**
 * @brief  handle http requests
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_http_server_request();

/**
 * @brief  handle http response
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_http_server_response();

/**
 * @brief delete http connection
 *
 * @param conn pointer to the connection information
 */
void mdf_server_conn_delete(server_http_connect_t *conn);

/**
 * @brief  find ota connection
 *
 * @return
 *     - pointer to connection information
 *     - NULL no ota connection found
 */
server_http_connect_t *mdf_server_conn_find_ota();

/**
 * @brief  set the configuration of the root node ota
 *
 * @param  dest_addr the address contains remote IP address and port(IPv4:PORT)
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
server_http_connect_t *mdf_server_conn_find(const wifi_mesh_addr_t *sockaddr);

/**
 * @brief  Connected connect for data send
 *
 * @param  conn point to the connection information
 * @param  buf  buffer
 * @param  len  buffer length
 *
 * @return
 *     - actual copy of the number of bytes
 *     - ESP_FAIL
 */
esp_err_t mdf_server_conn_send(const server_http_connect_t *conn, const void *buf,
                               ssize_t len);

#ifdef __cplusplus
}
#endif /**< _cplusplus */

#endif /**< __MDF_HTTP_SERVER_H__ */
