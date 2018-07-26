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

#include "espnow_lcd.h"
#include "espnow_sdcard.h"

#define ESPNOW_WRITE_TIMEOUT (500 / portTICK_RATE_MS)
#define RECV_MAX_WAITS_MS    (100 / portTICK_RATE_MS)
#define RECV_MAX_DATA_LEN    (2000)

#define MAX_RECEIVER_NUM     (20)
#define RETRY_COUNT          (3)

#define ESPNOW_RECV_LOG(dut_mac, length, data) \
    ESP_LOGI("dut_recv", "[%02x:%02x:%02x:%02x:%02x:%02x][%u] %s", MAC2STR(dut_mac), length, (char *) data)
#define ESPNOW_RECV_CORE_DUMP(dut_mac, length, data) \
    ESP_LOGI("dut_core_dump", "[%02x:%02x:%02x:%02x:%02x:%02x][%u] %s", MAC2STR(dut_mac), length, (char *) data)

static uint8_t g_dev_num                                     = 0;
static const uint8_t g_null_mac[ESP_NOW_ETH_ALEN]            = { 0 };
static uint32_t g_dev_runtime[MAX_RECEIVER_NUM]              = { 0 };
static uint8_t g_dev_mac[MAX_RECEIVER_NUM][ESP_NOW_ETH_ALEN] = { { 0 }, };

static const char *TAG = "espnow_recv_handle";

static esp_err_t espnow_log_analysis(const uint8_t *mac, const uint8_t *data, size_t len)
{
    static uint32_t mdf_runtime = 0;

    /**< get log tag */
    uint8_t log_tag_index = (data[0] == 0x1b) ? 7 : 0;
    char log_tag = data[log_tag_index];

    switch (log_tag) {
        case 'E':
            espnow_lcd_ana_write(ESPNOW_NET_ANA_ERR, NULL, 0);
            return ESP_OK;

        case 'W':
            espnow_lcd_ana_write(ESPNOW_NET_ANA_WARN, NULL, 0);
            return ESP_OK;

        default:
            break;
    }

    /**< run time tag */
    uint32_t runtime = 0;

    for (uint8_t index = log_tag_index + 3; data[index] != ')'; ++index) {
        runtime = runtime * 10 + data[index] - '0';
    }

    /**< update global variables */
    for (int i = 0; i < MAX_RECEIVER_NUM; i++) {
        /**< first recv device */
        if (g_dev_num == 0) {
            memcpy(g_dev_mac[0], mac, ESP_NOW_ETH_ALEN);
            g_dev_runtime[0] = runtime;
            g_dev_num++;

            espnow_lcd_ana_write(ESPNOW_NET_ANA_DEV_NUM, &g_dev_num, 0);
            break;
        }

        /**< new device */
        if (!memcmp(g_dev_mac[i], g_null_mac, ESP_NOW_ETH_ALEN)) {
            memcpy(g_dev_mac[i], mac, ESP_NOW_ETH_ALEN);
            g_dev_runtime[i] = runtime;
            g_dev_num++;

            espnow_lcd_ana_write(ESPNOW_NET_ANA_DEV_NUM, &g_dev_num, 0);
            break;
        }

        /**< update device's runtime */
        if (!memcmp(g_dev_mac[i], mac, ESP_NOW_ETH_ALEN)) {
            if (g_dev_runtime[i] > runtime) {
                /**< recv runtime less than record, treat ad reboot */
                espnow_lcd_ana_write(ESPNOW_NET_ANA_REBOOT_NUM, NULL, 0);
                g_dev_runtime[i] = 0;
            } else {
                g_dev_runtime[i] = runtime;
            }

            break;
        }
    }

    /**< update network runtime at least 10s */
    if (runtime > mdf_runtime + 10 * 1000) {
        espnow_lcd_ana_write(ESPNOW_NET_ANA_RUNTIME, (uint8_t *)&runtime, 0);
        mdf_runtime = runtime;
    }

    return ESP_OK;
}

static void espnow_recv_handle_task(void *arg)
{
    MDF_LOGI("espnow_recv_handle_task created");

    esp_err_t ret                      = 0;
    size_t coredump_total_len          = 0;
    size_t coredump_recv_len           = 0;
    uint8_t retry_count                = 0;
    uint8_t mac_addr[ESP_NOW_ETH_ALEN] = { 0 };
    uint8_t *recv_data                 = mdf_calloc(1, RECV_MAX_DATA_LEN);

    mdf_espnow_debug_pkt_t *response         = mdf_calloc(1, RECV_MAX_DATA_LEN);
    mdf_espnow_debug_pkt_t *espnow_debug_pkt = NULL;

    for (;;) {
        memset(recv_data, 0, RECV_MAX_DATA_LEN);
        memset(response, 0, RECV_MAX_DATA_LEN);

        if (mdf_espnow_read(MDF_ESPNOW_DEBUG, mac_addr, recv_data,
                            RECV_MAX_DATA_LEN, portMAX_DELAY) <= 0) {
            continue;
        }

        espnow_debug_pkt = (mdf_espnow_debug_pkt_t *)recv_data;

        if (espnow_debug_pkt->type == MDF_ESPNOW_DEBUG_LOG && espnow_debug_pkt->size > 0) {
            ESPNOW_RECV_LOG(mac_addr, espnow_debug_pkt->size, espnow_debug_pkt->data);


#ifdef CONFIG_SHOW_IN_LCD
            ret = espnow_log_analysis(mac_addr, espnow_debug_pkt->data, espnow_debug_pkt->size);
            MDF_ERROR_CONTINUE(ret, "espnow_log_analysis ret: %d", ret);
#endif /**< CONFIG_SHOW_IN_LCD */

#ifdef CONFIG_SAVE_TO_SDCARD
            ret = espnow_sd_log_write(mac_addr, (char *)espnow_debug_pkt->data);

            if (ret < 0) {
                MDF_LOGW("espnow_sd_log_write, ret: %d", ret);
            }

#endif /**< CONFIG_SAVE_TO_SDCARD */

        }

        if (espnow_debug_pkt->type == MDF_ESPNOW_DEBUG_COREDUMP) {
            if (!esp_now_is_peer_exist(mac_addr)) {
                mdf_espnow_add_peer_no_encrypt(mac_addr);
            }

            switch (espnow_debug_pkt->params) {
                case MDF_ESPNOW_COREDUMP_NULL: {
                    ESPNOW_RECV_CORE_DUMP(mac_addr, sizeof("devices has no coredump data"),
                                          "devices has no coredump data");
                    break;
                }

                case MDF_ESPNOW_COREDUMP_START: {
                    if (!esp_now_is_peer_exist(mac_addr)) {
                        mdf_espnow_add_peer_no_encrypt(mac_addr);
                    }

                    ESPNOW_RECV_CORE_DUMP(mac_addr, sizeof("start recv coredump"), "start recv coredump");

#ifdef CONFIG_SAVE_TO_SDCARD
                    /**< del exit device's correspondding coredump */
                    ret = espnow_sd_coredump_write(mac_addr, NULL, 0, ESPNOW_SDCARD_DEL_TEM);

                    if (ret < 0) {
                        MDF_LOGW("espnow_sd_coredump_write MDF_ESPNOW_COREDUMP_START, ret: %d", ret);
                    }

#endif /**< CONFIG_SAVE_TO_SDCARD */

                    coredump_total_len = espnow_debug_pkt->size;
                    coredump_recv_len  = 0;
                    response->type     = MDF_ESPNOW_DEBUG_ACK;
                    response->params   = MDF_ESPNOW_RECV_OK;
                    ret = mdf_espnow_write(MDF_ESPNOW_DEBUG, mac_addr, response,
                                           sizeof(mdf_espnow_debug_pkt_t), ESPNOW_WRITE_TIMEOUT);

                    if (ret < 0) {
                        MDF_LOGW("mdf_espnow_write coredump start fail, ret: %d", ret);
                    }

                    break;
                }


                case MDF_ESPNOW_COREDUMP_TRANSFERRING: {
                    ESPNOW_RECV_CORE_DUMP(mac_addr, sizeof("transferring coredump"),
                                          "transferring coredump");

                    coredump_recv_len += espnow_debug_pkt->size;
                    response->type   = MDF_ESPNOW_DEBUG_ACK;
                    response->params = MDF_ESPNOW_RECV_OK;

#ifdef CONFIG_SAVE_TO_SDCARD
                    /**< save date to sdcard temp file */
                    ret = espnow_sd_coredump_write(mac_addr, espnow_debug_pkt->data, espnow_debug_pkt->size, ESPNOW_SDCARD_WRITE_TEM);

                    if (ret < 0) {
                        MDF_LOGW("espnow_sd_coredump_write MDF_ESPNOW_COREDUMP_TRANSFERRING, ret: %d", ret);
                    }

#endif /**< CONFIG_SAVE_TO_SDCARD */

                    ret = mdf_espnow_write(MDF_ESPNOW_DEBUG, mac_addr, response,
                                           sizeof(mdf_espnow_debug_pkt_t), ESPNOW_WRITE_TIMEOUT);

                    if (ret < 0) {
                        MDF_LOGW("mdf_espnow_write coredump tranfering fail, ret: %d", ret);
                    }

                    break;
                }

                case MDF_ESPNOW_COREDUMP_END: {
                    ESPNOW_RECV_CORE_DUMP(mac_addr, sizeof("end recv coredump"),
                                          "end recv coredump");

                    if (coredump_total_len != coredump_recv_len) {
                        retry_count++;

                        if (retry_count >= RETRY_COUNT) {
                            MDF_LOGE("============================================================");
                            MDF_LOGE("coredump data not perfect, retry_count reach RETRY_COUNT 5");
                            MDF_LOGE("============================================================");
                            coredump_recv_len = 0;
                            retry_count       = 0;
                            break;
                        }

                        MDF_LOGE("============================================================");
                        MDF_LOGE("coredump data not perfect, retry count: %d, total len: %d, recv len: %d",
                                 retry_count, coredump_total_len, coredump_recv_len);
                        MDF_LOGE("============================================================");

                        response->type = MDF_ESPNOW_DEBUG_COREDUMP;
                        response->oprt = MDF_ESPNOW_COREDUMP_RETRANS;
                    } else {
                        MDF_LOGI("============================================================");
                        MDF_LOGI("coredump data perfect, retry count: %d, total len: %d, recv len: %d",
                                 retry_count, coredump_total_len, coredump_recv_len);
                        MDF_LOGI("============================================================");
                        retry_count      = 0;
                        response->type   = MDF_ESPNOW_DEBUG_ACK;
                        response->params = MDF_ESPNOW_RECV_OK;

#ifdef CONFIG_SAVE_TO_SDCARD
                        /**< save to sdcard */
                        ret = espnow_sd_coredump_write(mac_addr, espnow_debug_pkt->data, espnow_debug_pkt->size, ESPNOW_SDCARD_SAVE);

                        if (ret < 0) {
                            MDF_LOGW("espnow_sd_coredump_write MDF_ESPNOW_COREDUMP_END, ret: %d", ret);
                        }

#endif /**< CONFIG_SAVE_TO_SDCARD */

#ifdef CONFIG_SHOW_IN_LCD
                        /**< update lcd mdf info */
                        espnow_lcd_ana_write(ESPNOW_NET_ANA_DUMP_NUM, NULL, 0);
#endif /**< CONFIG_SHOW_IN_LCD */

                    }

                    coredump_recv_len = 0;
                    ret = mdf_espnow_write(MDF_ESPNOW_DEBUG, mac_addr, response,
                                           sizeof(mdf_espnow_debug_pkt_t), ESPNOW_WRITE_TIMEOUT);

                    if (ret < 0) {
                        MDF_LOGW("mdf_espnow_write coredump end fail,, ret: %d", ret);
                    }

                    break;
                }

                default:
                    MDF_LOGW("end recv coredump data, state error");
                    break;
            }
        }
    }

    mdf_free(response);
    mdf_free(recv_data);
    vTaskDelete(NULL);
}

esp_err_t espnow_handle_init(void)
{
    esp_err_t ret = ESP_OK;

    ret = mdf_espnow_init();
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "init fail");

    ret = mdf_espnow_enable(MDF_ESPNOW_DEBUG);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "enable MDF_ESPNOW_DEBUG fail");

    xTaskCreate(espnow_recv_handle_task, "espnow_recv_handle_task", 1024 * 3, NULL, 6, NULL);

    return ESP_OK;
}
