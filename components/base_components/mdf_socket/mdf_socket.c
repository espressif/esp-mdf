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

#include "lwip/sockets.h"

#include "mdf_socket.h"
static const char *TAG = "mdf_socket";

sockfd_t mdf_socket_tcp_server_create(uint16_t port, uint16_t backlog)
{
    esp_err_t ret  = ESP_OK;
    int opt_val     = 1;
    sockfd_t sockfd = MDF_SOCKET_INVALID_FD;
    struct sockaddr_in server_addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(port),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    MDF_ERROR_GOTO(sockfd < 0, ERR_EXIT, "socket create");

    ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));
    MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "setsockopt SO_REUSEADDR, sockfd: %d", sockfd);

    ret = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "socket bind, sockfd: %d", sockfd);

    ret = listen(sockfd, backlog);
    MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "socket listen, sockfd: %d", sockfd);

    MDF_LOGD("create tcp server, port: %d, backlog: %d, sockfd: %d", port, backlog, sockfd);

    return sockfd;

ERR_EXIT:

    if (sockfd != MDF_SOCKET_INVALID_FD) {
        close(sockfd);
    }

    return MDF_SOCKET_INVALID_FD;
}

sockfd_t mdf_socket_tcp_client_create(const char *ip, uint16_t port)
{
    MDF_PARAM_CHECK(ip);

    esp_err_t ret                  = ESP_OK;
    sockfd_t sockfd                = MDF_SOCKET_INVALID_FD;
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = inet_addr(ip),
    };

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    MDF_ERROR_GOTO(sockfd < 0, ERR_EXIT, "socket create: sockfd: %d", sockfd);

    ret = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
    MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "socket connect, ret: %d, ip: %s, port: %d",
                   ret, ip, port);

    MDF_LOGD("connect tcp server ip: %s, port: %d, sockfd: %d", ip, port, sockfd);

    return sockfd;

ERR_EXIT:

    if (sockfd != MDF_SOCKET_INVALID_FD) {
        close(sockfd);
    }

    return MDF_SOCKET_INVALID_FD;
}

sockfd_t mdf_socket_udp_server_create(uint16_t port)
{
    esp_err_t ret = ESP_OK;
    struct sockaddr_in server_addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(port),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    MDF_ERROR_GOTO(sockfd < 0, ERR_EXIT, "socket create:%s", strerror(errno));

    ret = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "socket bind:%s", strerror(errno));

    struct timeval socket_timeout = {0, 500000};
    ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &socket_timeout, sizeof(struct timeval));
    MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "socket bind:%s", strerror(errno));

    MDF_LOGD("create udp server, port: %d, sockfd: %d", port, sockfd);

    return sockfd;

ERR_EXIT:

    if (sockfd != MDF_SOCKET_INVALID_FD) {
        ret = close(sockfd);

        if (ret != ESP_OK) {
            MDF_LOGW("close fail, ret: %d", ret);
        }
    }

    return MDF_SOCKET_INVALID_FD;
}

sockfd_t mdf_socket_udp_broadcast_create(void)
{
    esp_err_t ret = ESP_OK;

    int opt_val = 1;
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    MDF_ERROR_GOTO(sockfd < 0, ERR_EXIT, "socket create:%s", strerror(errno));

    ret = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &opt_val, sizeof(int));
    MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "socket setsockopt: %s", strerror(errno));

    ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(int));
    MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "setsockopt SO_REUSEADDR, sockfd: %d", sockfd);

    MDF_LOGD("create udp multicast, sockfd: %d", sockfd);

    return sockfd;

ERR_EXIT:

    if (sockfd != MDF_SOCKET_INVALID_FD) {
        close(sockfd);
    }

    return MDF_SOCKET_INVALID_FD;
}

ssize_t mdf_socket_send(sockfd_t sockfd, void *buf, size_t len)
{
    MDF_PARAM_CHECK(buf);

    return send(sockfd, buf, len, 0);
}

ssize_t mdf_socket_recv(sockfd_t sockfd, void *buf, size_t len)
{
    MDF_PARAM_CHECK(buf);

    return recv(sockfd, buf, len, 0);
}

void mdf_socket_close(sockfd_t sockfd)
{
    esp_err_t ret = ESP_OK;

    if (sockfd != MDF_SOCKET_INVALID_FD) {
        ret = close(sockfd);

        if (ret != ESP_OK) {
            MDF_LOGW("close fail, ret: %d", ret);
        }
    }
}

esp_err_t mdf_socket_timeout(sockfd_t sockfd, uint32_t send_timeout_ms, uint32_t recv_timeout_ms)
{
    esp_err_t ret = 0;
    struct timeval timeout = {
        .tv_usec = (send_timeout_ms % 1000) * 1000,
        .tv_sec = send_timeout_ms / 1000,
    };

    ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "socket setsockopt, ret: %d", ret);

    timeout.tv_usec = (recv_timeout_ms % 1000) * 1000;
    timeout.tv_sec  = recv_timeout_ms / 1000;
    ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "socket setsockopt, ret: %d", ret);

    return ESP_OK;
}

esp_err_t mdf_socket_keepalive(sockfd_t sockfd, int keep_idle, int keep_interval, int keep_count)
{
    MDF_PARAM_CHECK(sockfd != MDF_SOCKET_INVALID_FD);

    esp_err_t ret = 0;
    int keep_alive = 1;

    ret = setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &keep_alive, sizeof(keep_alive));
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "setsockopt SO_KEEPALIVE, ret: %d", ret);

    ret = setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPIDLE, &keep_idle, sizeof(keep_idle));
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "setsockopt TCP_KEEPIDLE, ret: %d", ret);

    ret = setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPINTVL, &keep_interval, sizeof(keep_interval));
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "setsockopt TCP_KEEPINTVL, ret: %d", ret);

    ret = setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPCNT, &keep_count, sizeof(keep_count));
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "setsockopt TCP_KEEPCNT, ret: %d", ret);

    return ESP_OK;
}

bool mdf_socket_isreadable(sockfd_t sockfd)
{
    fd_set read_fds;
    struct timeval timeout = {0, 0};

    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);

    if (select(sockfd + 1, &read_fds, NULL, NULL, &timeout) <= 0) {
        return false;
    }

    return true;
}

void mdf_socket_fflush(sockfd_t sockfd)
{
    fd_set fds;
    char tmp[2] = {0};
    struct timeval timeout = {0, 0};

    FD_ZERO(&fds);
    FD_SET(sockfd, &fds);

    while (select(sockfd + 1, &fds, NULL, NULL, &timeout)) {
        mdf_socket_recv(sockfd, tmp, 1);
    }
}
