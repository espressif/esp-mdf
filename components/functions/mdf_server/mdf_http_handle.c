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
#include "mdf_http_handle.h"
#include "mdf_json.h"
#include "mdf_notice.h"
#include "mdf_openssl.h"
#include "mdf_socket.h"
#include "mdf_http.h"
#include "mdf_notice.h"
#include "mdf_http_parser.h"

static const char *TAG                    = "mdf_http_handle";
static server_http_connect_t *g_conn_list = NULL;
static void *g_conn_list_lock             = NULL;
static sockfd_t g_http_server_sockfd      = MDF_SOCKET_INVALID_FD;
static SSL_CTX *g_https_server_ctx        = NULL;

static bool mdf_server_conn_list_lock()
{
    if (!g_conn_list_lock) {
        g_conn_list_lock = xSemaphoreCreateMutex();
    }

    return xSemaphoreTake(g_conn_list_lock, portMAX_DELAY);
}

static bool mdf_server_conn_list_unlock()
{
    if (!g_conn_list_lock) {
        g_conn_list_lock = xSemaphoreCreateMutex();
    }

    return xSemaphoreGive(g_conn_list_lock);
}

void mdf_server_conn_delete(server_http_connect_t *conn)
{
    MDF_ASSERT(conn);

    mdf_server_conn_list_lock();

    mdf_socket_close(conn->sockfd);
    mdf_ssl_close(conn->ssl);

    if (conn->next) {
        conn->next->prior = conn->prior;
    }

    conn->prior->next = conn->next;

    mdf_free(conn->http_head);
    mdf_free(conn->dest_addrs);
    mdf_free(conn);

    mdf_server_conn_list_unlock();
}

server_http_connect_t *mdf_server_conn_find_ota()
{
    MDF_ASSERT(g_conn_list);

    for (server_http_connect_t *conn = g_conn_list->next; conn; conn = conn->next) {
        if (conn->type.ota) {
            return conn;
        }
    }

    MDF_LOGD("no find ota_conn");
    return NULL;
}

server_http_connect_t *mdf_server_conn_find(const wifi_mesh_addr_t *sockaddr)
{
    MDF_ASSERT(sockaddr);
    MDF_ASSERT(g_conn_list);

    for (server_http_connect_t *conn = g_conn_list->next; conn; conn = conn->next) {
        if (!memcmp(&conn->src_addr, sockaddr, sizeof(wifi_mesh_addr_t))) {
            return conn;
        }
    }

    MDF_LOGW("conn: %p, no find, ip: %s, port: %d", g_conn_list->next, inet_ntoa(sockaddr->ip), ntohs(sockaddr->port));
    return NULL;
}

static esp_err_t mdf_server_conn_insert(int sockfd, const wifi_mesh_addr_t *sockaddr)
{
    MDF_ASSERT(g_conn_list);
    MDF_PARAM_CHECK(sockfd != MDF_SOCKET_INVALID_FD);
    MDF_PARAM_CHECK(sockaddr);

    int conn_num             = 0;
    server_http_connect_t *new_conn = NULL;

    for (server_http_connect_t *conn = g_conn_list->next; conn; conn = conn->next) {
        MDF_LOGD("conn_num: %d, sockfd: %d, ip: %s, port: %d",
                 conn_num, conn->sockfd, inet_ntoa(conn->src_addr.ip), ntohs(conn->src_addr.port));
        conn_num++;
    }

    MDF_LOGD("insert, conn_num: %d, sockfd: %d, ip: %s, port: %d",
             conn_num, sockfd, inet_ntoa(sockaddr->ip), ntohs(sockaddr->port));

    if (conn_num >= MDF_CONNECT_NUM) {
        MDF_LOGW("the number of connions has reached the limit: %d", conn_num);
        MDF_LOGW("ip: %s, port: %d", inet_ntoa(sockaddr->ip), ntohs(sockaddr->port));

        while (g_conn_list->next) {
            mdf_server_conn_delete(g_conn_list->next);
        }
    }

    new_conn                = mdf_calloc(1, sizeof(server_http_connect_t));
    new_conn->sockfd        = sockfd;
    memcpy(&new_conn->src_addr, sockaddr, sizeof(wifi_mesh_addr_t));

    if (g_conn_list->next) {
        g_conn_list->next->prior = new_conn;
    }

    new_conn->next  = g_conn_list->next;
    new_conn->prior = g_conn_list;
    g_conn_list->next = new_conn;

    return ESP_OK;
}

esp_err_t mdf_server_conn_send(const server_http_connect_t *conn, const void *buf,
                               ssize_t len)
{
    esp_err_t ret = ESP_OK;

#ifdef CONFIG_MDF_USE_HTTPS
    ret = mdf_ssl_send(conn->ssl, (void *)buf, len);
#else
    ret = mdf_socket_send(conn->sockfd, (void *)buf, len);
#endif

    return ret;
}

static bool mdf_http_server_pkt_is_intact(void *buf, ssize_t size, size_t *pkt_size)
{
    MDF_PARAM_CHECK(buf);
    MDF_PARAM_CHECK(size > 0);

    esp_err_t ret       = ESP_OK;
    char *header_tail   = NULL;
    int content_len     = -1;
    uint16_t header_len = 0;

    header_tail = strstr(buf, "\r\n\r\n");

    if (!header_tail) {
        MDF_LOGD("mdf_http_server_pkt header is not intact");
        return false;
    }

    header_len = (void *)header_tail + strlen("\r\n\r\n") - buf;

    ret = mdf_http_get_header(buf, "Content-Length", &content_len);

    /**< http pkt has no Content-Length field or Content-Length is '0' */
    if ((ret == 0 && content_len == -1) || (ret == 1 && content_len == 0)) {
        *pkt_size = header_len;
        return true;
    }

    /**< if content_len < 10, means Content-Length field not intact */
    if (content_len < 10 || header_len + content_len > size) {
        MDF_LOGD("mdf_http_server_pkt not intact, header_len: %d, content_len: %d",
                 header_len, content_len);
        return false;
    }

    *pkt_size = header_len + content_len;
    return true;
}

esp_err_t mdf_http_server_init()
{
    if (!g_conn_list) {
        g_conn_list = mdf_calloc(1, sizeof(server_http_connect_t));
    }

#ifdef CONFIG_MDF_USE_HTTPS
    g_http_server_sockfd = mdf_socket_tcp_server_create(MDF_HTTPS_SERVER_PORT, MDF_CONNECT_NUM);
    MDF_ERROR_GOTO(g_http_server_sockfd == MDF_SOCKET_INVALID_FD, ERR_EXIT,
                   "mdf_socket_tcp_server_create");

    extern const uint8_t cacert_pem_start[] asm("_binary_cacert_pem_start");
    extern const uint8_t cacert_pem_end[]   asm("_binary_cacert_pem_end");
    const size_t cacert_pem_bytes = cacert_pem_end - cacert_pem_start;

    extern const uint8_t prvtkey_pem_start[] asm("_binary_prvtkey_pem_start");
    extern const uint8_t prvtkey_pem_end[]   asm("_binary_prvtkey_pem_end");
    const size_t prvtkey_pem_bytes = prvtkey_pem_end - prvtkey_pem_start;

    g_https_server_ctx = mdf_ssl_server_create(g_http_server_sockfd, cacert_pem_start, cacert_pem_bytes,
                         prvtkey_pem_start, prvtkey_pem_bytes);
    MDF_ERROR_GOTO(!g_https_server_ctx, ERR_EXIT, "mdf_ssl_server_create");
#else
    g_http_server_sockfd = mdf_socket_tcp_server_create(MDF_HTTP_SERVER_PORT, MDF_CONNECT_NUM);
    MDF_ERROR_GOTO(g_http_server_sockfd == MDF_SOCKET_INVALID_FD, ERR_EXIT,
                   "mdf_socket_tcp_server_create");
#endif

    return ESP_OK;

ERR_EXIT:

    if (g_https_server_ctx) {
        SSL_CTX_free(g_https_server_ctx);
        g_https_server_ctx = NULL;
    }

    mdf_socket_close(g_http_server_sockfd);
    g_http_server_sockfd = MDF_SOCKET_INVALID_FD;

    return ESP_FAIL;
}

esp_err_t mdf_http_server_deinit()
{
    if (g_https_server_ctx) {
        SSL_CTX_free(g_https_server_ctx);
        g_https_server_ctx = NULL;
    }

    mdf_socket_close(g_http_server_sockfd);
    g_http_server_sockfd = MDF_SOCKET_INVALID_FD;

    while (g_conn_list->next) {
        mdf_server_conn_delete(g_conn_list->next);
    }

    mdf_free(g_conn_list);
    g_conn_list = NULL;

    return ESP_OK;
}

esp_err_t mdf_http_server_request()
{
    esp_err_t ret                 = ESP_OK;
    int max_fd                    = g_http_server_sockfd;
    fd_set readset                = {0};
    struct timeval select_timeout = {0, 500 * 1000}; /**< 500ms */

    FD_ZERO(&readset);
    FD_SET(g_http_server_sockfd, &readset);

    for (server_http_connect_t *conn = g_conn_list->next; conn; conn = conn->next) {
        FD_SET(conn->sockfd, &readset);
        max_fd = (conn->sockfd > max_fd) ? conn->sockfd : max_fd;
    }

    if (select(max_fd + 1, &readset, NULL, NULL, &select_timeout) <= 0) {
        return ESP_OK;
    }

    if (FD_ISSET(g_http_server_sockfd, &readset)) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(struct sockaddr_in);
        int client_fd = accept(g_http_server_sockfd, (struct sockaddr *)&client_addr, &client_len);
        MDF_ERROR_CHECK(client_fd < 0, ESP_FAIL, "accept, sockfd: %d", client_fd);

        ret = mdf_server_conn_insert(client_fd, (wifi_mesh_addr_t *)&client_addr.sin_port);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_server_conn_insert, client_fd: %d, ip: %s, port: %d, ret: %d",
                        client_fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), ret);

        server_http_connect_t *client_conn = g_conn_list->next;

        if (mdf_socket_keepalive(client_fd, 10, 3, 3) < 0) {
            MDF_LOGW("mdf_socket_keepalive, ret: %d", ret);
            mdf_server_conn_delete(client_conn);
            return ESP_OK;
        }

#ifdef CONFIG_MDF_USE_HTTPS
        client_conn->ssl = mdf_ssl_accept(client_fd, g_https_server_ctx);

        if (!client_conn->ssl) {
            mdf_server_conn_delete(client_conn);
        }

#endif
    }

    for (server_http_connect_t *conn = g_conn_list->next; conn; conn = conn->next) {
        if (FD_ISSET(conn->sockfd, &readset)) {
            if (!conn->http_head) {
                conn->http_size = 0;
                conn->http_head = mdf_calloc(1, MDF_CONNECT_BUF_SIZE);
            }

            MDF_LOGD("conn->sockfd: %d, conn->http_size: %d", conn->sockfd, conn->http_size);

#ifdef CONFIG_MDF_USE_HTTPS
            ret = mdf_ssl_recv(conn->ssl, conn->http_head + conn->http_size, MDF_CONNECT_BUF_SIZE - conn->http_size);
#else
            ret = mdf_socket_recv(conn->sockfd, conn->http_head + conn->http_size, MDF_CONNECT_BUF_SIZE - conn->http_size);
#endif

            if (ret <= 0 || (!conn->type.ota && conn->http_size > MDF_CONNECT_BUF_SIZE)) {
                MDF_LOGD("socket close, sockfd: %d, conn->http_size: %d, errno_str: %s, ret: %d",
                         conn->sockfd, conn->http_size, strerror(errno), ret);

                server_http_connect_t *conn_prior = conn->prior;
                mdf_server_conn_delete(conn);
                conn = conn_prior;
                return ESP_OK;
            }

            /**< update recv_size */
            conn->http_size += ret;

            if (conn->type.ota) {
                MDF_LOGV("ota data, sockfd: %d, conn->http_size: %d, conn->data: %02x %02x %02x %02x",
                         conn->sockfd, conn->http_size, conn->http_head[0], conn->http_head[1],
                         conn->http_head[2], conn->http_head[3]);
                ret = mdf_http_upgrade_parser(conn);
            } else {
                MDF_LOGD("http request, sockfd: %d, conn->http_size: %d, data: \n%s",
                         conn->sockfd, conn->http_size, conn->http_head);

                size_t pkt_size = 0;

                if (!mdf_http_server_pkt_is_intact(conn->http_head, conn->http_size, &pkt_size)) {
                    return ESP_OK;
                }

                ret = mdf_server_request_parser(conn);

                conn->http_size -= pkt_size;

                if (!conn->http_size) {
                    mdf_free(conn->http_head);
                } else {
                    memcpy(conn->http_head, conn->http_head + pkt_size, conn->http_size);
                }
            }


            if (ret < 0) {
                MDF_LOGD("mdf_server_request_parser, ret: %d sockfd: %d, conn->size: %d, errno_str: %s",
                         ret, conn->sockfd, conn->http_size, strerror(errno));
                server_http_connect_t *conn_prior = conn->prior;
                mdf_server_conn_delete(conn);
                conn = conn_prior;
                return ESP_OK;
            }
        }
    }

    return ESP_OK;
}

esp_err_t mdf_http_server_response()
{
    esp_err_t ret                   = ESP_OK;
    char *http_response_data        = NULL;
    ssize_t http_response_size      = 0;
    wifi_mesh_addr_t dest_addr      = {0};
    wifi_mesh_addr_t src_addr       = {0};
    wifi_mesh_data_type_t data_type = {0};
    ssize_t recv_size               = 0;
    uint8_t *recv_data              = mdf_malloc(WIFI_MESH_PACKET_MAX_SIZE * 2);

    ret = mdf_wifi_mesh_root_recv(&src_addr, &dest_addr, &data_type,
                                  recv_data, WIFI_MESH_PACKET_MAX_SIZE * 2, 1000);

    if (ret == -ESP_ERR_MESH_TIMEOUT) {
        goto EXIT;
    }

    MDF_ERROR_GOTO(ret <= 0, EXIT, "mdf_wifi_mesh_root_recv ret: %x", ret);

    recv_size = ret;

    if (data_type.notice) {
        MDF_LOGD("mdf_wifi_mesh_root_recv, recv_size: %d, src_addr: "MACSTR", data_type: 0x%x",
                 recv_size, MAC2STR(src_addr.mac), data_type.val);
        notice_type_t notice_type = recv_data[0];

        /**< dealing with multiple devices while upgrading duplicate packages */
        if (notice_type == NOTICE_OTAFINISH) {
            mdf_server_conn_list_lock();

            server_http_connect_t *ota_conn = mdf_server_conn_find_ota();

            if (!ota_conn) {
                mdf_server_conn_list_unlock();
                MDF_LOGW("mdf_server_conn_find, ota_conn: %p", ota_conn);
                goto EXIT;
            }

            /**< the device address that will complete the upgrade data is deleted */
            for (int i = 0; i < ota_conn->dest_addrs_num; ++i) {
                if (!memcmp(ota_conn->dest_addrs + i, &src_addr, 6)) {
                    MDF_LOGD("delete mdf_addrs ota data: "MACSTR, MAC2STR(src_addr.mac));
                    memcpy(ota_conn->dest_addrs + i, ota_conn->dest_addrs + ota_conn->dest_addrs_num - 1, 6);
                    ota_conn->dest_addrs_num--;
                    break;
                }
            }

            mdf_server_conn_list_unlock();
        } else {
            ret = mdf_notice_udp_send(notice_type, src_addr.mac);
            MDF_ERROR_GOTO(ret < 0, EXIT, "mdf_notice_udp_send, notice type: %d, ret: %d",
                           notice_type, ret);
        }

        goto EXIT;
    }

    http_response_data = mdf_calloc(1, MDF_CONNECT_BUF_SIZE);
    http_response_size = mdf_http_response_create(http_response_data, &data_type, &src_addr, recv_data, recv_size);
    MDF_ERROR_GOTO(http_response_size < 0, EXIT, "mdf_http_response_create http_response_size: %d", http_response_size);

    mdf_server_conn_list_lock();

    do {
        server_http_connect_t *conn = mdf_server_conn_find(&dest_addr);
        MDF_ERROR_BREAK(!conn, "mdf_server_conn_find, conn: %p", conn);

        if (conn->chunk == MDF_HTTP_CHUNK_HEADER) {
            char *chunk_header =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/http\r\n"
                "Transfer-Encoding: chunked\r\n\r\n";

            MDF_LOGD("send chunk_header, sockfd: %d, chunk_header: %s", conn->sockfd, chunk_header);
            ret = mdf_server_conn_send(conn, chunk_header, strlen(chunk_header));
            MDF_ERROR_BREAK(ret <= 0, "mdf_server_conn_send, sockfd: %d", conn->sockfd);

            conn->chunk = MDF_HTTP_CHUNK_BODY;
        }

        if (conn->chunk == MDF_HTTP_CHUNK_BODY) {
            char chunk_size[16] = {0};
            sprintf(chunk_size, "%x\r\n", http_response_size);

            MDF_LOGD("send chunk_size, sockfd: %d, data: %s", conn->sockfd, chunk_size);
            ret = mdf_server_conn_send(conn, chunk_size, strlen(chunk_size));
            MDF_ERROR_BREAK(ret <= 0, "mdf_server_conn_send, sockfd: %d", conn->sockfd);

            conn->chunk_num--;
            MDF_LOGD("conn->chunk_num: %d", conn->chunk_num);

            http_response_data[http_response_size++] = '\r';
            http_response_data[http_response_size++] = '\n';
        }

        http_response_data[http_response_size] = '\0';
        MDF_LOGD("http response, sockfd: %d, size: %d, data: \n%s",
                 conn->sockfd, http_response_size, http_response_data);

        ret = mdf_server_conn_send(conn, http_response_data, http_response_size);
        MDF_ERROR_BREAK(ret <= 0, "mdf_server_conn_send, sockfd: %d", conn->sockfd);

        if (conn->chunk && !conn->chunk_num) {
            char *chunk_footer = "0\r\n\r\n";

            MDF_LOGD("send chunk_footer, sockfd: %d, data: %s", conn->sockfd, chunk_footer);
            ret = mdf_server_conn_send(conn, chunk_footer, strlen(chunk_footer));
            MDF_ERROR_BREAK(ret <= 0, "mdf_server_conn_send, sockfd: %d", conn->sockfd);
        }
    } while (0);

    mdf_server_conn_list_unlock();

EXIT:
    mdf_free(http_response_data);
    mdf_free(recv_data);
    return ESP_OK;
}
