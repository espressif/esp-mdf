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

#include "esp_wifi.h"

#include "mlink.h"

/**
 * @brief Wi-Fi packet format
 */
typedef struct {
    uint8_t header[4];
    uint8_t dest_addr[6];
    uint8_t source_addr[6];
    uint8_t addr[6];
    uint8_t payload[0];
} sniffer_payload_t;

typedef struct sniffer_data_list {
    struct sniffer_data_list *next;
    mlink_sniffer_data_t data;
} __attribute__((packed)) sniffer_data_list_t;

#define sniffer_timestamp() (xTaskGetTickCount() * portTICK_RATE_MS)

static const char *TAG       = "mlink_sniffer";
static void *g_sniffer_lock  = NULL;
static uint32_t g_device_num = 0;
static sniffer_data_list_t *g_sniffer_list = NULL;
static mlink_sniffer_config_t g_sniffer_config   = {
    .enable_type       = MLINK_SNIFFER_NONE,
    .notice_percentage = 50,
    .buffer_num        = 100,
    .esp_filter        = true,
    .ble_scan_interval = 1000,
    .ble_scan_window   = 50,
};

static const uint8_t esp_module_addr[32][3] = {
    {0x54, 0x5A, 0xA6}, {0x24, 0x0A, 0xC4}, {0xD8, 0xA0, 0x1D}, {0xEC, 0xFA, 0xBC},
    {0xA0, 0x20, 0xA6}, {0x90, 0x97, 0xD5}, {0x18, 0xFE, 0x34}, {0x60, 0x01, 0x94},
    {0x2C, 0x3A, 0xE8}, {0xA4, 0x7B, 0x9D}, {0xDC, 0x4F, 0x22}, {0x5C, 0xCF, 0x7F},
    {0xAC, 0xD0, 0x74}, {0x30, 0xAE, 0xA4}, {0x24, 0xB2, 0xDE}, {0x68, 0xC6, 0x3A},
};

static uint8_t ltv_data_insert(uint8_t *data, uint8_t type, uint8_t length, const void *value)
{
    if (!length) {
        return 0;
    }

    *(data)     = length + sizeof(length);
    *(data + 1) = type;
    memcpy(data + 2, value, length);

    return length + sizeof(length) + sizeof(type);
}

static uint8_t *ltv_data_get(const uint8_t *data, uint8_t type,
                             uint8_t total_length, uint8_t *length)
{
    uint8_t *value = (uint8_t *)data;

    for (uint8_t tmp_len = *value++; tmp_len && (value - data <= total_length);
            value += tmp_len - sizeof(tmp_len) /* skip the length of data */, tmp_len = *value++) {
        if (*value++ == type) {
            /* length doesn't include itself */
            *length = tmp_len - sizeof(tmp_len); /* minus the length of type */
            return value;
        }
    }

    *length = 0;
    return NULL;
}

static mdf_err_t mlink_sniffer_list_insert(const mlink_sniffer_data_t *sniffer_data)
{
    sniffer_data_list_t *sniffer_node = NULL;
    uint8_t addr_len                  = 0;
    static int notice_percentage      = 0;
    uint8_t *sniffer_data_addr        = ltv_data_get(sniffer_data->data, MLINK_SNIFFER_DATA_ADDR,
                                        sniffer_data->size - 1, &addr_len);

    if (!g_sniffer_list) {
        g_sniffer_list       = MDF_MALLOC(sizeof(sniffer_data_list_t));
        g_sniffer_list->next = NULL;
    }

    if (!g_sniffer_lock) {
        g_sniffer_lock  = xSemaphoreCreateMutex();
    }

    xSemaphoreTake(g_sniffer_lock, portMAX_DELAY);

    if (/* esp_get_free_heap_size() < 20 * 1024 || */ g_device_num > g_sniffer_config.buffer_num) {
        g_device_num = 0;

        for (sniffer_node = g_sniffer_list->next; sniffer_node; sniffer_node = g_sniffer_list->next) {
            g_sniffer_list->next = sniffer_node->next;
            MDF_FREE(sniffer_node);
        }
    }

    notice_percentage = (!g_device_num) ? g_sniffer_config.notice_percentage : notice_percentage;

    if (notice_percentage && g_device_num > g_sniffer_config.buffer_num * notice_percentage / 100) {
        mdf_event_loop_send(MDF_EVENT_MLINK_BUFFER_FULL, NULL);
        MDF_LOGD("sniffer notice percentage: %d%%, g_device_num: %d",
                 notice_percentage, g_device_num);
        notice_percentage += notice_percentage;
    }


    for (int i = 0; g_sniffer_config.esp_filter && i < 32; ++i) {
        if (!memcmp(sniffer_data_addr, esp_module_addr[i], 3)) {
            xSemaphoreGive(g_sniffer_lock);
            return MDF_OK;
        }
    }

    for (sniffer_node = g_sniffer_list->next; sniffer_node; sniffer_node = sniffer_node->next) {
        uint8_t *sniffer_node_addr = ltv_data_get(sniffer_node->data.data, MLINK_SNIFFER_DATA_ADDR,
                                     sniffer_node->data.size - 1, &addr_len);

        if (!sniffer_node_addr) {
            esp_log_buffer_hex(TAG, sniffer_node->data.data, sniffer_node->data.size);
        }

        MDF_ERROR_BREAK(!sniffer_node_addr, "ltv_data_get, ret: %p", sniffer_node_addr);

        if (!memcmp(sniffer_node_addr, sniffer_data_addr, addr_len)) {
            if (sniffer_node->data.size < sniffer_data->size) {
                MDF_LOGD("new broadcast packet data length changes");
                sniffer_node = NULL;
            }

            break;
        }
    }

    if (!sniffer_node) {
        sniffer_node         = MDF_MALLOC(sizeof(sniffer_data_list_t) + sniffer_data->size);
        sniffer_node->next   = g_sniffer_list->next;
        g_sniffer_list->next = sniffer_node;
    }

    memcpy(&sniffer_node->data, sniffer_data, sniffer_data->size + sizeof(sniffer_data->size));
    g_device_num++;

    xSemaphoreGive(g_sniffer_lock);

    return MDF_OK;
}

static void sniffer_wifi_cb(void *recv_buf, wifi_promiscuous_pkt_type_t type)
{
    wifi_promiscuous_pkt_t *wifi_promiscuous_pkt = (wifi_promiscuous_pkt_t *)recv_buf;
    sniffer_payload_t *sniffer_payload           = (sniffer_payload_t *)wifi_promiscuous_pkt->payload;

    if (sniffer_payload->header[0] != 0x40) {
        return;
    }

    int8_t rssi        = wifi_promiscuous_pkt->rx_ctrl.rssi;
    uint8_t channel    = wifi_promiscuous_pkt->rx_ctrl.channel;
    uint32_t timestamp = sniffer_timestamp();
    mlink_sniffer_data_t *sniffer_data = MDF_CALLOC(1, 22);

    MDF_LOGD("Wi-Fi, addr: "MACSTR", rssi: %d, channel: %d",
             MAC2STR(sniffer_payload->source_addr), rssi, channel);

    sniffer_data->type  = MLINK_SNIFFER_WIFI;
    sniffer_data->size  = ltv_data_insert(sniffer_data->data, MLINK_SNIFFER_DATA_ADDR,
                                          6, sniffer_payload->source_addr);
    sniffer_data->size += ltv_data_insert(sniffer_data->data + sniffer_data->size, MLINK_SNIFFER_DATA_TIMESTAMP,
                                          sizeof(uint32_t), &timestamp);
    sniffer_data->size += ltv_data_insert(sniffer_data->data + sniffer_data->size, MLINK_SNIFFER_DATA_RSSI,
                                          sizeof(uint8_t), &rssi);
    sniffer_data->size += ltv_data_insert(sniffer_data->data + sniffer_data->size, MLINK_SNIFFER_DATA_CHANNEL,
                                          sizeof(uint8_t), &channel);
    sniffer_data->size += sizeof(sniffer_data->type);

    mlink_sniffer_list_insert(sniffer_data);

    MDF_FREE(sniffer_data);
}

#if CONFIG_BT_ENABLED
#if !CONFIG_BT_NIMBLE_ENABLED
#include "esp_gap_ble_api.h"
#endif /**< CONFIG_BT_ENABLED */
#endif /**< !CONFIG_BT_NIMBLE_ENABLED */
#ifdef CONFIG_IDF_TARGET_ESP32
static mdf_err_t mlink_sniffer_ble_cb(void *data, size_t size)
{
#if CONFIG_BT_ENABLED
#if !CONFIG_BT_NIMBLE_ENABLED

    esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)data;
    uint8_t adv_name_len = 0;
    uint8_t *adv_name    = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                           ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);

    uint8_t adv_manufacturer_len = 0;
    uint8_t *adv_manufacturer    = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                                   ESP_BLE_AD_MANUFACTURER_SPECIFIC_TYPE, &adv_manufacturer_len);

    adv_manufacturer_len = adv_manufacturer_len < 2 ? 0 : adv_manufacturer_len;
    MDF_LOGD("BLE, addr: " MACSTR ", rssi: %d, name_len: %d, name: %.*s, adv_manufacturer_len: %d, manufacturer: 0x%04x",
             MAC2STR(scan_result->scan_rst.bda), scan_result->scan_rst.rssi,
             adv_name_len, adv_name_len, (char *)adv_name, adv_manufacturer_len,
             (adv_manufacturer_len ? * ((uint16_t *)adv_manufacturer) : 0));

    uint32_t timestamp = sniffer_timestamp();
    mlink_sniffer_data_t *sniffer_data = MDF_CALLOC(1, sizeof(mlink_sniffer_data_t) + 21 + adv_name_len + adv_manufacturer_len);

    sniffer_data->type = MLINK_SNIFFER_BLE;
    sniffer_data->size += ltv_data_insert(sniffer_data->data, MLINK_SNIFFER_DATA_ADDR,
                                          6, scan_result->scan_rst.bda);
    sniffer_data->size += ltv_data_insert(sniffer_data->data + sniffer_data->size, MLINK_SNIFFER_DATA_TIMESTAMP,
                                          sizeof(uint32_t), &timestamp);
    sniffer_data->size += ltv_data_insert(sniffer_data->data + sniffer_data->size, MLINK_SNIFFER_DATA_RSSI,
                                          sizeof(uint8_t), &scan_result->scan_rst.rssi);
    sniffer_data->size += ltv_data_insert(sniffer_data->data + sniffer_data->size, MLINK_SNIFFER_DATA_NAME,
                                          adv_name_len, adv_name);
    sniffer_data->size += ltv_data_insert(sniffer_data->data + sniffer_data->size, MLINK_SNIFFER_DATA_MANUFACTURER,
                                          adv_manufacturer_len, adv_manufacturer);
    sniffer_data->size += sizeof(sniffer_data->type);
    mlink_sniffer_list_insert(sniffer_data);

    MDF_FREE(sniffer_data);
#endif /**< !CONFIG_BT_NIMBLE_ENABLED */
#endif /**< CONFIG_BT_ENABLED */

    return ESP_OK;
}
#endif

mdf_err_t mlink_sniffer_set_config(const mlink_sniffer_config_t *config)
{
    MDF_PARAM_CHECK(config);

    memcpy(&g_sniffer_config, config, sizeof(mlink_sniffer_config_t));

    return MDF_OK;
}

mdf_err_t mlink_sniffer_get_config(mlink_sniffer_config_t *config)
{
    MDF_PARAM_CHECK(config);

    memcpy(config, &g_sniffer_config, sizeof(mlink_sniffer_config_t));

    return MDF_OK;
}

mdf_err_t mlink_sniffer_data(uint8_t **data, size_t *size)
{
    MDF_PARAM_CHECK(data);
    MDF_PARAM_CHECK(size);

    *size = 0;
    *data = NULL;

    if (!g_sniffer_list || !g_sniffer_list->next || !g_sniffer_lock) {
        MDF_LOGD("sniffer data NULL");
        return ESP_OK;
    }

    xSemaphoreTake(g_sniffer_lock, portMAX_DELAY);

    uint32_t timestamp  = sniffer_timestamp();

    for (sniffer_data_list_t *sniffer_node = g_sniffer_list->next; sniffer_node;
            sniffer_node = g_sniffer_list->next) {
        size_t data_size       = *size;
        uint8_t timestamp_len  = 0;
        uint8_t *timestamp_ptr = ltv_data_get(sniffer_node->data.data, MLINK_SNIFFER_DATA_TIMESTAMP,
                                              sniffer_node->data.size - 1, &timestamp_len);

        *size += sniffer_node->data.size + sizeof(sniffer_node->data.size);

        if (!(*data = MDF_REALLOC(*data, *size))) {
            MDF_FREE(*data);
            return MDF_ERR_NO_MEM;
        }

        *((uint32_t *)timestamp_ptr) = timestamp - *((uint32_t *)timestamp_ptr);
        memcpy(*data + data_size, &sniffer_node->data, sniffer_node->data.size + sizeof(sniffer_node->data.size));

        g_sniffer_list->next = sniffer_node->next;
        MDF_FREE(sniffer_node);
    }

    g_device_num = 0;
    MDF_LOGD("sniffer_node, total_size: %d", *size);

    xSemaphoreGive(g_sniffer_lock);

    return MDF_OK;
}

mdf_err_t mlink_sniffer_wifi_start()
{
    mdf_err_t ret = MDF_OK;

    ret = esp_wifi_set_promiscuous_rx_cb(sniffer_wifi_cb);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "esp_wifi_set_promiscuous_rx_cb");

    ret = esp_wifi_set_promiscuous(1);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "esp_wifi_set_promiscuous");

    MDF_LOGI("mesh wifi sniffer start");

    return MDF_OK;
}

mdf_err_t mlink_sniffer_wifi_stop()
{
    mdf_err_t ret = MDF_OK;

    ret = esp_wifi_set_promiscuous_rx_cb(NULL);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "esp_wifi_set_promiscuous_rx_cb");

    ret = esp_wifi_set_promiscuous(0);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "esp_wifi_set_promiscuous");

    MDF_LOGI("mesh wifi sniffer stop");

    return MDF_OK;
}

#ifdef CONFIG_IDF_TARGET_ESP32
mdf_err_t mlink_sniffer_ble_start()
{
    mdf_err_t ret = MDF_OK;

    ret = mlink_ble_set_scan((uint16_t)(g_sniffer_config.ble_scan_interval * 1.6),
                             (uint16_t)g_sniffer_config.ble_scan_window * 1.6, mlink_sniffer_ble_cb);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "ble_set_scan");

    MDF_LOGI("mesh ble sniffer start");

    return MDF_OK;
}

mdf_err_t mlink_sniffer_ble_stop()
{
    mdf_err_t ret = MDF_OK;

    ret = mlink_ble_set_scan(0, 0, NULL);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "ble_set_scan, ret: %x", ret);

    MDF_LOGI("mesh ble sniffer stop");

    return MDF_OK;
}
#endif

mdf_err_t mlink_sniffer_init()
{
    if (!g_sniffer_list) {
        g_sniffer_list = MDF_MALLOC(sizeof(sniffer_data_list_t));
        g_sniffer_list->next = NULL;
    }

    if (!g_sniffer_lock) {
        g_sniffer_lock  = xSemaphoreCreateMutex();
    }

    return MDF_OK;
}

mdf_err_t mlink_sniffer_deinit()
{
    if (!g_sniffer_lock) {
        return MDF_OK;
    }

    xSemaphoreTake(g_sniffer_lock, portMAX_DELAY);

    g_device_num = 0;

    for (sniffer_data_list_t *sniffer_node = g_sniffer_list->next; sniffer_node;
            sniffer_node = g_sniffer_list->next) {
        g_sniffer_list->next = sniffer_node->next;
        MDF_FREE(sniffer_node);
    }

    xSemaphoreGive(g_sniffer_lock);

    return MDF_OK;
}
