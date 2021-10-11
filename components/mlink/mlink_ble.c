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

#include "sdkconfig.h"
#include "mlink_ble.h"

#if CONFIG_BT_ENABLED
#if !CONFIG_BT_NIMBLE_ENABLED

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_wifi.h"

#define SPP_PROFILE_NUM     1
#define SPP_PROFILE_APP_IDX 0
#define ESP_SPP_APP_ID      0x56
#define SPP_DATA_MAX_LEN    (512)

enum {
    SPP_IDX_SVC,

    SPP_IDX_DATA_CHAR,
    SPP_IDX_DATA_VAL,
    SPP_IDX_DATA_CFG,

    SPP_IDX_NB,
};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

static bool g_data_config_is_enable             = false;
static bool g_gatts_is_connected                = false;
static uint16_t g_spp_conn_id                   = 0xffff;
static esp_gatt_if_t g_spp_gatts_if             = 0xff;
static uint16_t spp_mtu_size                    = 23;
static uint16_t g_spp_handle_table[SPP_IDX_NB]  = {0};

static bool g_ble_init_flag    = false;
static bool g_ble_scan_flag    = false;
static const char *TAG         = "mlink_ble";
static mlink_ble_cb_t g_gatts_event_read_cb  = NULL;
static mlink_ble_cb_t g_gatts_event_write_cb = NULL;
static mlink_ble_cb_t g_gatts_event_scan_cb  = NULL;

static esp_ble_adv_params_t g_ble_adv_params = {
    .adv_int_min        = 0x100,
    .adv_int_max        = 0x100,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static esp_ble_scan_params_t g_ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x800,
    .scan_window            = 0x50,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};

typedef struct {
    uint16_t company_id; /**< Company Identifiers (https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers) */
    uint8_t OUI[3];      /**< Used to filter other Bluetooth broadcast packets */
    struct {
        uint8_t version  : 2; /**< Version of the agreement */
        uint8_t reserved : 6; /**< Reserved for late expansion */
    };
    uint8_t sta_addr[6]; /**< STA's MAC address, used to distinguish different devices*/
} mlink_ble_adv_manufacturer_t;

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

/* one gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst spp_profile_tab[] = {
    [SPP_PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

static mdf_err_t spp_gatts_db_config(esp_gatt_if_t gatts_if)
{
    mdf_err_t ret                        = ESP_OK;
    uint16_t primary_service_uuid         = ESP_GATT_UUID_PRI_SERVICE;
    uint16_t character_declaration_uuid   = ESP_GATT_UUID_CHAR_DECLARE;
    uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
    uint16_t spp_service_uuid             = 0xABF0;
    uint16_t spp_data_uuid                = 0xABF1;

    uint8_t char_prop_read_write_notify   = ESP_GATT_CHAR_PROP_BIT_WRITE
                                            | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;

    /* SPP Service - data notify characteristic, notify&read */
    uint8_t  spp_data_val[]               = {0x00};
    uint8_t  spp_data_ccc[2]              = {0x00, 0x00};

    /* Full HRS Database Description - Used to add attributes into the database */
    esp_gatts_attr_db_t spp_gatt_db[SPP_IDX_NB] = {
        /* SPP -  Service Declaration */
        [SPP_IDX_SVC] = {
            {ESP_GATT_AUTO_RSP}, {
                ESP_UUID_LEN_16, (uint8_t *) &primary_service_uuid, ESP_GATT_PERM_READ,
                sizeof(spp_service_uuid), sizeof(spp_service_uuid), (uint8_t *) &spp_service_uuid
            }
        },

        /* SPP -  data notify characteristic Declaration */
        [SPP_IDX_DATA_CHAR]  = {
            {ESP_GATT_AUTO_RSP}, {
                ESP_UUID_LEN_16, (uint8_t *) &character_declaration_uuid, ESP_GATT_PERM_READ,
                sizeof(uint8_t), sizeof(uint8_t), (uint8_t *) &char_prop_read_write_notify
            }
        },

        /* SPP -  data notify characteristic Value */
        [SPP_IDX_DATA_VAL] = {
            {ESP_GATT_AUTO_RSP}, {
                ESP_UUID_LEN_16, (uint8_t *) &spp_data_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                SPP_DATA_MAX_LEN, sizeof(spp_data_val), (uint8_t *)spp_data_val
            }
        },

        /* SPP -  data notify characteristic - Client Characteristic Configuration Descriptor */
        [SPP_IDX_DATA_CFG] = {
            {ESP_GATT_AUTO_RSP}, {
                ESP_UUID_LEN_16, (uint8_t *) &character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                sizeof(uint16_t), sizeof(spp_data_ccc), (uint8_t *)spp_data_ccc
            }
        },
    };

    ret = esp_ble_gatts_create_attr_tab(spp_gatt_db, gatts_if, SPP_IDX_NB, SPP_IDX_SVC);
    MDF_ERROR_CHECK(ret, ret, "esp_ble_gatts_create_attr_tab");

    return ESP_OK;
}

static mlink_ble_config_t g_ble_config = {
    .name = "BLE-GATEWAY",
    .company_id = MCOMMON_ESPRESSIF_ID,
};

static mdf_err_t find_char_and_desr_index(uint16_t handle)
{
    for (int i = 0; i < SPP_IDX_NB ; i++) {
        if (handle == g_spp_handle_table[i]) {
            return i;
        }
    }

    return MDF_FAIL;
}

static void ble_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT: {
            MDF_LOGI("start adv successfully");
            esp_ble_gap_start_advertising(&g_ble_adv_params);
            break;
        }

        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            /* adv start complete event to indicate adv start successfully or failed */
            MDF_ERROR_BREAK(param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS,
                            "ESP_GAP_BLE_ADV_START_COMPLETE_EVT, status: %d", param->adv_start_cmpl.status);
            break;

        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            MDF_ERROR_BREAK(param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS,
                            "ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT, status: %d", param->adv_stop_cmpl.status);

            MDF_LOGI("stop adv successfully");
            break;

        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
            /* the unit of the duration is second, 0 means scan permanently */
            esp_ble_gap_start_scanning(0);
            break;

        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
            /* scan start complete event to indicate scan start successfully or failed */
            MDF_ERROR_BREAK(param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS,
                            "ESP_GAP_BLE_SCAN_START_COMPLETE_EVT, status: %d", param->scan_start_cmpl.status);

            MDF_LOGI("start scan successfully");
            break;

        case ESP_GAP_BLE_SCAN_RESULT_EVT: {
            esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;

            MDF_ERROR_BREAK(scan_result->scan_rst.search_evt != ESP_GAP_SEARCH_INQ_RES_EVT,
                            "ESP_GAP_BLE_SCAN_RESULT_EVT, status: %d", param->scan_start_cmpl.status);

            if (g_gatts_event_scan_cb) {
                mdf_err_t ret = g_gatts_event_scan_cb(scan_result, sizeof(esp_ble_gap_cb_param_t));
                MDF_ERROR_BREAK(ret < 0, "g_gatts_event_scan_cb");
            }

            break;
        }

        default:
            break;
    }
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    esp_ble_gatts_cb_param_t *p_data        = (esp_ble_gatts_cb_param_t *)param;
    mdf_err_t ret                           = ESP_OK;
    static uint8_t *s_write_prep_ringbuffer = NULL;
    static size_t s_write_prep_size         = 0;

    MDF_LOGD("esp_gatts_cb_event: %d", event);

    switch (event) {
        case ESP_GATTS_REG_EVT:
            MDF_LOGI("ESP_GATTS_REG_EVT");
            spp_gatts_db_config(gatts_if);
            break;

        case ESP_GATTS_READ_EVT:
            ret = find_char_and_desr_index(p_data->read.handle);
            MDF_LOGI("ESP_GATTS_READ_EVT");

            if (ret == SPP_IDX_DATA_VAL && g_gatts_event_read_cb) {
                uint8_t *data = MDF_REALLOC_RETRY(NULL, SPP_DATA_MAX_LEN);
                size_t size = g_gatts_event_read_cb(data, SPP_DATA_MAX_LEN);

                esp_ble_gatts_send_indicate(g_spp_gatts_if, g_spp_conn_id, g_spp_handle_table[SPP_IDX_DATA_VAL],
                                            size, (uint8_t *)data, false);
                MDF_FREE(data);
            }

            break;

        case ESP_GATTS_WRITE_EVT: {
            ret = find_char_and_desr_index(p_data->write.handle);
            MDF_LOGD("ESP_GATTS_WRITE_EVT, prepare: %d, handle: %d, len: %d, value: %s",
                     p_data->write.is_prep, ret, p_data->write.len, p_data->write.value);

            if (ret == SPP_IDX_DATA_CFG) {
                if ((p_data->write.len == 2) && (p_data->write.value[0] == 0x01)
                        && (p_data->write.value[1] == 0x00)) {
                    g_data_config_is_enable = true;
                } else if ((p_data->write.len == 2) && (p_data->write.value[0] == 0x00)
                           && (p_data->write.value[1] == 0x00)) {
                    g_data_config_is_enable = false;
                }
            } else if (ret == SPP_IDX_DATA_VAL) {
                if (p_data->write.is_prep) {
                    if (!s_write_prep_ringbuffer) {
                        s_write_prep_ringbuffer = MDF_CALLOC(1, SPP_DATA_MAX_LEN);
                    }

                    memcpy(s_write_prep_ringbuffer + s_write_prep_size, p_data->write.value, p_data->write.len);
                    s_write_prep_size += p_data->write.len;
                } else if (g_gatts_event_write_cb) {
                    ret = g_gatts_event_write_cb(p_data->write.value, p_data->write.len);
                    MDF_ERROR_BREAK(ret < 0, "g_gatts_event_write_cb");
                }
            }

            break;
        }

        case ESP_GATTS_EXEC_WRITE_EVT: {
            MDF_LOGI("ESP_GATTS_EXEC_WRITE_EVT");

            if (p_data->exec_write.exec_write_flag) {
                MDF_LOGD("print_write_buffer: %s", s_write_prep_ringbuffer);
                g_gatts_event_write_cb(s_write_prep_ringbuffer, s_write_prep_size);
            }

            MDF_FREE(s_write_prep_ringbuffer);
            s_write_prep_size = 0;
            break;
        }

        case ESP_GATTS_MTU_EVT:
            spp_mtu_size = p_data->mtu.mtu;
            MDF_LOGI("ESP_GATTS_MTU_EVT, mtu_size: %d", p_data->mtu.mtu);
            break;

        case ESP_GATTS_CONNECT_EVT:
            MDF_LOGI("ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);

            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
            conn_params.latency = 0;
            conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
            conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
            conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
            //start sent the update connection parameters to the peer device.
            esp_ble_gap_update_conn_params(&conn_params);
            g_gatts_is_connected = true;
            g_spp_gatts_if       = gatts_if;
            g_spp_conn_id        = param->connect.conn_id;
            break;

        case ESP_GATTS_DISCONNECT_EVT:
            esp_ble_gap_start_advertising(&g_ble_adv_params);
            g_gatts_is_connected = false;
            break;

        case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
            MDF_LOGI("The number handle =%x", param->add_attr_tab.num_handle);

            if (param->add_attr_tab.status != ESP_GATT_OK) {
                MDF_LOGE("Create attribute table failed, error code=0x%x", param->add_attr_tab.status);
            } else if (param->add_attr_tab.num_handle != SPP_IDX_NB) {
                MDF_LOGE("Create attribute table abnormally, num_handle (%d) doesn't equal to HRS_IDX_NB(%d)",
                         param->add_attr_tab.num_handle, SPP_IDX_NB);
            } else {
                memcpy(g_spp_handle_table, param->add_attr_tab.handles, sizeof(g_spp_handle_table));
                esp_ble_gatts_start_service(g_spp_handle_table[SPP_IDX_SVC]);
            }

            break;
        }

        default:
            break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event,
                                esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    MDF_LOGD("gatts_event_handler, event: %d, gatts_if: %d", event, gatts_if);

    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status != ESP_GATT_OK) {
            MDF_LOGW("reg app, app_id: %04x, status: %d",
                     param->reg.app_id, param->reg.status);
        }

        spp_profile_tab[SPP_PROFILE_APP_IDX].gatts_if = gatts_if;
    }

    /* If the gatts_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    for (int i = 0; i < sizeof(spp_profile_tab) / sizeof(struct gatts_profile_inst); i++) {
        if (gatts_if == ESP_GATT_IF_NONE || gatts_if == spp_profile_tab[i].gatts_if) {
            if (spp_profile_tab[i].gatts_cb) {
                spp_profile_tab[i].gatts_cb(event, gatts_if, param);
            }
        }
    }
}

mdf_err_t mlink_ble_set_scan(uint16_t interval, uint16_t window, mlink_ble_cb_t scan_cb)
{
    mdf_err_t ret = ESP_OK;

    if (g_ble_scan_flag) {
        ret = esp_ble_gap_stop_scanning();
        MDF_ERROR_CHECK(ret != MDF_OK, ret, "stop the device scanning the peer device");
    }

    g_ble_scan_flag = false;

    if (!interval && !window && !scan_cb) {
        MDF_LOGD("stop the device scanning the peer device");
        return MDF_OK;
    }

    MDF_ERROR_CHECK(!interval || !window || !scan_cb || interval < window, MDF_ERR_INVALID_ARG,
                    "interval: %d, window: %d, scan_cb: %p", interval, window, scan_cb);

    g_ble_scan_params.scan_interval = interval;
    g_ble_scan_params.scan_window   = window;
    g_gatts_event_scan_cb           = scan_cb;

    ret = esp_ble_gap_set_scan_params(&g_ble_scan_params);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "set scan parameters, interval: %d, window: %d", interval, window);

    g_ble_scan_flag = true;

    return ESP_OK;
}

mdf_err_t mlink_ble_get_scan(uint16_t *interval, uint16_t *window)
{
    MDF_PARAM_CHECK(interval);
    MDF_PARAM_CHECK(window);

    *interval = g_ble_scan_params.scan_interval;
    *window   = g_ble_scan_params.scan_window;

    return ESP_OK;
}

mdf_err_t mlink_ble_set_cb(const mlink_ble_cb_t read_cb,
                           const mlink_ble_cb_t write_cb)
{
    g_gatts_event_read_cb  = read_cb;
    g_gatts_event_write_cb = write_cb;

    return ESP_OK;
}

mdf_err_t mlink_ble_send(const void *data, ssize_t size)
{
    MDF_PARAM_CHECK(data);
    MDF_PARAM_CHECK(size <= spp_mtu_size - 3);
    MDF_ERROR_CHECK(!g_gatts_is_connected, MDF_ERR_NOT_INIT, "ble disconnect");

    mdf_err_t ret = ESP_OK;

    ret = esp_ble_gatts_send_indicate(g_spp_gatts_if, g_spp_conn_id, g_spp_handle_table[SPP_IDX_DATA_VAL],
                                      size, (uint8_t *)data, false);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "esp_ble_gatts_send_indicate");

    return MDF_OK;
}

mdf_err_t mlink_ble_set_config(const mlink_ble_config_t *config)
{
    MDF_PARAM_CHECK(config);

    mdf_err_t ret   = MDF_OK;
    size_t name_len = strlen(config->name);

    mlink_ble_adv_manufacturer_t manufacturer_data = {
        .company_id = config->company_id,
        .OUI        = CONFIG_BLE_GATEWAY_OUI,
    };

    esp_ble_adv_data_t scan_rsp_data = {
        .set_scan_rsp        = config->custom_size > 0 || name_len > 12 ? true : false,
        .include_name        = name_len > 12 ? false : true,
        .manufacturer_len    = sizeof(mlink_ble_adv_manufacturer_t),
        .p_manufacturer_data = (uint8_t *) &manufacturer_data,
        .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
    };

    ESP_ERROR_CHECK(esp_read_mac(manufacturer_data.sta_addr, ESP_MAC_WIFI_STA));

    ret = esp_ble_gap_set_device_name(config->name);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "esp_ble_gap_set_device_name");

    if (name_len > 12) {
        esp_ble_adv_data_t scan_req_data = {
            .set_scan_rsp = false,
            .include_name = true,
            .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
        };

        ret = esp_ble_gap_config_adv_data(&scan_req_data);
        MDF_ERROR_CHECK(ret != MDF_OK, ret, "esp_ble_gap_config_adv_data");
    } else if (scan_rsp_data.set_scan_rsp) {
        ret = esp_ble_gap_config_adv_data_raw((uint8_t *)config->custom_data, config->custom_size);
        MDF_ERROR_CHECK(ret != MDF_OK, ret, "esp_ble_gap_config_adv_data_raw");
    }

    ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "esp_ble_gap_config_adv_data");

    memcpy(&g_ble_config, config, sizeof(mlink_ble_config_t));

    return MDF_OK;
}

mdf_err_t mlink_ble_get_config(mlink_ble_config_t *config)
{
    MDF_PARAM_CHECK(config);

    memcpy(config, &g_ble_config, sizeof(mlink_ble_config_t));

    return MDF_OK;
}

mdf_err_t mlink_ble_init(const mlink_ble_config_t *config)
{
    mdf_err_t ret = ESP_OK;

    if (g_ble_init_flag) {
        return ESP_OK;
    }

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    MDF_ERROR_ASSERT(esp_wifi_set_ps(WIFI_PS_MIN_MODEM));

    ret = esp_bt_controller_init(&bt_cfg);
    MDF_ERROR_CHECK(ret != ESP_GATT_OK, ret, "esp_bt_controller_init");

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    MDF_ERROR_CHECK(ret != ESP_GATT_OK, ret, "esp_bt_controller_enable");

    ret = esp_bluedroid_init();
    MDF_ERROR_CHECK(ret != ESP_GATT_OK, ret, "esp_bluedroid_init");

    ret = esp_bluedroid_enable();
    MDF_ERROR_CHECK(ret != ESP_GATT_OK, ret, "esp_bluedroid_enable");

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    MDF_ERROR_CHECK(ret != ESP_GATT_OK, ret, "esp_ble_gatts_register_callback");

    ret = esp_ble_gap_register_callback(ble_gap_event_handler);
    MDF_ERROR_CHECK(ret != ESP_GATT_OK, ret, "esp_ble_gattc_register_callback");

    ret = esp_ble_gatts_app_register(ESP_SPP_APP_ID);
    MDF_ERROR_CHECK(ret != ESP_GATT_OK, ret, "esp_ble_gatts_app_register");

    if (g_ble_scan_flag) {
        ret = esp_ble_gap_set_scan_params(&g_ble_scan_params);
        MDF_ERROR_CHECK(ret != ESP_OK, ret, "esp_ble_gap_set_scan_params");
    }

    ret = mlink_ble_set_config(config);
    MDF_ERROR_CHECK(ret != ESP_GATT_OK, ret, "mlink_ble_set_config");

    g_ble_init_flag = true;

    MDF_LOGI("mesh ble initialize");

    return ESP_OK;
}

mdf_err_t mlink_ble_deinit()
{
    if (!g_ble_init_flag) {
        return ESP_OK;
    }

    mdf_err_t ret = ESP_OK;

    ret = esp_bluedroid_disable();
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "esp_bluedroid_disable");

    ret = esp_bluedroid_deinit();
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "esp_bluedroid_deinit");

    ret = esp_bt_controller_disable();
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "esp_bt_controller_disable");

    ret = esp_bt_controller_deinit();
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "esp_bt_controller_deinit");

    g_ble_init_flag = false;
    MDF_LOGI("mesh ble deinitialize");

    return ESP_OK;
}

#else

mdf_err_t mlink_ble_init(const mlink_ble_config_t *config)
{
    return MDF_ERR_NOT_SUPPORTED;
}

mdf_err_t mlink_ble_deinit()
{
    return MDF_ERR_NOT_SUPPORTED;
}

mdf_err_t mlink_ble_send(const void *data, ssize_t size)
{
    return MDF_ERR_NOT_SUPPORTED;
}

mdf_err_t mlink_ble_set_config(const mlink_ble_config_t *config)
{
    return MDF_ERR_NOT_SUPPORTED;
}

mdf_err_t mlink_ble_get_config(mlink_ble_config_t *config)
{
    return MDF_ERR_NOT_SUPPORTED;
}

mdf_err_t mlink_ble_set_cb(const mlink_ble_cb_t read_cb, const mlink_ble_cb_t write_cb)
{
    return MDF_ERR_NOT_SUPPORTED;
}

#endif /**< CONFIG_BT_ENABLED */
#endif /**< !CONFIG_BT_NIMBLE_ENABLED */
