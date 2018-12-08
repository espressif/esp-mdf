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

#include "mdf_common.h"
#include "mdf_espnow.h"
#include "mdf_espnow_debug.h"
#include "mdf_info_store.h"
#include "mdf_wifi_mesh.h"
#include "mdf_event_loop.h"
#include "mdf_network_config.h"

/**< in espnow_debug module, all ESP_LOG* will be redirected to espnow_debug,
     so the log of itself should be restored to other log channel, use ets_printf here */
#undef MDF_LOGE
#undef MDF_LOGW
#undef MDF_LOGI
#undef MDF_LOGD
#undef MDF_LOGV

#define MDF_LOGE( format, ... ) ets_printf("\033[31m" "mdf_espnow_debug(E)[%d]:" format "\033[0m\n", __LINE__, ##__VA_ARGS__)
#define MDF_LOGW( format, ... ) ets_printf("\033[33m" "mdf_espnow_debug(W)[%d]:" format "\033[0m\n", __LINE__, ##__VA_ARGS__)
#define MDF_LOGI( format, ... ) ets_printf("\033[32m" "mdf_espnow_debug(I)[%d]:" format "\033[0m\n", __LINE__, ##__VA_ARGS__)
#define MDF_LOGD( format, ... ) /**< no debug log in mdf_espnow module */
#define MDF_LOGV( format, ... ) /**< no verbose log in mdf_espnow module */

#define RECV_CB_OK    BIT0
#define RECV_CB_FAIL  BIT1

#define MAX_WAIT_TICKS_MS         (50)
#define ESPNOW_DEBUG_LOG_QSIZE    (10)
#define ESPNOW_COREDUMP_DATA_LEN  (200)

/**< As log transferred via ESP-NOW will influence performance of MDF network,
     therefore if the log is too long, it will be divided
     and only the first 450 Bytes will be transferred. */
#define DEBUG_MAX_DATA_LEN        (450)
/**< if pkt send fail count reach this, it is assumed that terminal has been offline */
#define MDF_ESPNOW_SEND_RETRY     (10)

typedef struct {
    esp_partition_t *coredump_p;
    size_t coredump_len;
} coredump_info_t;

static uint8_t g_send_fail_count        = 0;
static bool g_coredump_is_sending       = false;
static bool g_espnow_debug_start        = false;
static EventGroupHandle_t g_event_group = NULL;
static xQueueHandle g_log_queue         = NULL;

static uint8_t g_espnow_log_level                        = MDF_ESPNOW_LOG_NONE;
static uint8_t g_espnow_dest_mac[ESP_NOW_ETH_ALEN]       = {0x00};
static const uint8_t g_espnow_null_mac[ESP_NOW_ETH_ALEN] = {0x00};

int mdf_espnow_log_write(const char *fmt, va_list vp);

const esp_partition_t *mdf_coredump_get_info(size_t *length)
{
    MDF_ERROR_CHECK(!length, NULL, "args err, length is NULL");

    esp_err_t ret                      = ESP_OK;
    size_t coredump_len                = 0;
    esp_partition_iterator_t part_itra = NULL;

    part_itra = esp_partition_find(ESP_PARTITION_TYPE_DATA,
                                   ESP_PARTITION_SUBTYPE_DATA_COREDUMP, NULL);
    MDF_ERROR_CHECK(!part_itra, NULL, "esp_partition_find fail");

    const esp_partition_t *coredump_part = esp_partition_get(part_itra);
    MDF_ERROR_CHECK(!coredump_part, NULL, "esp_partition_get fail");

    ret = esp_partition_read(coredump_part, 4, &coredump_len, 4);
    MDF_ERROR_CHECK(ret, NULL, "esp_partition_read fail, ret: %d", ret);

    memcpy(length, &coredump_len, sizeof(size_t));

    return coredump_part;
}

esp_err_t mdf_coredump_erase(void)
{
    esp_err_t ret       = ESP_OK;
    size_t coredump_len = 0;

    const esp_partition_t *coredump_p = mdf_coredump_get_info(&coredump_len);
    MDF_ERROR_CHECK(!coredump_p, ESP_FAIL, "mdf_coredump_get_info fail, coredump_p NULL");

    if (coredump_len == -1) {
        MDF_LOGI("mdf_coredump_get_info, coredump_len == -1");
        return ESP_OK;
    }

    MDF_LOGI("addr: %d, len: %d", coredump_p->address, coredump_len);

    /**< find next partition of coredump partition */
    esp_partition_iterator_t part_itra = esp_partition_find(ESP_PARTITION_TYPE_DATA, 0x99, NULL);

    /**< for compatible with previous version */
    if (!part_itra) {
        ret = esp_partition_erase_range(coredump_p, 0, 65536);
        return ESP_OK;
    }

    const esp_partition_t *flag_part = esp_partition_get(part_itra);
    MDF_ERROR_CHECK(!flag_part, NULL, "esp_partition_get fail");

    /**< erase all coredump partition */
    ret = esp_partition_erase_range(coredump_p, 0, flag_part->address - coredump_p->address);
    MDF_ERROR_CHECK(ret, ESP_FAIL, "esp_partition_erase_range fail, ret: %d", ret);

    MDF_LOGI("coredump erase successful");

    return ESP_OK;
}

static inline esp_err_t mdf_espnow_set_config_dest(const uint8_t dest_addr[ESP_NOW_ETH_ALEN])
{
    esp_err_t ret = ESP_OK;

    /**< delete dest_addr if config to null_mac */
    if (!memcmp(dest_addr, g_espnow_null_mac, ESP_NOW_ETH_ALEN)) {
        ret = mdf_espnow_del_peer(g_espnow_dest_mac);
        MDF_ERROR_CHECK(ret, ESP_FAIL, "mdf_espnow_del_peer ret: %d", ret);

        memset(g_espnow_dest_mac, 0, ESP_NOW_ETH_ALEN);
        mdf_info_erase("dest_mac");

        g_espnow_debug_start = false;
        return ESP_OK;
    }

    ret = mdf_espnow_add_peer_no_encrypt(dest_addr);

    if (ret < 0) {
        g_espnow_debug_start = false;
        return ESP_FAIL;
    }

    memcpy(g_espnow_dest_mac, dest_addr, ESP_NOW_ETH_ALEN);

    ret = mdf_info_save("dest_mac", g_espnow_dest_mac, ESP_NOW_ETH_ALEN);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_save espnow_mac, ret: %d", ret);

    /**< if g_espnow_dest_mac exist, set g_espnow_debug_start true */
    g_espnow_debug_start = true;

    /**< reset g_send_fail_count */
    g_send_fail_count = 0;

    return ESP_OK;
}

esp_err_t mdf_espnow_set_config(const mdf_espnow_debug_pkt_t *espnow_config)
{
    esp_err_t ret = ESP_OK;

    switch (espnow_config->oprt) {
        case MDF_ESPNOW_CONFIG_LOG:
            MDF_ERROR_CHECK(espnow_config->params > MDF_ESPNOW_LOG_RESTORE,
                            ESP_FAIL, "config log_level params err");

            g_espnow_log_level = espnow_config->params;
            ret = mdf_info_save("log_level", &g_espnow_log_level, sizeof(uint8_t));
            MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_save, log_level, ret: %d", ret);

            break;

        case MDF_ESPNOW_CONFIG_DEST:
            MDF_ERROR_CHECK(espnow_config->size != ESP_NOW_ETH_ALEN || !espnow_config->data, ESP_FAIL,
                            "config dest_mac args error, size: %d, data: %p",
                            espnow_config->size, espnow_config->data);

            ret = mdf_espnow_set_config_dest(espnow_config->data);
            MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_espnow_set_config_dest fail");
            break;

        case MDF_ESPNOW_CONFIG_ALL:
            MDF_ERROR_CHECK(espnow_config->params > MDF_ESPNOW_LOG_RESTORE,
                            ESP_FAIL, "config log_level params err");

            g_espnow_log_level = espnow_config->params;
            ret = mdf_info_save("log_level", &g_espnow_log_level, sizeof(uint8_t));
            MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_save, log_level, ret: %d", ret);

            MDF_ERROR_CHECK(espnow_config->size != ESP_NOW_ETH_ALEN || !espnow_config->data, ESP_FAIL,
                            "config dest_mac args error, size: %d, data: %p",
                            espnow_config->size, espnow_config->data);

            ret = mdf_espnow_set_config_dest(espnow_config->data);
            MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_espnow_set_config_dest fail");
            break;

        default:
            MDF_LOGI("oprt err, oprt: %d", espnow_config->oprt);

            return ESP_FAIL;
    }

    return ESP_OK;
}

#define mdf_espnow_debug_send_log(send_pkt, pkt_len) mdf_espnow_debug_send(send_pkt, pkt_len, false)
#define mdf_espnow_debug_send_coredump(send_pkt, pkt_len) mdf_espnow_debug_send(send_pkt, pkt_len, true)

static esp_err_t mdf_espnow_debug_send(const mdf_espnow_debug_pkt_t *send_pkt,
                                       size_t pkt_len, bool is_coredump)
{
    MDF_ERROR_CHECK(!send_pkt || pkt_len < DEBUG_PKT_STRUCT_LEN, ESP_FAIL,
                    "args err, send_pkt: %p, pkt_len: %d", send_pkt, pkt_len);

    esp_err_t ret      = ESP_OK;
    EventBits_t uxBits = 0;

#ifdef CONFIG_MDF_ESPNOW_DEBUG_ALL_TIME
    uint8_t original_primary_channel;
    wifi_second_chan_t original_secondary_channel;
    bool channel_changed = false;

    /* we need to change channel if not
     * connected with WiFi and current channel is not the same as pre-configured channel */
    if (!mdf_wifi_mesh_is_connect()) {
        esp_wifi_get_channel(&original_primary_channel, &original_secondary_channel);

        if (original_primary_channel != CONFIG_MDF_ESPNOW_DEFAULT_DEBUG_PRIMARY_CHANNEL
                || original_primary_channel != CONFIG_MDF_ESPNOW_DEFAULT_DEBUG_SECONDARY_CHANNEL) {
            esp_wifi_set_channel(CONFIG_MDF_ESPNOW_DEFAULT_DEBUG_PRIMARY_CHANNEL,
                                 CONFIG_MDF_ESPNOW_DEFAULT_DEBUG_SECONDARY_CHANNEL);
            channel_changed = true;
        }
    }

#endif

    ret = mdf_espnow_write(MDF_ESPNOW_DEBUG, g_espnow_dest_mac, send_pkt, pkt_len, 0);

    if (ret <= 0) {
        g_send_fail_count++;
        MDF_LOGI("mdf_espnow_write fail, g_send_fail_count: %d, ret: %d", g_send_fail_count, ret);
        goto EXIT;
    }

    if (is_coredump) {
        /**< in order to ensure coredump data send ok, wait recv cb for enough time
        and it will afect transferring of espnow log */
        uxBits = xEventGroupWaitBits(g_event_group, RECV_CB_OK | RECV_CB_FAIL,
                                     pdTRUE, pdFALSE, 20 * MAX_WAIT_TICKS_MS / portTICK_RATE_MS);
        MDF_ERROR_CHECK((uxBits & RECV_CB_OK) != RECV_CB_OK, ESP_FAIL,
                        "g_event_group RECV_CB_OK fail");
    }

    g_send_fail_count = 0;

EXIT:
#ifdef CONFIG_MDF_ESPNOW_DEBUG_ALL_TIME

    if (channel_changed) {
        esp_wifi_set_channel(original_primary_channel, original_secondary_channel);
    }

#endif
    return ret;
}

void coredump_send_task(void *pvParameter)
{
    MDF_LOGI("coredump_send_task created");

    g_coredump_is_sending = true;

    coredump_info_t *coredump_info = (coredump_info_t *)pvParameter;
    esp_partition_t *coredump_p    = coredump_info->coredump_p;
    size_t coredump_len            = coredump_info->coredump_len;

    esp_err_t ret   = ESP_OK;
    size_t send_seq = 0; /**< not used yet */
    size_t read_len = 0;
    size_t rest_len = coredump_len;
    mdf_espnow_debug_pkt_t *coredump_pkt = NULL;

    do {
        /**< coredump start pkg */
        coredump_pkt = mdf_calloc(1, DEBUG_PKT_STRUCT_LEN + ESPNOW_COREDUMP_DATA_LEN);
        coredump_pkt->type   = MDF_ESPNOW_DEBUG_COREDUMP;
        coredump_pkt->oprt   = MDF_ESPNOW_COREDUMP_RSP;
        coredump_pkt->params = MDF_ESPNOW_COREDUMP_START;
        coredump_pkt->size   = coredump_len;
        coredump_pkt->seq    = (coredump_len + ESPNOW_COREDUMP_DATA_LEN - 1) / ESPNOW_COREDUMP_DATA_LEN;

        ret = mdf_espnow_debug_send_coredump(coredump_pkt, DEBUG_PKT_STRUCT_LEN);
        MDF_ERROR_GOTO(ret <= 0, EXIT, "mdf_espnow_debug_send_coredump fail, ret: %d", ret);

        for (size_t offset = 4; offset < (coredump_len + 4);
                offset += ESPNOW_COREDUMP_DATA_LEN, rest_len -= read_len) {
            read_len = MIN(ESPNOW_COREDUMP_DATA_LEN, rest_len);

            ret = esp_partition_read(coredump_p, offset, coredump_pkt->data, read_len);
            MDF_ERROR_GOTO(ret < 0, EXIT, "esp_partition_read fail, ret: %d", ret);

            /**< coredump transferring pkg */
            coredump_pkt->params = MDF_ESPNOW_COREDUMP_TRANSFERRING;
            coredump_pkt->seq    = send_seq++;
            coredump_pkt->size   = read_len;

            ret = mdf_espnow_debug_send_coredump(coredump_pkt, DEBUG_PKT_STRUCT_LEN + read_len);
            MDF_ERROR_GOTO(ret <= 0, EXIT, "mdf_espnow_debug_send_coredump fail, ret: %d", ret);
        }

        /**< coredump end pkg */
        coredump_pkt->params = MDF_ESPNOW_COREDUMP_END;

        ret = mdf_espnow_debug_send_coredump(coredump_pkt, DEBUG_PKT_STRUCT_LEN);
        MDF_ERROR_GOTO(ret <= 0, EXIT, "mdf_espnow_debug_send_coredump fail, ret: %d", ret);

        ret = mdf_coredump_erase();
        MDF_ERROR_GOTO(ret < 0, EXIT, "mdf_coredump_erase fail, ret: %d", ret);

        MDF_LOGI("coredump data send succeed, task be deleted");

    } while (0);

EXIT:
    mdf_free(coredump_info);
    mdf_free(coredump_pkt);
    g_coredump_is_sending = false;
    vTaskDelete(NULL);
}

static esp_err_t mdf_coredump_detect(void)
{
    MDF_ERROR_CHECK(!g_espnow_debug_start, ESP_FAIL, "g_espnow_debug_start not set");
    MDF_ERROR_CHECK(g_coredump_is_sending, ESP_OK, "coredump_send_task has been created");

    esp_err_t ret                  = ESP_OK;
    size_t coredump_len            = 0;
    coredump_info_t *coredump_info = NULL;

    const esp_partition_t *coredump_p = mdf_coredump_get_info(&coredump_len);
    MDF_ERROR_CHECK(!coredump_p, ESP_FAIL,
                    "mdf_coredump_get_info fail, coredump_p NULL");

    MDF_LOGI("mdf_coredump_detect coredump_p addr: %d, len: %d",
             coredump_p, coredump_len);

    if ((ssize_t)coredump_len < 1024) {
        MDF_LOGI("mdf_coredump_get_info no coredump data, len: %d", coredump_len);
        mdf_espnow_debug_pkt_t coredump_pkt = {
            .type   = MDF_ESPNOW_DEBUG_COREDUMP,
            .oprt   = MDF_ESPNOW_COREDUMP_RSP,
            .params = MDF_ESPNOW_COREDUMP_NULL,
        };

        ret = mdf_espnow_debug_send_log(&coredump_pkt, DEBUG_PKT_STRUCT_LEN);
        MDF_ERROR_CHECK(ret <= 0, ESP_FAIL, "mdf_espnow_debug_send_log fail, ret: %d", ret);

        return ESP_OK;
    }

    /**< coredump data exists */
    coredump_info               = mdf_calloc(1, sizeof(coredump_info_t));
    coredump_info->coredump_p   = (esp_partition_t *)coredump_p;
    coredump_info->coredump_len = coredump_len;

    xTaskCreate(coredump_send_task, "coredump_send_task", 2048,
                (void *)coredump_info, 6, NULL);

    return ESP_OK;
}

static esp_err_t mdf_espnow_start(void)
{
    esp_err_t ret = ESP_OK;

    g_log_queue = xQueueCreate(ESPNOW_DEBUG_LOG_QSIZE, sizeof(mdf_espnow_debug_pkt_t *));
    MDF_ERROR_CHECK(!g_log_queue, ESP_FAIL, "g_log_queue create fail");

    g_event_group = xEventGroupCreate();
    MDF_ERROR_CHECK(!g_event_group, ESP_FAIL, "g_event_group creat fail");

    /**< register espnow log redirect function */
    esp_log_set_vprintf(mdf_espnow_log_write);

    /**< load g_espnow_dest_mac, and add to espnow peer */
    mdf_info_load("log_level", &g_espnow_log_level, sizeof(uint8_t));

    ret = mdf_info_load("dest_mac", g_espnow_dest_mac, ESP_NOW_ETH_ALEN);

    if (ret < 0) {
        return ESP_OK;
    }

    ret = mdf_espnow_add_peer_no_encrypt(g_espnow_dest_mac);

    if (ret == ESP_OK) {
        MDF_LOGI("mdf_espnow_debug_init espnow_log_level: %d, espnow_dest_mac: "MACSTR"",
                 g_espnow_log_level, MAC2STR(g_espnow_dest_mac));

        g_espnow_debug_start = true;

        /**< detect coredump partition when chip start up */
        ret = mdf_coredump_detect();

        if (ret < 0) {
            MDF_LOGI("mdf_coredump_detect fail, ret: %d", ret);
            g_espnow_debug_start = false;
        }
    }

    return ESP_OK;
}

void mdf_espnow_debug_task(void *pvParameter)
{
    esp_err_t ret                            = ESP_OK;
    char nvs_key[16]                         = {0};
    uint8_t mac_addr[ESP_NOW_ETH_ALEN]       = {0};
    uint8_t *recv_data                       = mdf_calloc(1, DEBUG_MAX_DATA_LEN);
    mdf_espnow_debug_pkt_t *espnow_debug_pkt = NULL;

#ifndef CONFIG_MDF_ESPNOW_DEBUG_ALL_TIME

    while (!mdf_wifi_mesh_is_connect()) {
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

#endif

    ESP_ERROR_CHECK(mdf_espnow_start());

    while (1) {
        /**< recv from espnow log write */
        ret = xQueueReceive(g_log_queue, (void *)&espnow_debug_pkt, MAX_WAIT_TICKS_MS / portTICK_RATE_MS);

        if (ret == pdPASS && espnow_debug_pkt) {
            ret = mdf_espnow_debug_send_log(espnow_debug_pkt, espnow_debug_pkt->size + DEBUG_PKT_STRUCT_LEN);

            if (ret <= 0) {
                MDF_LOGW("mdf_espnow_debug_send_log fail, ret: %d", ret);
            }

            mdf_free(espnow_debug_pkt);
        }

        /**< read data from controller */
        ret = mdf_espnow_read(MDF_ESPNOW_DEBUG, mac_addr, recv_data, DEBUG_MAX_DATA_LEN, 0);

        if (ret <= 0) {
            continue;
        }

        /**< mdf_espnow_read successful */
        espnow_debug_pkt = (mdf_espnow_debug_pkt_t *)recv_data;

        switch (espnow_debug_pkt->type) {
            case MDF_ESPNOW_DEBUG_ACK:
                MDF_LOGI("MDF_ESPNOW_DEBUG_ACK, params: %d", espnow_debug_pkt->params);

                if (espnow_debug_pkt->params == MDF_ESPNOW_RECV_OK) {
                    xEventGroupSetBits(g_event_group, RECV_CB_OK);
                } else {
                    xEventGroupSetBits(g_event_group, RECV_CB_FAIL);
                }

                break;

            case MDF_ESPNOW_DEBUG_COREDUMP:
                MDF_LOGI("MDF_ESPNOW_DEBUG_COREDUMP, oprt: %d", espnow_debug_pkt->oprt);

                if (espnow_debug_pkt->oprt == MDF_ESPNOW_COREDUMP_DETECT ||
                        espnow_debug_pkt->oprt == MDF_ESPNOW_COREDUMP_RETRANS) {
                    ret = mdf_coredump_detect();

                    if (ret < 0) {
                        MDF_LOGW("mdf_coredump_detect error, ret: %d", ret);
                    }
                } else if (espnow_debug_pkt->oprt == MDF_ESPNOW_COREDUMP_ERASE) {
                    ret = mdf_coredump_erase();

                    if (ret < 0) {
                        MDF_LOGW("mdf_coredump_erase fail, ret: %d", ret);
                    }
                }

                break;

            case MDF_ESPNOW_DEBUG_CONFIG:
                MDF_LOGI("MDF_ESPNOW_DEBUG_CONFIG, oprt: %d", espnow_debug_pkt->oprt);

                ret = mdf_espnow_set_config(espnow_debug_pkt);

                if (ret < 0) {
                    MDF_LOGW("mdf_espnow_set_config fail, ret: %d", ret);
                }

                break;
            case MDF_ESPNOW_DEBUG_MANUAL:
                MDF_LOGI("MDF_ESPNOW_DEBUG_MANUAL, oprt: %d", espnow_debug_pkt->oprt);

                switch (espnow_debug_pkt->oprt) {
                    case MDF_ESPNOW_MANUAL_REBOOT:
                        ret = mdf_event_loop_delay_send(MDF_EVENT_SYSTEM_REBOOT, NULL, 5000);
                        if (ret < 0) {
                            MDF_LOGW("mdf_event_loop_delay_send, ret: %d", ret);
                        }
                        break;

                    case MDF_ESPNOW_MANUAL_RESET:
                        ret = mdf_coredump_erase();
                        if (ret < 0) {
                            MDF_LOGW("mdf_coredump_erase, ret: %d", ret);
                        }
                        ret = mdf_event_loop_delay_send(MDF_EVENT_SYSTEM_RESET, NULL, 5000);
                        if (ret < 0) {
                            MDF_LOGW("mdf_event_loop_delay_send, ret: %d", ret);
                        }
                        break;

                    case MDF_ESPNOW_MANUAL_CONFIG:
                        ret = mdf_network_clear_config();
                        if (ret < 0) {
                            MDF_LOGW("mdf_network_clear_config, ret: %d", ret);
                        }
                        ret = mdf_event_loop_delay_send(MDF_EVENT_SYSTEM_REBOOT, NULL, 5000);
                        if (ret < 0) {
                            MDF_LOGW("mdf_event_loop_delay_send, ret: %d", ret);
                        }
                        break;

                    case MDF_ESPNOW_MANUAL_ERASE:
                        MDF_ERROR_BREAK(espnow_debug_pkt->size >= 16, "nvs key too long: %d", espnow_debug_pkt->size);
                        memset(nvs_key, 0 , 16);
                        memcpy(nvs_key, espnow_debug_pkt->data, espnow_debug_pkt->size);
                        MDF_LOGI("mdf_info_erase key: %s", espnow_debug_pkt->data);
                        ret = mdf_info_erase(nvs_key);
                        if (ret < 0) {
                            MDF_LOGW("mdf_info_erase, ret: %d", ret);
                        }
                        break;

                    default:
                        MDF_LOGI("MDF_ESPNOW_DEBUG, type error: %d", espnow_debug_pkt->type);
                        break;
                }
                break;

            default:
                MDF_LOGI("MDF_ESPNOW_DEBUG, type error: %d", espnow_debug_pkt->type);
                break;
        }
    }

    mdf_free(recv_data);
    vTaskDelete(NULL);
}

int mdf_espnow_log_write(const char *fmt, va_list vp)
{
    int len = 0;

    /**< go to original ESP_LOG* channel on condition:
    1. espnow_debug module is not started
    2. g_espnow_log_level is set to MDF_ESPNOW_LOG_NONE
    3. g_espnow_log_level is not set or set to MDF_ESPNOW_LOG_NONE
    4. g_send_fail_count reach to MDF_ESPNOW_SEND_RETRY */
    if (!g_espnow_debug_start || g_espnow_log_level == MDF_ESPNOW_LOG_NONE
            || g_espnow_log_level == MDF_ESPNOW_LOG_RESTORE
            || g_send_fail_count >= MDF_ESPNOW_SEND_RETRY) {
        len = vprintf(fmt, vp);
        return len;
    }

    static SemaphoreHandle_t s_espnow_log_mutex = NULL;

    if (!s_espnow_log_mutex) {
        s_espnow_log_mutex = xSemaphoreCreateMutex();
    }

    if (xSemaphoreTake(s_espnow_log_mutex, 0) == pdFALSE) {
        return 0;
    }

    /**< espnow log channel */
    mdf_espnow_debug_pkt_t *espnow_log_pkt = mdf_calloc(1, DEBUG_PKT_STRUCT_LEN + DEBUG_MAX_DATA_LEN);

    len = vsnprintf((char *)espnow_log_pkt->data, DEBUG_MAX_DATA_LEN, fmt, vp);
    MDF_ERROR_GOTO(len <= 0, EXIT, "log_write vsnprintf error, len: %d", len);

    /**< check log level */
    char log_tag = espnow_log_pkt->data[0] == 0x1b ? espnow_log_pkt->data[7] : espnow_log_pkt->data[0];
    uint8_t s_log_level = MDF_ESPNOW_LOG_NONE;

    switch (log_tag) {
        case 'E':
            s_log_level = MDF_ESPNOW_LOG_ERROR;
            break;

        case 'W':
            s_log_level = MDF_ESPNOW_LOG_WARN;
            break;

        case 'I':
            s_log_level = MDF_ESPNOW_LOG_INFO;
            break;

        case 'D':
            s_log_level = MDF_ESPNOW_LOG_DEBUG;
            break;

        case 'V':
            s_log_level = MDF_ESPNOW_LOG_VERBOSE;
            break;

        default:
            s_log_level = MDF_ESPNOW_LOG_NONE;
            break;
    }

    if (!s_log_level || s_log_level > g_espnow_log_level) {
        goto EXIT;
    }

    if (len >= DEBUG_MAX_DATA_LEN) {
        len = DEBUG_MAX_DATA_LEN - 1;
        espnow_log_pkt->data[len] = '\0';
    }

    espnow_log_pkt->type = MDF_ESPNOW_DEBUG_LOG;
    espnow_log_pkt->size = len + 1;

    int ret = xQueueSend(g_log_queue, (void *)&espnow_log_pkt,
                         MAX_WAIT_TICKS_MS / portTICK_RATE_MS);
    MDF_ERROR_GOTO(ret != pdPASS, EXIT, "g_log_queue fail, ret: %d", ret);

    xSemaphoreGive(s_espnow_log_mutex);
    return len;

EXIT:
    mdf_free(espnow_log_pkt);
    xSemaphoreGive(s_espnow_log_mutex);
    return 0;
}

esp_err_t mdf_espnow_debug_init(void)
{

#ifdef CONFIG_MDF_ESPNOW_DEBUG
    static bool espnow_debug_inited = false;

    MDF_ERROR_CHECK(espnow_debug_inited, ESP_OK, "mdf_debug has been inited");

    esp_err_t ret = ESP_OK;

    ret = mdf_espnow_init();
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_espnow_init fail");

    ret = mdf_espnow_enable(MDF_ESPNOW_DEBUG);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_espnow_enable debug queue fail");

    espnow_debug_inited = true;

    xTaskCreate(mdf_espnow_debug_task, "mdf_espnow_debug_task", 2048, NULL, 6, NULL);
#endif /**< CONFIG_MDF_ESPNOW_DEBUG */

    return ESP_OK;
}
