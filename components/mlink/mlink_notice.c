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
#include "esp_wifi.h"
#include "mdf_common.h"

#define MLINK_HTTP_SERVER_PORT            (80)
#define MLINK_NOTICE_UDP_QUEUE_NUM        (10)
#define MLINK_NOTICE_UDP_RETRY_COUNT      (3)
#define MLINK_NOTICE_SOCKET_INVALID_FD    (-1)
#define MLINK_NOTICE_UDP_RECV_TIMEROUT_MS (100)
#define MLINK_NOTICE_UDP_BUF_SIZE         (64)

#ifndef CONFIG_MLINK_NOTIC_UDP_CLIENT_PORT
#define CONFIG_MLINK_NOTIC_UDP_CLIENT_PORT    1025
#endif /**< CONFIG_MLINK_NOTIC_UDP_CLIENT_PORT */
#define MLINK_NOTIC_UDP_CLIENT_PORT CONFIG_MLINK_NOTIC_UDP_CLIENT_PORT

#ifndef CONFIG_MLINK_NOTICE_UDP_SERVER_PORT
#define CONFIG_MLINK_NOTICE_UDP_SERVER_PORT 3232
#endif /**< CONFIG_MLINK_NOTICE_UDP_SERVER_PORT */
#define MLINK_NOTICE_UDP_SERVER_PORT CONFIG_MLINK_NOTICE_UDP_SERVER_PORT

static QueueHandle_t *g_notice_udp_queue       = NULL;
static SemaphoreHandle_t g_notice_udp_exit_sem = NULL;
static bool g_notice_udp_exit_flag             = true;
static bool g_notice_mdns_init_flag            = false;
static const char *TAG                         = "mlink_notice";

typedef struct {
    uint8_t mac[6];
    char message[0];
} notice_udp_data_t;

static mdf_err_t mlink_notice_mdns_init(void)
{
    if (g_notice_mdns_init_flag) {
        return MDF_OK;
    }

    mdf_err_t ret                   = MDF_OK;
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

    uint8_t retry_count = 30;

    do {
        ret = mdns_service_add(NULL, "_mesh-http", "_tcp", MLINK_HTTP_SERVER_PORT,
                               ardu_txt_data, sizeof(ardu_txt_data) / sizeof(mdns_txt_item_t));

        if (ret != MDF_OK) {
            vTaskDelay(100 / portTICK_RATE_MS);
        }
    } while (ret != MDF_OK && retry_count--);

    MDF_ERROR_CHECK(ret != MDF_OK, ret, "mdns_service_add");

    g_notice_mdns_init_flag = true;

    return MDF_OK;
}

static void mlink_notice_mdns_deinit(void)
{
    if (!g_notice_mdns_init_flag) {
        return ;
    }

    mdns_free();
    g_notice_mdns_init_flag = false;
}

static int mlink_notice_udp_broadcast_create(void)
{
    mdf_err_t ret = ESP_OK;

    int opt_val = 1;
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    MDF_ERROR_GOTO(sockfd < 0, ERR_EXIT, "socket create:%s", strerror(errno));

    ret = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &opt_val, sizeof(int));
    MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "socket setsockopt: %s", strerror(errno));

    ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(int));
    MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "setsockopt SO_REUSEADDR, sockfd: %d", sockfd);

    MDF_LOGD("create udp broadcast, sockfd: %d", sockfd);

    return sockfd;

ERR_EXIT:

    if (sockfd != MLINK_NOTICE_SOCKET_INVALID_FD) {
        close(sockfd);
    }

    return MLINK_NOTICE_SOCKET_INVALID_FD;
}

static int mlink_notice_udp_server_create(uint16_t port)
{
    mdf_err_t ret = ESP_OK;
    struct sockaddr_in server_addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(port),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    MDF_ERROR_GOTO(sockfd < 0, ERR_EXIT, "socket create:%s", strerror(errno));

    ret = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "socket bind:%s", strerror(errno));

    struct timeval socket_timeout = {0, MLINK_NOTICE_UDP_RECV_TIMEROUT_MS * 1000};
    ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &socket_timeout, sizeof(struct timeval));
    MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "socket bind:%s", strerror(errno));

    MDF_LOGD("create udp server, port: %d, sockfd: %d", port, sockfd);

    return sockfd;

ERR_EXIT:

    if (sockfd != MLINK_NOTICE_SOCKET_INVALID_FD) {
        ret = close(sockfd);

        if (ret != ESP_OK) {
            MDF_LOGW("close fail, ret: %d", ret);
        }
    }

    return MLINK_NOTICE_SOCKET_INVALID_FD;
}

static void mlink_notice_udp_task(void *arg)
{
    mdf_err_t ret                = MDF_OK;
    char message_tmp[32]         = {0};
    char *broadcast_msg_buf      = NULL;
    size_t broadcast_msg_size    = 0;
    notice_udp_data_t *q_data    = NULL;
    struct sockaddr_in from_addr = {0};
    uint8_t root_mac[6]          = {0};
    char *udp_server_buf         = MDF_MALLOC(MLINK_NOTICE_UDP_BUF_SIZE);
    socklen_t from_addr_len      = sizeof(struct sockaddr_in);
    int udp_client_sockfd        = mlink_notice_udp_broadcast_create();
    int udp_server_sockfd        = mlink_notice_udp_server_create(MLINK_NOTIC_UDP_CLIENT_PORT);

    if (udp_client_sockfd == MLINK_NOTICE_SOCKET_INVALID_FD
            || udp_server_sockfd == MLINK_NOTICE_SOCKET_INVALID_FD) {
        MDF_LOGE("Failed to create UDP notification service");

        if (udp_client_sockfd != MLINK_NOTICE_SOCKET_INVALID_FD) {
            close(udp_client_sockfd);
        }

        if (udp_server_sockfd != MLINK_NOTICE_SOCKET_INVALID_FD) {
            close(udp_server_sockfd);
        }

        vTaskDelete(NULL);
        return ;
    }

    ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, root_mac));

    while (!g_notice_udp_exit_flag) {
        memset(udp_server_buf, 0, MLINK_NOTICE_UDP_BUF_SIZE);

        if (xQueueReceive(g_notice_udp_queue, &q_data, MLINK_NOTICE_UDP_RECV_TIMEROUT_MS / portTICK_RATE_MS)) {
            strncpy(message_tmp, q_data->message, sizeof(message_tmp));
            broadcast_msg_buf  = MDF_MALLOC(MLINK_NOTICE_UDP_QUEUE_NUM * 13 + 64);
            broadcast_msg_size = sprintf(broadcast_msg_buf, "mac=%02x%02x%02x%02x%02x%02x", MAC2STR(q_data->mac));
            MDF_FREE(q_data);

            do {
                if (!g_notice_udp_queue || !xQueueReceive(g_notice_udp_queue, &q_data,
                        MLINK_NOTICE_UDP_RECV_TIMEROUT_MS / portTICK_RATE_MS)) {
                    break;
                }

                if (strncmp(q_data->message, message_tmp, sizeof(message_tmp))) {
                    if (!xQueueSend(g_notice_udp_queue, &q_data, 0)) {
                        MDF_FREE(q_data);
                    }

                    break;
                }

                if (strcmp(q_data->message, "http")) {
                    broadcast_msg_size += sprintf(broadcast_msg_buf + broadcast_msg_size,
                                                  ",%02x%02x%02x%02x%02x%02x", MAC2STR(q_data->mac));
                }

                MDF_FREE(q_data);
            } while (broadcast_msg_size < MLINK_NOTICE_UDP_QUEUE_NUM * 13);

            broadcast_msg_size += sprintf(broadcast_msg_buf + broadcast_msg_size,
                                          "\r\nflag=%d\r\ntype=%s\r\n", xTaskGetTickCount(), message_tmp);

            MDF_LOGD("Mlink notice udp broadcast, size: %d, data:\n%s", broadcast_msg_size, broadcast_msg_buf);

            for (int i = 0, delay_time_ms = 0; i < MLINK_NOTICE_UDP_RETRY_COUNT; ++i, delay_time_ms += delay_time_ms) {
                vTaskDelay(delay_time_ms);
                delay_time_ms = (i == 0) ? (50 / portTICK_RATE_MS) : delay_time_ms;
                delay_time_ms = (delay_time_ms > 200) ? 200 : delay_time_ms;

                struct sockaddr_in broadcast_addr = {
                    .sin_family      = AF_INET,
                    .sin_port        = htons(MLINK_NOTICE_UDP_SERVER_PORT),
                    .sin_addr.s_addr = htonl(INADDR_BROADCAST),
                };

                ret = sendto(udp_client_sockfd, broadcast_msg_buf, strlen(broadcast_msg_buf), 0,
                             (struct sockaddr *)&broadcast_addr, sizeof(struct sockaddr));

                if (ret < 0 && errno == ENOMEM) {
                    MDF_LOGD("sendto not enough space, delay 100ms");
                    vTaskDelay(100 / portMAX_DELAY);
                }
            }

            MDF_FREE(broadcast_msg_buf);
        } else if (recvfrom(udp_server_sockfd, udp_server_buf, MLINK_NOTICE_UDP_BUF_SIZE,
                            0, (struct sockaddr *)&from_addr, (socklen_t *)&from_addr_len) > 0) {
            MDF_LOGD("Mlink notice udp recvfrom, sockfd: %d, port: %d, ip: %s, udp_server_buf: %s",
                     udp_server_sockfd, ntohs(((struct sockaddr_in *)&from_addr)->sin_port),
                     inet_ntoa(((struct sockaddr_in *)&from_addr)->sin_addr), udp_server_buf);

            if (strcmp(udp_server_buf, "Are You Espressif IOT Smart Device?")) {
                continue;
            }

            sprintf(udp_server_buf, "ESP32 Mesh %02x%02x%02x%02x%02x%02x http %d",
                    MAC2STR(root_mac), MLINK_HTTP_SERVER_PORT);

            MDF_LOGD("Mlink notice udp sendto, sockfd: %d, data: %s", udp_server_sockfd, udp_server_buf);

            for (int i = 0, delay_time_ms = 0; i < MLINK_NOTICE_UDP_RETRY_COUNT; ++i, delay_time_ms += delay_time_ms) {
                vTaskDelay(delay_time_ms);
                delay_time_ms = (i == 0) ? (10 / portTICK_RATE_MS) : delay_time_ms;

                if (sendto(udp_server_sockfd, udp_server_buf, strlen(udp_server_buf),
                           0, (struct sockaddr *)&from_addr, from_addr_len) <= 0) {
                    MDF_LOGW("Mlink notice udp sendto, errno: %d, errno_str: %s", errno, strerror(errno));
                    break;
                }
            }
        }
    }

    if (g_notice_udp_exit_sem) {
        xSemaphoreGive(g_notice_udp_exit_sem);
    }

    close(udp_client_sockfd);
    close(udp_server_sockfd);
    MDF_FREE(udp_server_buf);
    vTaskDelete(NULL);
}

mdf_err_t mlink_notice_write(const char *message, size_t size, const uint8_t *addr)
{
    MDF_PARAM_CHECK(addr);
    MDF_PARAM_CHECK(message);
    MDF_PARAM_CHECK(size > 0);

    if (!g_notice_udp_queue) {
        g_notice_udp_queue = xQueueCreate(MLINK_NOTICE_UDP_QUEUE_NUM, sizeof(void *));
    }

    notice_udp_data_t *q_data = NULL;

    if (!uxQueueSpacesAvailable(g_notice_udp_queue)
            && xQueueReceive(g_notice_udp_queue, &q_data, 0)) {
        MDF_LOGD("g_notice_udp_queue is full, delete the front item");
        MDF_FREE(q_data);
    }

    q_data = MDF_CALLOC(1, size + sizeof(notice_udp_data_t) + 1);
    memcpy(q_data->message, message, size);
    memcpy(q_data->mac, addr, 6);
    xQueueSend(g_notice_udp_queue, &q_data, 0);

    return MDF_OK;
}

static mdf_err_t mlink_notice_udp_init()
{
    if (!g_notice_udp_exit_flag) {
        return MDF_OK;
    }

    g_notice_udp_exit_flag = false;

    if (!g_notice_udp_queue) {
        g_notice_udp_queue = xQueueCreate(MLINK_NOTICE_UDP_QUEUE_NUM, sizeof(void *));
    }

    if (!g_notice_udp_exit_sem) {
        g_notice_udp_exit_sem = xSemaphoreCreateBinary();
    }

    xTaskCreatePinnedToCore(mlink_notice_udp_task, "mlink_notice_udp", 3 * 1024,
                            NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY,
                            NULL, CONFIG_MDF_TASK_PINNED_TO_CORE);

    return MDF_OK;
}

static void mlink_notice_udp_deinit()
{
    if (g_notice_udp_exit_flag) {
        return ;
    }

    g_notice_udp_exit_flag = true;

    if (g_notice_udp_exit_sem) {
        xSemaphoreTake(g_notice_udp_exit_sem, portMAX_DELAY);
        vQueueDelete(g_notice_udp_exit_sem);
        g_notice_udp_exit_sem  = NULL;
    }

    if (g_notice_udp_queue) {
        vQueueDelete(g_notice_udp_queue);
        g_notice_udp_queue = NULL;
    }
}

mdf_err_t mlink_notice_init()
{
    mdf_err_t ret = MDF_OK;

    ret = mlink_notice_mdns_init();
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "mlink_notice_mdns_init");

    ret = mlink_notice_udp_init();
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "mlink_notice_mdns_init");

    return MDF_OK;
}

mdf_err_t mlink_notice_deinit()
{
    mlink_notice_mdns_deinit();
    mlink_notice_udp_deinit();

    return MDF_OK;
}
