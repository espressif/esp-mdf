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

#include "tcpip_adapter.h"

#include "mdf_wifi_mesh.h"
#include "mdf_network_config.h"
#include "mdf_event_loop.h"
#include "mdf_espnow.h"
#include "mdf_server.h"
#include "mdf_notice.h"
#include "mdf_upgrade.h"

static const char *TAG                      = "mdf_wifi_mesh";
static bool wifi_mesh_connect_flag          = false;
static void *g_wifi_mesh_send_lock          = NULL;
static bool g_wifi_mesh_running_flag        = false;

const uint8_t WIFI_MESH_MULTICAST_ADDR[]    = {0x01, 0x00, 0x5E, 0x00, 0x00, 0x00};
const uint8_t WIFI_MESH_BROADCAST_ADDR[]    = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
const uint8_t WIFI_MESH_ROOT_DEFAULT_ADDR[] = {0x00, 0x00, 0xc0, 0xa8, 0x00, 0x00};

static bool mdf_wifi_mesh_send_lock()
{
    if (!g_wifi_mesh_send_lock) {
        g_wifi_mesh_send_lock = xSemaphoreCreateMutex();
    }

    return xSemaphoreTake(g_wifi_mesh_send_lock, portMAX_DELAY);
}

static bool mdf_wifi_mesh_send_unlock()
{
    if (!g_wifi_mesh_send_lock) {
        g_wifi_mesh_send_lock = xSemaphoreCreateMutex();
    }

    return xSemaphoreGive(g_wifi_mesh_send_lock);
}

static void mdf_tcpip_adapter_dhcpc()
{
    static bool s_is_dhcp_stopped = false;

    if (esp_mesh_is_root() && s_is_dhcp_stopped) {
        tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
        s_is_dhcp_stopped = false;
    } else if (!esp_mesh_is_root() && !s_is_dhcp_stopped) {
        tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
        tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP);
        s_is_dhcp_stopped = true;
    }
}

static void esp_mesh_event_cb(mesh_event_t event)
{
    esp_err_t ret = 0;
    MDF_LOGI("esp_mesh_event_cb event.id: %d", event.id);

    switch (event.id) {
        case MESH_EVENT_PARENT_CONNECTED:
            MDF_LOGI("wifi connected");
            wifi_mesh_connect_flag = true;
            mdf_tcpip_adapter_dhcpc();
            break;

        case MESH_EVENT_PARENT_DISCONNECTED: {

            MDF_LOGI("wifi disconnected");
            wifi_mesh_connect_flag = false;

            mesh_event_disconnected_t *disconnected = &event.info.disconnected;

            switch (disconnected->reason) {
                /**< pwd err */
                case WIFI_REASON_HANDSHAKE_TIMEOUT:
                case WIFI_REASON_MIC_FAILURE:
                case WIFI_REASON_BEACON_TIMEOUT:
                case WIFI_REASON_AUTH_LEAVE:

                    /**< root has children, when router destoried or pwd err,
                    in mdf_network_enable_blufi, wait some time */
                    if (esp_mesh_is_root()) {
                        mdf_network_enable_blufi();
                    }

                    break;

                case WIFI_REASON_AUTH_EXPIRE: {
                    static int reason2_count = 0;

                    if (reason2_count++ > 10) {
                        MDF_LOGE("sta disconnected, reason: WIFI_REASON_AUTH_EXPIRE(2)");
                        MDF_LOGE("system restart");
                        esp_restart();
                    }

                    break;
                }

                default:
                    break;
            }

            ret = mdf_server_deinit();
            MDF_ERROR_CHECK(ret < 0, ; , "mdf_server_deinit, ret: %d", ret);

            break;
        }

        case MESH_EVENT_ROOT_GOT_IP:
            MDF_LOGI("wifi get ip");
            wifi_mesh_connect_flag = true;
            ret = mdf_server_init();
            MDF_ERROR_CHECK(ret < 0, ; , "mdf_server_init, ret: %d", ret);
            break;

        case MESH_EVENT_ROUTING_TABLE_ADD:
        case MESH_EVENT_ROUTING_TABLE_REMOVE:
            if (esp_mesh_is_root()) {
                uint8_t root_mac[6] = {0};
                ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, root_mac));
                mdf_notice_udp_send(NOTICE_ROUTING_TABLE_CHANGE, root_mac);
            }

        default:
            break;
    }

    ret = mdf_event_loop_send(event.id, &event.info);
    MDF_ERROR_CHECK(ret < 0, ; , "mdf_event_loop_send, ret: %d", ret);
}

bool mdf_wifi_mesh_is_running()
{
    return g_wifi_mesh_running_flag;
}

bool mdf_wifi_mesh_is_connect()
{
    return wifi_mesh_connect_flag;
}

esp_err_t mdf_wifi_mesh_init(const wifi_mesh_config_t *config)
{
    g_wifi_mesh_send_lock    = xSemaphoreCreateMutex();
    mesh_cfg_t mesh_config   = {0};
    mesh_attempts_t attempts = {
        .scan       = WIFI_MESH_ATTEMPT_SCAN_TIME,
        .vote       = 1000,
        .fail       = 20,
        .monitor_ie = 3
    };

    /* mesh event callback */
    mesh_config.event_cb               = &esp_mesh_event_cb;
    mesh_config.channel                = config->channel;
    mesh_config.router.ssid_len        = strlen(config->ssid);
    mesh_config.mesh_ap.max_connection = WIFI_MESH_AP_CONNECTIONS;

    memcpy(mesh_config.router.ssid, config->ssid, mesh_config.router.ssid_len);
    memcpy(mesh_config.router.bssid, config->bssid, sizeof(mesh_config.router.bssid));
    memcpy(mesh_config.router.password, config->password, strlen(config->password));
    memcpy(mesh_config.mesh_id.addr, config->mesh_id.mac, 6);
    memcpy(mesh_config.mesh_ap.password, mesh_config.router.password,
           sizeof(mesh_config.mesh_ap.password));

    ESP_ERROR_CHECK(esp_mesh_init());
    ESP_ERROR_CHECK(esp_mesh_set_config(&mesh_config));
    ESP_ERROR_CHECK(esp_mesh_set_attempts(&attempts));
    ESP_ERROR_CHECK(esp_mesh_set_max_layer(WIFI_MESH_MAX_LAYER));
    ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(WIFI_MESH_VOTE_PERCENTAGE / 100.0));
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(WIFI_MESH_AP_ASSOC_EXPIRE));
    ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(WIFI_WIFI_MESH_AUTH_MODE));
    ESP_ERROR_CHECK(esp_mesh_set_xon_qsize(WIFI_MESH_XON_QSIZE));
    ESP_ERROR_CHECK(esp_mesh_allow_root_conflicts(false));
    ESP_ERROR_CHECK(esp_mesh_start());

    g_wifi_mesh_running_flag = true;

    return ESP_OK;
}

esp_err_t mdf_wifi_mesh_deinit()
{
    ESP_ERROR_CHECK(esp_mesh_stop());
    ESP_ERROR_CHECK(esp_mesh_deinit());
    ESP_ERROR_CHECK(mdf_server_deinit());

    vSemaphoreDelete(g_wifi_mesh_send_lock);
    g_wifi_mesh_send_lock = NULL;

    g_wifi_mesh_running_flag = false;

    return ESP_OK;
}

ssize_t mdf_wifi_mesh_send(const wifi_mesh_addr_t *dest_addr, const wifi_mesh_data_type_t *data_type,
                           const void *data, ssize_t size)
{
    MDF_PARAM_CHECK(dest_addr);
    MDF_PARAM_CHECK(data_type);

    static int packet_id = 0;
    esp_err_t ret        = ESP_OK;
    int mesh_flag        = (data_type->to_server) ? MESH_DATA_TODS : MESH_DATA_P2P;

    mesh_data_t mesh_data = {
        .proto = data_type->proto,
        .tos   = (data_type->group) ? MESH_TOS_DEF : MESH_TOS_P2P,
    };

    wifi_mesh_head_data_t mesh_head_data = {
        .size = size,
        .seq  = 0,
        .id   = packet_id++,
    };

    mesh_opt_t mesh_opt = {
        .len  = sizeof(wifi_mesh_head_data_t),
        .val  = (void *) &mesh_head_data,
        .type = (data_type->group) ? MESH_OPT_SEND_GROUP : MESH_OPT_RECV_DS_ADDR,
    };

    memcpy(&mesh_head_data.type, data_type, sizeof(wifi_mesh_data_type_t));
    mesh_flag = (data_type->group) ? mesh_flag | MESH_DATA_GROUP : mesh_flag;
    mesh_flag = (data_type->to_server) ? mesh_flag | MESH_DATA_DROP : mesh_flag;

    for (mesh_head_data.seq = 0; size > 0; mesh_head_data.seq++, size -= WIFI_MESH_PACKET_MAX_SIZE) {
        mesh_data.data = (uint8_t *)data + mesh_head_data.seq * WIFI_MESH_PACKET_MAX_SIZE;
        mesh_data.size = (size > WIFI_MESH_PACKET_MAX_SIZE) ? WIFI_MESH_PACKET_MAX_SIZE : size;

        MDF_LOGV("mesh_data size: %d, seq: %d, flag: 0x%x, data: %s",
                 mesh_data.size, mesh_head_data.seq, data_type->val, mesh_data.data);

        mdf_wifi_mesh_send_lock();
        ret = esp_mesh_send((mesh_addr_t *)dest_addr, &mesh_data, mesh_flag, &mesh_opt, 1);
        mdf_wifi_mesh_send_unlock();
        MDF_ERROR_CHECK(ret != ESP_OK, -ret,
                        "esp_mesh_send, ret: %x dest_mac:"MACSTR", size:%d, data: %s",
                        ret, MAC2STR(dest_addr->mac), mesh_data.size, mesh_data.data);
    }

    return mesh_head_data.size;
}

ssize_t mdf_wifi_mesh_recv(wifi_mesh_addr_t *src_addr, wifi_mesh_data_type_t *data_type,
                           void *data, ssize_t size, int timeout_ms)
{
    MDF_PARAM_CHECK(src_addr);
    MDF_PARAM_CHECK(data_type);
    MDF_PARAM_CHECK(data);
    MDF_PARAM_CHECK(size >= WIFI_MESH_PACKET_MAX_SIZE);

    static int last_packet_id            = -1;
    esp_err_t ret                        = ESP_OK;
    ssize_t recv_size                    = 0;
    int mesh_flag                        = 0;
    mesh_data_t mesh_data                = {0};
    uint32_t end_tick                    = xTaskGetTickCount() + timeout_ms * portTICK_RATE_MS;
    wifi_mesh_head_data_t mesh_head_data = {0};
    mesh_opt_t mesh_opt                  = {
        .len  = sizeof(wifi_mesh_head_data_t),
        .val  = (void *) &mesh_head_data,
        .type = MESH_OPT_RECV_DS_ADDR,
    };

    do {
        mesh_data.size = size - recv_size;
        mesh_data.data = data + recv_size;
        timeout_ms     = (timeout_ms != portMAX_DELAY) ?
                         (end_tick - xTaskGetTickCount()) / portTICK_RATE_MS : portMAX_DELAY;

        ret = esp_mesh_recv((mesh_addr_t *)src_addr, &mesh_data, timeout_ms,
                            &mesh_flag, &mesh_opt, 1);
        MDF_ERROR_CHECK(ret != ESP_OK || mesh_data.size <= 0, -ret,
                        "esp_mesh_recv , ret: %x, size: %d, data: %p",
                        ret, mesh_data.size, mesh_data.data);

        /**< filter retransmitted packets */
        if (mesh_head_data.id == last_packet_id) {
            MDF_LOGW("received duplicate packets, packet_id: %d", mesh_head_data.id);
            continue;
        }

        MDF_LOGV("esp_mesh_recv , ret: %x, size: %d, data: %p, mesh_head_data.type: %x, src_addr: " MACSTR,
                 ret, mesh_data.size, mesh_data.data, mesh_head_data.type.val, MAC2STR(src_addr->mac));

        if (mesh_head_data.type.ota) {
            mdf_upgrade_write(mesh_data.data, mesh_data.size);
            continue;
        }

        recv_size      += mesh_data.size;
        last_packet_id  = mesh_head_data.id;
    } while ((recv_size < mesh_head_data.size)
             && (timeout_ms == portMAX_DELAY || xTaskGetTickCount() < end_tick));

    memcpy(data_type, &mesh_head_data.type, sizeof(wifi_mesh_data_type_t));

    if (data_type->to_server) {
        memcpy(src_addr, &mesh_head_data.addr, sizeof(wifi_mesh_addr_t));
    }

    return recv_size;
}

ssize_t mdf_wifi_mesh_root_send(wifi_mesh_addr_t *src_addr, wifi_mesh_addr_t *dest_addr,
                                const wifi_mesh_data_type_t *data_type,
                                const void *data, ssize_t size)
{
    MDF_PARAM_CHECK(src_addr);
    MDF_PARAM_CHECK(dest_addr);
    MDF_PARAM_CHECK(data_type);
    MDF_PARAM_CHECK(data);

    static int packet_id  = 0;
    esp_err_t ret         = ESP_OK;
    int mesh_flag         = (data_type->to_server) ? MESH_DATA_FROMDS : MESH_DATA_P2P;

    mesh_data_t mesh_data = {
        .proto = data_type->proto,
        .tos   = (data_type->group) ? MESH_TOS_DEF : MESH_TOS_P2P,
    };

    wifi_mesh_head_data_t mesh_head_data = {
        .size = size,
        .seq  = 0,
        .id   = packet_id++,
    };

    mesh_opt_t mesh_opt = {
        .len  = sizeof(wifi_mesh_head_data_t),
        .val  = (void *) &mesh_head_data,
        .type = (data_type->group) ? MESH_OPT_SEND_GROUP : MESH_OPT_RECV_DS_ADDR,
    };

    memcpy(&mesh_head_data.type, data_type, sizeof(wifi_mesh_data_type_t));
    memcpy(&mesh_head_data.addr, src_addr, sizeof(wifi_mesh_addr_t));
    mesh_flag = (data_type->group) ? mesh_flag | MESH_DATA_GROUP : mesh_flag;

    for (mesh_head_data.seq = 0; size > 0; mesh_head_data.seq++, size -= WIFI_MESH_PACKET_MAX_SIZE) {
        mesh_data.data = (uint8_t *)data + mesh_head_data.seq * WIFI_MESH_PACKET_MAX_SIZE;
        mesh_data.size = (size > WIFI_MESH_PACKET_MAX_SIZE) ? WIFI_MESH_PACKET_MAX_SIZE : size;

        MDF_LOGV("mesh_data size: %d, seq: %d, flag: 0x%x, data: %s",
                 mesh_data.size, mesh_head_data.seq, data_type->val, mesh_data.data);

        while (esp_get_free_heap_size() < 30 * 1024) {
            vTaskDelay(20 / portTICK_RATE_MS);
            MDF_LOGW("not enough memory(%d Byte), delay: 20ms", esp_get_free_heap_size());
        }

        mdf_wifi_mesh_send_lock();
        ret = esp_mesh_send((mesh_addr_t *)dest_addr, &mesh_data, mesh_flag, &mesh_opt, 1);
        mdf_wifi_mesh_send_unlock();
        MDF_ERROR_CHECK(ret != ESP_OK, -ret, "esp_mesh_send, ret: %x dest_mac:"MACSTR", size:%d, data: %s",
                        ret, MAC2STR(dest_addr->mac), mesh_data.size, mesh_data.data);
    }

    return mesh_head_data.size;
}

ssize_t mdf_wifi_mesh_root_recv(wifi_mesh_addr_t *src_addr, wifi_mesh_addr_t *dest_addr,
                                wifi_mesh_data_type_t *data_type,
                                void *data, ssize_t size, int timeout_ms)
{
    MDF_PARAM_CHECK(src_addr);
    MDF_PARAM_CHECK(dest_addr);
    MDF_PARAM_CHECK(data_type);
    MDF_PARAM_CHECK(data);

    esp_err_t ret                        = ESP_OK;
    ssize_t recv_size                    = 0;
    int mesh_flag                        = 0;
    mesh_data_t mesh_data                = {0};
    uint32_t end_tick                    = xTaskGetTickCount() + timeout_ms * portTICK_RATE_MS;
    wifi_mesh_head_data_t mesh_head_data = {0};
    mesh_opt_t mesh_opt                  = {
        .len = sizeof(wifi_mesh_head_data_t),
        .val = (void *) &mesh_head_data,
        .type = MESH_OPT_RECV_DS_ADDR,
    };

    do {
        mesh_data.size = size - recv_size;
        mesh_data.data = data + recv_size;
        timeout_ms     = (end_tick - xTaskGetTickCount()) / portTICK_RATE_MS;

        ret = esp_mesh_recv_toDS((mesh_addr_t *)src_addr, (mesh_addr_t *)dest_addr,
                                 &mesh_data, timeout_ms, &mesh_flag, &mesh_opt, 1);

        if (ret == ESP_ERR_MESH_TIMEOUT) {
            MDF_LOGV("esp_mesh_recv_toDS, timeout");
            return -ESP_ERR_MESH_TIMEOUT;
        }

        MDF_ERROR_CHECK(ret != ESP_OK || mesh_data.size <= 0, -ret,
                        "esp_mesh_recv_toDS , ret: %x, size: %d, data: %p",
                        ret, mesh_data.size, mesh_data.data);

        MDF_LOGV("esp_mesh_recv , ret: %x, size: %d, data: %p, type: %x",
                 ret, mesh_data.size, mesh_data.data, mesh_head_data.type.val);

        recv_size      += mesh_data.size;
    } while ((recv_size < mesh_head_data.size) && xTaskGetTickCount() < end_tick);

    memcpy(data_type, &mesh_head_data.type, sizeof(wifi_mesh_data_type_t));

    return recv_size;
}
