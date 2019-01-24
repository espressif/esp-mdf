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
#include "miniz.h"
#include "mbedtls/md5.h"
#include "mbedtls/aes.h"

#include "mdf_common.h"
#include "mespnow.h"
#include "mconfig_security.h"
#include "mconfig_queue.h"
#include "mconfig_chain.h"

#define VENDOR_OUI_TYPE_CONFIG       (0x0F)
#define MCONFIG_CHAIN_EXIT_DELAY     (100)
#define MCONFIG_CHAIN_SEND_RETRY_NUM (3)

/**
 * @brief Used to save wifi scanned information
 */
typedef struct {
    int8_t rssi;                   /**< Received Signal Strength Indicator(RSSI) of packet. unit: dBm */
    uint8_t bssid[MWIFI_ADDR_LEN]; /**< MAC address of AP */
} mconfig_scan_info_t;

/**
 * @brief Host and slave communication data
 */
typedef struct {
    uint8_t aes_key[MCONFIG_AES_KEY_LEN]; /**< The encryption key */
    mconfig_data_t mconfig_data;          /**< Mesh network configuration information */
} mconfig_chain_data_t;

static const char *TAG                    = "mconfig_chain";
static bool g_chain_slave_flag            = false;
static bool g_switch_channel_flag         = false;
static QueueHandle_t g_mconfig_scan_queue = NULL;
static SemaphoreHandle_t g_chain_slave_exit_sem = NULL;

static uint32_t g_chain_master_duration_ticks          = 0;
static SemaphoreHandle_t g_chain_master_exit_sem       = NULL;

static const uint8_t MDF_VENDOR_OUI[3] = {0x18, 0xFE, 0x34};
static const uint8_t MCONFIG_AES_CFB_IV[MCONFIG_AES_KEY_LEN] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

static void wifi_sniffer_cb(void *recv_buf, wifi_promiscuous_pkt_type_t type)
{
    if (type != WIFI_PKT_MGMT) {
        return;
    }

    wifi_promiscuous_pkt_t *promiscuous_pkt = (wifi_promiscuous_pkt_t *)recv_buf;
    uint8_t *payload                        = promiscuous_pkt->payload;

    for (int i = 36; i < promiscuous_pkt->rx_ctrl.sig_len; i += payload[i + 1] + 2) {
        vendor_ie_data_t *ie_data = (vendor_ie_data_t *)(payload + i);

        if (ie_data->element_id == WIFI_VENDOR_IE_ELEMENT_ID && ie_data->length == 4
                && !memcmp(ie_data->vendor_oui, MDF_VENDOR_OUI, sizeof(ie_data->vendor_oui))
                && ie_data->vendor_oui_type == VENDOR_OUI_TYPE_CONFIG) {
            mconfig_scan_info_t scan_info = {0};

            /**< If g_mconfig_scan_queue is full, delete the front item */
            if (!uxQueueSpacesAvailable(g_mconfig_scan_queue)) {
                xQueueReceive(g_mconfig_scan_queue, &scan_info, 0);
            }

            scan_info.rssi = promiscuous_pkt->rx_ctrl.rssi;
            memcpy(scan_info.bssid, payload + 10, MWIFI_ADDR_LEN);
            xQueueSend(g_mconfig_scan_queue, &scan_info, 0);
        }
    }
}

static bool mconfig_device_verify(mconfig_whitelist_t *whitelist_data, size_t whitelist_size,
                                  uint8_t *addr, char *pubkey_pem)
{
#ifndef CONFIG_MCONFIG_WHITELIST_ENABLE
    return true;
#endif /**< CONFIG_MCONFIG_WHITELIST_ENABLE */

    if (MWIFI_ADDR_IS_ANY(addr)) {
        return true;
    }

    for (int i = 0; i < whitelist_size / sizeof(mconfig_whitelist_t); ++i) {
        if (!memcmp((whitelist_data + i)->addr, addr, MWIFI_ADDR_LEN)) {

#ifdef CONFIG_MCONFIG_WHITELIST_SECURITY_ENABLE

            if (mbedtls_md5_ret((uint8_t *)pubkey_pem, MCONFIG_RSA_PUBKEY_PEM_SIZE,
                                (whitelist_data + i)->checksum) != ESP_OK) {
                MDF_LOGW("RSA public key verification error");
                return false;
            }

#endif

            return true;
        }
    }

    return false;
}

static void mconfig_chain_master_task(void *arg)
{
    mdf_err_t ret                  = 0;
    size_t espnow_size             = 0;
    uint8_t *espnow_data           = MDF_MALLOC(MESPNOW_PAYLOAD_LEN);
    char *pubkey_pem               = MDF_MALLOC(MCONFIG_RSA_PUBKEY_PEM_SIZE + 1);
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0};
    uint32_t start_ticks           = xTaskGetTickCount();
    bool vendor_ie_flag            = true;

    mbedtls_aes_context aes_ctx    = {0};
    uint8_t aes_iv[16]             = {0};
    size_t aes_iv_offset           = 0;

    mconfig_chain_data_t *chain_data = (mconfig_chain_data_t *)arg;
    mconfig_data_t *mconfig_data     = &chain_data->mconfig_data;

    vendor_ie_data_t ie_data = {
        .element_id      = WIFI_VENDOR_IE_ELEMENT_ID,
        .length          = 4,
        .vendor_oui      = {MDF_VENDOR_OUI[0], MDF_VENDOR_OUI[1], MDF_VENDOR_OUI[2]},
        .vendor_oui_type = VENDOR_OUI_TYPE_CONFIG,
    };

    MDF_LOGI("Start send network configured");

    mdf_event_loop_send(MDF_EVENT_MCONFIG_CHAIN_MASTER_STARTED, NULL);
    mbedtls_aes_init(&aes_ctx);

    MDF_LOGD("g_chain_master_duration_ticks: %d", g_chain_master_duration_ticks);

    while ((xTaskGetTickCount() - start_ticks) < g_chain_master_duration_ticks) {
        /**
         * @brief 1. Notify the device to request network configuration information from itself
         */
        if (vendor_ie_flag) {
            esp_wifi_set_vendor_ie(true, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_1, &ie_data);
            vendor_ie_flag = false;
        }

        /**
         * @brief 2. Receive device request for network information
         */
        espnow_size = MESPNOW_PAYLOAD_LEN;
        ret = mespnow_read(MESPNOW_TRANS_PIPE_MCONFIG, src_addr, espnow_data,
                           &espnow_size, MCONFIG_CHAIN_EXIT_DELAY / portTICK_RATE_MS);

        if (ret != ESP_OK || espnow_size != MCONFIG_RSA_PUBKEY_PEM_DATA_SIZE + 1) {
            MDF_LOGV("receive, size: %d, data:\n%s", espnow_size, espnow_data);
            continue;
        }

        esp_wifi_set_vendor_ie(false, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_1, &ie_data);
        vendor_ie_flag = true;
        MDF_LOGI("Add device addr: " MACSTR, MAC2STR(src_addr));

        /**
         * @brief 3. Verify that the device is a forged device
         */
        espnow_data[MCONFIG_RSA_PUBKEY_PEM_DATA_SIZE] = '\0';
        MDF_LOGV("size: %d, espnow_data: %s", espnow_size, espnow_data);
        sprintf(pubkey_pem, PEM_BEGIN_PUBLIC_KEY "%s" PEM_END_PUBLIC_KEY, espnow_data);
        MDF_LOGV("pubkey_pem: %s", pubkey_pem);

        if (!mconfig_device_verify(mconfig_data->whitelist_data, mconfig_data->whitelist_size, src_addr, pubkey_pem)) {
            MDF_LOGD("this device("MACSTR") is not on the whitelist of the device configuration network device",
                     MAC2STR(src_addr));
            continue;
        }

        /**
         * @brief 4. Send network configuration information
         */
        mconfig_random(NULL, chain_data->aes_key, MCONFIG_AES_KEY_LEN);

        ret = mconfig_rsa_encrypt(chain_data, MCONFIG_RSA_PLAINTEXT_MAX_SIZE,
                                  pubkey_pem, (uint8_t *)espnow_data);
        MDF_ERROR_CONTINUE(ret != ESP_OK, "mconfig_rsa_encrypt, ret: %d", ret);

        ret = mbedtls_aes_setkey_enc(&aes_ctx, chain_data->aes_key, MCONFIG_AES_KEY_LEN * 8);
        MDF_ERROR_CONTINUE(ret < 0, "mbedtls_aes_setkey_enc, ret: %d", ret);

        aes_iv_offset = 0;
        memcpy(aes_iv, MCONFIG_AES_CFB_IV, MCONFIG_AES_KEY_LEN);
        MDF_LOGV("whitelist_size: %d encrypted_size: %d, chain_data:" MACSTR,
                 chain_data->mconfig_data.whitelist_size,
                 sizeof(mconfig_chain_data_t) - MCONFIG_RSA_PLAINTEXT_MAX_SIZE,
                 MAC2STR((uint8_t *)chain_data + MCONFIG_RSA_PLAINTEXT_MAX_SIZE));

        ret = mbedtls_aes_crypt_cfb128(&aes_ctx, MBEDTLS_AES_ENCRYPT,
                                       sizeof(mconfig_chain_data_t) - MCONFIG_RSA_PLAINTEXT_MAX_SIZE,
                                       &aes_iv_offset, aes_iv, (uint8_t *)chain_data + MCONFIG_RSA_PLAINTEXT_MAX_SIZE,
                                       espnow_data + MCONFIG_RSA_CIPHERTEXT_SIZE);
        MDF_ERROR_CONTINUE(ret != ESP_OK, "mbedtls_aes_crypt_cfb128, ret: %d", ret);

        ESP_ERROR_CHECK(mespnow_add_peer(ESP_IF_WIFI_AP, src_addr, (uint8_t *)CONFIG_MCONFIG_CHAIN_LMK));
        ret = mespnow_write(MESPNOW_TRANS_PIPE_MCONFIG, src_addr, espnow_data,
                            (MCONFIG_RSA_CIPHERTEXT_SIZE - MCONFIG_RSA_PLAINTEXT_MAX_SIZE) + sizeof(mconfig_chain_data_t), portMAX_DELAY);
        ESP_ERROR_CHECK(mespnow_del_peer(src_addr));
        MDF_ERROR_CONTINUE(ret != ESP_OK, "<%s> mespnow_write AES key & config", mdf_err_to_name(ret));


        /**
         * @brief 5. Send network configuration information
         */
#ifdef CONFIG_MCONFIG_WHITELIST_ENABLE


        int retry_count = MCONFIG_CHAIN_SEND_RETRY_NUM;
        uint8_t *whitelist_compress_data = NULL;
        ESP_ERROR_CHECK(mespnow_add_peer(ESP_IF_WIFI_AP, src_addr, (uint8_t *)CONFIG_MCONFIG_CHAIN_LMK));

        do {
            /**< Compression date to improve transmission efficiency */
            mz_ulong whitelist_compress_size = compressBound(mconfig_data->whitelist_size);
            whitelist_compress_data          = MDF_CALLOC(1, (int)whitelist_compress_size);

            ret = compress(whitelist_compress_data, &whitelist_compress_size,
                           (uint8_t *)mconfig_data->whitelist_data, mconfig_data->whitelist_size);
            MDF_ERROR_BREAK(ret != MZ_OK, "<%s> Compressed whitelist failed", mz_error(ret));

            aes_iv_offset = 0;
            memcpy(aes_iv, MCONFIG_AES_CFB_IV, MCONFIG_AES_KEY_LEN);
            mbedtls_aes_crypt_cfb128(&aes_ctx, MBEDTLS_AES_ENCRYPT, whitelist_compress_size,
                                     &aes_iv_offset, aes_iv, whitelist_compress_data,
                                     whitelist_compress_data);

            do {
                ret = mespnow_write(MESPNOW_TRANS_PIPE_MCONFIG, src_addr, whitelist_compress_data,
                                    whitelist_compress_size, portMAX_DELAY);
            } while (ret != ESP_OK && --retry_count);
        } while (0);

        MDF_FREE(whitelist_compress_data);
        ESP_ERROR_CHECK(mespnow_del_peer(src_addr));

        if (ret != ESP_OK) {
            MDF_LOGW("<%s> mespnow_write", mdf_err_to_name(ret));
        }

#endif /**< CONFIG_MCONFIG_WHITELIST_ENABLE */
    }

    MDF_LOGI("End send network configured");

    esp_wifi_set_vendor_ie(false, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_1, &ie_data);
    MDF_FREE(pubkey_pem);
    MDF_FREE(espnow_data);
    MDF_FREE(chain_data);
    mbedtls_aes_free(&aes_ctx);
    g_chain_master_duration_ticks = 0;

    mdf_event_loop_send(MDF_EVENT_MCONFIG_CHAIN_MASTER_STOPED, NULL);

    if (g_chain_master_exit_sem) {
        xSemaphoreGive(g_chain_master_exit_sem);
    }

    vTaskDelete(NULL);
}

static bool scan_mesh_device(uint8_t *bssid, int8_t *rssi)
{
    uint8_t channel               = 1;
    wifi_second_chan_t second     = 0;
    mconfig_scan_info_t scan_info = {0};
    uint32_t start_ticks          = xTaskGetTickCount();

    if (!xQueueReceive(g_mconfig_scan_queue, &scan_info, MCONFIG_CHAIN_EXIT_DELAY / portTICK_RATE_MS)) {
        if (g_switch_channel_flag && g_chain_slave_flag) {
            ESP_ERROR_CHECK(esp_wifi_get_channel(&channel, &second));

            channel = channel % 13 + 1;

            if (esp_wifi_set_channel(channel, second) != ESP_OK) {
                MDF_LOGW("channel: %d, second: %d", channel, second);
            }
        }

        return false;
    }

    *rssi = scan_info.rssi;
    memcpy(bssid, scan_info.bssid, MWIFI_ADDR_LEN);

    /**< Device that achieves the best signal strength */
    for (TickType_t wait_ticks = MCONFIG_CHAIN_EXIT_DELAY / portTICK_RATE_MS;
            xQueueReceive(g_mconfig_scan_queue, &scan_info, wait_ticks) == pdTRUE;
            wait_ticks = xTaskGetTickCount() - start_ticks < wait_ticks ?
                         wait_ticks - (xTaskGetTickCount() - start_ticks) : 0) {
        if (scan_info.rssi > *rssi) {
            *rssi = scan_info.rssi;
            memcpy(bssid, scan_info.bssid, MWIFI_ADDR_LEN);
        }
    }

    return true;
}

static void mconfig_chain_slave_task(void *arg)
{
    mdf_err_t ret                   = MDF_OK;
    size_t espnow_size              = 0;
    uint8_t *espnow_data            = MDF_MALLOC(MESPNOW_PAYLOAD_LEN);
    char *privkey_pem               = MDF_CALLOC(1, MCONFIG_RSA_PRIVKEY_PEM_SIZE);
    char *pubkey_pem                = MDF_CALLOC(1, MCONFIG_RSA_PUBKEY_PEM_SIZE);
    g_mconfig_scan_queue            = xQueueCreate(10, sizeof(mconfig_scan_info_t));
    uint8_t dest_addr[MWIFI_ADDR_LEN] = {0};
    int8_t rssi                     = 0;

    mbedtls_aes_context aes_ctx     = {0};
    uint8_t aes_iv[16]              = {0};
    size_t aes_iv_offset            = 0;

    mconfig_chain_data_t *chain_data = MDF_MALLOC(sizeof(mconfig_chain_data_t));

    ESP_ERROR_CHECK(mconfig_rsa_gen_key(privkey_pem, pubkey_pem));
    MDF_LOGI("Generate RSA public and private keys");
    MDF_LOGV("privkey_pem: %s, pubkey_pem: %s", privkey_pem, pubkey_pem);

    mbedtls_aes_init(&aes_ctx);

    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(wifi_sniffer_cb));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));

    while (g_chain_slave_flag) {
        /**
         * @brief 1. Find the mesh device that has configured the network
         */
        if (!scan_mesh_device(dest_addr, &rssi)) {
            continue;
        }

        MDF_LOGD("rssi: %d, bssid: " MACSTR, rssi, MAC2STR(dest_addr));

        /**
         * @brief 2. Request network configuration information
         */
        ESP_ERROR_CHECK(mespnow_add_peer(ESP_IF_WIFI_STA, dest_addr, NULL));

        /**< Remove headers and footers to reduce data length */

        memset(espnow_data, 0, MESPNOW_PAYLOAD_LEN);
        memcpy(espnow_data, pubkey_pem + strlen(PEM_BEGIN_PUBLIC_KEY),
               MCONFIG_RSA_PUBKEY_PEM_DATA_SIZE);
        espnow_data[MCONFIG_RSA_PUBKEY_PEM_DATA_SIZE] = rssi;
        espnow_size = MCONFIG_RSA_PUBKEY_PEM_DATA_SIZE + 1;

        ret = mespnow_write(MESPNOW_TRANS_PIPE_MCONFIG, dest_addr,
                            espnow_data, espnow_size, portMAX_DELAY);

        ESP_ERROR_CHECK(mespnow_del_peer(dest_addr));
        MDF_ERROR_CONTINUE(ret != ESP_OK, "<%s> mespnow_write", mdf_err_to_name(ret));

        /**
         * @brief 3. Receive network configuration information
         */
        ESP_ERROR_CHECK(mespnow_add_peer(ESP_IF_WIFI_STA, dest_addr, (uint8_t *)CONFIG_MCONFIG_CHAIN_LMK));
        espnow_size = MESPNOW_PAYLOAD_LEN;
        ret = mespnow_read(MESPNOW_TRANS_PIPE_MCONFIG, dest_addr,
                           espnow_data, &espnow_size, 1000 / portTICK_RATE_MS);

        if (ret != ESP_OK || espnow_size != (MCONFIG_RSA_CIPHERTEXT_SIZE - MCONFIG_RSA_PLAINTEXT_MAX_SIZE) + sizeof(mconfig_chain_data_t)) {
            MDF_LOGW("<%s> mespnow_read", mdf_err_to_name(ret));
            ESP_ERROR_CHECK(mespnow_del_peer(dest_addr));
            continue;
        }

        ret = mconfig_rsa_decrypt(espnow_data, privkey_pem, chain_data, MCONFIG_RSA_PLAINTEXT_MAX_SIZE);

        if (ret != ESP_OK) {
            MDF_LOGW("Configuration information decryption failed, ret: 0x%x", -ret);
            ESP_ERROR_CHECK(mespnow_del_peer(dest_addr));
            continue;
        }

        ret = mbedtls_aes_setkey_enc(&aes_ctx, chain_data->aes_key, MCONFIG_AES_KEY_LEN * 8);
        MDF_ERROR_CONTINUE(ret < 0, "mbedtls_aes_setkey_enc, ret: 0x%x", -ret);

        aes_iv_offset = 0;
        memcpy(aes_iv, MCONFIG_AES_CFB_IV, MCONFIG_AES_KEY_LEN);
        ret = mbedtls_aes_crypt_cfb128(&aes_ctx, MBEDTLS_AES_DECRYPT, sizeof(mconfig_chain_data_t) - MCONFIG_RSA_PLAINTEXT_MAX_SIZE,
                                       &aes_iv_offset, aes_iv, espnow_data + MCONFIG_RSA_CIPHERTEXT_SIZE,
                                       (uint8_t *)chain_data + MCONFIG_RSA_PLAINTEXT_MAX_SIZE);
        MDF_ERROR_CONTINUE(ret != ESP_OK, "mbedtls_aes_crypt_cfb128, ret: 0x%x", -ret);

        /**
         * @brief 4. Receive device whitelist
         */
#ifdef CONFIG_MCONFIG_WHITELIST_ENABLE

        chain_data = MDF_REALLOC(chain_data, sizeof(mconfig_chain_data_t) + chain_data->mconfig_data.whitelist_size);
        int retry_count                  = MCONFIG_CHAIN_SEND_RETRY_NUM;
        mconfig_data_t *mconfig_data     = &chain_data->mconfig_data;
        mz_ulong whitelist_size          = mconfig_data->whitelist_size;
        mz_ulong whitelist_compress_size = 0;
        uint8_t *whitelist_compress_data = MDF_MALLOC(mconfig_data->whitelist_size + 64);

        do {
            whitelist_compress_size = mconfig_data->whitelist_size + 64;
            ret = mespnow_read(MESPNOW_TRANS_PIPE_MCONFIG, dest_addr, whitelist_compress_data,
                               (size_t *)&whitelist_compress_size, 3000 / portTICK_RATE_MS);
        } while (ret != ESP_OK && ret != ESP_ERR_TIMEOUT && --retry_count);

        ESP_ERROR_CHECK(mespnow_del_peer(dest_addr));

        if (ret != ESP_OK) {
            MDF_FREE(whitelist_compress_data);
            MDF_LOGW("<%s> Failed to receive whitelist", mdf_err_to_name(ret));
            continue;
        }

        aes_iv_offset = 0;
        memcpy(aes_iv, MCONFIG_AES_CFB_IV, MCONFIG_AES_KEY_LEN);
        mbedtls_aes_crypt_cfb128(&aes_ctx, MBEDTLS_AES_DECRYPT, whitelist_compress_size,
                                 &aes_iv_offset, aes_iv, whitelist_compress_data, whitelist_compress_data);

        ret = uncompress((uint8_t *)mconfig_data->whitelist_data, &whitelist_size,
                         whitelist_compress_data, whitelist_compress_size);
        mconfig_data->whitelist_size = whitelist_size;

        MDF_FREE(whitelist_compress_data);
        MDF_ERROR_CONTINUE(ret != MZ_OK, "<%s> Failed to uncompress whitelist", mz_error(ret));

#endif /**< CONFIG_MCONFIG_WHITELIST_ENABLE */

        ESP_ERROR_CHECK(mespnow_del_peer(dest_addr));

        /**
         * @brief 5. Send network configuration information to the queue
         */
        ret = mconfig_queue_write(&chain_data->mconfig_data, 0);
        MDF_ERROR_CONTINUE(ret != ESP_OK, "mconfig_queue_write failed, ret: %d", ret);

        mdf_event_loop_send(MDF_EVENT_MCONFIG_CHAIN_FINISH, NULL);
        break;
    }

    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(NULL));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));

    MDF_FREE(pubkey_pem);
    MDF_FREE(privkey_pem);
    MDF_FREE(espnow_data);
    MDF_FREE(chain_data);
    vQueueDelete(g_mconfig_scan_queue);
    g_mconfig_scan_queue = NULL;
    mbedtls_aes_free(&aes_ctx);
    xSemaphoreGive(g_chain_slave_exit_sem);

    vTaskDelete(NULL);
}

mdf_err_t mconfig_chain_slave_channel_switch_disable()
{
    g_switch_channel_flag = false;

    mdf_event_loop_send(MDF_EVENT_MCONFIG_CHAIN_SLAVE_STOPED, NULL);

    return MDF_OK;
}

mdf_err_t mconfig_chain_slave_channel_switch_enable()
{
    g_switch_channel_flag = true;

    mdf_event_loop_send(MDF_EVENT_MCONFIG_CHAIN_SLAVE_STARTED, NULL);

    return MDF_OK;
}

mdf_err_t mconfig_chain_slave_init()
{
    if (g_chain_slave_flag) {
        return MDF_OK;
    }

    if (!g_chain_slave_exit_sem) {
        g_chain_slave_exit_sem = xSemaphoreCreateBinary();
    }

    g_chain_slave_flag = true;

    mconfig_chain_slave_channel_switch_enable();
    xTaskCreatePinnedToCore(mconfig_chain_slave_task, "mconfig_chain_slave", 4 * 1024,
                            NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL, CONFIG_MDF_TASK_PINNED_TO_CORE);

    return MDF_OK;
}

mdf_err_t mconfig_chain_slave_deinit()
{
    if (!g_chain_slave_flag) {
        return MDF_OK;
    }

    g_chain_slave_flag = false;
    MDF_LOGD("mconfig_chain_slave_deinit");

    xSemaphoreTake(g_chain_slave_exit_sem, portMAX_DELAY);
    vQueueDelete(g_chain_slave_exit_sem);
    g_chain_slave_exit_sem = NULL;

    return MDF_OK;
}

mdf_err_t mconfig_chain_master(const mconfig_data_t *mconfig_data, TickType_t duration_ticks)
{
    MDF_PARAM_CHECK(mconfig_data);
#ifdef CONFIG_MCONFIG_WHITELIST_ENABLE
    MDF_PARAM_CHECK(mconfig_data->whitelist_size);
#endif

    if (g_chain_master_duration_ticks > 0) {
        g_chain_master_exit_sem       = xSemaphoreCreateBinary();
        g_chain_master_duration_ticks = 0;

        xSemaphoreTake(g_chain_master_exit_sem, portMAX_DELAY);
        vQueueDelete(g_chain_master_exit_sem);
        g_chain_master_exit_sem = NULL;

        MDF_LOGD("mconfig chain master is exit");
    }

    size_t chain_size = sizeof(mconfig_chain_data_t) + mconfig_data->whitelist_size;
    mconfig_chain_data_t *chain_data = MDF_MALLOC(chain_size);
    memcpy(&chain_data->mconfig_data, mconfig_data, sizeof(mconfig_data_t) + mconfig_data->whitelist_size);
    g_chain_master_duration_ticks = duration_ticks;

    xTaskCreatePinnedToCore(mconfig_chain_master_task, "mconfig_chain_master", 8 * 1024,
                            chain_data, CONFIG_MDF_TASK_DEFAULT_PRIOTY,
                            NULL, CONFIG_MDF_TASK_PINNED_TO_CORE);
    return MDF_OK;
}
