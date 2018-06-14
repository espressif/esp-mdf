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

#include <string.h>

#include "mdf_http_handle.h"
#include "mdf_network_config.h"
#include "mdf_http.h"
#include "mdf_socket.h"
#include "mdf_openssl.h"

typedef esp_err_t (*http_request_func_t)(server_http_connect_t *conn);

typedef struct {
    char *url;                  /**< http link */
    http_request_func_t func;   /**< http processing function */
} server_http_reqest_handle_t;

static const char *TAG = "mdf_http_server_parser";
static const char *proto_str[] = {
    "application/json",
    "application/bin",
    "application/mqtt",
    "application/http",
    "application/ota_bin",
    NULL,
};

static esp_err_t mdf_root_ota_init(server_http_connect_t *conn);
static esp_err_t mdf_get_device_mac(server_http_connect_t *conn);
static esp_err_t mdf_device_request(server_http_connect_t *conn);

const static server_http_reqest_handle_t g_reqest_handle_list[] = {
    {"/mesh_ota", mdf_root_ota_init},
    {"/mesh_info", mdf_get_device_mac},
    {"/device_request", mdf_device_request},
    {NULL, NULL},
};

static esp_err_t mdf_http_header_parse(server_http_connect_t *conn)
{
    MDF_PARAM_CHECK(conn);
    MDF_PARAM_CHECK(conn->http_head);

    esp_err_t ret          = 0;
    size_t mac_str_len     = 0;
    char *mdf_node_mac_str = NULL;

    mac_str_len = mdf_http_get_header(conn->http_head, "Mesh-Node-Mac", &mdf_node_mac_str);
    MDF_ERROR_CHECK(mac_str_len <= 0, ESP_FAIL,
                    "mdf_http_get_header Mesh-Node-Mac, mac_str_len: %d", mac_str_len);

    conn->dest_addrs = mdf_calloc(1, mac_str_len / 2);

    for (char *tmp = mdf_node_mac_str; *tmp != '\r'; tmp++) {
        if (*tmp == ',') {
            str2mac(mdf_node_mac_str, conn->dest_addrs[conn->dest_addrs_num++].mac);
            mdf_node_mac_str = tmp + 1;
        }
    }

    str2mac(mdf_node_mac_str, conn->dest_addrs[conn->dest_addrs_num++].mac);
    MDF_LOGV("dest_addrs_num: %d", conn->dest_addrs_num);

    conn->http_body = mdf_http_get_body(conn->http_head, &conn->http_body_size);

    conn->type.to_server = true;
    conn->type.proto     = -1;

    /**< get the data protocol type */
    char content_type[64] = {0};
    ret = mdf_http_get_header(conn->http_head, "Content-Type", content_type);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_http_get_header Content-Type, ret: %d", ret);

    for (int i = 0; proto_str[i] != NULL; ++i) {
        if (!strcmp(proto_str[i], content_type)) {
            conn->type.proto = i;
            break;
        }
    }

    MDF_LOGV("proto, str: %s, hex: %d", content_type, conn->type.proto);
    MDF_ERROR_CHECK(conn->type.proto == -1, ESP_FAIL,
                    "content-type: %s is not support", content_type);

    bool no_response = false;
    mdf_http_get_header(conn->http_head, "Root-Response", &no_response);
    conn->type.no_response = no_response;

    return ESP_OK;
}

static esp_err_t mdf_device_request(server_http_connect_t *conn)
{
    esp_err_t ret = ESP_OK;

    ret = mdf_http_header_parse(conn);
    MDF_ERROR_GOTO(ret < 0, EXIT, "mdf_http_header_parse, ret: %d", ret);
    MDF_ERROR_GOTO(conn->http_body_size <= 0 || !conn->http_body, EXIT,
                   "mdf_http_header_parse, conn->http_head: %s", conn->http_head);

    if (conn->type.no_response) {
        char *response_buf = conn->http_head;
        ret = mdf_http_set_response_status(response_buf, 200);
        ssize_t response_size = mdf_http_set_body(response_buf, NULL, 0);

        MDF_LOGD("http response, sockfd: %d, size: %d, data: \n%s",
                 conn->sockfd, response_size, response_buf);

        ret = mdf_server_conn_send(conn, response_buf, response_size);
        MDF_ERROR_GOTO(ret <= 0, EXIT, "socket send, ret: %d", ret);
    }

    conn->chunk     = conn->dest_addrs_num > 1 ? MDF_HTTP_CHUNK : MDF_HTTP_CHUNK_NONE;
    conn->chunk_num = conn->dest_addrs_num > 1 ? conn->dest_addrs_num : 0;

    for (int i = 0; i < conn->dest_addrs_num; ++i) {
        ret = mdf_wifi_mesh_root_send(&conn->src_addr, conn->dest_addrs + i, &conn->type,
                                      conn->http_body, conn->http_body_size);

        if (ret <= 0) {
            conn->chunk_num--;
            MDF_LOGW("mdf_wifi_mesh_root_send, ret: %d, fail: %d",
                     ret, (conn->dest_addrs_num - conn->chunk_num));
        }
    }

    MDF_LOGD("mdf_wifi_mesh_root_send conn->chunk: %d, conn->chunk_num: %d",
             conn->chunk, conn->chunk_num);

EXIT:
    mdf_free(conn->dest_addrs);
    return ret < 0 ? ESP_FAIL : ESP_OK;
}

static esp_err_t mdf_get_device_mac(server_http_connect_t *conn)
{
    esp_err_t ret          = ESP_OK;
    ssize_t response_size  = 0;
    char *response         = conn->http_head;
    char *mdf_note_mac     = conn->http_head;
    int mdf_node_num       = 0;
    char *mdf_note_mac_str = NULL;

    ret = esp_mesh_get_routing_table((mesh_addr_t *)mdf_note_mac, MDF_CONNECT_BUF_SIZE, &mdf_node_num);
    MDF_ERROR_GOTO(ret < 0, EXIT, "esp_mesh_get_routing_table, ret: %d", ret);

    mdf_note_mac_str = mdf_malloc(mdf_node_num * 13);
    char *tmp_ptr = mdf_note_mac_str;

    for (int i = 0; i < mdf_node_num; i++, mdf_note_mac += 6) {
        tmp_ptr += sprintf(tmp_ptr, "%s%2x%02x%02x%02x%02x%02x",
                           (i == 0) ? "" : ",", MAC2STR(mdf_note_mac));
    }

    ret = mdf_http_set_response_status(response, 200);
    MDF_ERROR_GOTO(ret < 0, EXIT, "mdf_http_set_response_status, ret: %d", ret);

    ret = mdf_http_set_header(response, "Mesh-Node-Num", mdf_node_num);
    MDF_ERROR_GOTO(ret < 0, EXIT, "mdf_http_set_header, ret: %d", ret);

    ret = mdf_http_set_header(response, "Mesh-Node-Mac", mdf_note_mac_str);
    MDF_ERROR_GOTO(ret < 0, EXIT, "mdf_http_set_header, ret: %d", ret);

    response_size = mdf_http_set_body(response, NULL, 0);
    MDF_ERROR_GOTO(response_size <= 0, EXIT, "mdf_http_set_body, ret: %d", ret);

    MDF_LOGD("http response, size: %d, data: \n%s", response_size, response);

    ret = mdf_server_conn_send(conn, response, response_size);
    MDF_ERROR_GOTO(ret <= 0, EXIT, "socket send, ret: %d", ret);

EXIT:
    mdf_free(mdf_note_mac_str);
    return ret < 0 ? ESP_FAIL : ESP_OK;
}

static esp_err_t mdf_root_ota_init(server_http_connect_t *conn)
{
    esp_err_t ret                = ESP_FAIL;
    ssize_t response_size        = 0;
    int response_status          = 400;
    char ota_addr_str[16]        = {0};
    size_t packet_size           = 0;
    server_http_connect_t *ota_conn = NULL;
    wifi_mesh_addr_t ota_conn_addr = {0};

    ret = mdf_http_header_parse(conn);
    MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "mdf_http_header_parse, ret: %d", ret);

    ret = mdf_http_get_header(conn->http_head, "Mesh-Ota-Length", &packet_size);
    MDF_ERROR_GOTO(ret < 0 || packet_size <= 0 || packet_size > WIFI_MESH_PACKET_MAX_SIZE,
                   ERR_EXIT, "Mesh-Ota-Length: %d", packet_size);

    ret = mdf_http_get_header(conn->http_head, "Mesh-Ota-Address", ota_addr_str);
    MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "mdf_http_get_header, ret: %d", ret);

    str2mac(ota_addr_str, (uint8_t *)&ota_conn_addr);
    ota_conn_addr.port = ntohs(ota_conn_addr.port);

    ota_conn = mdf_server_conn_find(&ota_conn_addr);
    MDF_ERROR_GOTO(!ota_conn, ERR_EXIT, "mdf_server_conn_find, ip: %s, port: %d",
                   inet_ntoa(ota_conn_addr.ip), ntohs(ota_conn_addr.port));

    memcpy(&ota_conn->type, &conn->type, sizeof(wifi_mesh_data_type_t));
    ota_conn->type.ota       = true;
    ota_conn->type.proto     = 1;
    ota_conn->dest_addrs_num = conn->dest_addrs_num;
    ota_conn->dest_addrs     = conn->dest_addrs;
    conn->dest_addrs         = NULL;
    ota_conn->ota_packet_size = packet_size;

    response_status =  200;

ERR_EXIT:

    do {
        char *response_buf = conn->http_head;

        ret = mdf_http_set_response_status(response_buf, response_status);
        MDF_ERROR_BREAK(ret < 0, "mdf_http_set_response_status, ret: %d", ret);

        response_size = mdf_http_set_body(response_buf, NULL, 0);
        MDF_ERROR_BREAK(ret < 0, "mdf_http_set_body, ret: %d", ret);

        MDF_LOGD("http response, sockfd: %d, size: %d, data: \n%s",
                 conn->sockfd, response_size, response_buf);

        ret = mdf_server_conn_send(conn, response_buf, response_size);
        MDF_ERROR_BREAK(ret < 0, "mdf_server_conn_send, ret: %d", ret);
    } while (0);

    return ret;
}

esp_err_t mdf_server_request_parser(server_http_connect_t *conn)
{
    MDF_PARAM_CHECK(conn);
    MDF_PARAM_CHECK(conn->http_head);

    esp_err_t ret       = 0;
    bool is_http_header = strstr(conn->http_head, "HTTP") ? true : false;
    MDF_ERROR_CHECK(!is_http_header, ESP_FAIL, "conn->size: %d", conn->http_size);

    if (is_http_header) {
#ifdef  CONFIG_MDF_USE_HTTP_TOKEN

        do {
            char *token                          = NULL;
            char *response_buf                   = conn->http_head;
            size_t response_size                 = 0;
            network_config_t network_config = {0};

            ret = mdf_network_get_config(&network_config);
            MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_network_get_config, ret: %d", ret);

            ret = mdf_http_get_header(conn->http_head, "token", &token);
            MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_http_get_header, ret: %d", ret);

            if (!token || memcmp(network_config.token, token, MDF_TOKEN_LEN - 1)) {
                MDF_LOGW("token is error, token: %p, token: %s", token, network_config.token);

                ret = mdf_http_set_response_status(response_buf, 400);
                response_size = mdf_http_set_body(response_buf, NULL, 0);

                MDF_LOGD("http response, sockfd: %d, size: %d, data: \n%s",
                         conn->sockfd, response_size, response_buf);

                ret = mdf_server_conn_send(conn, response_buf, response_size);
                MDF_ERROR_BREAK(ret < 0, "mdf_server_conn_send, ret: %d", ret);

                return ESP_FAIL;
            }
        } while (0);

#endif
        char *http_url = mdf_http_get_request_line(conn->http_head);
        MDF_ERROR_CHECK(!http_url, ESP_FAIL , "http_url: %p", http_url);

        for (int i = 0; g_reqest_handle_list[i].func; i++) {
            if (strncmp(http_url, g_reqest_handle_list[i].url,
                        strlen(g_reqest_handle_list[i].url))) {
                continue;
            }

            ret = g_reqest_handle_list[i].func(conn);
            MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "g_reqest_handle_list, url: %s ret: %d",
                            g_reqest_handle_list[i].url, ret);
            break;
        }
    }

    return ESP_OK;
}

ssize_t mdf_http_response_create(char *http_response, const wifi_mesh_data_type_t *type,
                                 const wifi_mesh_addr_t *addr, void *data, size_t size)
{
    MDF_PARAM_CHECK(http_response);
    MDF_PARAM_CHECK(data);
    MDF_PARAM_CHECK(addr);

    esp_err_t ret              = ESP_OK;
    char mac_str[13]           = {0};
    ssize_t http_response_size = 0;

    ret = mdf_http_set_response_status(http_response, 200);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_http_set_response_status, ret: %d", ret);

    ret = mdf_http_set_header(http_response, "Content-Type", proto_str[type->proto]);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_http_set_header, ret: %d", ret);

    mac2str(addr->mac, mac_str);
    ret = mdf_http_set_header(http_response, "Mesh-Node-Mac", mac_str);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_http_set_header, ret: %d", ret);

    uint32_t layer = type->layer;
    ret = mdf_http_set_header(http_response, "Mesh-Layer", layer);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_http_set_header, ret: %d", ret);

    http_response_size = mdf_http_set_body(http_response, (char *)data, size);
    MDF_ERROR_CHECK(http_response_size < 0, ESP_FAIL, "mdf_http_set_header, ret: %d", ret);

    return http_response_size;
}

esp_err_t mdf_http_upgrade_parser(server_http_connect_t *conn)
{
    MDF_PARAM_CHECK(conn);
    MDF_PARAM_CHECK(conn->http_head);

    esp_err_t ret                           = 0;
    static const uint8_t s_ota_data_tag[]   = {0xA5, 0xA5, 0xA5, 0xA5};
    static const uint8_t s_ota_finish_tag[] = {0xC8, 0xF4, 0x01};

    if (conn->http_size < conn->ota_packet_size) {
        return ESP_OK;
    }

    do {
        if (memcmp(conn->http_head, s_ota_data_tag, sizeof(s_ota_data_tag))) {
            MDF_LOGW("ota tcp recv finish, sockfd: %d", conn->sockfd);
            mdf_socket_fflush(conn->sockfd);

            ret = mdf_server_conn_send(conn, (void *)s_ota_finish_tag, sizeof(s_ota_finish_tag));
            MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_server_conn_send, ret: %d", ret);

            return ESP_OK;
        }

        for (int i = 0; i < conn->dest_addrs_num; ++i) {

            ret = mdf_wifi_mesh_root_send(&conn->src_addr, conn->dest_addrs + i, &conn->type,
                                          conn->http_head, conn->ota_packet_size);
            MDF_ERROR_CONTINUE(ret <= 0, "mdf_wifi_mesh_root_send, ret: %d", ret);
        }

        conn->http_size -= conn->ota_packet_size;

        if (!conn->http_size) {
            mdf_free(conn->http_head);
        } else {
            memcpy(conn->http_head, conn->http_head + conn->ota_packet_size, conn->http_size);
        }

    } while (conn->http_size >= conn->ota_packet_size);

    return ESP_OK;
}
