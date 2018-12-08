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

#include "esp_wifi.h"
#include "esp_now.h"

#include "mdf_common.h"
#include "mdf_espnow.h"

/**< in espnow_debug module, all ESP_LOG* will be redirected to espnow_debug,
     so the log of itself should be restored to other log channel, use ets_printf here */
#undef MDF_LOGE
#undef MDF_LOGW
#undef MDF_LOGI
#undef MDF_LOGD
#undef MDF_LOGV

#define MDF_LOGE( format, ... ) ets_printf("\033[31m" "mdf_espnow(E)[%d]:" format "\033[0m\n", __LINE__, ##__VA_ARGS__)
#define MDF_LOGW( format, ... ) ets_printf("\033[33m" "mdf_espnow(W)[%d]:" format "\033[0m\n", __LINE__, ##__VA_ARGS__)
#define MDF_LOGI( format, ... ) ets_printf("\033[32m" "mdf_espnow(I)[%d]:" format "\033[0m\n", __LINE__, ##__VA_ARGS__)
#define MDF_LOGD( format, ... ) /**< no debug log in mdf_espnow module */
#define MDF_LOGV( format, ... ) /**< no verbose log in mdf_espnow module */

#define SEND_CB_OK               BIT0
#define SEND_CB_FAIL             BIT1
#define MDF_ESPNOW_OUI_LEN       (3)
#define MDF_ESPNOW_QUEUE_COUNT   (4)
#define MDF_ESPNOW_MAX_DATA_LEN  (ESP_NOW_MAX_DATA_LEN - sizeof(mdf_espnow_pkt_t))

static bool g_mdf_espnow_inited                    = false;
static const uint8_t g_oui[MDF_ESPNOW_OUI_LEN]     = { 0x4E, 0x4F, 0x57 }; /**< 'N', 'O', 'W' */
static const uint8_t g_null_mac[ESP_NOW_ETH_ALEN]  = { 0x00 };
static const uint8_t g_bcast_mac[ESP_NOW_ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

static EventGroupHandle_t g_event_group                        = NULL;
static xQueueHandle g_mdf_espnow_queue[MDF_ESPNOW_QUEUE_COUNT] = { NULL };
static uint8_t g_mdf_espnow_queue_size[MDF_ESPNOW_QUEUE_COUNT] = { 3, 3, 10, 1};

typedef struct {
    uint8_t oui[MDF_ESPNOW_OUI_LEN]; /**< use oui to filter unwanted espnow package */
    uint8_t mac[ESP_NOW_ETH_ALEN];   /**< mac address of source device */
    uint8_t type;                    /**< mdf espnow packet type */
    uint8_t seq;                     /**< sequence of espnow package when sending or receiving multiple package */
    uint8_t size;                    /**< data size*/
    uint8_t data[0];                 /**< data */
} mdf_espnow_pkt_t;

static void mdf_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    MDF_ERROR_CHECK(!mac_addr, ; , "args error mac_addr is NULL");

    if (status == ESP_NOW_SEND_SUCCESS) {
        xEventGroupSetBits(g_event_group, SEND_CB_OK);
    } else {
        xEventGroupSetBits(g_event_group, SEND_CB_FAIL);
    }
}

static void mdf_espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    MDF_ERROR_CHECK(!mac_addr || !data || len < sizeof(mdf_espnow_pkt_t), ; ,
                    "args error, mac_addr: %p, data: %p, len: %d", mac_addr, data, len);

    if (memcmp(data, g_oui, MDF_ESPNOW_OUI_LEN)) {
        return; /**< mdf espnow oui field err */
    }

    mdf_espnow_pkt_t *trans_pkt = (mdf_espnow_pkt_t *)data;
    MDF_ERROR_CHECK(trans_pkt->type >= MDF_ESPNOW_MAX, ; ,
                    "type error, type: %d", trans_pkt->type);

    xQueueHandle s_espnow_queue = g_mdf_espnow_queue[trans_pkt->type - 1];

    if (!s_espnow_queue) {
        return;
    }

    /**< if s_espnow_queue is full, delete the front item */
    if (!uxQueueSpacesAvailable(s_espnow_queue)
            && xQueueReceive(s_espnow_queue, (void *)&trans_pkt, 0)) {
        mdf_free(trans_pkt);
    }

    trans_pkt = mdf_malloc(len);
    memcpy(trans_pkt, data, len);
    int ret = xQueueSend(s_espnow_queue, (void *)&trans_pkt, 0);

    if (ret != pdPASS) {
        mdf_free(trans_pkt);
    }
}

esp_err_t mdf_espnow_add_peer_base(const uint8_t dest_addr[ESP_NOW_ETH_ALEN],
                                   bool default_encrypt, const uint8_t lmk[ESP_NOW_KEY_LEN])
{
    MDF_ERROR_CHECK(!dest_addr, ESP_FAIL, "args err, dest_addr is NULL");
    MDF_ERROR_CHECK(!memcmp(dest_addr, g_null_mac, ESP_NOW_ETH_ALEN), ESP_FAIL,
                    "args err, dest_addr is g_null_mac");

    static SemaphoreHandle_t s_addpeer_mutex = NULL;

    if (!s_addpeer_mutex) {
        s_addpeer_mutex = xSemaphoreCreateMutex();
    }

    xSemaphoreTake(s_addpeer_mutex, portMAX_DELAY);

    if (esp_now_is_peer_exist(dest_addr)) {
        ESP_ERROR_CHECK(esp_now_del_peer(dest_addr));
    }

    esp_err_t ret                  = ESP_OK;
    uint8_t pri_channel            = 0;
    wifi_second_chan_t sec_channel = 0;
    ESP_ERROR_CHECK(esp_wifi_get_channel(&pri_channel, &sec_channel));

    esp_now_peer_info_t *peer = mdf_calloc(1, sizeof(esp_now_peer_info_t));
    peer->channel             = pri_channel;
    peer->ifidx               = MDF_ESPNOW_INTERFACE;

    if (!memcmp(dest_addr, g_bcast_mac, ESP_NOW_ETH_ALEN)) {
        /**< broadcast addr cannot encrypt */
        peer->encrypt = false;
    } else {
        if (default_encrypt) {
            peer->encrypt = true;
            memcpy(peer->lmk, MDF_ESPNOW_LMK, ESP_NOW_KEY_LEN);
        } else if (lmk) {
            peer->encrypt = true;
            memcpy(peer->lmk, lmk, ESP_NOW_KEY_LEN);
        } else {
            peer->encrypt = false;
        }
    }

    memcpy(peer->peer_addr, dest_addr, ESP_NOW_ETH_ALEN);
    ret = esp_now_add_peer(peer);
    mdf_free(peer);

    if (ret) {
        MDF_LOGW("esp_now_add_peer fail, ret: %d", ret);
    }

    xSemaphoreGive(s_addpeer_mutex);
    return ESP_OK;
}

esp_err_t mdf_espnow_del_peer(const uint8_t dest_addr[ESP_NOW_ETH_ALEN])
{
    MDF_ERROR_CHECK(!dest_addr, ESP_FAIL, "args err, dest_addr is NULL");

    esp_err_t ret = ESP_OK;

    if (esp_now_is_peer_exist(dest_addr)) {
        ret = esp_now_del_peer(dest_addr);
        MDF_ERROR_CHECK(ret, ESP_FAIL, "esp_now_del_peer fail, ret: %d", ret);
    }

    return ESP_OK;
}

size_t mdf_espnow_read(mdf_espnow_pkt_type_t type, uint8_t src_addr[ESP_NOW_ETH_ALEN],
                       void *data, size_t data_len, TickType_t block_ticks)
{
    MDF_ERROR_CHECK(type >= MDF_ESPNOW_MAX || !src_addr || !data || data_len <= 0,
                    ESP_FAIL, "args err,  type: %d, src_addr: %p, data: %p, data_len: %d",
                    type, src_addr, data, data_len);

    size_t ret                  = 0;
    mdf_espnow_pkt_t *trans_pkt = NULL;
    xQueueHandle s_espnow_queue = g_mdf_espnow_queue[type - 1];
    uint32_t end_timer          = xTaskGetTickCount() + block_ticks;

    if (!s_espnow_queue) {
        return ESP_FAIL;
    }

    ret = xQueueReceive(s_espnow_queue, (void *)&trans_pkt, block_ticks);

    if (ret != pdPASS || !trans_pkt) {
        return ESP_FAIL;
    }

    /**< not head pkt, drop it */
    if (!memcmp(trans_pkt->mac, g_null_mac, ESP_NOW_ETH_ALEN)) {
        mdf_free(trans_pkt);
        return ESP_FAIL;
    }

    int real_len      = 0;
    int recv_data_len = 0;
    int pkt_count     = trans_pkt->seq;

    uint8_t *recv_data = mdf_calloc(1, (pkt_count + 1) * MDF_ESPNOW_MAX_DATA_LEN);
    memcpy(src_addr, trans_pkt->mac, ESP_NOW_ETH_ALEN);
    memcpy(recv_data, trans_pkt->data, trans_pkt->size);
    recv_data_len += trans_pkt->size;
    mdf_free(trans_pkt);

    /**< if multiple pkt exist */
    for (int i = 0; i < pkt_count && xTaskGetTickCount() < end_timer; i++) {
        block_ticks = end_timer - xTaskGetTickCount();

        if (xQueueReceive(s_espnow_queue, (void *)&trans_pkt, block_ticks) != pdTRUE) {
            goto EXIT;
        }

        memcpy(&recv_data[recv_data_len], trans_pkt->data, trans_pkt->size);
        recv_data_len += trans_pkt->size;
        mdf_free(trans_pkt);
    }

EXIT:
    real_len = MIN(recv_data_len, data_len);
    memcpy(data, recv_data, real_len);
    mdf_free(recv_data);

    return real_len;
}

size_t mdf_espnow_write(mdf_espnow_pkt_type_t type, const uint8_t dest_addr[ESP_NOW_ETH_ALEN],
                        const void *data, size_t data_len, TickType_t block_ticks)
{
    MDF_ERROR_CHECK(type >= MDF_ESPNOW_MAX || !dest_addr || !data || data_len <= 0,
                    ESP_FAIL, "args err, type: %d, dest_addr: %p, data: %p, data_len: %d",
                    type, dest_addr, data, data_len);

    static SemaphoreHandle_t s_send_mutex = NULL;

    if (!s_send_mutex) {
        s_send_mutex = xSemaphoreCreateMutex();
    }

    if (xSemaphoreTake(s_send_mutex, block_ticks) != pdPASS) {
        return ESP_FAIL;
    }

    size_t ret         = ESP_OK;
    EventBits_t uxBits = 0;
    int write_len      = 0;
    int pkt_len        = 0;
    int rest_len       = data_len;
    int pkt_count      = (data_len + MDF_ESPNOW_MAX_DATA_LEN - 1) / MDF_ESPNOW_MAX_DATA_LEN;

    mdf_espnow_pkt_t *trans_pkt = mdf_calloc(1, ESP_NOW_MAX_DATA_LEN);

    for (int i = 0; i < pkt_count; i++) {
        /**< only first pkt have mac field */
        if (i == 0) {
            ESP_ERROR_CHECK(esp_wifi_get_mac(MDF_ESPNOW_INTERFACE, trans_pkt->mac));
        }

        pkt_len         = MIN(rest_len, MDF_ESPNOW_MAX_DATA_LEN);
        trans_pkt->type = type;
        trans_pkt->seq  = (pkt_count - 1) - i;
        trans_pkt->size = pkt_len;
        memcpy(trans_pkt->oui, g_oui, MDF_ESPNOW_OUI_LEN);
        memcpy(trans_pkt->data, data + (i * MDF_ESPNOW_MAX_DATA_LEN), pkt_len);

        xEventGroupClearBits(g_event_group, SEND_CB_OK | SEND_CB_FAIL);

        /**< espnow send and wait cb */
        ret = esp_now_send(dest_addr, (uint8_t *)trans_pkt, pkt_len + sizeof(mdf_espnow_pkt_t));
        MDF_ERROR_GOTO(ret, EXIT, "esp_now_send fail, ret: %x", ret);

        uxBits = xEventGroupWaitBits(g_event_group, SEND_CB_OK | SEND_CB_FAIL,
                                     pdTRUE, pdFALSE, portMAX_DELAY);
        MDF_ERROR_GOTO((uxBits & SEND_CB_OK) != SEND_CB_OK, EXIT,
                       "g_event_group wait SEND_CB_OK fail");

        /**< update data len */
        rest_len  -= pkt_len;
        write_len += pkt_len;
        memset(trans_pkt, 0, ESP_NOW_MAX_DATA_LEN);
    }

EXIT:
    mdf_free(trans_pkt);
    xSemaphoreGive(s_send_mutex);

    return write_len;
}

esp_err_t mdf_espnow_deinit(void)
{
    MDF_ERROR_CHECK(!g_mdf_espnow_inited, ESP_FAIL, "mdf_espnow not inited");

    esp_err_t ret = ESP_OK;

    for (int i = 0; i < MDF_ESPNOW_QUEUE_COUNT; ++i) {
        ret = mdf_espnow_disable(i + 1);
        MDF_ERROR_CHECK(ret, ESP_FAIL, "mdf_espnow_disable fail, type: %d", i + 1);
    }

    ESP_ERROR_CHECK(esp_now_unregister_recv_cb());
    ESP_ERROR_CHECK(esp_now_unregister_send_cb());
    ESP_ERROR_CHECK(esp_now_deinit());
    vEventGroupDelete(g_event_group);
    g_event_group = NULL;

    g_mdf_espnow_inited = false;
    return ESP_OK;
}

esp_err_t mdf_espnow_init(void)
{
    if (g_mdf_espnow_inited) {
        return ESP_OK;
    }

    /**< event group for espnow sent cb */
    g_event_group = xEventGroupCreate();
    MDF_ERROR_CHECK(!g_event_group, ESP_FAIL, "creat g_event_group fail");

    /**< init espnow, register cb and set pmk*/
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(mdf_espnow_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(mdf_espnow_recv_cb));
    ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t *)MDF_ESPNOW_PMK));

    g_mdf_espnow_inited = true;
    return ESP_OK;
}

esp_err_t mdf_espnow_disable(mdf_espnow_pkt_type_t type)
{
    MDF_ERROR_CHECK(!g_mdf_espnow_inited, ESP_FAIL, "mdf_espnow not inited");
    MDF_ERROR_CHECK(type >= MDF_ESPNOW_MAX, ESP_FAIL, "type err, type: %d", type);

    mdf_espnow_pkt_t *trans_pkt = NULL;

    if (g_mdf_espnow_queue[type - 1]) {
        while (xQueueReceive(g_mdf_espnow_queue[type - 1], (void *)&trans_pkt, 0)) {
            mdf_free(trans_pkt);
        }

        vQueueDelete(g_mdf_espnow_queue[type - 1]);
        g_mdf_espnow_queue[type - 1] = NULL;
    }

    return ESP_OK;
}

esp_err_t mdf_espnow_enable(mdf_espnow_pkt_type_t type)
{
    MDF_ERROR_CHECK(!g_mdf_espnow_inited, ESP_FAIL, "mdf_espnow not inited");
    MDF_ERROR_CHECK(type >= MDF_ESPNOW_MAX, ESP_FAIL, "type err, type: %d", type);

    if (!g_mdf_espnow_queue[type - 1]) {
        g_mdf_espnow_queue[type - 1] = xQueueCreate(g_mdf_espnow_queue_size[type - 1],
                                       sizeof(mdf_espnow_pkt_t *));
        MDF_ERROR_CHECK(!g_mdf_espnow_queue[type - 1], ESP_FAIL,
                        "create g_espnow_debug_queue fail, type: %d", type);
    }

    return ESP_OK;
}
