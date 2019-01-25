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


#include "esp_bt.h"
#ifdef CONFIG_BT_ENABLED

#include "esp_blufi_api.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_wifi.h"

#include "mbedtls/aes.h"
#include "mbedtls/dhm.h"
#include "mbedtls/md5.h"
#include "rom/crc.h"

#include "mdf_common.h"
#include "mdf_event_loop.h"
#include "mconfig_blufi.h"
#include "mconfig_security.h"

#define MCONFIG_BLUFI_VERSION     (0)
#define MCONFIG_BLUFI_TIMEROUT_MS (30000)

// #define ESP_BLUFI_STA_CONN_SUCCESS (0)
// #define ESP_BLUFI_STA_CONN_FAIL    (1)
#define BLUFI_STA_PASSWORD_ERR    (0x10)
#define BLUFI_STA_AP_FOUND_ERR    (0x11)
#define BLUFI_STA_TOOMANY_ERR     (0x12)
#define BLUFI_STA_CONFIG_ERR      (0x13)

/*
   The SEC_TYPE_xxx is for self-defined packet data type in the procedure of "BLUFI negotiate key"
   If user use other negotiation procedure to exchange(or generate) key, should redefine the type by yourself.
 */
enum blufi_sec_type {
    SEC_TYPE_DH_PARAM_LEN        = 0x0,
    SEC_TYPE_DH_PARAM_DATA       = 0x1,

    SEC_TYPE_RSA_PUBKEY_REQUEST  = 0x10,
    SEC_TYPE_RSA_AESKEY_RESPONSE = 0x11,
};

enum blufi_data_type {
    /**
     * @brief mwifi router configuration
     */
    BLUFI_DATA_ROUTER_SSID           = 1,
    BLUFI_DATA_ROUTER_PASSWORD       = 2,
    BLUFI_DATA_ROUTER_BSSID          = 3,
    BLUFI_DATA_MESH_ID               = 4,
    BLUFI_DATA_MESH_PASSWORD         = 5,
    BLUFI_DATA_MESH_TYPE             = 6,

    /**
     * @brief mwifi network configuration
     */
    BLUFI_DATA_VOTE_PERCENTAGE       = 16,
    BLUFI_DATA_VOTE_MAX_COUNT        = 17,
    BLUFI_DATA_BACKOFF_RSSI          = 18,
    BLUFI_DATA_SCAN_MIN_COUNT        = 19,
    BLUFI_DATA_ATTEMPT_COUNT         = 20,
    BLUFI_DATA_MONITOR_IE_COUNT      = 21,
    BLUFI_DATA_ROOT_HEALING_MS       = 22,
    BLUFI_DATA_ROOT_CONFLICTS_ENABLE = 23,
    BLUFI_DATA_FIX_ROOT_ENABLE       = 24,
    BLUFI_DATA_CAPACITY_NUM          = 25,
    BLUFI_DATA_MAX_LAYER             = 26,
    BLUFI_DATA_MAX_CONNECTION        = 27,
    BLUFI_DATA_ASSOC_EXPIRE_MS       = 28,
    BLUFI_DATA_BEACON_INTERVAL_MS    = 29,
    BLUFI_DATA_PASSIVE_SCAN_MS       = 30,
    BLUFI_DATA_MONITOR_DURATION_MS   = 31,
    BLUFI_DATA_CNX_RSSI              = 32,
    BLUFI_DATA_SELECT_RSSI           = 33,
    BLUFI_DATA_SWITCH_RSSI           = 34,
    BLUFI_DATA_XON_QSIZE             = 35,
    BLUFI_DATA_RETRANSMIT_ENABLE     = 36,
    BLUFI_DATA_DROP_ENABLE           = 37,

    /**
     * @brief Device whitelist
     */
    BLUFI_DATA_WHITELIST             = 64,
};

/**
 * @brief Type of each packet
 */
typedef struct {
    uint8_t type;    /**< Type of packet (blufi data type)*/
    uint8_t len;     /**< Packet length */
    uint8_t data[0]; /**< Packet payload */
} blufi_data_element_t;

/**
 * @brief Manufacturer data carried by Bluetooth broadcast packets
 */
typedef struct {
    uint16_t company_id; /**< Company Identifiers (https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers) */
    uint8_t OUI[3];      /**< Used to filter other Bluetooth broadcast packets */
    struct {
        uint8_t version         : 2; /**< Version of the agreement */
        bool whitelist          : 1; /**< Enable whitelist filtering */
        bool whitelist_security : 1; /**< Enable whitelist security */
        bool only_beacon        : 1; /**< Send only beacon does not support connection */
        uint8_t reserved        : 3; /**< Reserved for late expansion */
    };
    uint8_t sta_addr[MWIFI_ADDR_LEN]; /**< STA's MAC address, used to distinguish different devices*/
    uint16_t tid;                   /**< Unique identifier for the type of device */
} blufi_adv_manufacturer_data_t;

static const char *TAG                    = "mconfig_blufi";
static mconfig_data_t *g_recv_config      = NULL;
static mconfig_blufi_config_t g_blufi_cfg = {0};
static QueueHandle_t g_rsa_decrypt_queue  = NULL;
static mbedtls_aes_context g_blufi_aes    = {0};
static esp_bd_addr_t g_spp_remote_bda     = {0x0};
static esp_timer_handle_t g_connect_timer = NULL;
static char *g_rsa_privkey                = NULL;
static char *g_rsa_pubkey                 = NULL;

static void mconfig_blufi_rsa_decrypt_task(void *arg)
{
    mdf_err_t ret       = MDF_OK;
    uint8_t *ciphertext = (uint8_t *)arg;
    uint8_t aes_key[MCONFIG_AES_KEY_LEN] = {0};

    ret = mconfig_rsa_decrypt(ciphertext, g_rsa_privkey, aes_key, MCONFIG_AES_KEY_LEN);
    MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "mconfig_rsa_decrypt, ret: 0x%x", -ret);

    MDF_LOGD("aes_key: " MACSTR, MAC2STR(aes_key));

    ret = mbedtls_aes_setkey_enc(&g_blufi_aes, aes_key, MCONFIG_AES_KEY_LEN * 8);
    MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "mbedtls_aes_setkey_enc, ret: 0x%x", -ret);

EXIT:
    xQueueSend(g_rsa_decrypt_queue, &ret, portMAX_DELAY);
    vTaskDelete(NULL);
}

extern void btc_blufi_report_error(esp_blufi_error_state_t state);
static void mconfig_blufi_dh_negotiate_data_handler(uint8_t *input_data, ssize_t input_len,
        uint8_t **output_data, ssize_t *output_size, bool *need_free)

{
    uint8_t type = input_data[0];
    static size_t s_dh_param_len = 0;

    switch (type) {
        case SEC_TYPE_RSA_PUBKEY_REQUEST: {
            MDF_LOGI("SEC_TYPE_RSA_PUBKEY_REQUEST");

            *output_data = (uint8_t *)g_rsa_pubkey;
            *output_size = strlen(g_rsa_pubkey);
            MDF_LOGV("output, size: %d, input_data: %s", *output_size, *output_data);

            *need_free = false;
            break;
        }

        case SEC_TYPE_RSA_AESKEY_RESPONSE: {
            mdf_err_t ret = 0;

            /**< RSA decryption requires 4k stack space,
            Stack overflow occurs when used directly in the callback function */
            g_rsa_decrypt_queue = xQueueCreate(1, sizeof(mdf_err_t));
            xTaskCreatePinnedToCore(mconfig_blufi_rsa_decrypt_task, "blufi_rsa_decrypt", 4 * 1024,
                                    input_data + 1, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL, CONFIG_MDF_TASK_PINNED_TO_CORE);

            xQueueReceive(g_rsa_decrypt_queue, &ret, portMAX_DELAY);
            vQueueDelete(g_rsa_decrypt_queue);
            g_rsa_decrypt_queue = NULL;

            if (ret != ESP_OK) {
                btc_blufi_report_error(ESP_BLUFI_ENCRYPT_ERROR);
            }

            break;
        }

        case SEC_TYPE_DH_PARAM_LEN:
            s_dh_param_len = ((input_data[1] << 8) | input_data[2]);
            MDF_LOGD("SEC_TYPE_DH_PARAM_LEN, len: %d", s_dh_param_len);
            break;

        case SEC_TYPE_DH_PARAM_DATA: {
            MDF_LOGD("SEC_TYPE_DH_PARAM_DATA");
            mdf_err_t ret = MDF_OK;
            uint8_t aes_key[MCONFIG_AES_KEY_LEN] = {0};
            uint8_t *param   = MDF_MALLOC(s_dh_param_len);
            uint8_t *pubkey  = MDF_MALLOC(MCONFIG_DH_PUBKEY_LEN);
            uint8_t *privkey = MDF_MALLOC(MCONFIG_DH_PRIVKEY_LEN);

            memcpy(param, input_data + 1, input_len - 1);

            ret = mconfig_dhm_gen_key(param, s_dh_param_len, privkey, pubkey);
            MDF_ERROR_GOTO(ret < 0, EXIT, "mconfig_dhm_gen_key, ret: %x", -ret);

            mbedtls_md5(privkey, MCONFIG_DH_PRIVKEY_LEN, aes_key);

            ret = mbedtls_aes_setkey_enc(&g_blufi_aes, aes_key, MCONFIG_AES_KEY_LEN * 8);
            MDF_ERROR_GOTO(ret < 0, EXIT, "mbedtls_aes_setkey_enc, ret: %x", -ret);

EXIT:
            MDF_FREE(privkey);
            MDF_FREE(param);

            if (ret != ESP_OK) {
                btc_blufi_report_error(ESP_BLUFI_READ_PARAM_ERROR);
                break;
            }

            *output_data = pubkey;
            *output_size = MCONFIG_DH_PUBKEY_LEN;
            *need_free   = true;
            break;
        }

        default:
            break;
    }
}

static ssize_t mconfig_blufi_aes_encrypt(uint8_t iv8, uint8_t *crypt_data, int crypt_len)
{
    MDF_PARAM_CHECK(crypt_data);

    int ret;
    size_t iv_offset = 0;
    uint8_t iv0[MCONFIG_AES_KEY_LEN] = {iv8};

    ret = mbedtls_aes_crypt_cfb128(&g_blufi_aes, MBEDTLS_AES_ENCRYPT, crypt_len,
                                   &iv_offset, iv0, crypt_data, crypt_data);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "mbedtls_aes_crypt_cfb128");

    return crypt_len;
}

static ssize_t mconfig_blufi_aes_decrypt(uint8_t iv8, uint8_t *crypt_data, int crypt_len)
{
    MDF_PARAM_CHECK(crypt_data);

    mdf_err_t ret             = MDF_OK;
    size_t iv_offset          = 0;
    uint8_t iv0[MCONFIG_AES_KEY_LEN] = {iv8};

    ret = mbedtls_aes_crypt_cfb128(&g_blufi_aes, MBEDTLS_AES_DECRYPT, crypt_len,
                                   &iv_offset, iv0, crypt_data, crypt_data);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "mbedtls_aes_crypt_cfb128");

    return crypt_len;
}

static uint16_t mconfig_blufi_crc_checksum(uint8_t iv8, uint8_t *data, int len)
{
    /* This iv8 ignore, not used */
    return crc16_be(0, data, len);
}

static mdf_err_t mconfig_blufi_security_init(void)
{
    g_rsa_privkey   = MDF_CALLOC(1, MCONFIG_RSA_PRIVKEY_PEM_SIZE);
    g_rsa_pubkey    = MDF_CALLOC(1, MCONFIG_RSA_PUBKEY_PEM_SIZE);
    mdf_err_t ret = mconfig_rsa_gen_key(g_rsa_privkey, g_rsa_pubkey);

    if (ret != ESP_OK) {
        MDF_LOGW("mconfig_rsa_gen_key, ret: 0x%x", -ret);
        MDF_FREE(g_rsa_privkey);
        MDF_FREE(g_rsa_pubkey);
        btc_blufi_report_error(ESP_BLUFI_INIT_SECURITY_ERROR);

        return MDF_FAIL;
    }

    mbedtls_aes_init(&g_blufi_aes);

    return MDF_OK;
}

static void mconfig_blufi_security_deinit(void)
{
    MDF_FREE(g_rsa_privkey);
    MDF_FREE(g_rsa_pubkey);

    mbedtls_aes_free(&g_blufi_aes);
}

static mdf_err_t blufi_wifi_event_handler(void *ctx, system_event_t *event)
{
    mdf_err_t ret = MDF_OK;
    static int s_disconnected_unknown_count   = 0;
    static int s_disconnected_handshake_count = 0;

    switch (event->event_id) {
        case SYSTEM_EVENT_STA_CONNECTED:
            mdf_event_loop_send(MDF_EVENT_MCONFIG_BLUFI_STA_CONNECTED, NULL);

            MDF_LOGI("BLUFI is success, Wi-Fi sta is connect");
            s_disconnected_unknown_count   = 0;
            s_disconnected_handshake_count = 0;

            esp_blufi_send_wifi_conn_report(WIFI_MODE_STA, ESP_BLUFI_STA_CONN_SUCCESS, 0, NULL);
            g_recv_config->config.channel = event->event_info.connected.channel;

            mconfig_queue_write(g_recv_config, 0);
            MDF_FREE(g_recv_config);

            mconfig_chain_slave_channel_switch_enable();
            mdf_event_loop_send(MDF_EVENT_MCONFIG_BLUFI_FINISH, NULL);
            break;

        case SYSTEM_EVENT_STA_DISCONNECTED: {
            esp_blufi_sta_conn_state_t sta_conn_state     = ESP_BLUFI_STA_CONN_SUCCESS;
            system_event_sta_disconnected_t *disconnected = &event->event_info.disconnected;

            MDF_LOGW("disconnected reason: %d", disconnected->reason);

            switch (disconnected->reason) {
                case WIFI_REASON_ASSOC_TOOMANY:
                    MDF_LOGW("WIFI_REASON_ASSOC_TOOMANY Disassociated because AP is unable to handle all currently associated STAs");
                    MDF_LOGW("The number of connected devices on the router exceeds the limit");

                    sta_conn_state = BLUFI_STA_TOOMANY_ERR;

                    break;

                case WIFI_REASON_MIC_FAILURE:              /**< disconnected reason code 14 */
                case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:   /**< disconnected reason code 15 */
                case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT: /**< disconnected reason code 16 */
                case WIFI_REASON_IE_IN_4WAY_DIFFERS:       /**< disconnected reason code 17 */
                case WIFI_REASON_HANDSHAKE_TIMEOUT:        /**< disconnected reason code 204 */
                    MDF_LOGW("Wi-Fi 4-way handshake failed, count: %d", s_disconnected_handshake_count);

                    if (++s_disconnected_handshake_count >= 3) {
                        MDF_LOGW("Router password error");
                        sta_conn_state = BLUFI_STA_PASSWORD_ERR;
                    }

                    break;

                default:
                    if (++s_disconnected_unknown_count > 10) {
                        MDF_LOGW("Router password error");
                        sta_conn_state = BLUFI_STA_PASSWORD_ERR;
                    }

                    break;
            }

            if (sta_conn_state != ESP_BLUFI_STA_CONN_SUCCESS) {
                uint32_t disconnected_reason = disconnected->reason;

                esp_blufi_send_error_info(sta_conn_state);
                mdf_event_loop_send(MDF_EVENT_MCONFIG_BLUFI_STA_DISCONNECTED, (void *)disconnected_reason);
                mconfig_chain_slave_channel_switch_enable();

                ret = esp_ble_gap_disconnect(g_spp_remote_bda);
                MDF_ERROR_BREAK(ret != ESP_OK, "<%s> esp_ble_gap_disconnect", mdf_err_to_name(ret));
            } else {
                ret = esp_wifi_connect();
                MDF_ERROR_CHECK(ret != ESP_OK, ret, "esp_wifi_connect");
            }

            break;
        }

        default:
            break;
    }

    return MDF_OK;
}

static mdf_err_t mconfig_blufi_adv_start()
{
    mdf_err_t ret = MDF_OK;
    esp_ble_adv_params_t adv_params = {
        .adv_int_min        = 0x100,
        .adv_int_max        = 0x100,
        .adv_type           = ADV_TYPE_IND,
        .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
        .channel_map        = ADV_CHNL_ALL,
        .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    };

    ret = esp_ble_gap_start_advertising(&adv_params);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "esp_ble_gap_config_adv_data");

    return MDF_OK;
}

static mdf_err_t mconfig_blufi_adv_config()
{
    mdf_err_t ret = MDF_OK;

    blufi_adv_manufacturer_data_t manufacturer_data = {
        .company_id = g_blufi_cfg.company_id,
        .tid        = g_blufi_cfg.tid,
        .OUI        = {0x4d, 0x44, 0x46},
        .version    = MCONFIG_BLUFI_VERSION,
        .only_beacon = g_blufi_cfg.only_beacon,

#ifdef CONFIG_MCONFIG_WHITELIST_ENABLE
        .whitelist  = true,
#endif /**< CONFIG_MCONFIG_WHITELIST_ENABLE */

#ifdef CONFIG_MCONFIG_WHITELIST_SECURITY_ENABLE
        .whitelist_security = true,
#endif /**< CONFIG_MCONFIG_WHITELIST_SECURITY_ENABLE */
    };

    esp_ble_adv_data_t g_scan_rsp_data = {
        .set_scan_rsp        = g_blufi_cfg.custom_size ? true : false,
        .include_name        = true,
        .include_txpower     = false,
        .min_interval        = 0x100,
        .max_interval        = 0x100,
        .appearance          = 0x00,
        .manufacturer_len    = sizeof(blufi_adv_manufacturer_data_t),
        .p_manufacturer_data = (uint8_t *) &manufacturer_data,
        .service_data_len    = 0,
        .p_service_data      = NULL,
        .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
    };

    ESP_ERROR_CHECK(esp_read_mac(manufacturer_data.sta_addr, ESP_MAC_WIFI_STA));

    if (g_scan_rsp_data.set_scan_rsp) {
        ret = esp_ble_gap_config_adv_data_raw(g_blufi_cfg.custom_data, g_blufi_cfg.custom_size);
        MDF_ERROR_CHECK(ret != MDF_OK, ret, "esp_ble_gap_config_adv_data_raw");
    }

    ret = esp_ble_gap_set_device_name(g_blufi_cfg.name);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "esp_ble_gap_set_device_name");

    ret = esp_ble_gap_config_adv_data(&g_scan_rsp_data);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "esp_ble_gap_config_adv_data");

    return MDF_OK;
}

static void mconfig_ble_connect_timer_delete(void)
{
    if (g_connect_timer) {
        esp_timer_stop(g_connect_timer);
        esp_timer_delete(g_connect_timer);
        g_connect_timer = NULL;
    }
}

static void mconfig_ble_connect_timercb(void *timer)
{
    if (!g_connect_timer) {
        MDF_LOGW("Timer has been deleted");
        return ;
    }

    mconfig_ble_connect_timer_delete();

    if (esp_ble_gap_disconnect(g_spp_remote_bda) != MDF_OK) {
        MDF_LOGW("Disconnect the physical connection of the peer device");
    } else {
        MDF_LOGI("Disconnect the physical connection of the peer device");
    }
}

static mdf_err_t mconfig_ble_connect_timer_create(void)
{
    mdf_err_t ret = MDF_OK;
    esp_timer_create_args_t timer_conf = {
        .callback = mconfig_ble_connect_timercb,
        .name     = "mconfig_ble_connect"
    };

    ret = esp_timer_create(&timer_conf, &g_connect_timer);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "Create an esp_timer instance");

    ret = esp_timer_start_once(g_connect_timer, MCONFIG_BLUFI_TIMEROUT_MS * 1000U);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "Start one-shot timer");

    return MDF_OK;
}

static void mconfig_blufi_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param)
{
    mdf_err_t ret              = MDF_OK;
    static uint8_t s_server_if = 0;
    static uint16_t s_conn_id  = 0;

    switch (event) {
        case ESP_BLUFI_EVENT_INIT_FINISH:
            MDF_LOGI("BLUFI init finish, set ble advertising data");
            ESP_ERROR_CHECK(mconfig_blufi_adv_config());
            break;

        case ESP_BLUFI_EVENT_DEINIT_FINISH:
            MDF_LOGI("BLUFI deinit finish");
            break;

        case ESP_BLUFI_EVENT_BLE_CONNECT:
            MDF_LOGD("BLUFI ble connect, server_if: %d, conn_id: %d",
                     param->connect.server_if, param->connect.conn_id);
            s_server_if = param->connect.server_if;
            s_conn_id   = param->connect.conn_id;
            memcpy(&g_spp_remote_bda, &param->connect.remote_bda, sizeof(esp_bd_addr_t));
            ESP_ERROR_CHECK(esp_ble_gap_stop_advertising());

            mdf_event_loop_send(MDF_EVENT_MCONFIG_BLUFI_CONNECTED, NULL);

            mconfig_ble_connect_timer_delete();
            ret = mconfig_ble_connect_timer_create();
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mconfig_ble_connect_timercb", mdf_err_to_name(ret));
            break;

        case ESP_BLUFI_EVENT_BLE_DISCONNECT:
            MDF_LOGI("BLUFI ble disconnect");

            mconfig_ble_connect_timer_delete();

            mconfig_blufi_adv_start();
            MDF_ERROR_BREAK(ret < 0, "<%s> mdf_event_loop_send", mdf_err_to_name(ret));

            ret = mdf_event_loop_send(MDF_EVENT_MCONFIG_BLUFI_DISCONNECTED, NULL);
            MDF_ERROR_BREAK(ret < 0, "<%s> mdf_event_loop_send", mdf_err_to_name(ret));
            break;

        case ESP_BLUFI_EVENT_RECV_SLAVE_DISCONNECT_BLE:
            MDF_LOGI("BLUFI close a gatt connection");
            esp_blufi_close(s_server_if, s_conn_id);
            break;

        case ESP_BLUFI_EVENT_RECV_USERNAME:
        case ESP_BLUFI_EVENT_RECV_CUSTOM_DATA: {
            bool config_flag = true;

            MDF_LOGV("param->custom_data.data_len: %d, param->custom_data.data: %s",
                     param->custom_data.data_len, param->custom_data.data);
            MDF_ERROR_BREAK(param->custom_data.data_len < 6,
                            "param->custom_data.data_len: %d", param->custom_data.data_len);

            mconfig_ble_connect_timer_delete();

            if (!g_recv_config) {
                g_recv_config = MDF_MALLOC(sizeof(mconfig_data_t));
            }

            memset(g_recv_config, 0, sizeof(mconfig_data_t));

            mwifi_init_config_t init_config = MWIFI_INIT_CONFIG_DEFAULT();
            memcpy(&g_recv_config->init_config, &init_config, sizeof(mwifi_init_config_t));

            for (int blufi_data_len = 0; blufi_data_len < param->custom_data.data_len;) {
                blufi_data_element_t *blufi_data = (blufi_data_element_t *)(param->custom_data.data + blufi_data_len);
                blufi_data_len += blufi_data->len + 2;
                MDF_LOGV("blufi_data type: %d, len: %d, data: %s",
                         blufi_data->type, blufi_data->len, blufi_data->data);

                /**
                 * @brief Compatible with previous version
                 *        enum blufi_data_type {
                 *            BLUFI_DATA_ROUTER_SSID   = 1,
                 *            BLUFI_DATA_ROUTER_PASSWD = 2,
                 *            BLUFI_DATA_MESH_ID       = 3,
                 *            BLUFI_DATA_RESERVED      = 4,
                 *            BLUFI_DATA_WHITELIST     = 5,
                 *        };
                 */
                if (event == ESP_BLUFI_EVENT_RECV_USERNAME) {
                    if (blufi_data->type == 3) {
                        blufi_data->type += (BLUFI_DATA_MESH_ID - 3);
                    } else if (blufi_data->type == 4) {
                        blufi_data->type = 0;
                    } else if (blufi_data->type == 5) {
                        blufi_data->type += (BLUFI_DATA_WHITELIST - 5);
                    }
                }

                switch (blufi_data->type) {
                    case BLUFI_DATA_ROUTER_SSID:
                        memcpy(g_recv_config->config.router_ssid, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Router ssid: %s", g_recv_config->config.router_ssid);
                        break;

                    case BLUFI_DATA_ROUTER_PASSWORD:
                        memcpy(g_recv_config->config.router_password, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Router password: %s", g_recv_config->config.router_password);
                        break;

                    case BLUFI_DATA_ROUTER_BSSID:
                        if (blufi_data->len != sizeof(g_recv_config->config.router_bssid)) {
                            MDF_LOGW("Router bssid: %s, len: %d", blufi_data->data, blufi_data->len);
                            config_flag = false;
                            break;
                        }

                        memcpy(g_recv_config->config.router_bssid, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Router bssid: " MACSTR, MAC2STR(g_recv_config->config.router_bssid));
                        break;

                    case BLUFI_DATA_MESH_ID:
                        if (blufi_data->len != sizeof(g_recv_config->config.mesh_id)
                                && !MWIFI_ADDR_IS_EMPTY(blufi_data->data)) {
                            MDF_LOGW("Mesh id: %s, len: %d", blufi_data->data, blufi_data->len);
                            config_flag = false;
                            break;
                        }

                        memcpy(g_recv_config->config.mesh_id, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Mesh id: " MACSTR, MAC2STR(g_recv_config->config.mesh_id));
                        break;

                    case BLUFI_DATA_MESH_PASSWORD:
                        if (blufi_data->len < 8) {
                            MDF_LOGW("Mesh password: %s, len: %d", blufi_data->data, blufi_data->len);
                            config_flag = false;
                            break;
                        }

                        memcpy(g_recv_config->config.mesh_password, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Mesh password: %s", g_recv_config->config.mesh_password);
                        break;

                    case BLUFI_DATA_MESH_TYPE:
                        memcpy(&g_recv_config->config.mesh_type, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Mesh type: %d", g_recv_config->config.mesh_type);
                        break;

                    case BLUFI_DATA_VOTE_PERCENTAGE:
                        memcpy(&g_recv_config->init_config.vote_percentage, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Mesh vote_percentage: %d", g_recv_config->init_config.vote_percentage);
                        break;

                    case BLUFI_DATA_VOTE_MAX_COUNT:
                        memcpy(&g_recv_config->init_config.vote_max_count, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Mesh vote_max_count: %d", g_recv_config->init_config.vote_max_count);
                        break;

                    case BLUFI_DATA_BACKOFF_RSSI:
                        memcpy(&g_recv_config->init_config.backoff_rssi, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Mesh backoff_rssi: %d", g_recv_config->init_config.backoff_rssi);
                        break;

                    case BLUFI_DATA_SCAN_MIN_COUNT:
                        memcpy(&g_recv_config->init_config.scan_min_count, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Mesh scan_min_count: %d", g_recv_config->init_config.scan_min_count);
                        break;

                    case BLUFI_DATA_ATTEMPT_COUNT:
                        memcpy(&g_recv_config->init_config.attempt_count, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Mesh attempt_count: %d", g_recv_config->init_config.attempt_count);
                        break;

                    case BLUFI_DATA_MONITOR_IE_COUNT:
                        memcpy(&g_recv_config->init_config.monitor_ie_count, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Mesh monitor_ie_count: %d", g_recv_config->init_config.monitor_ie_count);
                        break;

                    case BLUFI_DATA_ROOT_HEALING_MS:
                        memcpy(&g_recv_config->init_config.root_healing_ms, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Mesh root_healing_ms: %d", g_recv_config->init_config.root_healing_ms);
                        break;

                    case BLUFI_DATA_ROOT_CONFLICTS_ENABLE:
                        memcpy(&g_recv_config->init_config.root_conflicts_enable, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Mesh root_conflicts_enable: %d", g_recv_config->init_config.root_conflicts_enable);
                        break;

                    case BLUFI_DATA_CAPACITY_NUM:
                        memcpy(&g_recv_config->init_config.capacity_num, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Mesh capacity_num: %d", g_recv_config->init_config.capacity_num);
                        break;

                    case BLUFI_DATA_MAX_LAYER:
                        if (blufi_data->data[0] > 25) {
                            MDF_LOGW("Mesh max_layer: %d, len: %d", blufi_data->data[25], blufi_data->len);
                            config_flag = false;
                            break;
                        }

                        memcpy(&g_recv_config->init_config.max_layer, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Mesh max_layer: %d", g_recv_config->init_config.max_layer);
                        break;

                    case BLUFI_DATA_MAX_CONNECTION:
                        if (blufi_data->data[0] > 10) {
                            MDF_LOGW("Mesh max_connection: %d, len: %d", blufi_data->data[0], blufi_data->len);
                            config_flag = false;
                            break;
                        }

                        memcpy(&g_recv_config->init_config.max_connection, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Mesh max_connection: %d", g_recv_config->init_config.max_connection);
                        break;

                    case BLUFI_DATA_ASSOC_EXPIRE_MS:
                        memcpy(&g_recv_config->init_config.assoc_expire_ms, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Mesh assoc_expire_ms: %d", g_recv_config->init_config.assoc_expire_ms);
                        break;

                    case BLUFI_DATA_BEACON_INTERVAL_MS:
                        memcpy(&g_recv_config->init_config.beacon_interval_ms, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Mesh beacon_interval_ms: %d", g_recv_config->init_config.beacon_interval_ms);
                        break;

                    case BLUFI_DATA_PASSIVE_SCAN_MS:
                        memcpy(&g_recv_config->init_config.passive_scan_ms, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Mesh passive_scan_ms: %d", g_recv_config->init_config.passive_scan_ms);
                        break;

                    case BLUFI_DATA_MONITOR_DURATION_MS:
                        memcpy(&g_recv_config->init_config.monitor_duration_ms, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Mesh monitor_duration_ms: %d", g_recv_config->init_config.monitor_duration_ms);
                        break;

                    case BLUFI_DATA_CNX_RSSI:
                        memcpy(&g_recv_config->init_config.cnx_rssi, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Mesh cnx_rssi: %d", g_recv_config->init_config.cnx_rssi);
                        break;

                    case BLUFI_DATA_SELECT_RSSI:
                        memcpy(&g_recv_config->init_config.select_rssi, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Mesh select_rssi: %d", g_recv_config->init_config.select_rssi);
                        break;

                    case BLUFI_DATA_SWITCH_RSSI:
                        memcpy(&g_recv_config->init_config.switch_rssi, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Mesh switch_rssi: %d", g_recv_config->init_config.switch_rssi);
                        break;

                    case BLUFI_DATA_XON_QSIZE:
                        memcpy(&g_recv_config->init_config.xon_qsize, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Mesh xon_qsize: %d", g_recv_config->init_config.xon_qsize);
                        break;

                    case BLUFI_DATA_RETRANSMIT_ENABLE:
                        memcpy(&g_recv_config->init_config.retransmit_enable, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Mesh retransmit_enable: %d", g_recv_config->init_config.retransmit_enable);
                        break;

                    case BLUFI_DATA_DROP_ENABLE:
                        memcpy(&g_recv_config->init_config.data_drop_enable, blufi_data->data, blufi_data->len);
                        MDF_LOGD("Mesh data_drop_enable: %d", g_recv_config->init_config.data_drop_enable);
                        break;

                    case BLUFI_DATA_WHITELIST: {
                        size_t tmp_size = sizeof(mconfig_data_t) + g_recv_config->whitelist_size;
                        g_recv_config = MDF_REALLOC(g_recv_config, tmp_size + blufi_data->len);

                        memcpy((uint8_t *)g_recv_config->whitelist_data + g_recv_config->whitelist_size,
                               blufi_data->data, blufi_data->len);
                        g_recv_config->whitelist_size += blufi_data->len;

                        MDF_LOGD("Whitelist, number: %d", g_recv_config->whitelist_size / 6);

                        for (int i = 0; i < g_recv_config->whitelist_size / 6; i++) {
                            /**
                            * @brief Compatible with previous version
                            *        Convert mac address from bt to sta
                            */
                            if (event == ESP_BLUFI_EVENT_RECV_USERNAME) {
                                uint8_t *mac = g_recv_config->whitelist_data[i].addr;
                                *((int *)(mac + 2)) = htonl(htonl(*((int *)(mac + 2))) - 2);
                            }

                            MDF_LOGD("count: %d, data:" MACSTR,
                                     i, MAC2STR((g_recv_config->whitelist_data + i)->addr));
                        }

                        break;
                    }

                    default:
                        MDF_LOGW("recv data type: %d", blufi_data->type);
                        break;
                }
            }

            if (!config_flag) {
                esp_blufi_send_error_info(BLUFI_STA_CONFIG_ERR);
                break;
            }

            mconfig_chain_slave_channel_switch_disable();

            wifi_config_t sta_config = {0};

            memcpy(sta_config.sta.ssid, g_recv_config->config.router_ssid, strlen(g_recv_config->config.router_ssid));
            memcpy(sta_config.sta.bssid, g_recv_config->config.router_bssid, sizeof(g_recv_config->config.router_bssid));
            memcpy(sta_config.sta.password, g_recv_config->config.router_password, strlen(g_recv_config->config.router_password));

            ret = esp_wifi_set_config(WIFI_IF_STA, &sta_config);
            MDF_ERROR_BREAK(ret != ESP_OK, "<%s> Set the configuration of the ESP32 STA", mdf_err_to_name(ret));

            esp_event_loop_set_cb(blufi_wifi_event_handler, NULL);

            ret = esp_wifi_connect();
            MDF_ERROR_BREAK(ret != ESP_OK, "<%s> Connect the ESP32 WiFi station to the AP", mdf_err_to_name(ret));

            ret = mdf_event_loop_send(MDF_EVENT_MCONFIG_BLUFI_STA_CONNECTED, NULL);
            MDF_ERROR_BREAK(ret < 0, "<%s> Send the event to the event handler", mdf_err_to_name(ret));

            break;
        }

        case ESP_BLUFI_EVENT_REPORT_ERROR:
            MDF_LOGE("BLUFI report error, error code %d\n", param->report_error.state);
            esp_blufi_send_error_info(param->report_error.state);
            break;

        default:
            break;
    }
}

static void mconfig_blufi_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
            MDF_LOGI("start ble advertising");
            mconfig_blufi_adv_start();
            break;

        default:
            break;
    }
}

mdf_err_t mconfig_blufi_init(const mconfig_blufi_config_t *cfg)
{
    MDF_PARAM_CHECK(cfg);
    MDF_PARAM_CHECK(cfg->name && strlen(cfg->name) < MCONFIG_BLUFI_NAME_SIZE);
    MDF_PARAM_CHECK(cfg->custom_size <= MCONFIG_BLUFI_CUSTOM_SIZE);

    mdf_event_loop_send(MDF_EVENT_MCONFIG_BLUFI_STARTED, NULL);

    mdf_err_t ret                     = MDF_OK;
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    static esp_blufi_callbacks_t mconfig_blufi_callbacks = {
        .event_cb               = mconfig_blufi_event_callback,
        .negotiate_data_handler = mconfig_blufi_dh_negotiate_data_handler,
        .encrypt_func           = mconfig_blufi_aes_encrypt,
        .decrypt_func           = mconfig_blufi_aes_decrypt,
        .checksum_func          = mconfig_blufi_crc_checksum,
    };

    memcpy(&g_blufi_cfg, cfg, sizeof(mconfig_blufi_config_t));

    ret = mconfig_blufi_security_init();
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "mconfig_blufi_security_init");

    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

    ret = esp_bt_controller_init(&bt_cfg);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "Initialize bt controller");

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "Enable bt controller");

    ret = esp_bluedroid_init();
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "Initialize bluedroid");

    ret = esp_bluedroid_enable();
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "Enable bluedroid");

    vTaskDelay(esp_random() % 100 / portTICK_RATE_MS);

    ret = esp_ble_gap_register_callback(mconfig_blufi_gap_event_handler);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "BLE gap register callback");

    ret = esp_blufi_register_callbacks(&mconfig_blufi_callbacks);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "BLUFI register callbacks");

    ret = esp_blufi_profile_init();
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "Init BLUFI profile");

    ret = esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P7);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "Set BLE advertising TX power");

    ret = esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_CONN_HDL0, ESP_PWR_LVL_P7);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "Set BLE connection TX power");

    return MDF_OK;
}

mdf_err_t mconfig_blufi_deinit()
{
    mdf_err_t ret = MDF_OK;

    mconfig_ble_connect_timer_delete();

    esp_event_loop_set_cb(NULL, NULL);
    esp_wifi_disconnect();
    MDF_FREE(g_recv_config);

    ret = esp_blufi_profile_deinit();
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "esp_blufi_profile_deinit");

    mconfig_blufi_security_deinit();

    ret = esp_bluedroid_disable();
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "esp_bluedroid_disable");

    ret = esp_bluedroid_deinit();
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "esp_bluedroid_deinit");

    ret = esp_bt_controller_disable();
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "esp_bt_controller_disable");

    ret = esp_bt_controller_deinit();
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "esp_bt_controller_deinit");

    mdf_event_loop_send(MDF_EVENT_MCONFIG_BLUFI_STOPED, NULL);

    return MDF_OK;
}

#endif /**< CONFIG_BT_ENABLED */
