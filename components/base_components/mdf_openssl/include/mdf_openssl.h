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

#ifndef __MDF_OPENSSL_H__
#define __MDF_OPENSSL_H__

#include "mdf_common.h"
#include "openssl/ssl.h"
#include "mdf_socket.h"

#ifdef  __cplusplus
extern "C" {
#endif

/**
 * @brief  create a ssl client
 * @param  sockfd          socket descriptor
 * @param  ctx             SSL context
 * @param  server_cert     server certification
 * @param  server_cert_len the length of the certificate
 * @return
 *     - ssl pointer
 *     - NULL
 */
SSL *mdf_ssl_client_create(sockfd_t sockfd, SSL_CTX *ctx,
                           const char *server_cert, ssize_t server_cert_len);

/**
 * @brief  create a ssl client
 * @param  sockfd          socket descriptor
 * @param  server_cert     server certification
 * @param  server_cert_len the length of the certificate
 * @param  prvtkey_pem     public key of service
 * @param  prvtkey_pem_len the length of the public key
 * @return
 *     - SSL context pointer
 *     - NULL
 */
SSL_CTX *mdf_ssl_server_create(sockfd_t sockfd, const uint8_t *server_cert, ssize_t server_cert_len,
                               const uint8_t *prvtkey_pem, ssize_t prvtkey_pem_len);

/**
 * @brief  receive ssl connection
 * @param  sockfd socket descriptor
 * @param  ctx    SSL context pointer
 * @return
 *     - ssl pointer
 *     - NULL
 */
SSL *mdf_ssl_accept(sockfd_t sockfd, SSL_CTX *ctx);

/**
 * @brief  connected socket for data recv
 *
 * @param  ssl  ssl pointer
 * @param  buf  buffer
 * @param  len  buffer length
 *
 * @return
 *     - actual copy of the number of bytes
 *     - ESP_FAIL
 */
ssize_t mdf_ssl_recv(SSL *ssl, char *buf, int len);

/**
 * @brief  Connected socket for data send
 *
 * @param  ssl  ssl pointer
 * @param  buf  buffer
 * @param  len  buffer length
 *
 * @return
 *     - actual copy of the number of bytes
 *     - ESP_FAIL
 */
ssize_t mdf_ssl_send(SSL *ssl, const char *buf, ssize_t len);

/**
 * @brief close the ssl
 *
 * @param ssl ssl pointer
 */
void mdf_ssl_close(SSL *ssl);

#ifdef __cplusplus
}
#endif

#endif /**< __MDF_OPENSSL_H__ */
