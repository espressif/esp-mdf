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

#include "rom/crc.h"

#include "mdf_common.h"
#include "mdf_device_handle.h"
#include "mdf_sniffer_handle.h"

#define DEVICE_INFO_NUM          300
#define DEVICE_NOTICE_PERCENTAGE 40

typedef struct {
    uint8_t header[4];
    uint8_t dest_mac[6];
    uint8_t source_mac[6];
    uint8_t bssid[6];
    uint8_t payload[0];
} sniffer_payload_t;

typedef struct sniffer_data {
    uint8_t  bssid[6];
    int8_t   rssi;
    uint8_t  channel;
    uint32_t timestamp;
    struct   sniffer_data *next;
} sniffer_data_t;

typedef struct {
    uint8_t  mac[6];
    int8_t   rssi;
    uint8_t  channel;
    uint32_t relative_time;
} sniffer_hex_data_t;

typedef struct {
    uint8_t type;
    uint8_t reserved[3];
    uint16_t size;
    uint16_t crc;
    sniffer_hex_data_t data[0];
} sniffer_hex_data_header_t;

static const char *TAG                     = "mdf_sniffer_handle";
static void *g_sniffer_lock                = NULL;
static sniffer_data_t *g_sniffer_list      = NULL;
static uint32_t sniffer_notice_percentage  = DEVICE_NOTICE_PERCENTAGE;
static uint32_t g_device_info_num          = 0;
static const uint8_t esp_module_mac[32][3] = {
    {0x54, 0x5A, 0xA6}, {0x24, 0x0A, 0xC4}, {0xD8, 0xA0, 0x1D}, {0xEC, 0xFA, 0xBC},
    {0xA0, 0x20, 0xA6}, {0x90, 0x97, 0xD5}, {0x18, 0xFE, 0x34}, {0x60, 0x01, 0x94},
    {0x2C, 0x3A, 0xE8}, {0xA4, 0x7B, 0x9D}, {0xDC, 0x4F, 0x22}, {0x5C, 0xCF, 0x7F},
    {0xAC, 0xD0, 0x74}, {0x30, 0xAE, 0xA4}, {0x24, 0xB2, 0xDE}, {0x68, 0xC6, 0x3A},
};

static uint32_t sniffer_timestamp()
{
    return xTaskGetTickCount() * (1000 / configTICK_RATE_HZ);
}

static void wifi_sniffer_cb(void *recv_buf, wifi_promiscuous_pkt_type_t type)
{
    sniffer_data_t *sniffer_data       = NULL;
    wifi_promiscuous_pkt_t *sniffer    = (wifi_promiscuous_pkt_t *)recv_buf;
    sniffer_payload_t *sniffer_payload = (sniffer_payload_t *)sniffer->payload;

    if (sniffer_payload->header[0] != 0x40) {
        return;
    }

    xSemaphoreTake(g_sniffer_lock, portMAX_DELAY);

    if (!g_sniffer_list) {
        g_sniffer_list = mdf_malloc(sizeof(sniffer_data_t));
        g_sniffer_list->next = NULL;
    }

    if (esp_get_free_heap_size() < 40 * 1024 || g_device_info_num > DEVICE_INFO_NUM) {
        g_device_info_num = 0;

        for (sniffer_data = g_sniffer_list->next; sniffer_data; sniffer_data = g_sniffer_list->next) {
            g_sniffer_list->next = sniffer_data->next;
            mdf_free(sniffer_data);
            sniffer_notice_percentage = DEVICE_NOTICE_PERCENTAGE;
        }
    }

    if (g_device_info_num > DEVICE_INFO_NUM * sniffer_notice_percentage / 100) {
        esp_err_t ret = mdf_event_loop_send(MDF_EVENT_SNIFFER_BUFFER_FULL, NULL);
        MDF_ERROR_CHECK(ret < 0, ; , "mdf_event_loop_send, ret: %d", ret);
        MDF_LOGV("sniffer notice percentage: %d, g_device_info_num: %d",
                 sniffer_notice_percentage, g_device_info_num);
        sniffer_notice_percentage += sniffer_notice_percentage;
    }

    for (int i = 0; i < 32; ++i) {
        if (!memcmp(sniffer_payload->source_mac, esp_module_mac[i], 3)) {
            xSemaphoreGive(g_sniffer_lock);
            return;
        }
    }

    for (sniffer_data = g_sniffer_list->next; sniffer_data; sniffer_data = sniffer_data->next) {
        if (!memcmp(sniffer_data->bssid, sniffer_payload->source_mac, sizeof(sniffer_data->bssid))) {
            break;
        }
    }

    if (!sniffer_data) {
        sniffer_data         = mdf_malloc(sizeof(sniffer_data_t));
        sniffer_data->next   = g_sniffer_list->next;
        g_sniffer_list->next = sniffer_data;
    }

    sniffer_data->rssi      = sniffer->rx_ctrl.rssi;
    sniffer_data->channel   = sniffer->rx_ctrl.channel;
    sniffer_data->timestamp = sniffer_timestamp();
    memcpy(sniffer_data->bssid, sniffer_payload->source_mac, sizeof(sniffer_data->bssid));
    g_device_info_num++;

    xSemaphoreGive(g_sniffer_lock);
}

static esp_err_t mdf_get_sniffer_info(device_data_t *device_data)
{
    device_data->response_size = mdf_sniffer_get_hex((uint8_t *)device_data->response);
    MDF_ERROR_CHECK(device_data->response_size <= 0, ESP_FAIL,
                    "mdf_sniffer_get_hex, ret: %d", device_data->response_size);

    MDF_LOGD("response_size: %d", device_data->response_size);

    return MESH_PROTO_DATA_BIN;
}

esp_err_t mdf_sniffer_init()
{
    if (!g_sniffer_list) {
        g_sniffer_lock       = xSemaphoreCreateMutex();
        g_sniffer_list       = mdf_malloc(sizeof(sniffer_data_t));
        g_sniffer_list->next = NULL;
        ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(wifi_sniffer_cb));
        ESP_ERROR_CHECK(esp_wifi_set_promiscuous(1));
    }

    ESP_ERROR_CHECK(mdf_device_add_handle("get_sniffer", mdf_get_sniffer_info));

    return ESP_OK;
}

sniffer_data_t *mdf_sniffer_get()
{
    if (!g_sniffer_list || !g_sniffer_list->next) {
        return NULL;
    }

    return g_sniffer_list->next;
}

esp_err_t mdf_sniffer_get_str(char *device_info)
{
    MDF_PARAM_CHECK(device_info);

    device_info[0] = '\0';

    if (!g_sniffer_list || !g_sniffer_list->next) {
        MDF_LOGV("sniffer data");
        return ESP_OK;
    }

    xSemaphoreTake(g_sniffer_lock, portMAX_DELAY);

    sniffer_data_t *sniffer_data = g_sniffer_list->next;

    for (int i = 0; sniffer_data && i < DEVICE_INFO_NUM; sniffer_data = sniffer_data->next, i++) {
        device_info += sprintf(device_info, "%08x%02x%02x%02x%02x%02x%02x%02x%02x,",
                               sniffer_data->timestamp, MAC2STR(sniffer_data->bssid),
                               -(sniffer_data->rssi), sniffer_data->channel);
    }

    xSemaphoreGive(g_sniffer_lock);

    return ESP_OK;
}

ssize_t mdf_sniffer_get_hex(uint8_t *device_info)
{
    MDF_PARAM_CHECK(device_info);

    if (!g_sniffer_list || !g_sniffer_list->next) {
        MDF_LOGD("sniffer data NULL");
        return 0;
    }

    xSemaphoreTake(g_sniffer_lock, portMAX_DELAY);

    sniffer_hex_data_header_t *sniffer_header = (sniffer_hex_data_header_t *)device_info;

    int i              = 0;
    uint32_t timestamp = sniffer_timestamp();

    for (sniffer_data_t *sniffer_data = g_sniffer_list->next; sniffer_data && i < DEVICE_INFO_NUM;
            sniffer_data = sniffer_data->next, i++) {
        memcpy(sniffer_header->data[i].mac, sniffer_data->bssid, 6);
        sniffer_header->data[i].rssi          = sniffer_data->rssi;
        sniffer_header->data[i].channel       = sniffer_data->channel;
        sniffer_header->data[i].relative_time = timestamp - sniffer_data->timestamp;
    }

    sniffer_header->type = 1;
    sniffer_header->size = i * sizeof(sniffer_hex_data_t);
    sniffer_header->crc  = crc16_le(UINT16_MAX, (uint8_t const *)sniffer_header->data, sniffer_header->size);

    for (sniffer_data_t *sniffer_data = g_sniffer_list->next; sniffer_data;
            sniffer_data = g_sniffer_list->next) {
        g_sniffer_list->next = sniffer_data->next;
        mdf_free(sniffer_data);
    }

    sniffer_notice_percentage = DEVICE_NOTICE_PERCENTAGE;
    g_device_info_num         = 0;

    MDF_LOGV("sniffer_data: %02x %02x %02x %02x %02x %02x %02x %02x",
             device_info[0], device_info[1], device_info[2], device_info[3],
             device_info[4], device_info[5], device_info[6], device_info[7]);

    xSemaphoreGive(g_sniffer_lock);

    return sniffer_header->size + sizeof(sniffer_hex_data_header_t);
}
