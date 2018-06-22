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
#include "mdf_ota.h"
#include "mdf_socket.h"
#include "mdf_http.h"
#include "mdf_info_store.h"
#include "mdf_event_loop.h"
#include "mdf_wifi_mesh.h"

typedef struct {
    uint8_t  tag[4];
    uint16_t seq;
    uint16_t size;
    char     data[0];
} mdf_ota_packet_head_t;

typedef struct {
    char     version[64];
    uint16_t packet_num;
    uint16_t packet_size;
    uint16_t packet_write_num;
    uint8_t  progress_array[0];
} mdf_ota_status_t;

#define MDF_OTA_STORE_KEY          "ota_status"
#define MDF_OTA_PACKET_MAX_NUM     1024
#define MDF_OTA_PACKET_HEAD_SIZE   sizeof(mdf_ota_packet_head_t)
#define MDF_OTA_STATUS_SIZE        (sizeof(mdf_ota_status_t) + MDF_OTA_PACKET_MAX_NUM)
#define MDF_UPGRADE_STATE_SAVE_CNT 50

static const char *TAG                = "mdf_ota_upgrade";
static mdf_ota_status_t *g_ota_status = NULL;
static bool ota_upgrade_data_finish   = false;

esp_err_t mdf_upgrade_init(ssize_t ota_bin_len, ssize_t ota_package_len,
                           const char *ota_bin_verson)
{
    MDF_PARAM_CHECK(ota_bin_len);
    MDF_PARAM_CHECK(ota_package_len);
    MDF_PARAM_CHECK(ota_bin_verson);

    esp_err_t ret            = ESP_OK;
    uint16_t packet_body_size = ota_package_len - MDF_OTA_PACKET_HEAD_SIZE;

    MDF_LOGD("ota_bin_len: %d, ota_package_len: %d, ota_bin_verson: %s",
             ota_bin_len, ota_package_len, ota_bin_verson);

    if (!g_ota_status) {
        g_ota_status = mdf_calloc(1, MDF_OTA_STATUS_SIZE);
        mdf_info_load(MDF_OTA_STORE_KEY, g_ota_status, MDF_OTA_STATUS_SIZE);

        ret = mdf_event_loop_send(MDF_EVENT_UPGRADE_START, NULL);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_event_loop_send, ret: %d", ret);
    }

    /**< if g_ota_status has been created and
         once again upgrade the same version bin, just return ESP_OK */
    if (!strncmp(g_ota_status->version, ota_bin_verson, strlen(ota_bin_verson))) {
        return ESP_OK;
    }

    strncpy(g_ota_status->version, ota_bin_verson, sizeof(g_ota_status->version));
    g_ota_status->packet_size      = ota_package_len;
    g_ota_status->packet_num       = (ota_bin_len + packet_body_size - 1) / packet_body_size;
    g_ota_status->packet_write_num = 0;
    memset(g_ota_status->progress_array, false, MDF_OTA_PACKET_MAX_NUM);
    ota_upgrade_data_finish        = false;

    /**< in some case, the time-cost of flash erase in esp_ota_begin (called by
         mdf_ota_start) can be too long to expire the mdf connection */
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(60));
    ret = mdf_ota_start();
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(WIFI_MESH_AP_ASSOC_EXPIRE));
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_ota_start, ret: %d", ret);

    ret = mdf_info_save(MDF_OTA_STORE_KEY, g_ota_status, MDF_OTA_STATUS_SIZE);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_save, ret: %d", ret);

    return ESP_OK;
}

esp_err_t mdf_upgrade_deinit(void)
{
    MDF_PARAM_CHECK(g_ota_status);

    MDF_ERROR_CHECK(g_ota_status->packet_write_num < g_ota_status->packet_num, ESP_FAIL,
                    "the bin file received is incomplete, packet_write_num: %d, packet_num: %d",
                    g_ota_status->packet_write_num, g_ota_status->packet_num);

    esp_err_t ret = ESP_OK;

    ret = mdf_ota_stop();

    if (ret < 0) {
        MDF_LOGE("mdf_ota_stop, ret: %d", ret);

        ret = mdf_event_loop_send(MDF_EVENT_UPGRADE_FAIL, NULL);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_event_loop_send, ret: %d", ret);

        return ESP_FAIL;
    }

    mdf_info_erase(MDF_OTA_STORE_KEY);
    mdf_free(g_ota_status);

    ret = mdf_event_loop_send(MDF_EVENT_UPGRADE_SUCCESS, NULL);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_event_loop_send, ret: %d", ret);

    return ESP_OK;
}

esp_err_t mdf_upgrade_status(uint8_t **progress_array, ssize_t *writted_num,
                             ssize_t *packet_num, ssize_t *packet_size)
{
    MDF_PARAM_CHECK(progress_array);
    MDF_PARAM_CHECK(packet_num);
    MDF_PARAM_CHECK(packet_size);
    MDF_PARAM_CHECK(g_ota_status);

    /**< the progress_array indicates whether packets have been received */
    *progress_array = g_ota_status->progress_array;
    *packet_num     = g_ota_status->packet_num;
    *writted_num    = g_ota_status->packet_write_num;
    *packet_size    = g_ota_status->packet_size;

    return ESP_OK;
}

esp_err_t mdf_upgrade_write(const void *ota_data, ssize_t ota_data_size)
{
    MDF_PARAM_CHECK(ota_data);
    MDF_PARAM_CHECK(ota_data_size);

    if (!g_ota_status) {
        g_ota_status = mdf_calloc(1, MDF_OTA_STATUS_SIZE);
        esp_err_t ret  = mdf_info_load(MDF_OTA_STORE_KEY, g_ota_status, MDF_OTA_STATUS_SIZE);

        if (ret < 0) {
            MDF_LOGW("mdf_upgrade_write, ret: %d", ret);
            mdf_free(g_ota_status);
            g_ota_status = NULL;
            return ESP_FAIL;
        }

        ret = mdf_event_loop_send(MDF_EVENT_UPGRADE_START, NULL);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_event_loop_send, ret: %d", ret);
    }

    if (ota_upgrade_data_finish) {
        static uint8_t s_count = 0;

        if (!(s_count++ % 10)) {
            esp_err_t ret = mdf_event_loop_send(MDF_EVENT_UPGRADE_DATA_FINISH, NULL);
            MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_event_loop_send, ret: %d", ret);
            s_count = 0;
        }

        MDF_LOGD("upgrade data has been received");

        return ESP_OK;
    }

    esp_err_t ret                  = ESP_OK;
    mdf_ota_packet_head_t *ota_bin = (mdf_ota_packet_head_t *)ota_data;
    uint16_t packet_body_size       = g_ota_status->packet_size - MDF_OTA_PACKET_HEAD_SIZE;

    MDF_ERROR_CHECK(ota_bin->seq >= g_ota_status->packet_num, ESP_FAIL,
                    "seq overflow, seq: %d, packet_num: %d", ota_bin->seq, g_ota_status->packet_num);

    MDF_LOGI("send_seq: %d, recved_num: %d, pkt_len: %d, progress: %03d%%",
             ota_bin->seq, g_ota_status->packet_write_num, ota_bin->size,
             g_ota_status->packet_write_num * 100 / g_ota_status->packet_num);

    /**< received a duplicate packet */
    if (g_ota_status->progress_array[ota_bin->seq] == true) {
        MDF_LOGD("received a duplicate packet");

        /**< check whether the upgrade data is received */
        for (int i = 0; i < g_ota_status->packet_num; ++i) {
            if (g_ota_status->progress_array[i] == false) {
                return ESP_OK;
            }
        }

        ret = mdf_event_loop_send(MDF_EVENT_UPGRADE_DATA_FINISH, NULL);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_event_loop_send, ret: %d", ret);

        ota_upgrade_data_finish = true;

        return ESP_OK;
    }

    ret = mdf_ota_write(ota_bin->data, ota_bin->size, ota_bin->seq * packet_body_size);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_ota_write fail, ret: %d, ota_bin->seq: %d",
                    ret, ota_bin->seq);

    /**< update g_ota_status */
    g_ota_status->progress_array[ota_bin->seq] = true;
    g_ota_status->packet_write_num++;

    /**< save OTA status periodically, it can be used to
         resumable data transfers from breakpoint after system reset */
    static uint16_t s_packet_write_num = MDF_UPGRADE_STATE_SAVE_CNT;

    if (g_ota_status->packet_write_num == s_packet_write_num
            || g_ota_status->packet_write_num == g_ota_status->packet_num) {
        s_packet_write_num += MDF_UPGRADE_STATE_SAVE_CNT;
        ret = mdf_info_save(MDF_OTA_STORE_KEY, g_ota_status, MDF_OTA_STATUS_SIZE);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_save, ret: %d", ret);
        MDF_LOGD("save the data of ota status to flash");
    }

    return ESP_OK;
}
