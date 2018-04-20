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

#include "mdf_common.h"
#include "mdf_openssl.h"

static const char *TAG = "mdf_openssl";

SSL *mdf_ssl_client_create(sockfd_t sockfd, SSL_CTX *ctx,
                           const char *server_cert, ssize_t server_cert_len)
{
    MDF_ASSERT(server_cert);
    MDF_ASSERT(server_cert_len > 0);

    esp_err_t ret = ESP_OK;
    SSL *ssl      = NULL;

    // ctx = SSL_CTX_new(TLSv1_1_client_method());
    // MDF_ERROR_GOTO(!ctx, ERR_EXIT, "SSL_CTX_new, ctx: %p", ctx);

    MDF_LOGD("set SSL new");
    ssl = SSL_new(ctx);
    MDF_ERROR_GOTO(!ssl, ERR_EXIT, "SSL_new, ssl: %p", ssl);

    MDF_LOGV("set SSL_set_fd");
    SSL_set_fd(ssl, sockfd);
    X509 *ca_cert = d2i_X509(NULL, (unsigned char *)server_cert, server_cert_len);
    MDF_ERROR_GOTO(!ca_cert, ERR_EXIT, "d2i_X509, ca_cert: %p", ca_cert);

    MDF_LOGV("set SSL_connect");
    ret = SSL_add_client_CA(ssl, ca_cert);
    MDF_ERROR_GOTO(ret == pdFALSE, ERR_EXIT, "SSL_add_client_CA, ret: %d", ret);

    ret = SSL_connect(ssl);
    MDF_ERROR_GOTO(ret == pdFALSE, ERR_EXIT, "SSL_connect, ret: %d", ret);

    MDF_LOGD("set SSL_connect is successed");

    return ssl;

ERR_EXIT:

    if (ssl) {
        mdf_ssl_close(ssl);
    }

    return NULL;
}

SSL_CTX *mdf_ssl_server_create(sockfd_t sockfd, const uint8_t *server_cert, ssize_t server_cert_len,
                               const uint8_t *prvtkey_pem, ssize_t prvtkey_pem_len)
{
    MDF_ASSERT(server_cert);
    MDF_ASSERT(server_cert_len > 0);
    MDF_ASSERT(prvtkey_pem);
    MDF_ASSERT(prvtkey_pem_len > 0);

    esp_err_t ret = ESP_OK;
    SSL_CTX *ctx  = NULL;

    MDF_LOGD("SSL server context create ......");
    ctx = SSL_CTX_new(TLS_server_method());
    MDF_ERROR_GOTO(!ctx, ERR_EXIT, "SSL_CTX_new, ctx: %p", ctx);

    MDF_LOGD("SSL server context set own certification ......");
    ret = SSL_CTX_use_certificate_ASN1(ctx, server_cert_len, server_cert);
    MDF_ERROR_GOTO(ret == pdFALSE, ERR_EXIT,
                   "SSL_CTX_use_certificate_ASN1, cert: %s, len: %d", server_cert, server_cert_len);

    MDF_LOGD("SSL server context set private key ......");
    ret = SSL_CTX_use_PrivateKey_ASN1(0, ctx, prvtkey_pem, prvtkey_pem_len);
    MDF_ERROR_GOTO(ret == pdFALSE, ERR_EXIT,
                   "SSL_CTX_use_certificate_ASN1, prvtkey: %s, len: %d", prvtkey_pem, prvtkey_pem_len);

    return ctx;

ERR_EXIT:

    if (ctx) {
        SSL_CTX_free(ctx);
        ctx = NULL;
    }

    return NULL;
}

SSL *mdf_ssl_accept(sockfd_t sockfd, SSL_CTX *ctx)
{
    MDF_ASSERT(ctx);

    esp_err_t ret = ESP_OK;
    SSL *ssl      = NULL;

    ssl = SSL_new(ctx);
    MDF_ERROR_GOTO(!ssl, ERR_EXIT, "SSL_new, ssl: %p", ssl);

    SSL_set_fd(ssl, sockfd);

    ret = SSL_accept(ssl);
    MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "SSL_accept, ret: %d", ret);

    return ssl;

ERR_EXIT:

    if (sockfd != MDF_SOCKET_INVALID_FD) {
        ret = close(sockfd);

        if (ret) {
            MDF_LOGW("close fail, ret: %d", ret);
        }
    }

    if (ssl) {
        SSL_free(ssl);
    }

    return NULL;
}

ssize_t mdf_ssl_recv(SSL *ssl, char *buf, ssize_t len)
{
    MDF_PARAM_CHECK(ssl);
    MDF_PARAM_CHECK(buf);
    MDF_PARAM_CHECK(len > 0);

    return SSL_read((SSL *)ssl, buf, len);
}

ssize_t mdf_ssl_send(SSL *ssl, const char *buf, ssize_t len)
{
    MDF_PARAM_CHECK(ssl);
    MDF_PARAM_CHECK(buf);
    MDF_PARAM_CHECK(len > 0);

    return SSL_write((SSL *)ssl, buf, len);
}

void mdf_ssl_close(SSL *ssl)
{
    if (!ssl) {
        return;
    }

    if (SSL_shutdown(ssl) == pdFALSE) {
        MDF_LOGW("SSL_shutdown, ssl: %p", ssl);
    }

    SSL_free(ssl);
}
