// Copyright 2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "esp_http_server.h"
#include "esp_http_client.h"

#include "mdf_common.h"
#include "mlink.h"
#include "mupgrade.h"

#define MLINK_HTTPD_FIRMWARE_URL_LEN (128)
#define MLINK_HTTPD_RESP_TIMEROUT_MS (15000)
#define MLINK_HTTPD_MAX_CONNECT      (CONFIG_LWIP_MAX_SOCKETS - 5)

/**
 * @brief The flag of http chunks
 */
enum {
    MLINK_HTTPD_CHUNKS_NONE = 0, /**< Invalid connection */
    MLINK_HTTPD_CHUNKS_DATA,     /**< Data only */
    MLINK_HTTPD_CHUNKS_HEADER,   /**< Add the header of http chunks */
    MLINK_HTTPD_CHUNKS_BODY,     /**< Add the body of http chunks */
    MLINK_HTTPD_CHUNKS_FOOTER,   /**< Add the body of http chunks */
};

/**
 * @brief Record the connected structure
 */
typedef struct {
    httpd_handle_t handle; /**< Every instance of the server will have a unique handle. */
    TimerHandle_t timer;   /**< Waiting for response timeout */
    uint16_t sockfd;       /**< Socket descriptor for sending data */
    uint16_t num;          /**< Number of destination addresses */
    uint8_t flag;          /**< The flag of http chunks */
} mlink_connection_t;

static const char *TAG                 = "mlink_httpd";
static httpd_handle_t g_httpd_handle   = NULL;
static QueueHandle_t g_mlink_queue     = NULL;
static mlink_connection_t *g_conn_list = NULL;

static void mlink_connection_remove(mlink_connection_t *mlink_conn);
static mlink_connection_t *mlink_connection_find(uint16_t sockfd);

static mdf_err_t mlink_get_mesh_info(httpd_req_t *req);
static esp_err_t mlink_device_request(httpd_req_t *req);
static esp_err_t mlink_ota_firmware(httpd_req_t *req);
static esp_err_t mlink_ota_url(httpd_req_t *req);
static mdf_err_t mlink_ota_stop(httpd_req_t *req);

static const httpd_uri_t basic_handlers[] = {
    {
        .uri      = "/mesh_info",
        .method   = HTTP_GET,
        .handler  = mlink_get_mesh_info,
        .user_ctx = NULL,
    },
    {
        .uri      = "/device_request",
        .method   = HTTP_POST,
        .handler  = mlink_device_request,
        .user_ctx = NULL,
    },
    {
        .uri      = "/ota/firmware",
        .method   = HTTP_POST,
        .handler  = mlink_ota_firmware,
        .user_ctx = NULL,
    },
    {
        .uri      = "/ota/url",
        .method   = HTTP_POST,
        .handler  = mlink_ota_url,
        .user_ctx = NULL,
    },
    {
        .uri      = "/ota/stop",
        .method   = HTTP_POST,
        .handler  = mlink_ota_stop,
        .user_ctx = NULL,
    }
};

static mdf_err_t mlink_socket_keepalive(int sockfd, int keep_idle, int keep_interval, int keep_count)
{
    MDF_PARAM_CHECK(sockfd != -1);

    esp_err_t ret = 0;
    int keep_alive = 1;

    ret = setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &keep_alive, sizeof(keep_alive));
    MDF_ERROR_CHECK(ret < 0, MDF_FAIL, "setsockopt SO_KEEPALIVE, ret: %d", ret);

    ret = setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPIDLE, &keep_idle, sizeof(keep_idle));
    MDF_ERROR_CHECK(ret < 0, MDF_FAIL, "setsockopt TCP_KEEPIDLE, ret: %d", ret);

    ret = setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPINTVL, &keep_interval, sizeof(keep_interval));
    MDF_ERROR_CHECK(ret < 0, MDF_FAIL, "setsockopt TCP_KEEPINTVL, ret: %d", ret);

    ret = setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPCNT, &keep_count, sizeof(keep_count));
    MDF_ERROR_CHECK(ret < 0, MDF_FAIL, "setsockopt TCP_KEEPCNT, ret: %d", ret);

    return ESP_OK;
}

static bool mlink_socket_iswritable(int sockfd)
{
    fd_set write_fds;
    struct timeval timeout = {0, 0};

    FD_ZERO(&write_fds);
    FD_SET(sockfd, &write_fds);

    if (select(sockfd + 1, NULL, &write_fds, NULL, &timeout) <= 0) {
        return false;
    }

    return true;
}

static int httpd_default_send(httpd_handle_t hd, int sockfd, const char *buf, size_t buf_len, int flags)
{
    MDF_PARAM_CHECK(buf);
    MDF_PARAM_CHECK(sockfd > 0);

    int ret = send(sockfd, buf, buf_len, flags);

    if (ret <= 0) {
        mlink_connection_t *mlink_conn = mlink_connection_find(sockfd);
        close(sockfd);
        mlink_connection_remove(mlink_conn);
        MDF_LOGW("socket send, err_str: %s, sockfd: %d, buf_len: %d",
                 strerror(errno), sockfd, buf_len);
    }

    return ret;
}

static void mlink_connection_remove(mlink_connection_t *mlink_conn)
{
    if (mlink_conn && mlink_conn->timer) {
        xTimerStop(mlink_conn->timer, 0);
        xTimerDelete(mlink_conn->timer, 0);
        memset(mlink_conn, 0, sizeof(mlink_connection_t));
    }
}

static void mlink_connection_timeout_cb(void *timer)
{
    char *chunk_footer             = "0\r\n\r\n";
    mlink_connection_t *mlink_conn = (mlink_connection_t *)pvTimerGetTimerID(timer);

    if (mlink_conn->timer == NULL || mlink_conn->flag == MLINK_HTTPD_CHUNKS_NONE) {
        return ;
    }

    if (mlink_conn->flag != MLINK_HTTPD_CHUNKS_BODY) {
        chunk_footer = "HTTP/1.1 400 Bad Request\r\n"
                       "Content-Type: application/json\r\n"
                       "Content-Length: 79\r\n\r\n"
                       "{\"status_code\":-1,\"status_msg\":\"Destination address error, No device response\"}";
    }

    mlink_conn->flag = MLINK_HTTPD_CHUNKS_DATA;

    if (mlink_socket_iswritable(mlink_conn->sockfd)) {
        if (httpd_default_send(mlink_conn->handle, mlink_conn->sockfd, chunk_footer, strlen(chunk_footer), 0) <= 0) {
            MDF_LOGW("<%s> httpd_default_send, sockfd: %d", strerror(errno), mlink_conn->sockfd);
        }

        MDF_LOGW("Mlink httpd response timeout, sockfd: %d, data: %s", mlink_conn->sockfd, chunk_footer);
    }

    close(mlink_conn->sockfd);
    mlink_connection_remove(mlink_conn);
}

static mlink_connection_t *mlink_connection_find(uint16_t sockfd)
{
    for (int i = 0; i < MLINK_HTTPD_MAX_CONNECT; ++i) {
        if (g_conn_list[i].sockfd == sockfd && g_conn_list[i].flag != MLINK_HTTPD_CHUNKS_NONE) {
            return g_conn_list + i;
        }
    }

    MDF_LOGW("Mlink chunks is no find, sockfd: %d", sockfd);
    return NULL;
}

static mdf_err_t mlink_connection_add(httpd_req_t *req, uint16_t chunks_num)
{
    for (int i = 0; i < MLINK_HTTPD_MAX_CONNECT; ++i) {
        if (g_conn_list[i].flag == MLINK_HTTPD_CHUNKS_NONE) {
            g_conn_list[i].num    = chunks_num;
            g_conn_list[i].flag   = (chunks_num > 1) ? MLINK_HTTPD_CHUNKS_HEADER : MLINK_HTTPD_CHUNKS_DATA;
            g_conn_list[i].handle = req->handle;
            g_conn_list[i].sockfd = httpd_req_to_sockfd(req);
            g_conn_list[i].timer  = xTimerCreate("chunk_timer", MLINK_HTTPD_RESP_TIMEROUT_MS / portTICK_RATE_MS,
                                                 false, g_conn_list + i, mlink_connection_timeout_cb);
            MDF_ERROR_CHECK(!g_conn_list[i].timer, MDF_FAIL, "xTimerCreate mlink_conn fail");
            xTimerStart(g_conn_list[i].timer, portMAX_DELAY);

            mlink_socket_keepalive(g_conn_list[i].sockfd, 10, 3, 3);
            return MDF_OK;
        }
    }

    MDF_LOGW("Mlink chunks add");
    return MDF_FAIL;
}

static mdf_err_t mlink_get_mesh_info(httpd_req_t *req)
{
    mdf_err_t ret           = MDF_ERR_NO_MEM;
    char *routing_table_str = NULL;
    const char *status_code = "{\"status_code\":0}";
    char routing_table_size_str[8] = {0};

    ssize_t routing_table_size = esp_mesh_get_routing_table_size();
    mesh_addr_t *routing_table = MDF_CALLOC(routing_table_size, sizeof(mesh_addr_t));
    MDF_ERROR_GOTO(!routing_table, EXIT, "");

    ret = esp_mesh_get_routing_table(routing_table, routing_table_size * sizeof(mesh_addr_t),
                                     &routing_table_size);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "esp_mesh_get_routing_table, ret: %d", ret);

    sprintf(routing_table_size_str, "%d", routing_table_size);
    routing_table_str = MDF_MALLOC(routing_table_size * 13);
    ret = MDF_ERR_NO_MEM;
    MDF_ERROR_GOTO(!routing_table_str, EXIT, "");

    char *tmp_ptr = routing_table_str;

    for (int i = 0; i < routing_table_size; i++) {
        tmp_ptr += sprintf(tmp_ptr, "%s%02x%02x%02x%02x%02x%02x",
                           (i == 0) ? "" : ",", MAC2STR(routing_table[i].addr));
    }

    MDF_FREE(routing_table);

    /**
     * @brief Set the HTTP status code
     */
    ret = httpd_resp_set_status(req, HTTPD_200);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "Set the HTTP status code");

    /**
     * @brief Set the HTTP content type
     */
    ret = httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "Set the HTTP content type");

    /**
     * @brief Set some custom headers
     */
    ret = httpd_resp_set_hdr(req, "Mesh-Node-Num", routing_table_size_str);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "Append any additional headers");
    ret = httpd_resp_set_hdr(req, "Mesh-Node-Mac", routing_table_str);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "Append any additional headers");

    ret = httpd_resp_send(req, status_code, strlen(status_code));
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "Send a complete HTTP response");

EXIT:
    MDF_FREE(routing_table);
    MDF_FREE(routing_table_str);

    if (ret != ESP_OK) {
        ret = httpd_resp_send_500(req);
        MDF_ERROR_CHECK(ret != MDF_OK, ret, "Helper function for HTTP 500");
    }

    return ret;
}

static ssize_t mlink_httpd_get_hdr(httpd_req_t *req, const char *field, char **value)
{
    size_t size = httpd_req_get_hdr_value_len(req, field) + 1;
    MDF_ERROR_CHECK(size <= 0, MDF_FAIL, "Search for %s in request headers", field);

    *value = MDF_CALLOC(1, size + 1);

    if (httpd_req_get_hdr_value_str(req, field, *value, size) != MDF_OK) {
        MDF_LOGD("Get the value string of %s from the request headers", field);
        MDF_FREE(*value);
        return MDF_FAIL;
    }

    MDF_LOGV("field: %s, value: %s", field, *value);

    return size;
}

static esp_err_t mlink_httpd_resp(httpd_req_t *req, const char *status_code, const char *message)
{
    char *data    = NULL;
    size_t size   = 0;
    mdf_err_t ret = MDF_FAIL;

    size = asprintf(&data, "{\"status_code\":-1,\"status_msg\":\"%s\"}", message);

    ret = httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "Set the HTTP content type");

    ret = httpd_resp_set_status(req, status_code);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "Set the HTTP status code");

    ret = httpd_resp_send(req, data, size);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "Send a complete HTTP response");

EXIT:
    MDF_FREE(data);
    return ret;
}

static esp_err_t mlink_httpd_resp_200(httpd_req_t *req)
{
    mdf_err_t ret           = MDF_FAIL;
    const char *status_code = "{\"status_code\":0,\"status_msg\":\"MDF_OK\"}";

    ret = httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "Set the HTTP content type");

    ret = httpd_resp_send(req, status_code, strlen(status_code));
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "Send a complete HTTP response");

    return MDF_OK;
}

static esp_err_t mlink_device_request(httpd_req_t *req)
{
    esp_err_t ret               = MDF_FAIL;
    char *httpd_hdr_value       = NULL;
    ssize_t httpd_hdr_value_len = 0;
    mlink_httpd_t *httpd_data   = MDF_CALLOC(1, sizeof(mlink_httpd_t));

    httpd_hdr_value_len = mlink_httpd_get_hdr(req, "Content-Type", &httpd_hdr_value);
    MDF_ERROR_GOTO(httpd_hdr_value_len <= 0, EXIT, "Get 'Content-Type' from the request headers");

    if (!strcasecmp(HTTPD_TYPE_JSON, httpd_hdr_value)) {
        httpd_data->type.format = MLINK_HTTPD_FORMAT_JSON;
    } else if (!strcasecmp(HTTPD_TYPE_TEXT, httpd_hdr_value)) {
        httpd_data->type.format = MLINK_HTTPD_FORMAT_HTML;
    } else {
        httpd_data->type.format = MLINK_HTTPD_FORMAT_HEX;
    }

    MDF_FREE(httpd_hdr_value);

    httpd_data->type.from = MLINK_HTTPD_FROM_SERVER;
    httpd_data->type.resp = true;

    if (mlink_httpd_get_hdr(req, "Root-Response", &httpd_hdr_value) > 0) {
        if ((!strcmp(httpd_hdr_value, "1") || !strcasecmp(httpd_hdr_value, "true"))) {
            httpd_data->type.resp = false;
        }
    }

    MDF_FREE(httpd_hdr_value);

    httpd_hdr_value_len = mlink_httpd_get_hdr(req, "Mesh-Node-Group", &httpd_hdr_value);

    if (httpd_hdr_value_len > 0) {
        httpd_data->group = true;
    } else {
        httpd_hdr_value_len = mlink_httpd_get_hdr(req, "Mesh-Node-Mac", &httpd_hdr_value);

        if (httpd_hdr_value_len <= 0) {
            MDF_LOGW("Get 'Mesh-Node-Mac' from the request headers");
            mlink_httpd_resp(req, HTTPD_400, "Must contain the 'Mesh-Node-Mac' field");
            goto EXIT;
        }
    }

    httpd_data->addrs_list = MDF_MALLOC(httpd_hdr_value_len / 2);
    MDF_ERROR_CHECK(!httpd_data->addrs_list, MDF_ERR_NO_MEM, "");

    for (char *tmp = httpd_hdr_value;; tmp++) {
        if (*tmp == ',' || *tmp == '\0') {
            mlink_mac_str2hex(tmp - 12, httpd_data->addrs_list + (httpd_data->addrs_num * MWIFI_ADDR_LEN));
            httpd_data->addrs_num++;

            if (*tmp == '\0') {
                break;
            }
        }
    }

    MDF_FREE(httpd_hdr_value);
    MDF_LOGD("dest_addrs_num: %d", httpd_data->addrs_num);

    if (httpd_data->addrs_num == 0) {
        MDF_LOGW("Destination address format error");
        mlink_httpd_resp(req, HTTPD_400, "Destination address format error");
        goto EXIT;
    }

    httpd_data->size = req->content_len;
    httpd_data->data = MDF_MALLOC(req->content_len);
    MDF_ERROR_CHECK(!httpd_data->data, MDF_ERR_NO_MEM, "");

    for (int i = 0, recv_size = 0; i < 5 && recv_size < req->content_len; ++i, recv_size += ret) {
        ret = httpd_req_recv(req, httpd_data->data + recv_size, req->content_len - recv_size);
        MDF_ERROR_CONTINUE(ret == HTTPD_SOCK_ERR_TIMEOUT, "<HTTPD_SOCK_ERR_TIMEOUT> Read content data from the HTTP request");
        MDF_ERROR_GOTO(ret <= 0, EXIT, "<%s> Read content data from the HTTP request",
                       mdf_err_to_name(ret));
    }

    if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
        ret = httpd_resp_send_408(req);
        MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "Helper function for HTTP 408");
    }

    if (!httpd_data->type.resp) {
        mlink_httpd_resp_200(req);
    } else {
        httpd_data->type.sockfd = httpd_req_to_sockfd(req);

        if (httpd_data->addrs_num == 1
                && (MWIFI_ADDR_IS_ANY(httpd_data->addrs_list)
                    || MWIFI_ADDR_IS_BROADCAST(httpd_data->addrs_list))) {
            mlink_connection_add(req, esp_mesh_get_routing_table_size());
        } else {
            mlink_connection_add(req, httpd_data->addrs_num);
        }
    }

    /**< If g_mlink_queue is full, delete the front item */
    mlink_httpd_t *q_data = NULL;

    if (!uxQueueSpacesAvailable(g_mlink_queue)
            && xQueueReceive(g_mlink_queue, &q_data, 0)) {
        MDF_LOGW("Mlink data queue is full, delete the front item");
        MDF_FREE(q_data->addrs_list);
        MDF_FREE(q_data->data);
        MDF_FREE(q_data);
    }

    if (xQueueSend(g_mlink_queue, &httpd_data, 0) == pdFALSE) {
        MDF_LOGW("xQueueSend failed");
        ret = ESP_FAIL;

        ret = httpd_resp_send_500(req);
        MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "Helper function for HTTP 408");

        goto EXIT;
    }

    return ESP_OK;

EXIT:

    if (httpd_data) {
        MDF_FREE(httpd_data->addrs_list);
        MDF_FREE(httpd_data->data);
        MDF_FREE(httpd_data);
    }

    MDF_FREE(httpd_hdr_value);
    return ret;
}

static void mlink_ota_send_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    uint32_t start_time = xTaskGetTickCount();
    mlink_httpd_t *mlink_httpd = (mlink_httpd_t *)arg;

    ret = mupgrade_firmware_send(mlink_httpd->addrs_list, mlink_httpd->addrs_num, NULL);

    if (ret != MDF_OK) {
        MDF_LOGW("<%s> Root sends firmware to other nodes", mdf_err_to_name(ret));
    } else {
        MDF_LOGI("Firmware is sent to the device to complete, Spend time: %ds",
                 (xTaskGetTickCount() - start_time) * portTICK_RATE_MS / 1000);
    }

    MDF_FREE(mlink_httpd->addrs_list);
    MDF_FREE(mlink_httpd);
    vTaskDelete(NULL);
}

static esp_err_t mlink_ota_firmware(httpd_req_t *req)
{
    mdf_err_t ret               = MDF_FAIL;
    char firmware_name[32]      = {0x0};
    int start_time              = xTaskGetTickCount();
    char *buf                   = NULL;
    size_t firmware_size        = req->content_len;
    uint8_t *addrs_list         = NULL;
    size_t addrs_num            = 0;
    char *httpd_hdr_value       = NULL;
    ssize_t httpd_hdr_value_len = 0;

    if (!httpd_req_get_hdr_value_len(req, "Firmware-Name")
            || httpd_req_get_hdr_value_str(req, "Firmware-Name", firmware_name, sizeof(firmware_name)) != MDF_OK) {
        MDF_LOGW("Get 'Firmware-Name' from the request headers");
        mlink_httpd_resp(req, HTTPD_400, "Request headers lost Firmware-Name");
        goto EXIT;
    }

    httpd_hdr_value_len = mlink_httpd_get_hdr(req, "Mesh-Node-Mac", &httpd_hdr_value);
    MDF_LOGV("httpd_hdr_value_len: %d", httpd_hdr_value_len);

    if (httpd_hdr_value_len < 13) {
        MDF_LOGW("Get 'Mesh-Node-Mac' from the request headers");
        mlink_httpd_resp(req, HTTPD_400, "Must contain the 'Mesh-Node-Mac' field");
        goto EXIT;
    }

    addrs_list = MDF_REALLOC_RETRY(NULL, httpd_hdr_value_len / 2);

    for (char *tmp = httpd_hdr_value;; tmp++) {
        if (*tmp == ',' || *tmp == '\0') {
            mlink_mac_str2hex(tmp - 12, addrs_list + addrs_num * MWIFI_ADDR_LEN);
            addrs_num++;

            if (*tmp == '\0') {
                break;
            }
        }
    }

    MDF_LOGD("dest_addrs_num: %d, httpd_data: %s", addrs_num, httpd_hdr_value);
    MDF_FREE(httpd_hdr_value);

    ret = mupgrade_firmware_init(firmware_name, firmware_size);

    if (ret != MDF_OK) {
        MDF_LOGW("<%s> Initialize the upgrade status", mdf_err_to_name(ret));
        mlink_httpd_resp(req, HTTPD_500, esp_err_to_name(ret));
        goto EXIT;
    }

    buf = MDF_REALLOC_RETRY(NULL, MUPGRADE_PACKET_MAX_SIZE);

    for (int i = 10; i > 0 && firmware_size > 0; --i) {
        ret = httpd_req_recv(req, buf, MIN(firmware_size, MUPGRADE_PACKET_MAX_SIZE));
        MDF_ERROR_CONTINUE(ret == HTTPD_SOCK_ERR_TIMEOUT, "<HTTPD_SOCK_ERR_TIMEOUT> Read content data from the HTTP request");
        MDF_ERROR_GOTO(ret < 0, EXIT, "<%s> Read content data from the HTTP request",
                       mdf_err_to_name(ret));
        i = 10;

        firmware_size -= ret;
        ret = mupgrade_firmware_download(buf, ret);

        if (ret == MDF_ERR_MUPGRADE_FIRMWARE_INVALID) {
            mlink_httpd_resp(req, HTTPD_400, "Non-project generated firmware");
        } else if (ret != MDF_OK) {
            mlink_httpd_resp(req, HTTPD_500, mdf_err_to_name(ret));
        }

        if (ret != MDF_OK) {
            MDF_LOGW("<%s> Write firmware to flash", mdf_err_to_name(ret));
            mlink_httpd_resp(req, HTTPD_400, mdf_err_to_name(ret));
            goto EXIT;
        }
    }

    MDF_LOGI("Firmware is sent to the device to complete, Spend time: %ds",
             (xTaskGetTickCount() - start_time) * portTICK_RATE_MS / 1000);
    start_time = xTaskGetTickCount();

    mlink_httpd_resp_200(req);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "Helper function for HTTP 200");

    mlink_httpd_t *mlink_httpd = MDF_REALLOC_RETRY(NULL, sizeof(mlink_httpd_t));
    memset(mlink_httpd, 0, sizeof(mlink_httpd_t));
    mlink_httpd->addrs_list = addrs_list;
    mlink_httpd->addrs_num  = addrs_num;
    addrs_list = NULL;
    xTaskCreatePinnedToCore(mlink_ota_send_task, "mlink_ota_send", 4 * 1024,
                            mlink_httpd, CONFIG_MDF_TASK_DEFAULT_PRIOTY,
                            NULL, CONFIG_MDF_TASK_PINNED_TO_CORE);

EXIT:

    MDF_FREE(buf);
    MDF_FREE(addrs_list);
    MDF_FREE(httpd_hdr_value);
    return ret;
}

static esp_err_t mlink_ota_url(httpd_req_t *req)
{
    mdf_err_t ret               = MDF_FAIL;
    char *firmware_name         = NULL;
    int start_time              = xTaskGetTickCount();
    char *buf                   = NULL;
    size_t firmware_size        = 0;
    uint8_t *addrs_list         = NULL;
    size_t addrs_num            = 0;
    char *httpd_hdr_value       = NULL;
    ssize_t httpd_hdr_value_len = 0;
    char *firmware_url          = MDF_REALLOC_RETRY(NULL, MLINK_HTTPD_FIRMWARE_URL_LEN);
    esp_http_client_handle_t http_client_handle = NULL;
    esp_http_client_config_t http_client_config = {
        .url            = firmware_url,
        .transport_type = HTTP_TRANSPORT_UNKNOWN,
    };

    if (!httpd_req_get_hdr_value_len(req, "Firmware-Url")
            || httpd_req_get_hdr_value_str(req, "Firmware-Url", firmware_url, MLINK_HTTPD_FIRMWARE_URL_LEN) != MDF_OK) {
        MDF_LOGW("Get 'Firmware-Url' from the request headers");
        mlink_httpd_resp(req, HTTPD_400, "Request headers lost Firmware-Url");
        MDF_FREE(firmware_url);
        goto EXIT;
    }

    httpd_hdr_value_len = mlink_httpd_get_hdr(req, "Mesh-Node-Mac", &httpd_hdr_value);
    MDF_LOGV("httpd_hdr_value_len: %d", httpd_hdr_value_len);

    if (httpd_hdr_value_len < 13) {
        MDF_LOGW("Get 'Mesh-Node-Mac' from the request headers");
        mlink_httpd_resp(req, HTTPD_400, "Must contain the 'Mesh-Node-Mac' field");
        goto EXIT;
    }

    addrs_list = MDF_REALLOC_RETRY(NULL, httpd_hdr_value_len / 2);

    for (char *tmp = httpd_hdr_value;; tmp++) {
        if (*tmp == ',' || *tmp == '\0') {
            mlink_mac_str2hex(tmp - 12, addrs_list + addrs_num * MWIFI_ADDR_LEN);
            addrs_num++;

            if (*tmp == '\0') {
                break;
            }
        }
    }

    MDF_LOGD("dest_addrs_num: %d, httpd_data: %s", addrs_num, httpd_hdr_value);
    MDF_FREE(httpd_hdr_value);

    http_client_handle = esp_http_client_init(&http_client_config);

    if (!http_client_handle) {
        MDF_LOGW("Initialise HTTP connection, URL: %s", firmware_url);
        mlink_httpd_resp(req, HTTPD_400, "Initialise HTTP connection");
        goto EXIT;
    }

    start_time = xTaskGetTickCount();

    MDF_LOGI("Open HTTP connection: %s", firmware_url);

    ret = esp_http_client_open(http_client_handle, 0);

    if (ret != MDF_OK) {
        MDF_LOGW("<%s> Connection service failed", mdf_err_to_name(ret));
        mlink_httpd_resp(req, HTTPD_500, "Connection service failed");
        goto EXIT;
    }

    firmware_size = esp_http_client_fetch_headers(http_client_handle);

    if (ret != MDF_OK) {
        MDF_LOGW("<%s> Download data length defined by content-length header", mdf_err_to_name(ret));
        mlink_httpd_resp(req, HTTPD_400, "stream doesn't contain content-length header");
        goto EXIT;
    }

    firmware_name = strrchr(firmware_url, '/') + 1;
    firmware_name[strlen(firmware_name) - 3] = '\0';

    MDF_LOGD("firmware, name: %s, size: %d", firmware_name, firmware_size);

    ret = mupgrade_firmware_init(firmware_name, firmware_size);

    if (ret != MDF_OK) {
        MDF_LOGW("<%s> Initialize the upgrade status", mdf_err_to_name(ret));
        mlink_httpd_resp(req, HTTPD_500, "Initialize the upgrade status");
        goto EXIT;
    }

    buf = MDF_REALLOC_RETRY(NULL, MUPGRADE_PACKET_MAX_SIZE);

    while (firmware_size > 0) {
        ssize_t size = esp_http_client_read(http_client_handle, buf, MIN(firmware_size, MUPGRADE_PACKET_MAX_SIZE));
        MDF_ERROR_GOTO(size < 0, EXIT, "<%s> Read data from http stream", mdf_err_to_name(ret));

        firmware_size -= size;
        ret = mupgrade_firmware_download(buf, size);
        MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "<%s> Write firmware to flash", mdf_err_to_name(ret));
    }

    MDF_LOGI("The service download firmware is complete, Spend time: %ds",
             (xTaskGetTickCount() - start_time) * portTICK_RATE_MS / 1000);

    mlink_httpd_resp_200(req);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "Helper function for HTTP 200");

    mlink_httpd_t *mlink_httpd = MDF_REALLOC_RETRY(NULL, sizeof(mlink_httpd_t));
    memset(mlink_httpd, 0, sizeof(mlink_httpd_t));
    mlink_httpd->addrs_list = addrs_list;
    mlink_httpd->addrs_num  = addrs_num;
    addrs_list = NULL;
    xTaskCreatePinnedToCore(mlink_ota_send_task, "mlink_ota_send", 4 * 1024,
                            mlink_httpd, CONFIG_MDF_TASK_DEFAULT_PRIOTY,
                            NULL, CONFIG_MDF_TASK_PINNED_TO_CORE);

EXIT:

    if (http_client_handle) {
        esp_http_client_close(http_client_handle);
        esp_http_client_cleanup(http_client_handle);
    }

    MDF_FREE(buf);
    MDF_FREE(addrs_list);
    MDF_FREE(firmware_url);
    MDF_FREE(httpd_hdr_value);
    return ret;
}

static mdf_err_t mlink_ota_stop(httpd_req_t *req)
{
    mdf_err_t ret = mupgrade_firmware_stop();

    if (ret != ESP_OK) {
        mlink_httpd_resp(req, HTTPD_400, "No device is upgrading");
    } else {
        mlink_httpd_resp_200(req);
    }

    MDF_ERROR_CHECK(ret != MDF_OK, ret, "mupgrade_firmware_stop");

    return MDF_OK;
}

static mdf_err_t mlink_httpd_resp_set_status(char **resp, const char *status)
{
    *resp = NULL;
    asprintf(resp, "HTTP/1.1 %s\r\n", status);

    return ESP_OK;
}

static mdf_err_t mlink_httpd_resp_set_hdr(char **resp, const char *field, const char *value)
{
    char *tmp_str   = NULL;
    size_t tmp_size = asprintf(&tmp_str, "%s: %s\r\n", field, value);

    *resp = MDF_REALLOC_RETRY(*resp, strlen(*resp) + tmp_size + 1);
    memcpy(*resp + strlen(*resp), tmp_str, tmp_size + 1);

    MDF_FREE(tmp_str);

    return ESP_OK;
}

static size_t mlink_httpd_resp_set_data(char **resp, const char *data, size_t size)
{
    MDF_PARAM_CHECK(resp);

    char *tmp_str     = NULL;
    size_t tmp_size   = asprintf(&tmp_str, "Content-Length: %d\r\n\r\n", size);
    size_t total_size = strlen(*resp) + tmp_size + size;

    *resp = MDF_REALLOC_RETRY(*resp, total_size + 8);
    memcpy(*resp + strlen(*resp), tmp_str, tmp_size + 1);

    if (data && size) {
        memcpy(*resp + strlen(*resp), data, size);
    }

    MDF_FREE(tmp_str);
    return total_size;
}

mdf_err_t mlink_httpd_write(const mlink_httpd_t *response, TickType_t wait_ticks)
{
    MDF_PARAM_CHECK(response);
    MDF_ERROR_CHECK(!g_httpd_handle, MDF_ERR_NOT_INIT, "mlink_httpd is stop");

    mdf_err_t ret    = MDF_FAIL;
    char mac_str[13] = {0};
    size_t resp_size = 0;
    char *resp_data  = NULL;

    /**
      * @brief For sending out data in response to an HTTP request.
      */
    mlink_connection_t *mlink_conn = mlink_connection_find(response->type.sockfd);
    MDF_ERROR_CHECK(mlink_conn == NULL, MDF_FAIL, "mlink_connection_find");

    /**
     * @brief Generate a packet for the http response
     */
    mlink_httpd_resp_set_status(&resp_data, response->type.resp == true ? HTTPD_200 : HTTPD_400);
    mlink_httpd_resp_set_hdr(&resp_data, "Content-Type",
                             response->type.format == MESH_PROTO_JSON ? HTTPD_TYPE_JSON : "application/bin");
    mlink_httpd_resp_set_hdr(&resp_data, "Mesh-Node-Mac", mlink_mac_hex2str(response->addrs_list, mac_str));
    resp_size = mlink_httpd_resp_set_data(&resp_data, response->data, response->size);

    if (wait_ticks != portMAX_DELAY) {
        int send_timeout_ms = wait_ticks * portTICK_PERIOD_MS;
        struct timeval timeout = {
            .tv_usec = (send_timeout_ms % 1000) * 1000,
            .tv_sec = send_timeout_ms / 1000,
        };

        ret = setsockopt(mlink_conn->sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        MDF_ERROR_GOTO(ret < 0, EXIT, "<%s> Set send timeout", strerror(errno));
    }

    if (mlink_conn->flag == MLINK_HTTPD_CHUNKS_HEADER) {
        char *chunk_header =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/http\r\n"
            "Transfer-Encoding: chunked\r\n\r\n";

        MDF_LOGD("send chunk_header, sockfd: %d, chunk_header: %s", mlink_conn->sockfd, chunk_header);
        ret = httpd_default_send(mlink_conn->handle, mlink_conn->sockfd, chunk_header, strlen(chunk_header), 0);
        MDF_ERROR_GOTO(ret <= 0, EXIT, "This is the low level default send function of the HTTPD");

        mlink_conn->flag = MLINK_HTTPD_CHUNKS_BODY;
    }

    if (mlink_conn->flag == MLINK_HTTPD_CHUNKS_BODY) {
        char chunk_size[16] = {0};
        sprintf(chunk_size, "%x\r\n", resp_size);

        if (mlink_conn->timer) {
            xTimerReset(mlink_conn->timer, portMAX_DELAY);
        }

        MDF_LOGD("send chunk_size, sockfd: %d, data: %s", mlink_conn->sockfd, chunk_size);
        ret = httpd_default_send(mlink_conn->handle, mlink_conn->sockfd, chunk_size, strlen(chunk_size), 0);
        MDF_ERROR_GOTO(ret <= 0, EXIT, "<%s> This is the low level default send function of the HTTPD", strerror(errno));

        resp_data[resp_size++] = '\r';
        resp_data[resp_size++] = '\n';
    }

    resp_data[resp_size] = '\0';

    MDF_LOGD("size: %d, resp_data: %.*s", resp_size, resp_size, resp_data);
    ret = httpd_default_send(mlink_conn->handle, mlink_conn->sockfd, resp_data, resp_size, 0);
    MDF_ERROR_GOTO(ret <= 0, EXIT, "<%s> httpd_default_send, sockfd: %d",
                   strerror(errno), mlink_conn->sockfd);

    mlink_conn->num--;
    MDF_LOGD("mlink_conn->num: %d", mlink_conn->num);

    if (mlink_conn->num > 0) {
        ret = MDF_OK;
        goto EXIT;
    }

    if (mlink_conn->flag == MLINK_HTTPD_CHUNKS_BODY) {
        char *chunk_footer = "0\r\n\r\n";

        MDF_LOGD("send chunk_footer, sockfd: %d, data: %s", mlink_conn->sockfd, chunk_footer);
        ret = httpd_default_send(mlink_conn->handle, mlink_conn->sockfd, chunk_footer, strlen(chunk_footer), 0);
        MDF_ERROR_GOTO(ret <= 0, EXIT, "This is the low level default send function of the HTTPD");
    }

    mlink_connection_remove(mlink_conn);
    ret = MDF_OK;

EXIT:
    MDF_FREE(resp_data);
    return ret;
}

mdf_err_t mlink_httpd_read(mlink_httpd_t **request, TickType_t wait_ticks)
{
    MDF_PARAM_CHECK(request);

    mdf_err_t ret = MDF_OK;

    if (!g_mlink_queue) {
        g_mlink_queue = xQueueCreate(3, sizeof(mlink_httpd_t *));
    }

    ret = xQueueReceive(g_mlink_queue, request, wait_ticks);

    if (!*request) {
        mlink_httpd_t *q_data = NULL;

        while (xQueueReceive(g_mlink_queue, &q_data, 0)) {
            if (q_data) {
                MDF_FREE(q_data->addrs_list);
                MDF_FREE(q_data->data);
                MDF_FREE(q_data);
            }
        }

        vQueueDelete(g_mlink_queue);
        g_mlink_queue = NULL;

        MDF_LOGW("MLINK HTTPD EXIT");
        return MDF_ERR_NOT_SUPPORTED;
    }

    MDF_ERROR_CHECK(ret != true, MDF_ERR_TIMEOUT, "xQueueSend failed");

    return MDF_OK;
}

mdf_err_t mlink_httpd_start(void)
{
    mdf_err_t ret         = MDF_OK;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /**
     * @brief This check should be a part of http_server
     */
    config.max_open_sockets = (MLINK_HTTPD_MAX_CONNECT);

    MDF_LOGI("Starting server");
    MDF_LOGD("HTTP port         : %d", config.server_port);
    MDF_LOGD("Max URI handlers  : %d", config.max_uri_handlers);
    MDF_LOGD("Max open sessions : %d", config.max_open_sockets);
    MDF_LOGD("Max header length : %d", HTTPD_MAX_REQ_HDR_LEN);
    MDF_LOGD("Max URI length    : %d", HTTPD_MAX_URI_LEN);
    MDF_LOGD("Max stack size    : %d", config.stack_size);

    if (!g_mlink_queue) {
        g_mlink_queue = xQueueCreate(3, sizeof(mlink_httpd_t *));
    }

    if (!g_conn_list) {
        g_conn_list = MDF_CALLOC(MLINK_HTTPD_MAX_CONNECT, sizeof(mlink_connection_t));
        MDF_ERROR_CHECK(!g_conn_list, MDF_ERR_NO_MEM, "");
    }

    ret = httpd_start(&g_httpd_handle, &config);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "Starts the web server");

    for (int i = 0, handlers_no = sizeof(basic_handlers) / sizeof(httpd_uri_t);
            i < handlers_no; i++) {
        ret = httpd_register_uri_handler(g_httpd_handle, basic_handlers + i);
        MDF_ERROR_CONTINUE(ret != ESP_OK, "Register uri failed for %d", i);
    }

    return MDF_OK;
}

mdf_err_t mlink_httpd_stop(void)
{
    if (!g_httpd_handle) {
        return MDF_OK;
    }

    mdf_err_t ret = MDF_OK;
    void *mlink_queue_exit = NULL;

    ret = httpd_stop(g_httpd_handle);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "Stops the web server");
    g_httpd_handle = NULL;
    xQueueSend(g_mlink_queue, &mlink_queue_exit, 0);

    if (g_conn_list) {
        for (int i = 0; i < MLINK_HTTPD_MAX_CONNECT; ++i) {
            mlink_connection_remove(g_conn_list + i);
        }

        MDF_FREE(g_conn_list);
    }

    return MDF_OK;
}
