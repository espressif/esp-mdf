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

#include "mdns.h"

#include "mdf_socket.h"
#include "mdf_notice.h"
#include "mdf_http_handle.h"

#define MDF_CONNECT_UDP_BUF_SIZE      128
#define MDF_NOTICE_ROUTING_COUNT      10
#define MDF_NOTICE_STATUS_COUNT       5

typedef struct {
    notice_type_t notice;
    uint8_t mac[6];
} notice_queue_data_t;

static QueueHandle_t *g_notice_queue   = NULL;
static sockfd_t g_udp_broadcast_sockfd = MDF_SOCKET_INVALID_FD;
static sockfd_t g_udp_server_sockfd    = MDF_SOCKET_INVALID_FD;
static const char *TAG                 = "mdf_notice";

esp_err_t mdf_notice_mdns_init(void)
{
    esp_err_t ret                   = ESP_OK;
    uint8_t root_mac[6]             = {0};
    char mac_str[16]                = {0};
    mdns_txt_item_t ardu_txt_data[] = {
        {"mac", mac_str},
    };

    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set("esp32_mesh"));
    ESP_ERROR_CHECK(mdns_instance_name_set("mesh"));

    ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, root_mac));
    sprintf(mac_str, "%02x%02x%02x%02x%02x%02x", MAC2STR(root_mac));

#ifdef CONFIG_MDF_USE_HTTPS
    ret = mdns_service_add(NULL, "_mesh-https", "_tcp", MDF_HTTPS_SERVER_PORT,
                           ardu_txt_data, sizeof(ardu_txt_data) / sizeof(mdns_txt_item_t));
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "mdns_service_add, ret: %d", ret);
#else
    ret = mdns_service_add(NULL, "_mesh-http", "_tcp", MDF_HTTP_SERVER_PORT,
                           ardu_txt_data, sizeof(ardu_txt_data) / sizeof(mdns_txt_item_t));
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "mdns_service_add, ret: %d", ret);
#endif

    MDF_LOGD("mdf_notice_mdns_init, key: %s, data: %s",
             ardu_txt_data[0].key, ardu_txt_data[0].value);

    return ESP_OK;
}

esp_err_t mdf_notice_mdns_deinit()
{
    mdns_free();

    return ESP_OK;
}

static const char *mdf_data_notice_name(notice_type_t type)
{
    const char *notice_name = NULL;

    switch (type) {
        case NOTICE_STATUS:
            notice_name = "status";
            break;

        case NOTICE_CONNECTED:
        case NOTICE_DISCONNECTED:
        case NOTICE_ROUTING_TABLE_CHANGE:
#ifdef CONFIG_MDF_USE_HTTPS
            notice_name = "https";
#else
            notice_name = "http";
#endif
            break;

        case NOTICE_SNIFFER:
            notice_name = "sniffer";
            break;

        default:
            notice_name = "NULL";
            break;
    }

    return notice_name;
}

esp_err_t mdf_notice_udp_send(notice_type_t notice, const uint8_t *addr)
{
    MDF_PARAM_CHECK(addr);

    if (!g_notice_queue) {
        g_notice_queue = xQueueCreate(SERVER_NOTICE_QUEUE_NUM, sizeof(void *));
    }

    notice_queue_data_t *q_data = mdf_malloc(sizeof(notice_queue_data_t));
    q_data->notice              = notice;
    memcpy(q_data->mac, addr, 6);

    if (!xQueueSend(g_notice_queue, &q_data, 0)) {
        MDF_LOGW("xQueueSend, notice: 0x%x, mac: "MACSTR, notice, MAC2STR(addr));
        mdf_free(q_data);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t mdf_notice_udp_init()
{
    esp_err_t ret       = ESP_OK;
    uint8_t root_mac[6] = {0};

    if (!g_notice_queue) {
        g_notice_queue = xQueueCreate(SERVER_NOTICE_QUEUE_NUM, sizeof(void *));
    }

    ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, root_mac));
    mdf_notice_udp_send(NOTICE_ROUTING_TABLE_CHANGE, root_mac);

    if ((g_udp_broadcast_sockfd = mdf_socket_udp_broadcast_create()) == MDF_SOCKET_INVALID_FD) {
        MDF_LOGW("mdf_socket_udp_broadcast_create, ret: %d", ret);
        return ESP_FAIL;
    }

    if ((g_udp_server_sockfd = mdf_socket_udp_server_create(MDF_UDP_SERVER_PORT)) == MDF_SOCKET_INVALID_FD) {
        MDF_LOGW("mdf_socket_udp_broadcast_create, ret: %d", ret);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t mdf_notice_udp_deinit()
{
    mdf_socket_close(g_udp_broadcast_sockfd);
    mdf_socket_close(g_udp_server_sockfd);
    vQueueDelete(g_notice_queue);
    g_notice_queue = NULL;

    return ESP_OK;
}

esp_err_t mdf_notice_udp_client()
{
    notice_queue_data_t *q_data = NULL;
    esp_err_t ret               = ESP_OK;
    int retry_count             = MDF_NOTICE_STATUS_COUNT;

    if (xQueueReceive(g_notice_queue, &q_data, 1000 / portTICK_RATE_MS) == false) {
        MDF_LOGV("xQueueReceive, q_data: %p", q_data);
        return ESP_OK;
    }

    notice_type_t notice_tmp  = q_data->notice;
    char *broadcast_msg       = mdf_malloc(SERVER_NOTICE_QUEUE_NUM * 13 + 128);
    size_t broadcast_msg_size = sprintf(broadcast_msg, "mac=%02x%02x%02x%02x%02x%02x", MAC2STR(q_data->mac));
    mdf_free(q_data);

    do {
        if (!g_notice_queue || !xQueueReceive(g_notice_queue, &q_data, 50 / portTICK_RATE_MS)) {
            break;
        }

        if (q_data->notice != notice_tmp) {
            if (!xQueueSend(g_notice_queue, &q_data, 100 / portTICK_RATE_MS)) {
                mdf_free(q_data);
            }

            break;
        }

        if (q_data->notice != NOTICE_ROUTING_TABLE_CHANGE) {
            broadcast_msg_size += sprintf(broadcast_msg + broadcast_msg_size,
                                          ",%02x%02x%02x%02x%02x%02x", MAC2STR(q_data->mac));
            retry_count = MDF_NOTICE_ROUTING_COUNT;
        }

        mdf_free(q_data);
    } while (broadcast_msg_size < SERVER_NOTICE_QUEUE_NUM * 13);

    broadcast_msg_size += sprintf(broadcast_msg + broadcast_msg_size,
                                  "\r\nflag=%d\r\ntype=%s\r\n",
                                  xTaskGetTickCount(), mdf_data_notice_name(notice_tmp));

    MDF_LOGD("udp broadcast sendto, size: %d, data:\n%s", broadcast_msg_size, broadcast_msg);

    for (int i = 0, delay_time_ms = 0; i < retry_count; ++i, delay_time_ms += delay_time_ms) {
        vTaskDelay(delay_time_ms);
        delay_time_ms = (i == 0) ? (20 / portTICK_RATE_MS) : delay_time_ms;
        delay_time_ms = (delay_time_ms > 200) ? 200 : delay_time_ms;

        struct sockaddr_in broadcast_addr = {
            .sin_family      = AF_INET,
            .sin_port        = htons(MDF_UDP_BROADCAST_PORT),
            .sin_addr.s_addr = htonl(INADDR_BROADCAST),
        };

        ret = sendto(g_udp_broadcast_sockfd, broadcast_msg, strlen(broadcast_msg), 0,
                     (struct sockaddr *)&broadcast_addr, sizeof(struct sockaddr));

        if (ret < 0 && errno == ENOMEM) {
            MDF_LOGD("sendto not enough space, delay 100ms");
            vTaskDelay(100 / portMAX_DELAY);
        }
    }

    mdf_free(broadcast_msg);

    return ESP_OK;
}

esp_err_t mdf_notice_udp_server()
{
    uint8_t dev_mac[6] = {0};
    struct sockaddr_in from_addr  = {0};
    socklen_t from_addr_len       = sizeof(struct sockaddr_in);
    char *udp_buf                 = mdf_malloc(MDF_CONNECT_UDP_BUF_SIZE);

    ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, dev_mac));

    if (g_udp_server_sockfd == MDF_SOCKET_INVALID_FD) {
        g_udp_server_sockfd = mdf_socket_udp_server_create(MDF_UDP_SERVER_PORT);

        if (g_udp_server_sockfd == MDF_SOCKET_INVALID_FD) {
            vTaskDelay(1000 / portTICK_RATE_MS);
            goto EXIT;
        }
    }

    memset(udp_buf, 0, MDF_CONNECT_UDP_BUF_SIZE);

    if (recvfrom(g_udp_server_sockfd, udp_buf, MDF_CONNECT_UDP_BUF_SIZE,
                 0, (struct sockaddr *)&from_addr, (socklen_t *)&from_addr_len) <= 0) {
        if (errno != EAGAIN) {
            mdf_socket_close(g_udp_server_sockfd);
            g_udp_server_sockfd = MDF_SOCKET_INVALID_FD;
            MDF_LOGW("recvfrom, sockfd: %d, errno: %d, errno_str: %s",
                     g_udp_server_sockfd, errno, strerror(errno));
        }

        goto EXIT;
    }

    MDF_LOGI("udp recvfrom, sockfd: %d, port: %d, ip: %s, udp_buf: %s",
             g_udp_server_sockfd, ntohs(((struct sockaddr_in *)&from_addr)->sin_port),
             inet_ntoa(((struct sockaddr_in *)&from_addr)->sin_addr), udp_buf);

    if (strcmp(udp_buf, "Are You Espressif IOT Smart Device?")) {
        goto EXIT;
    }

#ifdef CONFIG_MDF_USE_HTTPS
    sprintf(udp_buf, "ESP32 Mesh %02x%02x%02x%02x%02x%02x https %d",
            MAC2STR(dev_mac), MDF_HTTPS_SERVER_PORT);
#else
    sprintf(udp_buf, "ESP32 Mesh %02x%02x%02x%02x%02x%02x http %d",
            MAC2STR(dev_mac), MDF_HTTP_SERVER_PORT);
#endif

    MDF_LOGI("udp sendto, sockfd: %d, data: %s", g_udp_server_sockfd, udp_buf);

    for (int i = 0, delay_time_ms = 0; i < 5; ++i, delay_time_ms += delay_time_ms) {
        vTaskDelay(delay_time_ms);
        delay_time_ms = (i == 0) ? (10 / portTICK_RATE_MS) : delay_time_ms;

        if (sendto(g_udp_server_sockfd, udp_buf, strlen(udp_buf),
                   0, (struct sockaddr *)&from_addr, from_addr_len) <= 0) {
            close(g_udp_server_sockfd);
            g_udp_server_sockfd = MDF_SOCKET_INVALID_FD;
            MDF_LOGW("sendto, errno: %d, errno_str: %s", errno, strerror(errno));

            continue;
        };
    }

EXIT:
    mdf_free(udp_buf);
    return ESP_OK;
}
