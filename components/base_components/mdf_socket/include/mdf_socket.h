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

#ifndef __MDF_SOCKET_H__
#define __MDF_SOCKET_H__

#include "mdf_common.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

#define MDF_SOCKET_INVALID_FD -1

typedef int sockfd_t;
typedef union {
    struct {
        in_port_t port; /**< port */
        in_addr_t ip;   /**< IP address */
    } __attribute__((packed));
    uint8_t mac[6];
} sockaddr_t;

/**
 * @brief  create a tcp server
 *
 * @param  port    the port number of the service
 * @param  backlog backup connection
 *
 * @return
 *     - the descriptor that references the new socket
 *     - MDF_SOCKET_INVALID_FD
 */
sockfd_t mdf_socket_tcp_server_create(uint16_t port, uint16_t backlog);

/**
 * @brief  create a tcp client
 *
 * @param  ip   dotted decimal
 * @param  port the port number of the service
 *
 * @return
 *     - the descriptor that references the new socket
 *     - MDF_SOCKET_INVALID_FD
 */
sockfd_t mdf_socket_tcp_client_create(const char *ip, uint16_t port);

/**
 * @brief  create a udp server
 *
 * @param  port the port number of the service
 *
 * @return
 *     - the descriptor that references the new socket
 *     - MDF_SOCKET_INVALID_FD
 */
sockfd_t mdf_socket_udp_server_create(uint16_t port);

/**
 * @brief  create a udp broadcast
 *
 * @param  port the port number of the service
 *
 * @return
 *     - the descriptor that references the new socket
 *     - MDF_SOCKET_INVALID_FD
 */
sockfd_t mdf_socket_udp_broadcast_create(void);

/**
 * @brief  Connected socket for data send
 *
 * @param  sockfd socket descriptor
 * @param  buf    buffer
 * @param  len    buffer length
 *
 * @return
 *     - actual copy of the number of bytes
 *     - ESP_FAIL
 */
ssize_t mdf_socket_send(sockfd_t sockfd, void *buf, size_t len);

/**
 * @brief  connected socket for data recv
 *
 * @param  sockfd socket descriptor
 * @param  buf    buffer
 * @param  len    buffer length
 *
 * @return
 *     - actual copy of the number of bytes
 *     - ESP_FAIL
 */
ssize_t mdf_socket_recv(sockfd_t sockfd, void *buf, size_t len);

/**
 * @brief close the socket
 *
 * @param sockfd socket descriptor
 */
void mdf_socket_close(sockfd_t sockfd);

/**
 * @brief  set the socket timeout
 *
 * @param  sockfd          the descriptor that references the new socket
 * @param  send_timeout_ms the transmit data timeout is in milliseconds
 * @param  recv_timeout_ms the receive data timeout is in milliseconds
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_socket_timeout(sockfd_t sockfd, uint32_t send_timeout_ms, uint32_t recv_timeout_ms);

/**
 * @brief  set the socket keepalive
 *
 * @param  sockfd        the descriptor that references the new socket
 * @param  keep_idle     keep idle
 * @param  keep_interval keep interval
 * @param  keep_count    keep count
 *
 * @return
 *     - ESP_OK
 *     - ESP_FAIL
 */
esp_err_t mdf_socket_keepalive(sockfd_t sockfd, int keep_idle, int keep_interval, int keep_count);

/**
 * @brief  determine whether the socket is connected
 *
 * @param  sockfd the descriptor that references the new socket
 *
 * @return
 *     - ture: connected
 *     - flase: disconnected
 */
bool mdf_socket_isconnect(sockfd_t sockfd);

/**
 * @brief  whether this connection is readable
 *
 * @param  sockfd the descriptor that references the new socket
 *
 * @return
 *     - ture: readable
 *     - flase: unreadable
 */
bool mdf_socket_isreadable(sockfd_t sockfd);

/**
 * @brief clean up the data in the socket connection
 *
 * @param  sockfd the descriptor that references the new socket
 */
void mdf_socket_fflush(sockfd_t sockfd);

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __MDF_SOCKET_H__ */
