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
#include "esp_blufi_api.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"

#include "mbedtls/aes.h"
#include "mbedtls/dhm.h"
#include "mbedtls/md5.h"
#include "rom/crc.h"

#include "mdf_network_blufi.h"
#include "mdf_network_config.h"

#include "mdf_common.h"
#include "mdf_device_handle.h"

#define BLUFI_DH_SELF_PUB_KEY_LEN     128
#define BLUFI_DH_SELF_PUB_KEY_BIT_LEN (BLUFI_DH_SELF_PUB_KEY_LEN * 8)
#define BLUFI_SHARE_KEY_LEN           128
#define BLUFI_SHARE_KEY_BIT_LEN       (BLUFI_SHARE_KEY_LEN * 8)
#define BLUFI_PSK_LEN                 16
#define BLUFI_IV_LEN                  16

/*
   The SEC_TYPE_xxx is for self-defined packet data type in the procedure of "BLUFI negotiate key"
   If user use other negotiation procedure to exchange(or generate) key, should redefine the type by yourself.
 */
enum mdf_blufi_sec_type {
    SEC_TYPE_DH_PARAM_LEN = 0,
    SEC_TYPE_DH_PARAM_DATA,
    SEC_TYPE_DH_P,
    SEC_TYPE_DH_G,
    SEC_TYPE_DH_PUBLIC,
};

typedef struct {
    uint8_t  self_public_key[BLUFI_DH_SELF_PUB_KEY_LEN];
    uint8_t  share_key[BLUFI_SHARE_KEY_LEN];
    size_t   share_len;
    uint8_t  psk[BLUFI_PSK_LEN];
    uint8_t  *dh_param;
    int      dh_param_len;
    uint8_t  iv[BLUFI_IV_LEN];
    mbedtls_dhm_context dhm;
    mbedtls_aes_context aes;
} mdf_blufi_security_t;

enum {
    MDF_BLUFI_DATA_ROUTER_SSID = 1,
    MDF_BLUFI_DATA_ROUTER_PASSWD,
    MDF_BLUFI_DATA_MDF_ID,
    MDF_BLUFI_DATA_MDF_TOKEN,
    MDF_BLUFI_DATA_DEVICE_WHITELIST,
};

typedef struct {
    uint8_t type;
    uint8_t len;
    uint8_t data[0];
} mdf_blufi_data_t;


static uint8_t mdf_blufi_service_uuid128[32] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    // first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
};

static esp_ble_adv_data_t s_mdf_blufi_adv_data = {
    .set_scan_rsp        = false,
    .include_name        = true,
    .include_txpower     = true,
    .min_interval        = 0x100,
    .max_interval        = 0x100,
    .appearance          = 0x00,
    .manufacturer_len    = 0,
    .p_manufacturer_data = NULL,
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = 16,
    .p_service_uuid      = (uint8_t *)mdf_blufi_service_uuid128,
    .flag                = 0x6,
};

static const char *TAG                         = "mdf_blufi_network_config";
static mdf_blufi_security_t *g_mdf_blufi_sec = NULL;
static bool mdf_blufi_mem_release_flag        = false;
static network_config_t g_network_config  = {0};
static bool g_blufi_connecting_wifi            = false;

static esp_err_t mdf_blufi_random(void *rng_state, uint8_t *output, size_t len)
{
    MDF_PARAM_CHECK(output);
    MDF_PARAM_CHECK(len > 0);

    while (len--) {
        *output++ = esp_random();
    }

    return ESP_OK;
}

extern void btc_blufi_report_error(esp_blufi_error_state_t state);

static void mdf_blufi_dh_negotiate_data_handler(uint8_t *data, int len,
        uint8_t **output_data, int *output_len, bool *need_free)

{
    MDF_ASSERT(data);
    MDF_ASSERT(output_data);
    MDF_ASSERT(output_len);
    MDF_ASSERT(need_free);
    MDF_ASSERT(g_mdf_blufi_sec);

    int ret;
    uint8_t type = data[0];
    uint8_t *dh_param = NULL;

    switch (type) {
        case SEC_TYPE_DH_PARAM_LEN:
            MDF_LOGD("SEC_TYPE_DH_PARAM_LEN");
            g_mdf_blufi_sec->dh_param_len = ((data[1] << 8) | data[2]);

            mdf_free(g_mdf_blufi_sec->dh_param);
            g_mdf_blufi_sec->dh_param = mdf_calloc(1, g_mdf_blufi_sec->dh_param_len);
            break;

        case SEC_TYPE_DH_PARAM_DATA:
            MDF_LOGD("SEC_TYPE_DH_PARAM_DATA: %p, %p g_mdf_blufi_sec->dh_param_len: %d",
                     &g_mdf_blufi_sec->dhm, &g_mdf_blufi_sec->dh_param, g_mdf_blufi_sec->dh_param_len);

            if (g_mdf_blufi_sec->dh_param == NULL) {
                MDF_LOGE("blufi_sec->dh_param == NULL\n");
                btc_blufi_report_error(ESP_BLUFI_DH_PARAM_ERROR);
                return;
            }

            memcpy(g_mdf_blufi_sec->dh_param, &data[1], g_mdf_blufi_sec->dh_param_len);

            dh_param = g_mdf_blufi_sec->dh_param;
            ret = mbedtls_dhm_read_params(&g_mdf_blufi_sec->dhm, &dh_param,
                                          &dh_param[g_mdf_blufi_sec->dh_param_len]);

            if (ret != ESP_OK) {
                MDF_LOGE("mbedtls_dhm_read_params, ret: %d", ret);
                btc_blufi_report_error(ESP_BLUFI_READ_PARAM_ERROR);
                return;
            }

            mdf_free(g_mdf_blufi_sec->dh_param);

            ret = mbedtls_dhm_make_public(&g_mdf_blufi_sec->dhm, (int) mbedtls_mpi_size(&g_mdf_blufi_sec->dhm.P),
                                          g_mdf_blufi_sec->self_public_key, g_mdf_blufi_sec->dhm.len, mdf_blufi_random, NULL);

            if (ret != ESP_OK) {
                MDF_LOGE("mbedtls_dhm_make_public, ret: %d", ret);
                btc_blufi_report_error(ESP_BLUFI_MAKE_PUBLIC_ERROR);
                return;
            }

            mbedtls_dhm_calc_secret(&g_mdf_blufi_sec->dhm, g_mdf_blufi_sec->share_key, BLUFI_SHARE_KEY_BIT_LEN,
                                    &g_mdf_blufi_sec->share_len, NULL, NULL);

            mbedtls_md5(g_mdf_blufi_sec->share_key, g_mdf_blufi_sec->share_len, g_mdf_blufi_sec->psk);

            mbedtls_aes_setkey_enc(&g_mdf_blufi_sec->aes, g_mdf_blufi_sec->psk, 128);

            /* alloc output data */
            *output_data = &g_mdf_blufi_sec->self_public_key[0];
            *output_len  = g_mdf_blufi_sec->dhm.len;
            *need_free   = false;
            break;

        default:
            break;
    }
}

static ssize_t mdf_blufi_aes_encrypt(uint8_t iv8, uint8_t *crypt_data, int crypt_len)
{
    MDF_PARAM_CHECK(g_mdf_blufi_sec);
    MDF_PARAM_CHECK(crypt_data);

    int ret;
    size_t iv_offset = 0;
    uint8_t iv0[BLUFI_IV_LEN];

    memcpy(iv0, g_mdf_blufi_sec->iv, sizeof(g_mdf_blufi_sec->iv));
    iv0[0] = iv8;   /* set iv8 as the iv0[0] */

    ret = mbedtls_aes_crypt_cfb128(&g_mdf_blufi_sec->aes, MBEDTLS_AES_ENCRYPT, crypt_len,
                                   &iv_offset, iv0, crypt_data, crypt_data);
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "mbedtls_aes_crypt_cfb128, ret: %d", ret);

    return crypt_len;
}

static ssize_t mdf_blufi_aes_decrypt(uint8_t iv8, uint8_t *crypt_data, int crypt_len)
{
    MDF_PARAM_CHECK(g_mdf_blufi_sec);
    MDF_PARAM_CHECK(crypt_data);

    int ret;
    size_t iv_offset = 0;
    uint8_t iv0[BLUFI_IV_LEN];

    memcpy(iv0, g_mdf_blufi_sec->iv, sizeof(g_mdf_blufi_sec->iv));
    iv0[0] = iv8;   /* set iv8 as the iv0[0] */

    ret = mbedtls_aes_crypt_cfb128(&g_mdf_blufi_sec->aes, MBEDTLS_AES_DECRYPT, crypt_len,
                                   &iv_offset, iv0, crypt_data, crypt_data);
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "mbedtls_aes_crypt_cfb128, ret: %d", ret);

    return crypt_len;
}

static uint16_t mdf_blufi_crc_checksum(uint8_t iv8, uint8_t *data, int len)
{
    /* This iv8 ignore, not used */
    return crc16_be(0, data, len);
}

static void mdf_blufi_security_init(void)
{
    if (!g_mdf_blufi_sec) {
        g_mdf_blufi_sec = mdf_calloc(1, sizeof(mdf_blufi_security_t));
        mbedtls_dhm_init(&g_mdf_blufi_sec->dhm);
        mbedtls_aes_init(&g_mdf_blufi_sec->aes);
        memset(g_mdf_blufi_sec->iv, 0x0, BLUFI_IV_LEN);
    }
}

static void mdf_blufi_security_deinit(void)
{
    if (g_mdf_blufi_sec) {
        mbedtls_dhm_free(&g_mdf_blufi_sec->dhm);
        mbedtls_aes_free(&g_mdf_blufi_sec->aes);
        mdf_free(g_mdf_blufi_sec->dh_param);
        mdf_free(g_mdf_blufi_sec);
    }
}

static esp_err_t mdf_blufi_set_name()
{
    esp_err_t ret             = ESP_OK;
    char blufi_name[16]       = {0};
    const uint8_t *blufi_addr = esp_bt_dev_get_address();

    sprintf(blufi_name, "MESH_%d_%02x%02x%02x",
            mdf_device_get_tid(), blufi_addr[3], blufi_addr[4], blufi_addr[5]);

    MDF_LOGI("mdf blufi name: %s", blufi_name);
    ret = esp_ble_gap_set_device_name(blufi_name);
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "esp_ble_gap_set_device_name, ret: %d", ret);

    return ESP_OK;
}

esp_err_t mdf_blufi_gap_start_advertising()
{
    esp_err_t ret = ESP_OK;
    static esp_ble_adv_params_t s_adv_params = {
        .adv_int_min        = 0x100,
        .adv_int_max        = 0x100,
        .adv_type           = ADV_TYPE_IND,
        .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
        .channel_map        = ADV_CHNL_ALL,
        .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    };

    ret = esp_ble_gap_start_advertising((esp_ble_adv_params_t *)&s_adv_params);
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "esp_ble_gap_config_adv_data, ret: %d", ret);

    return ESP_OK;
}

bool mdf_blufi_connecting_wifi()
{
    return g_blufi_connecting_wifi;
}

static esp_err_t blufi_wifi_event_handler(void *ctx, system_event_t *event)
{
    esp_err_t ret               = ESP_OK;
    esp_blufi_extra_info_t info = {0};

    switch (event->event_id) {
        case SYSTEM_EVENT_STA_CONNECTED:
            MDF_LOGI("wifi is connect");

            g_network_config.channel = event->event_info.connected.channel;
            mdf_network_send_config(&g_network_config);

            esp_blufi_send_wifi_conn_report(WIFI_MODE_STA, ESP_BLUFI_STA_CONN_SUCCESS, 0, &info);
            memset(&g_network_config, 0, sizeof(network_config_t));
            break;

        case SYSTEM_EVENT_STA_DISCONNECTED: {
            static int disconnected_count                 = 0;
            system_event_sta_disconnected_t *disconnected = &event->event_info.disconnected;

            if (disconnected->reason == WIFI_REASON_HANDSHAKE_TIMEOUT) {
                MDF_LOGE("password is error");
                ret = mdf_event_loop_send(MDF_EVENT_NETWORK_FAIL, NULL);
                MDF_ERROR_BREAK(ret < 0, "mdf_event_loop_send, ret: %d", ret);
                esp_blufi_send_wifi_conn_report(WIFI_MODE_STA, 0x02, 0, &info);
                g_blufi_connecting_wifi = false;
                break;
            }

            if (disconnected_count++ > 10) {
                esp_blufi_send_wifi_conn_report(WIFI_MODE_STA, ESP_BLUFI_STA_CONN_FAIL, 0, &info);

                ret = mdf_event_loop_send(MDF_EVENT_NETWORK_FAIL, NULL);
                MDF_ERROR_BREAK(ret < 0, "mdf_event_loop_send, ret: %d", ret);

                g_blufi_connecting_wifi = false;
                disconnected_count      = 0;
                break;
            }

            ret = esp_wifi_connect();
            MDF_ERROR_BREAK(ret != ESP_OK, "esp_wifi_connect, ret: %d", ret);

            break;
        }

        default:
            break;
    }

    return ESP_OK;
}

static void mdf_blufi_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param)
{
    static uint8_t s_server_if = 0;
    static uint16_t s_conn_id  = 0;
    esp_err_t ret              = ESP_OK;

    switch (event) {
        case ESP_BLUFI_EVENT_INIT_FINISH:
            MDF_LOGI("blufi init finish");
            ESP_ERROR_CHECK(mdf_blufi_set_name());
            ESP_ERROR_CHECK(esp_ble_gap_config_adv_data(&s_mdf_blufi_adv_data));
            break;

        case ESP_BLUFI_EVENT_DEINIT_FINISH:
            MDF_LOGI("blufi deinit finish");
            break;

        case ESP_BLUFI_EVENT_BLE_CONNECT:
            MDF_LOGD("blufi ble connect, server_if: %d, conn_id: %d",
                     param->connect.server_if, param->connect.conn_id);
            s_server_if = param->connect.server_if;
            s_conn_id   = param->connect.conn_id;
            ESP_ERROR_CHECK(esp_ble_gap_stop_advertising());
            mdf_blufi_security_init();
            break;

        case ESP_BLUFI_EVENT_BLE_DISCONNECT:
            MDF_LOGI("blufi ble disconnect");
            mdf_blufi_security_deinit();
            ESP_ERROR_CHECK(mdf_blufi_gap_start_advertising());
            break;

        case ESP_BLUFI_EVENT_RECV_SLAVE_DISCONNECT_BLE:
            MDF_LOGI("blufi close a gatt connection");
            esp_blufi_close(s_server_if, s_conn_id);
            break;

        case ESP_BLUFI_EVENT_RECV_USERNAME: {
            MDF_LOGV("param->username.name_len: %d, param->username.name: %s",
                     param->username.name_len, param->username.name);
            MDF_ERROR_CHECK(param->username.name_len < 6, ; ,
                            "param->username.name_len: %d", param->username.name_len);

            memset(&g_network_config, 0, sizeof(network_config_t));

            mdf_network_delete_whitelist();

            for (int blufi_data_len = 0; blufi_data_len < param->username.name_len;) {
                mdf_blufi_data_t *blufi_data = (mdf_blufi_data_t *)(param->username.name + blufi_data_len);
                blufi_data_len += blufi_data->len + 2;
                MDF_LOGV("blufi_data type: %d, len: %d, data: %s",
                         blufi_data->type, blufi_data->len, blufi_data->data);

                switch (blufi_data->type) {
                    case MDF_BLUFI_DATA_ROUTER_SSID:
                        memcpy(g_network_config.ssid, blufi_data->data, blufi_data->len);
                        MDF_LOGD("recv ssid: %s", g_network_config.ssid);
                        break;

                    case MDF_BLUFI_DATA_ROUTER_PASSWD:
                        memcpy(g_network_config.password, blufi_data->data, blufi_data->len);
                        MDF_LOGD("recv password:%s", g_network_config.password);
                        break;

                    case MDF_BLUFI_DATA_MDF_ID:
                        memcpy(g_network_config.mesh_id, blufi_data->data, blufi_data->len);
                        MDF_LOGD("recv mesh_id: "MACSTR, MAC2STR(g_network_config.mesh_id));
                        break;

                    case MDF_BLUFI_DATA_MDF_TOKEN:
                        memcpy(g_network_config.token, blufi_data->data, blufi_data->len);
                        MDF_LOGD("recv token: %s", g_network_config.token);
                        break;

                    case MDF_BLUFI_DATA_DEVICE_WHITELIST: {
                        int auto_network_whitelist_num = (blufi_data->len / 6 > AUTO_NETWORK_WHITELIST_MAX_NUM) ?
                                                         AUTO_NETWORK_WHITELIST_MAX_NUM : (blufi_data->len / 6);
                        ret = mdf_network_add_whitelist(blufi_data->data, auto_network_whitelist_num);
                        MDF_ERROR_BREAK(ret < 0, "mdf_network_add_whitelist, ret: %d", ret);
                        MDF_LOGD("recv mdf_addr, len: %d, data:" MACSTR,
                                 blufi_data->len, MAC2STR(blufi_data->data));
                        break;
                    }

                    default:
                        MDF_LOGW("recv data type: %d", blufi_data->type);
                        break;
                }
            }

            wifi_config_t sta_config = {0};

            memcpy(sta_config.sta.ssid, g_network_config.ssid, strlen(g_network_config.ssid));
            memcpy(sta_config.sta.password, g_network_config.password, strlen(g_network_config.password));

            ret = esp_wifi_set_config(WIFI_IF_STA, &sta_config);
            MDF_ERROR_BREAK(ret != ESP_OK, "esp_wifi_set_config, ret: %d", ret);

            ret = esp_wifi_connect();
            MDF_ERROR_BREAK(ret != ESP_OK, "esp_wifi_connect, ret: %d", ret);

            g_blufi_connecting_wifi = true;

            ret = mdf_event_loop_send(MDF_EVENT_CONNECT_ROUTER, NULL);
            MDF_ERROR_BREAK(ret < 0, "mdf_event_loop_send, ret: %d", ret);

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

static void mdf_blufi_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            ESP_ERROR_CHECK(mdf_blufi_gap_start_advertising());
            break;

        default:
            break;
    }
}

esp_err_t mdf_blufi_init()
{
    esp_err_t ret                    = ESP_OK;
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    static esp_blufi_callbacks_t mdf_blufi_callbacks = {
        .event_cb               = mdf_blufi_event_callback,
        .negotiate_data_handler = mdf_blufi_dh_negotiate_data_handler,
        .encrypt_func           = mdf_blufi_aes_encrypt,
        .decrypt_func           = mdf_blufi_aes_decrypt,
        .checksum_func          = mdf_blufi_crc_checksum,
    };

    esp_event_loop_set_cb(blufi_wifi_event_handler, NULL);

    ret = esp_bt_controller_init(&bt_cfg);
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "initialize bt controller, ret: %d", ret);

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "enable bt controller, ret: %d", ret);

    ret = esp_bluedroid_init();
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "init bluedroid, ret: %d", ret);

    ret = esp_bluedroid_enable();
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "enable bluedroid, ret: %d", ret);

    MDF_LOGI("bt addr: "ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(esp_bt_dev_get_address()));
    MDF_LOGI("blufi version: %04x", esp_blufi_get_version());

    mdf_blufi_security_init();

    ret = esp_ble_gap_register_callback(mdf_blufi_gap_event_handler);
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "ble gap register callback, ret: %d", ret);

    ret = esp_blufi_register_callbacks(&mdf_blufi_callbacks);
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "blufi register callbacks, ret: %d", ret);

    ret = esp_blufi_profile_init();
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "init blufi profile, ret: %d", ret);

    ret = esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P7);
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "esp_ble_tx_power_set, ret: %d", ret);

    ret = esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_CONN_HDL0, ESP_PWR_LVL_P7);
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "esp_ble_tx_power_set, ret: %d", ret);

    return ESP_OK;
}

bool mdf_blufi_mem_is_release()
{
    return mdf_blufi_mem_release_flag;
}

esp_err_t mdf_blufi_mem_release()
{
    if (mdf_blufi_mem_is_release()) {
        return ESP_OK;
    }

    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

#ifdef CONFIG_RELEASE_BLE_MEMORY
    esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
    mdf_blufi_mem_release_flag = true;
#endif

    return ESP_OK;
}

esp_err_t mdf_blufi_deinit()
{
    esp_err_t ret = ESP_OK;

    ret = esp_blufi_profile_deinit();
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "esp_blufi_profile_deinit, ret: %d", ret);

    mdf_blufi_security_deinit();

    ret = esp_bluedroid_disable();
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "esp_bluedroid_disable, ret: %d", ret);

    ret = esp_bluedroid_deinit();
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "esp_bluedroid_deinit, ret: %d", ret);

    ret = esp_bt_controller_disable();
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "esp_bt_controller_disable, ret: %d", ret);

    ret = esp_bt_controller_deinit();
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "esp_bt_controller_deinit, ret: %d", ret);

    ret = mdf_blufi_mem_release();
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "mdf_blufi_mem_release, ret: %d", ret);

    esp_event_loop_set_cb(NULL, NULL);
    g_blufi_connecting_wifi = false;

    return ESP_OK;
}
