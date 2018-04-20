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
#include "mdf_network_config.h"
#include "mdf_network_blufi.h"
#include "mdf_network_rsa.h"
#include "mdf_info_store.h"
#include "mdf_device_handle.h"

typedef struct {
    uint8_t oui[4];     /**< filtering useless packets */
    int8_t rssi;        /**< signal strength */
    uint8_t data[0];    /**< data */
} auto_network_data_t;

typedef struct {
    uint16_t num;               /**< the number of device */
    wifi_mesh_addr_t data[0];   /**< device address */
} auto_network_addrs_t;

static const char *TAG                      = "mdf_network_config";
static QueueHandle_t g_network_config_queue = NULL;
static bool g_mdf_network_enable_auto_flag  = false;
static bool g_mdf_network_enable_blufi_flag = false;
static const uint8_t MDF_AUTO_NETWORK_OUI[] = { 0x45, 0x53, 0x50, 0x4D };

static const void *g_auto_network_whitelist_lock = NULL;
static auto_network_addrs_t *g_auto_network_whitelist    = NULL;

static bool auto_network_whitelist_lock()
{
    if (!g_auto_network_whitelist_lock) {
        g_auto_network_whitelist_lock = xSemaphoreCreateMutex();
    }

    return xSemaphoreTake(g_auto_network_whitelist_lock, portMAX_DELAY);
}

static bool auto_network_whitelist_unlock()
{
    if (!g_auto_network_whitelist_lock) {
        g_auto_network_whitelist_lock = xSemaphoreCreateMutex();
    }

    return xSemaphoreGive(g_auto_network_whitelist_lock);
}

esp_err_t mdf_network_add_whitelist(const uint8_t device_addr[6], uint32_t num)
{
    auto_network_whitelist_lock();

    if (g_auto_network_whitelist) {
        auto_network_addrs_t *auto_network_whitelist_tmp = mdf_malloc(sizeof(auto_network_addrs_t) + (g_auto_network_whitelist->num + num) * 6);
        memcpy(auto_network_whitelist_tmp, g_auto_network_whitelist,
               sizeof(auto_network_addrs_t) + g_auto_network_whitelist->num * 6);
        memcpy((uint8_t *)auto_network_whitelist_tmp + (sizeof(auto_network_addrs_t) + g_auto_network_whitelist->num * 6),
               device_addr, num * 6);
        mdf_free(g_auto_network_whitelist);
        g_auto_network_whitelist       = auto_network_whitelist_tmp;
        g_auto_network_whitelist->num += num;
    } else {
        g_auto_network_whitelist = mdf_calloc(1, sizeof(auto_network_addrs_t) + num * 6);
        memcpy(g_auto_network_whitelist->data, device_addr, num * 6);
        g_auto_network_whitelist->num = num;
    }

    MDF_LOGD("g_auto_network_whitelist->num: %d", g_auto_network_whitelist->num);

    auto_network_whitelist_unlock();

    return ESP_OK;
}

bool mdf_auto_network_find_whitelist(const uint8_t device_addr[6])
{
    if (!g_auto_network_whitelist) {
        return false;
    }

    auto_network_whitelist_lock();

    for (int i = 0; i < g_auto_network_whitelist->num; ++i) {
        uint8_t addr_tmp[6] = {0};

        memcpy(addr_tmp, g_auto_network_whitelist->data + i, 6);
        *((int *)(addr_tmp + 2)) = htonl(htonl(*((int *)(addr_tmp + 2))) - 2);
        MDF_LOGV("num: %d, addr_tmp: "MACSTR", device_addr: "MACSTR,
                 g_auto_network_whitelist->num, MAC2STR(addr_tmp), MAC2STR(device_addr));

        if (!memcmp(addr_tmp, device_addr, 6)) {
            auto_network_whitelist_unlock();
            return true;
        }
    }

    MDF_LOGD("find device false, num: %d, device_addr: "MACSTR,
             g_auto_network_whitelist->num, MAC2STR(device_addr));

    auto_network_whitelist_unlock();

    return false;
}

esp_err_t mdf_network_delete_whitelist()
{
    auto_network_whitelist_lock();

    mdf_free(g_auto_network_whitelist);
    g_auto_network_whitelist = NULL;

    auto_network_whitelist_unlock();

    return ESP_OK;
}

#ifdef MDF_USE_DEFAULT_NETWORK_CONFIG
static esp_err_t mdf_channel_get(char *ssid, uint8_t *channel)
{
    MDF_PARAM_CHECK(ssid);
    MDF_PARAM_CHECK(channel);

    esp_err_t ret                  = ESP_OK;
    uint16_t wifi_ap_num           = 0;
    wifi_ap_record_t *ap_info      = NULL;
    wifi_scan_config_t scan_config = {
        .ssid = (uint8_t *)ssid,
    };

    ESP_ERROR_CHECK(esp_wifi_scan_stop());

    for (int i = 0; wifi_ap_num == 0 && i < 3; ++i) {
        ret = esp_wifi_scan_start(&scan_config, true);
        MDF_ERROR_CONTINUE(ret != ESP_OK, "esp_wifi_scan_start, ret: %x", ret);
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&wifi_ap_num));
    }

    MDF_ERROR_CHECK(wifi_ap_num < 1, ESP_FAIL,
                    "esp_wifi_scan_get_ap_num, ap ssid: %s, num: %d", ssid, wifi_ap_num);

    ap_info = (wifi_ap_record_t *)mdf_malloc(sizeof(wifi_ap_record_t) * wifi_ap_num);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&wifi_ap_num, ap_info));

    MDF_LOGD("esp_wifi_scan_get_ap_records, num: %d, channel: %d",
             wifi_ap_num, ap_info[0].primary);

    *channel = ap_info[0].primary;

    mdf_free(ap_info);
    return (*channel < 1 || *channel > 14) ? ESP_FAIL : ESP_OK;
}
#endif

static esp_err_t mdf_ap_info_get(int8_t *rssi, uint8_t mac[6])
{
    MDF_PARAM_CHECK(rssi);
    MDF_PARAM_CHECK(mac);

    esp_err_t ret             = ESP_OK;
    uint8_t data[4]           = {0};
    *rssi                     = 0;
    uint8_t channel           = 1;
    wifi_second_chan_t second = 0;

    ret = esp_wifi_get_channel(&channel, &second);
    MDF_ERROR_CHECK(ret != ESP_OK || channel > 13 || channel < 1, ESP_FAIL,
                    "esp_wifi_set_channel fail, ret: %d, channel: %d, second: %d",
                    ret, channel, second);
    channel = channel > 13 ? 1 : channel;

    for (int i = 0; i < 13; ++i) {
        ret = esp_wifi_set_channel(channel, second);
        MDF_ERROR_CHECK(ret != ESP_OK,  ESP_FAIL,
                        "esp_wifi_set_channel fail, ret: %d, channel: %d", ret, channel);

        ret = mdf_espnow_read(MDF_ESPNOW_RESERVED, mac, data,
                              sizeof(data), 100 / portTICK_RATE_MS);

        if (ret > 0 && !memcmp(data, MDF_AUTO_NETWORK_OUI, sizeof(MDF_AUTO_NETWORK_OUI))) {
            return ESP_OK;
        }

        channel = channel % 13 + 1;
    }

    return ESP_FAIL;
}

esp_err_t mdf_network_send_config(const network_config_t *network_config)
{
    MDF_PARAM_CHECK(network_config);

    if (!g_network_config_queue) {
        g_network_config_queue = xQueueCreate(1, sizeof(network_config_t));
    }

    if (!xQueueSend(g_network_config_queue, network_config, 0)) {
        MDF_LOGW("xQueueSend failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t mdf_network_clear_config()
{
    esp_err_t ret = ESP_OK;

    ret = mdf_info_erase(MDF_NETWORK_CONFIG_KEY);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_erase, ret: %d", ret);

    return ESP_OK;
}

esp_err_t mdf_network_set_config(const network_config_t *network_config)
{
    esp_err_t ret = ESP_OK;

    ret = mdf_info_save(MDF_NETWORK_CONFIG_KEY, network_config, sizeof(network_config_t));
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_save, ret: %d", ret);

    return ESP_OK;
}

static esp_err_t mdf_network_handle(network_config_t *network_config)
{
    MDF_PARAM_CHECK(network_config);

    esp_err_t ret                     = ESP_OK;
    auto_network_data_t *request_data  = mdf_calloc(1, ESP_NOW_MAX_DATA_LEN);
    auto_network_data_t *response_data = mdf_calloc(1, ESP_NOW_MAX_DATA_LEN);
    char *privkey_pem                 = mdf_calloc(1, MDF_RSA_PRIVKEY_PEM_SIZE);
    char *pubkey_pem                  = mdf_calloc(1, MDF_RSA_PUBKEY_PEM_SIZE);
    uint8_t dest_addr[6]   = {0};

    if (!g_network_config_queue) {
        g_network_config_queue = xQueueCreate(1, sizeof(network_config_t));
    }

    ret = mdf_rsa_gen_key(privkey_pem, pubkey_pem);
    MDF_ERROR_GOTO(ret < 0, EXIT, "mdf_rsa_gen_key, ret: %d", ret);
    MDF_LOGI("generate RSA public and private keys");

    memcpy(request_data->data, pubkey_pem + strlen("-----BEGIN PUBLIC KEY-----\n"), MDF_RSA_PUBKEY_PEM_DATA_SIZE - 1);
    mdf_free(pubkey_pem);
    MDF_LOGV("pubkey_pem, data:\n%s", request_data->data);

    ESP_ERROR_CHECK(mdf_espnow_enable(MDF_ESPNOW_RESERVED));
    ESP_ERROR_CHECK(mdf_espnow_enable(MDF_ESPNOW_NETCONFIG));

    for (;;) {
        if (xQueueReceive(g_network_config_queue, network_config, 0)) {
            MDF_LOGD("blufi network configured success");
            ret = ESP_OK;
            break;
        }

        if (mdf_blufi_connecting_wifi()) {
            vTaskDelay(1000 / portTICK_RATE_MS);
            continue;
        }

        if (mdf_ap_info_get(&request_data->rssi, dest_addr) != ESP_OK) {
            MDF_LOGV("did not find the network equipment has been configured network");
            continue;
        }

        MDF_LOGD("rssi: %d, dest_addr: "MACSTR, request_data->rssi, MAC2STR(dest_addr));

        int retry_count = 0;

        ESP_ERROR_CHECK(mdf_espnow_add_peer_no_encrypt(dest_addr));

        do {
            vTaskDelay(retry_count * 10 / portTICK_RATE_MS);
            ret = mdf_espnow_write(MDF_ESPNOW_NETCONFIG, dest_addr,
                                   request_data, MDF_RSA_PUBKEY_PEM_DATA_SIZE + sizeof(auto_network_data_t), 0);
        } while (ret <= 0 && retry_count++ < 5);

        ESP_ERROR_CHECK(mdf_espnow_del_peer(dest_addr));
        MDF_ERROR_CONTINUE(ret < 0, "mdf_espnow_write, ret: %d", ret);

        ESP_ERROR_CHECK(mdf_espnow_add_peer_default_encrypt(dest_addr));
        ret = mdf_espnow_read(MDF_ESPNOW_NETCONFIG, dest_addr,
                              response_data, WIFI_MESH_PACKET_MAX_SIZE, 5000 / portTICK_RATE_MS);

        if (ret < 0) {
            ESP_ERROR_CHECK(mdf_espnow_del_peer(dest_addr));
            MDF_LOGW("mdf_espnow_read, ret: %d", ret);
            continue;
        }

        uint8_t *device_addr = mdf_calloc(AUTO_NETWORK_WHITELIST_MAX_NUM, 6);
        ret = mdf_espnow_read(MDF_ESPNOW_NETCONFIG, dest_addr, device_addr,
                              AUTO_NETWORK_WHITELIST_MAX_NUM * 6, 1000 / portTICK_RATE_MS);
        ESP_ERROR_CHECK(mdf_espnow_del_peer(dest_addr));

        if (ret <= 0 || ret % 6) {
            MDF_LOGW("mdf_espnow_read auto network whitelist, ret: %d, device addr: " MACSTR,
                     ret, MAC2STR(device_addr));
            mdf_free(device_addr);
        } else {
            ret = mdf_network_add_whitelist(device_addr, ret / 6);
            mdf_free(device_addr);
            MDF_ERROR_CONTINUE(ret < 0, "mdf_network_add_whitelist, ret: %d", ret);
        }

        ret = mdf_rsa_decrypt(response_data->data, (uint8_t *)privkey_pem,
                              network_config, sizeof(network_config_t));
        MDF_ERROR_CONTINUE(ret < 0, "mdf_rsa_decrypt, ret: %d", ret);

        MDF_LOGD("network_config, ssid: %s, password: %s",
                 network_config->ssid, network_config->password);
        break;
    }

EXIT:
    ESP_ERROR_CHECK(mdf_espnow_disable(MDF_ESPNOW_RESERVED));
    ESP_ERROR_CHECK(mdf_espnow_disable(MDF_ESPNOW_NETCONFIG));
    mdf_free(pubkey_pem);
    mdf_free(privkey_pem);
    mdf_free(response_data);
    mdf_free(request_data);
    vQueueDelete(g_network_config_queue);
    g_network_config_queue = NULL;
    return ret < 0 ? ESP_FAIL : ESP_OK;
}

esp_err_t mdf_network_get_config(network_config_t *network_config)
{
    MDF_PARAM_CHECK(network_config);

    esp_err_t ret  = ESP_OK;

    memset(network_config, 0, sizeof(network_config_t));

#ifdef MDF_USE_DEFAULT_NETWORK_CONFIG
    MDF_LOGI("***********************************");
    MDF_LOGI("*    USE DEFAULT NETWORK CONFIG   *");
    MDF_LOGI("***********************************");

    strncpy(network_config->ssid, MDF_DEFAULT_ROUTER_SSID, MDF_SSID_LEN);
    strncpy(network_config->password, MDF_DEFAULT_ROUTER_PASSWD, MDF_PASSWD_LEN);
    memcpy(network_config->mdf_id, MDF_DEFAULT_ID, 6);

    if (mdf_channel_get(network_config->ssid, &network_config->channel) == ESP_OK) {
        mdf_blufi_mem_release();
        return ESP_OK;
    }

#endif /**< MDF_USE_DEFAULT_NETWORK_CONFIG */

    if (mdf_info_load(MDF_NETWORK_CONFIG_KEY, network_config, sizeof(network_config_t)) > 0) {
        mdf_blufi_mem_release();
        return ESP_OK;
    }

    ret = mdf_event_loop_send(MDF_EVENT_NETWORK_START_CONFIG, NULL);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_event_loop_send, ret: %d", ret);

    ret = mdf_blufi_init();
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_blufi_init, ret: %d", ret);

    ret = mdf_network_handle(network_config);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_network_config, ret: %d", ret);

    ret = mdf_event_loop_send(MDF_EVENT_NETWORK_ORGANIZE, NULL);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_event_loop_send, ret: %d", ret);

    ret = mdf_info_save(MDF_NETWORK_CONFIG_KEY, network_config, sizeof(network_config_t));
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_save, ret: %d", ret);

    ret = mdf_blufi_deinit();
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_blufi_deinit, ret: %d", ret);

    ret = mdf_network_enable_auto(MDF_AUTO_NETWORK_TIMEROUT);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_network_enable_auto, ret: %d", ret);

    return ESP_OK;
}

static void mdf_auto_network_task(void *arg)
{
    esp_err_t ret                        = 0;
    network_config_t network_config      = {0};
    auto_network_data_t *request_data    = mdf_calloc(1, ESP_NOW_MAX_DATA_LEN);
    auto_network_data_t *response_data   = mdf_calloc(1, ESP_NOW_MAX_DATA_LEN);
    char *pubkey_pem                     = mdf_malloc(MDF_RSA_PUBKEY_PEM_SIZE);
    uint8_t source_addr[6]               = {0};
    uint32_t auto_network_window_timeout = (uint32_t)arg;
    uint32_t auto_network_start_time     = xTaskGetTickCount();

    ret = mdf_info_load(MDF_NETWORK_CONFIG_KEY, &network_config, sizeof(network_config_t));
    MDF_ERROR_GOTO(ret < 0, EXIT, "mdf_info_load, ret: %d", ret);

    MDF_LOGD("start auto network configured");
    MDF_LOGV("network_config, ssid: %s, password: %s",
             network_config.ssid, network_config.password);

    ESP_ERROR_CHECK(mdf_espnow_enable(MDF_ESPNOW_RESERVED));
    ESP_ERROR_CHECK(mdf_espnow_enable(MDF_ESPNOW_NETCONFIG));
    ESP_ERROR_CHECK(mdf_espnow_add_peer_no_encrypt(WIFI_MESH_BROADCAST_ADDR));

    mdf_espnow_write(MDF_ESPNOW_RESERVED, WIFI_MESH_BROADCAST_ADDR,
                     MDF_AUTO_NETWORK_OUI, sizeof(MDF_AUTO_NETWORK_OUI), 100 / portTICK_RATE_MS);

    while (g_mdf_network_enable_auto_flag
            && (xTaskGetTickCount() - auto_network_start_time) * portTICK_RATE_MS / 1000  < auto_network_window_timeout) {

        ret = mdf_espnow_read(MDF_ESPNOW_NETCONFIG, source_addr, request_data,
                              ESP_NOW_MAX_DATA_LEN, 100 / portTICK_RATE_MS);

        if (ret < 0) {
            ret = mdf_espnow_write(MDF_ESPNOW_RESERVED, WIFI_MESH_BROADCAST_ADDR,
                                   MDF_AUTO_NETWORK_OUI, sizeof(MDF_AUTO_NETWORK_OUI), 100 / portTICK_RATE_MS);
            MDF_ERROR_CONTINUE(ret < 0, "mdf_espnow_write, ret: %d", ret);
            continue;
        }

        if (!mdf_auto_network_find_whitelist(source_addr)) {
            MDF_LOGW("this device("MACSTR") is not on the whitelist of the device configuration network device",
                     MAC2STR(source_addr));
            continue;
        }

        if (ret != sizeof(auto_network_data_t) + MDF_RSA_PUBKEY_PEM_DATA_SIZE) {
            MDF_LOGW("receive, size: %d, data:\n%s", ret, request_data->data);
            continue;
        }

        MDF_LOGD("add device addr: "MACSTR, MAC2STR(source_addr));

        sprintf(pubkey_pem, "-----BEGIN PUBLIC KEY-----\n%s\n-----END PUBLIC KEY-----\n", request_data->data);
        ret = mdf_rsa_encrypt(&network_config, sizeof(network_config_t), (uint8_t *)pubkey_pem, response_data->data);
        MDF_ERROR_CONTINUE(ret < 0, "mdf_rsa_encrypt, ret: %d", ret);

        ESP_ERROR_CHECK(mdf_espnow_add_peer_default_encrypt(source_addr));

        ssize_t write_size = -1;

        for (int i = 0; i < 5 && write_size <= 0; i++) {
            vTaskDelay(i * 10 / portTICK_RATE_MS);
            write_size = mdf_espnow_write(MDF_ESPNOW_NETCONFIG, source_addr, response_data,
                                          MDF_RSA_CIPHERTEXT_SIZE + sizeof(auto_network_data_t), 100 / portTICK_RATE_MS);
        }

        if (write_size <= 0) {
            ESP_ERROR_CHECK(mdf_espnow_del_peer(source_addr));
            MDF_LOGW("mdf_espnow_write public key, ret: %d", ret);
            continue;
        }

        write_size = -1;

        for (int i = 0; i < 5 && g_auto_network_whitelist && write_size <= 0; i++) {
            vTaskDelay(i * 10 / portTICK_RATE_MS);
            write_size = mdf_espnow_write(MDF_ESPNOW_NETCONFIG, source_addr, g_auto_network_whitelist->data,
                                          g_auto_network_whitelist->num * 6, 100 / portTICK_RATE_MS);
        }

        ESP_ERROR_CHECK(mdf_espnow_del_peer(source_addr));

        if (write_size <= 0) {
            MDF_LOGW("mdf_espnow_write whitelist, ret: %d", ret);
        }
    }

    ESP_ERROR_CHECK(mdf_espnow_del_peer(WIFI_MESH_BROADCAST_ADDR));

    MDF_LOGD("end auto network configured");

EXIT:
    mdf_espnow_disable(MDF_ESPNOW_RESERVED);
    mdf_espnow_disable(MDF_ESPNOW_NETCONFIG);
    mdf_network_delete_whitelist();
    g_mdf_network_enable_auto_flag = false;
    mdf_free(pubkey_pem);
    mdf_free(response_data);
    mdf_free(request_data);
    vTaskDelete(NULL);
}

esp_err_t mdf_network_enable_auto(uint32_t timeout)
{
    if (!g_auto_network_whitelist) {
        return ESP_OK;
    }

    if (!g_mdf_network_enable_auto_flag) {
        g_mdf_network_enable_auto_flag = true;
        xTaskCreate(mdf_auto_network_task, "mdf_auto_network_task", 4 * 1024,
                    (void *)timeout, 6 - 3, NULL);
    }

    return ESP_OK;
}

static void mdf_blufi_network_task(void *arg)
{
    esp_err_t ret                   = ESP_OK;
    network_config_t network_config = {0};

    vTaskDelay(10000 / portTICK_RATE_MS);

    if (mdf_wifi_mesh_is_connect()) {
        goto EXIT;
    }

    if (!g_network_config_queue) {
        g_network_config_queue = xQueueCreate(1, sizeof(network_config_t));
    }

    MDF_LOGD("start blue network configured");

    ret = mdf_blufi_init();
    MDF_ERROR_GOTO(ret < 0, EXIT, "mdf_blufi_init, ret: %d", ret);

    while (!mdf_wifi_mesh_is_connect() && !mdf_blufi_mem_is_release()) {
        if (xQueueReceive(g_network_config_queue, &network_config, 1000 / portTICK_RATE_MS)) {
            ret = mdf_reconfig_network(&network_config);
            MDF_ERROR_CONTINUE(ret < 0, "mdf_reconfig_network, ret: %d", ret);
            break;
        }
    }

    ret = mdf_blufi_deinit();
    MDF_ERROR_GOTO(ret < 0, EXIT, "mdf_blufi_deinit, ret: %d", ret);

    MDF_LOGD("end blue network configured");

EXIT:

    if (g_network_config_queue) {
        vQueueDelete(g_network_config_queue);
        g_network_config_queue = NULL;
    }

    g_mdf_network_enable_blufi_flag = false;
    mdf_network_delete_whitelist();
    vTaskDelete(NULL);
}

esp_err_t mdf_network_enable_blufi()
{
    if (!g_mdf_network_enable_blufi_flag && !mdf_blufi_mem_is_release()) {
        g_mdf_network_enable_blufi_flag = true;
        xTaskCreate(mdf_blufi_network_task, "mdf_blufi_network_task", 3 * 1024,
                    NULL, MDF_TASK_DEFAULT_PRIOTY - 3, NULL);
    } else {
        MDF_LOGW("blufi memory released, cannot enter blufi config mode rightnow");
        return ESP_FAIL;
    }

    return ESP_OK;
}
