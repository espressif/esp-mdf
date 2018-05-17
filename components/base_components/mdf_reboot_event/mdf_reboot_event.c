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

#include "mdf_info_store.h"
#include "mdf_event_loop.h"
#include "mdf_network_config.h"
#include "mdf_espnow_debug.h"

#define REBOOT_TRIGGER_WAIT_MS  3000
#define REBOOT_NUM_KEY          "reboot_num"

static const char *TAG = "mdf_reboot_handle";

/*********************** mdf coredump detect ***********************/
#ifdef CONFIG_MDF_COREDUMP_DETECT

#define MDF_COREDUMP_DATA_MAX_LEN  (1000)
static SemaphoreHandle_t xSemConnet = NULL;

typedef struct {
    char ssid[32];
    char password[64];
    char server_ip[16];
    uint16_t server_port;
    char file_name[32];
    int timeout_ms;
    uint32_t light_io_num[5];
} mdf_server_conn_t;

static mdf_server_conn_t default_coredump_server_conn = {
    .router_ssid     = "ASUS_MDF_TEST",
    .router_password = "mdf_test",
    .server_ip       = "192.168.99.254",
    .server_port     = 8765,
    .file_name       = "/mdf_demo.bin",
    .timeout_ms      = 5 * 60 * 1000,
};

typedef struct {
    uint8_t mac[6];
    char    version[20];
    uint8_t flag;
    size_t  size;
    uint8_t data[0];
} mdf_coredump_data_t;

typedef enum {
    MDF_COREDUMP_FLAG_START = 0,
    MDF_COREDUMP_FLAG_ING,
    MDF_COREDUMP_FLAG_END,
    MDF_COREDUMP_FLAG_MAX,
} mdf_coredump_trans_flag_t;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_GOT_IP:
            xSemaphoreGive(xSemConnet);
            MDF_LOGI("SYSTEM_EVENT_STA_GOT_IP");
            break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
            MDF_LOGI("SYSTEM_EVENT_STA_DISCONNECTED, free_heap: %d", esp_get_free_heap_size());
            MDF_ERROR_CHECK(esp_wifi_connect(), ESP_FAIL, "esp_wifi_connect fail");
            break;

        default:
            break;
    }

    return ESP_OK;
}

static esp_err_t mdf_server_connect(mdf_server_conn_t *connect_data)
{
    esp_err_t ret                          = ESP_OK;
    wifi_config_t wifi_config              = { 0 };
    mdf_server_conn_t mdf_server_conn      = { 0 };
    mdf_server_conn_t *mdf_server_conn_ptr = &mdf_server_conn;

    if (mdf_info_load("internal_ota", mdf_server_conn_ptr, sizeof(mdf_server_conn_t)) < 0) {
        mdf_server_conn_ptr = (mdf_server_conn_t *)&default_coredump_server_conn;
    }

    if (!xSemConnet) {
        xSemConnet = xSemaphoreCreateBinary();
        MDF_ERROR_CHECK(!xSemConnet, ESP_FAIL, "xSemaphoreCreateBinary, xSemConnet: %p", xSemConnet);
    }

    esp_event_loop_set_cb(event_handler, NULL);
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config));
    memcpy(wifi_config.sta.ssid, mdf_server_conn_ptr->router_ssid, sizeof(wifi_config.sta.ssid));
    memcpy(wifi_config.sta.password, mdf_server_conn_ptr->router_password, sizeof(wifi_config.sta.password));
    MDF_LOGI("conn ap ssid: %s, password: %s", wifi_config.sta.ssid, wifi_config.sta.password);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());

    ret = xSemaphoreTake(xSemConnet, 10 * 1000 / portTICK_RATE_MS);
    MDF_ERROR_CHECK(!ret, ESP_FAIL, "xSemaphoreTake, ret: %d", ret);

    memcpy(connect_data, mdf_server_conn_ptr, sizeof(mdf_server_conn_t));
    return ESP_OK;
}

static esp_err_t mdf_debug_coredump_send(mdf_server_conn_t *coredump_conn,
        const esp_partition_t *coredump_p, size_t coredump_len)
{
    MDF_ERROR_CHECK(!coredump_conn || !coredump_p || coredump_len <= 0, ESP_FAIL,
                    "mdf_debug_coredump_send args err, coredump_conn: %p, coredump_p: %p, coredump_len: %d",
                    coredump_conn, coredump_p, coredump_len);

    esp_err_t ret   = ESP_OK;
    size_t rest_len = 0;
    size_t read_len = 0;
    int sockfd      = MDF_SOCKET_INVALID_FD;
    uint8_t *buffer = mdf_calloc(1, WIFI_MESH_PACKET_MAX_SIZE);
    mdf_coredump_data_t *mdf_coredump_data = NULL;

    sockfd = mdf_socket_tcp_client_create(coredump_conn->server_ip,
                                          coredump_conn->server_port);
    MDF_ERROR_GOTO(sockfd < 0, ERR_EXIT, "mdf_socket_tcp_client_create, sockfd: %d", sockfd);

    mdf_coredump_data = (mdf_coredump_data_t *)buffer;
    ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, mdf_coredump_data->mac));
    memcpy(mdf_coredump_data->version, MDF_DEVICE_VERSION, MIN((20 - 1), sizeof(MDF_DEVICE_VERSION)));
    mdf_coredump_data->flag = MDF_COREDUMP_FLAG_START;
    mdf_coredump_data->size = coredump_len;

    ret = mdf_socket_send(sockfd, mdf_coredump_data, sizeof(mdf_coredump_data_t));
    MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "mdf_socket_send to coredump server failed: %d", ret);
    MDF_LOGI("******   MDF_COREDUMP_FLAG_START, len: %d   ******", coredump_len);

    mdf_coredump_data->flag = MDF_COREDUMP_FLAG_ING;
    rest_len = coredump_len;

    for (size_t offset = 4; offset < (coredump_len + 4); offset += read_len, rest_len -= read_len) {
        read_len = MIN(MDF_COREDUMP_DATA_MAX_LEN, rest_len);
        ret = esp_partition_read(coredump_p, offset, mdf_coredump_data->data, read_len);
        MDF_ERROR_GOTO(ret, ERR_EXIT, "esp_partition_read fail, ret: %d", ret);

        mdf_coredump_data->size = read_len;
        ret = mdf_socket_send(sockfd, mdf_coredump_data, sizeof(mdf_coredump_data_t) + read_len);
        MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "mdf_socket_send to coredump server failed: %d", ret);
        MDF_LOGI("MDF_COREDUMP_FLAG_ING, payload len: %d", read_len);
    }

    mdf_coredump_data->flag = MDF_COREDUMP_FLAG_END;
    mdf_coredump_data->size = 0;
    ret = mdf_socket_send(sockfd, mdf_coredump_data, sizeof(mdf_coredump_data_t));
    MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "mdf_socket_send to coredump server failed: %d", ret);
    MDF_LOGI("******   MDF_COREDUMP_FLAG_END   ******");
    mdf_socket_close(sockfd);

    ret = mdf_coredump_erase();
    MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "mdf_coredump_erase fail, ret: %d", ret);

    mdf_free(buffer);
    return ESP_OK;

ERR_EXIT:
    mdf_socket_close(sockfd);
    mdf_free(buffer);
    return ESP_FAIL;
}

esp_err_t mdf_coredump_part_detect(void)
{
    esp_err_t ret      = ESP_OK;
    size_t coredump_len = 0;
    mdf_server_conn_t coredump_server_conn = { 0 };

    const esp_partition_t *coredump_p = mdf_coredump_get_info(&coredump_len);
    MDF_ERROR_CHECK(!coredump_p, ESP_FAIL, "mdf_coredump_get_info fail");

    if (coredump_len == -1) {
        MDF_LOGD("mdf_coredump_get_info coredump_len == -1");
        return ESP_OK;
    }

    ret = mdf_server_connect(&coredump_server_conn);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_server_connect fail, ret:%d", ret);

    ret = mdf_debug_coredump_send(&coredump_server_conn, coredump_p, coredump_len);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_debug_coredump_send fail, ret:%d", ret);

    return ESP_OK;
}
#endif /**< CONFIG_MDF_COREDUMP_DETECT */

static esp_err_t mdf_reboot_handle()
{
    esp_err_t ret     = ESP_OK;
    uint8_t reboot_num = 0;

    /**< If the device restarts within the instruction time,
    the event_mdoe value will be incremented by one */
    mdf_info_load(REBOOT_NUM_KEY, &reboot_num, sizeof(uint8_t));
    reboot_num++;

    ret = mdf_info_save(REBOOT_NUM_KEY, &reboot_num, sizeof(uint8_t));
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_save, ret:%d", ret);

    switch (reboot_num) {
        case 1:
        case 2:

#ifdef CONFIG_MDF_COREDUMP_DETECT
            ret = mdf_coredump_part_detect();
            MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_coredump_part_detect fail");
#endif /**< CONFIG_MDF_COREDUMP_DETECT */

            break;

        case 3:
        case 4:
            MDF_LOGW("reboot_num: %d, enter the configuration network mode", reboot_num);

            ret = mdf_event_loop_send(MDF_EVENT_NETWORK_START_CONFIG, NULL);
            MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_event_loop_send, MDF_EVENT_NETWORK_START_CONFIG");

            mdf_network_clear_config();

            return ESP_OK;
            break;

        default:
            MDF_LOGW("reboot_num: %d, system reset", reboot_num);

            mdf_info_erase(MDF_SPACE_NAME);

            esp_restart();
            break;
    }

    network_config_t network_config = {0};
    size_t coredump_len                  = 0;
    /**< detect coredump partition, if coredump data exist, set light red */
    const esp_partition_t *coredump_p    = mdf_coredump_get_info(&coredump_len);
    MDF_ERROR_CHECK(!coredump_p, ESP_FAIL, "mdf_coredump_get_info fail");

    if ((ssize_t)coredump_len > 0) {
        MDF_LOGW("mdf_coredump_get_info coredump exist");
        ret = mdf_event_loop_send(MDF_EVENT_SYSTEM_CRASH, NULL);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_event_loop_delay_send, ret: %d", ret);
    } else if (mdf_info_load(MDF_NETWORK_CONFIG_KEY, &network_config, sizeof(network_config_t)) > 0) {
        MDF_LOGD("reboot_num: %d, running normally", reboot_num);
        ret = mdf_event_loop_send(MDF_EVENT_SYSTEM_RUNNING, NULL);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_event_loop_delay_send, ret: %d", ret);
    }

    return ESP_OK;
}

static void mdf_reboot_num_clear(void *timer)
{
    if (!xTimerStop(timer, 0)) {
        MDF_LOGE("xTimerStop timer %p", timer);
    }

    if (!xTimerDelete(timer, 0)) {
        MDF_LOGE("xTimerDelete timer %p", timer);
    }

    if (mdf_info_erase(REBOOT_NUM_KEY) < 0) {
        MDF_LOGE("mdf_info_erase");
    }

    MDF_LOGD("erase reboot number");
}

esp_err_t mdf_reboot_event_init()
{
    if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_UNDEFINED) {
        mdf_info_erase(REBOOT_NUM_KEY);
        return ESP_OK;
    }

    TimerHandle_t timer = NULL;
    esp_err_t ret      = ESP_OK;

    timer = xTimerCreate("mdf_reboot_num_clear", REBOOT_TRIGGER_WAIT_MS / portTICK_RATE_MS,
                         false, NULL, mdf_reboot_num_clear);
    MDF_ERROR_CHECK(!timer, ESP_FAIL, "xTaskCreate, timer: %p", timer);
    xTimerStart(timer, 0);

    ret = mdf_reboot_handle();
    MDF_LOGD("mdf_reboot_handle, ret: %d", ret);

    return ESP_OK;
}
