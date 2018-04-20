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

#include "mdf_device_handle.h"

#define MDF_RUNNING_MODE_KEY  "running_mode"

enum low_power_data_type {
    ESPNOW_CONTROL_PAIR,
    ESPNOW_CONTROL_GET,
    ESPNOW_CONTROL_SET,
    ESPNOW_CONTROL_REQUEST,
    ESPNOW_CONTROL_RESPONSE,
};

typedef struct {
    uint8_t type;
    wifi_mesh_addr_t dest_addr;
    uint8_t size;
    uint8_t data[0];
} low_power_data_t;

typedef struct {
    uint8_t num;
    mesh_addr_t addr[ESP_NOW_MAX_ENCRYPT_PEER_NUM];
} low_power_addr_t;

static const char *TAG  = "mdf_low_power";

static esp_err_t low_power_erase_parent()
{
    esp_err_t ret = ESP_OK;

    ret = mdf_info_erase("parent_addr");
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_erase, ret: %d", ret);

    return ESP_OK;
}

esp_err_t mdf_low_power_get_parent(wifi_mesh_addr_t *parent_addr)
{
    esp_err_t ret              = ESP_OK;
    char request[64]           = {0};
    char mac_str[16]           = {0};
    uint8_t sta_mac[6]         = {0};
    wifi_mesh_data_type_t type = {
        .no_response = true,
        .proto       = MDF_PROTO_JSON,
    };

    if (mdf_info_load("parent_addr", parent_addr, sizeof(wifi_mesh_addr_t)) > 0) {
        return ESP_OK;
    }

    if (mdf_get_running_mode() & TRANS_ESPNOW) {
        network_config_t network_config = {0};
        wifi_mesh_config_t mesh_config  = {0};

        ret = mdf_network_get_config(&network_config);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_get_network_config, ret: %d", ret);

        mesh_config.channel = network_config.channel;
        strncpy(mesh_config.ssid, network_config.ssid, sizeof(mesh_config.ssid));
        strncpy(mesh_config.password, network_config.password, sizeof(mesh_config.password));
        memcpy(&mesh_config.mesh_id, network_config.mesh_id, sizeof(mesh_config.mesh_id));

        ret = mdf_wifi_mesh_init(&mesh_config);
        MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "mdf_wifi_mesh_init, ret: %d", ret);

        while (!mdf_wifi_mesh_is_connect()) {
            vTaskDelay(500 / portTICK_RATE_MS);
        }
    }

    if (esp_mesh_is_root()) {
        wifi_sta_list_t sta_list = {0};
        ESP_ERROR_CHECK(esp_wifi_ap_get_sta_list(&sta_list));
        MDF_ERROR_CHECK(sta_list.num == 0, ESP_FAIL, "there is only one node in the mdf");
        memcpy(parent_addr, sta_list.sta[0].mac, 6);
    } else {
        ESP_ERROR_CHECK(esp_mesh_get_parent_bssid((mesh_addr_t *)parent_addr));
        *((int *)(parent_addr + 2)) = htonl(htonl(*((int *)(parent_addr + 2))) - 1);
    }

    ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac));
    mac2str(sta_mac, mac_str);

    ret = mdf_json_pack(request, "request", "add_low_power_device");
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);

    ret = mdf_json_pack(request, "addr", mac_str);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);

    MDF_LOGD("request: %s", request);

    ret = mdf_wifi_mesh_send(parent_addr, &type, request, strlen(request));
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "_mdf_wifi_mesh_send, ret: %d", ret);

    ret = mdf_info_save("parent_addr", parent_addr, 6);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_save, ret: %d", ret);

    if (mdf_get_running_mode() & TRANS_ESPNOW) {
        ret = mdf_wifi_mesh_deinit();
        MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "mdf_wifi_mesh_init, ret: %d", ret);
    }

    return ESP_OK;
}

static esp_err_t mdf_low_power_add_device(device_data_t *device_data)
{
    esp_err_t ret                = 0;
    char addr_str[16]            = {0};
    uint8_t addr[6]              = {0};
    low_power_addr_t device_addr = {0};

    MDF_LOGD("espnow_device: %s", device_data->request);

    ret = mdf_json_parse(device_data->request, "addr", addr_str);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_parse, ret: %d", ret);

    mdf_info_load("child_addr", &device_addr, sizeof(low_power_addr_t));
    MDF_ERROR_CHECK(device_addr.num >= ESP_NOW_MAX_ENCRYPT_PEER_NUM, ESP_FAIL,
                    "maximum number of ESPNOW encrypted peers, num: %d", device_addr.num);

    str2mac(addr_str, addr);
    MDF_LOGI("addr_str: %s, addr: "MACSTR, addr_str, MAC2STR(addr));
    mdf_espnow_add_peer_default_encrypt(addr);

    for (int i = 0; i < device_addr.num; ++i) {
        MDF_LOGD("num: %d, device_addr: "MACSTR", addr: "MACSTR,
                 device_addr.num, MAC2STR((uint8_t *)(device_addr.addr + i)), MAC2STR(addr));

        if (!memcmp(device_addr.addr + i, addr, 6)) {
            return ESP_OK;
        }
    }

    memcpy(device_addr.addr + device_addr.num, addr, 6);
    device_addr.num++;

    for (int i = 0; i < device_addr.num; ++i) {
        MDF_LOGI("addr espnow device: "MACSTR, MAC2STR((uint8_t *)(device_addr.addr + i)));
        mdf_espnow_add_peer_default_encrypt((uint8_t *)(device_addr.addr + i));
    }

    ret = mdf_info_save("child_addr", &device_addr, sizeof(low_power_addr_t));
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_save, ret: %d", ret);

    return ESP_OK;
}

esp_err_t mdf_low_power_send(wifi_mesh_addr_t *dest_addr,
                             const void *data, size_t size)
{
    esp_err_t ret                 = ESP_OK;
    wifi_mesh_addr_t parent_addr  = {0};
    size_t espnow_size            = size + sizeof(low_power_data_t);
    low_power_data_t *espnow_data = mdf_calloc(1, espnow_size);
    espnow_data->type             = ESPNOW_CONTROL_REQUEST;
    espnow_data->size             = size;

    memcpy(espnow_data->data, data, size);
    memcpy(&espnow_data->dest_addr, dest_addr, sizeof(wifi_mesh_addr_t));

    for (;;) {
        ret = mdf_low_power_get_parent(&parent_addr);
        MDF_ERROR_GOTO(ret < 0, EXIT, "low_power_get_parent, ret: %d", ret);

        MDF_LOGV("parent_addr:" MACSTR ", espnow_data, size: %d, data: %s, ret: %d",
                 MAC2STR(parent_addr.mac), espnow_data->size, espnow_data->data, ret);

        int retry_count = 0;

        do {
            retry_count++;

            vTaskDelay(retry_count * 10 / portTICK_RATE_MS);

            ESP_ERROR_CHECK(mdf_espnow_enable(MDF_ESPNOW_CONTROL));
            mdf_espnow_add_peer_default_encrypt(parent_addr.mac);
            ret = mdf_espnow_write(MDF_ESPNOW_CONTROL, parent_addr.mac,
                                   espnow_data, espnow_size, portMAX_DELAY);
            ESP_ERROR_CHECK(mdf_espnow_del_peer(parent_addr.mac));
            ESP_ERROR_CHECK(mdf_espnow_disable(MDF_ESPNOW_CONTROL));

            if (ret > 0) {
                goto EXIT;
            }
        } while (ret < 0 && retry_count < 5);

        ret = low_power_erase_parent();
        MDF_ERROR_GOTO(ret < 0, EXIT, "low_power_erase_parent, ret: %d", ret);
    }

EXIT:
    mdf_free(espnow_data);
    return ret;
}

mdf_running_mode_t mdf_get_running_mode()
{
    mdf_running_mode_t mode = 0;

    if (mdf_info_load(MDF_RUNNING_MODE_KEY, &mode, sizeof(mdf_running_mode_t)) < 0) {
        return POWER_ACTIVE | TRANS_WIFI_MESH;
    }

    return ESP_OK;
}

esp_err_t mdf_set_running_mode(mdf_running_mode_t mode)
{
    esp_err_t ret = ESP_OK;

    if (mode & TRANS_ESPNOW) {
        wifi_mesh_addr_t parent_addr = {0};

        ret = mdf_low_power_get_parent(&parent_addr);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_low_power_get_parent, ret: %d", ret);

        MDF_LOGD("mdf_low_power_get_parent, addr:"MACSTR, MAC2STR(parent_addr.mac));
    }

    ret = mdf_info_save(MDF_RUNNING_MODE_KEY, &mode, sizeof(mdf_running_mode_t));
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_save, ret: %d", ret);

    return ESP_OK;
}

static void mdf_low_power_task(void *arg)
{
    esp_err_t ret                      = ESP_OK;
    low_power_data_t *espnow_data = mdf_malloc(ESP_NOW_MAX_DATA_LEN);
    uint8_t source_addr[6]  = {0};
    wifi_mesh_data_type_t type         = {
        .no_response = true,
        .proto       = MDF_PROTO_JSON,
    };

    ESP_ERROR_CHECK(mdf_espnow_enable(MDF_ESPNOW_CONTROL));

    for (;;) {
        ret = mdf_espnow_read(MDF_ESPNOW_CONTROL, source_addr, espnow_data,
                              WIFI_MESH_PACKET_MAX_SIZE, portMAX_DELAY);
        MDF_ERROR_CONTINUE(ret < 0, "receive, size: %d, data:\n%s", ret, espnow_data->data);

        MDF_LOGD("espnow read, data: %s", espnow_data->data);

        ret = mdf_wifi_mesh_send(&espnow_data->dest_addr, &type,
                                 espnow_data->data, espnow_data->size);
        MDF_ERROR_CONTINUE(ret < 0, "mdf_wifi_mesh_send, ret: %d", ret);
    }

    ESP_ERROR_CHECK(mdf_espnow_disable(MDF_ESPNOW_CONTROL));
    mdf_free(espnow_data);
    vTaskDelete(NULL);
}

static esp_err_t mdf_low_power_recv()
{
    esp_err_t ret                = ESP_OK;
    low_power_addr_t device_addr = {0};

    ret = mdf_info_load("child_addr", &device_addr, sizeof(low_power_addr_t));

    if (ret <= 0 || device_addr.num > ESP_NOW_MAX_ENCRYPT_PEER_NUM || device_addr.num == 0) {
        MDF_LOGD("no need to add espnow device");
        return ESP_OK;
    }

    for (int i = 0; i < device_addr.num; ++i) {
        MDF_LOGI("addr espnow device: "MACSTR, MAC2STR((uint8_t *)(device_addr.addr + i)));
        mdf_espnow_add_peer_default_encrypt((uint8_t *)(device_addr.addr + i));
    }

    xTaskCreate(mdf_low_power_task, "mdf_low_power_task", 1024 * 2,
                NULL, MDF_TASK_DEFAULT_PRIOTY, NULL);


    return ESP_OK;
}

esp_err_t mdf_low_power_init()
{
    esp_err_t ret = ESP_OK;

    if (mdf_get_running_mode() & TRANS_ESPNOW) {
        /* espnow need to set the channel */
        network_config_t network_config = {0};
        ret = mdf_network_get_config(&network_config);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_get_network_config, ret: %d", ret);
        ESP_ERROR_CHECK(esp_wifi_set_channel(network_config.channel, 0));
    } else {
        ESP_ERROR_CHECK(mdf_device_add_handle("add_low_power_device", mdf_low_power_add_device));
        ESP_ERROR_CHECK(mdf_low_power_recv());
    }

    return ESP_OK;
}
