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

#include "mlink.h"
#include "mwifi.h"
#include "mconfig_chain.h"
#include "mupgrade.h"

#define MLINK_RESTART_DELAY_TIME_MS (5000)
#define MLINK_HANDLES_MAX_SIZE      (64)
#define CHARACTERISTICS_MAX_NUM     (32)
#define MLINK_DEVICE_NAME_KEY       "ML_NAME"
#define MLINK_DEVICE_POSITION_KEY   "ML_POSITION"

/**
 * @brief The data type of the characteristic value
 */
typedef union {
    char *value_string;  /* The item's string */
    int value_int;       /* writing to value int is DEPRECATED*/
    double value_double; /* The item's number*/
} characteristic_value_t;

/**
 * @brief The characteristics of the device
 */
typedef struct {
    uint16_t cid;   /**< The identifier of the characteristics */
    uint8_t perms;  /**< The permissions of the characteristics*/
    uint8_t format; /**< The format of the characteristics */
    int step;       /**< The step of the attribute */
    int min;        /**< The minimum value of the attribute */
    int max;        /**< The maximum value of the attribute */
    char name[32];  /**< The name of the characteristics */
} mlink_characteristics_t;

/**
 * @brief device configuration information
 */
typedef struct {
    uint32_t tid;                             /**< The identifier of the device type */
    char name[32];                            /**< The name of the device */
    char position[32];                        /**< The position the device */
    char version[16];                         /**< The version of the device */
    uint8_t characteristics_num;              /**< The number of device attributes */
    mlink_characteristics_t *characteristics; /**< The characteristics of the device */
} mlink_device_t;

/**
 * @brief Type of handler list
 */
typedef struct {
    const char *name;         /**< The name of the function */
    mlink_handle_func_t func; /**< The pointer of the function */
} mlink_handle_t;

static const char *TAG               = "mlink_handle";
static mlink_device_t *g_device_info = NULL;
mlink_characteristic_func_t mlink_device_get_value = NULL;
static mlink_characteristic_func_t mlink_device_set_value = NULL;

mdf_err_t mlink_add_device(uint32_t tid, const char *name, const char *version)
{
    MDF_PARAM_CHECK(name);
    MDF_PARAM_CHECK(version);

    if (!g_device_info) {
        g_device_info = MDF_MALLOC(sizeof(mlink_device_t));
    }

    memset(g_device_info, 0, sizeof(mlink_device_t));

    if (mdf_info_load(MLINK_DEVICE_NAME_KEY, g_device_info->name, sizeof(g_device_info->name)) != MDF_OK) {
        strncpy(g_device_info->name, name, sizeof(g_device_info->name) - 1);
    }

    mdf_info_load(MLINK_DEVICE_POSITION_KEY, g_device_info->position, sizeof(g_device_info->position));

    g_device_info->tid = tid;
    strncpy(g_device_info->version, version, sizeof(g_device_info->version) - 1);

    g_device_info->characteristics_num = 0;
    g_device_info->characteristics     = NULL;

    return MDF_OK;
}

mdf_err_t mlink_device_set_name(const char *name)
{
    MDF_PARAM_CHECK(g_device_info);
    MDF_PARAM_CHECK(name);

    mdf_err_t ret = MDF_OK;

    strncpy(g_device_info->name, name, sizeof(g_device_info->name) - 1);
    ret = mdf_info_save(MLINK_DEVICE_NAME_KEY, g_device_info->name,
                        sizeof(g_device_info->name));
    MDF_ERROR_CHECK(ret < 0, MDF_FAIL, "mdf_info_save, ret: %d", ret);

    return MDF_OK;
}

const char *mlink_device_get_name()
{
    if (!g_device_info) {
        return NULL;
    }

    return g_device_info->name;
}

mdf_err_t mlink_device_set_position(const char *position)
{
    MDF_PARAM_CHECK(g_device_info);
    MDF_PARAM_CHECK(position);

    mdf_err_t ret = MDF_OK;

    strncpy(g_device_info->position, position, sizeof(g_device_info->position) - 1);

    ret = mdf_info_save(MLINK_DEVICE_POSITION_KEY, g_device_info->position,
                        sizeof(g_device_info->position));
    MDF_ERROR_CHECK(ret < 0, MDF_FAIL, "mdf_info_save, ret: %d", ret);

    return MDF_OK;
}

const char *mlink_device_get_position()
{
    if (!g_device_info) {
        return NULL;
    }

    return g_device_info->position;
}

const char *mlink_device_get_version()
{
    if (!g_device_info) {
        return NULL;
    }

    return g_device_info->version;
}

int mlink_device_get_tid()
{
    MDF_PARAM_CHECK(g_device_info);

    return g_device_info->tid;
}

mdf_err_t mlink_add_characteristic(uint16_t cid, const char *name, characteristic_format_t format,
                                   characteristic_perms_t perms, int min, int max, uint16_t step)
{
    MDF_PARAM_CHECK(g_device_info);
    MDF_PARAM_CHECK(name);

    g_device_info->characteristics = MDF_REALLOC(g_device_info->characteristics,
                                     (g_device_info->characteristics_num + 1) * sizeof(mlink_characteristics_t));
    mlink_characteristics_t *characteristics = g_device_info->characteristics + g_device_info->characteristics_num;

    characteristics->cid    = cid;
    characteristics->format = format;
    characteristics->perms  = perms;
    characteristics->min    = min;
    characteristics->max    = max;
    characteristics->step   = step;
    g_device_info->characteristics_num++;

    memset(characteristics->name, 0, sizeof(characteristics->name));
    strncpy(characteristics->name, name, sizeof(characteristics->name) - 1);

    return MDF_OK;
}

static characteristic_format_t mlink_get_characteristics_format(uint16_t cid)
{
    mlink_characteristics_t *characteristic = g_device_info->characteristics;

    for (int i = 0; i < g_device_info->characteristics_num; ++i) {
        if (characteristic[i].cid == cid) {
            return characteristic[i].format;
        }
    }

    return CHARACTERISTIC_FORMAT_NONE;
}

mdf_err_t mlink_add_characteristic_handle(mlink_characteristic_func_t get_value_func, mlink_characteristic_func_t set_value_func)
{
    MDF_PARAM_CHECK(get_value_func);

    mlink_device_get_value = get_value_func;
    mlink_device_set_value = set_value_func;

    return MDF_OK;
}

static mdf_err_t mlink_handle_system_reboot(mlink_handle_data_t *handle_data)
{
    int delay_time = MLINK_RESTART_DELAY_TIME_MS;
    mlink_json_parse(handle_data->req_data, "delay", &delay_time);

    mdf_err_t ret = mdf_event_loop_delay_send(MDF_EVENT_MLINK_SYSTEM_REBOOT, NULL, delay_time);
    MDF_ERROR_CHECK(ret < 0, MDF_FAIL, "mdf_event_loop_delay_send, ret: %d", ret);

    return MDF_OK;
}

static mdf_err_t mlink_handle_system_reset(mlink_handle_data_t *handle_data)
{
    mdf_err_t ret  = 0;
    int delay_time = MLINK_RESTART_DELAY_TIME_MS;
    mlink_json_parse(handle_data->req_data, "delay", &delay_time);

    ret = mdf_event_loop_delay_send(MDF_EVENT_MLINK_SYSTEM_RESET, NULL, delay_time);
    MDF_ERROR_CHECK(ret < 0, MDF_FAIL, "mdf_event_loop_delay_send, ret: %d", ret);

    return MDF_OK;
}

static mdf_err_t mlink_handle_get_info(mlink_handle_data_t *handle_data)
{
    mdf_err_t ret                = MDF_OK;
    char tmp_str[64]             = {0};
    char position[32]            = {0x0};
    size_t position_len          = sizeof(position);
    mesh_addr_t mesh_id          = {0};
    handle_data->resp_data       = NULL;
    char *characteristics_list   = NULL;
    characteristic_value_t value = {0};
    mlink_characteristics_t *characteristic = g_device_info->characteristics;

    const char *characteristic_format_int =
        "{\"cid\":%d,\"name\":\"%s\",\"format\":\"int\","
        "\"perms\":%d,\"value\":%d,\"min\":%d,\"max\":%d,\"step\":%d}";
    const char *characteristic_format_double =
        "{\"cid\":%d,\"name\":\"%s\",\"format\":\"double\","
        "\"perms\":%d,\"value\":%lf,\"min\":%d,\"max\":%d,\"step\":%d}";
    const char *characteristic_format_string =
        "{\"cid\":%d,\"name\":\"%s\",\"format\":\"string\","
        "\"perms\":%d,\"value\":%s,\"min\":%d,\"max\":%d,\"step\":%d}";

    ESP_ERROR_CHECK(esp_mesh_get_id(&mesh_id));

    if (mdf_info_load(MLINK_DEVICE_POSITION_KEY, position, &position_len) == MDF_OK) {
        mlink_json_pack(&handle_data->resp_data, "position", position);
    }

    sprintf(tmp_str, "%d", g_device_info->tid);

    mlink_json_pack(&handle_data->resp_data, "tid", tmp_str);
    mlink_json_pack(&handle_data->resp_data, "name", g_device_info->name);
    mlink_json_pack(&handle_data->resp_data, "mesh_id", mlink_mac_hex2str(mesh_id.addr, tmp_str));
    mlink_json_pack(&handle_data->resp_data, "version", g_device_info->version);
    mlink_json_pack(&handle_data->resp_data, "idf_version", esp_get_idf_version());
    mlink_json_pack(&handle_data->resp_data, "mdf_version", mdf_get_version());
    mlink_json_pack(&handle_data->resp_data, "mlink_version", 2);

    for (int i = 0; i < g_device_info->characteristics_num; ++i) {
        char *tmp_str = NULL;

        switch (characteristic[i].format) {
            case CHARACTERISTIC_FORMAT_INT: {
                ret = mlink_device_get_value(characteristic[i].cid, &value.value_int);
                MDF_ERROR_CONTINUE(ret != MDF_OK, "Get the value of the device's cid: %d", characteristic[i].cid);

                asprintf(&tmp_str, characteristic_format_int, characteristic[i].cid, characteristic[i].name,
                         characteristic[i].perms, value.value_int,
                         characteristic[i].min, characteristic[i].max, characteristic[i].step);
                break;
            }

            case CHARACTERISTIC_FORMAT_DOUBLE: {
                ret = mlink_device_get_value(characteristic[i].cid, &value.value_double);
                MDF_ERROR_CONTINUE(ret != MDF_OK, "Get the value of the device's cid: %d", characteristic[i].cid);

                asprintf(&tmp_str, characteristic_format_double, characteristic[i].cid, characteristic[i].name,
                         characteristic[i].perms, value.value_double,
                         characteristic[i].min, characteristic[i].max, characteristic[i].step);
                break;
            }

            case CHARACTERISTIC_FORMAT_STRING: {
                ret = mlink_device_get_value(characteristic[i].cid, value.value_string);
                MDF_ERROR_CONTINUE(ret != MDF_OK, "Get the value of the device's cid: %d", characteristic[i].cid);

                asprintf(&tmp_str, characteristic_format_string, characteristic[i].cid, characteristic[i].name,
                         characteristic[i].perms, value.value_string,
                         characteristic[i].min, characteristic[i].max, characteristic[i].step);
                break;
            }

            default:
                break;
        }

        if (tmp_str) {
            mlink_json_pack(&characteristics_list, "[]", tmp_str);
            MDF_FREE(tmp_str);
        }
    }

    handle_data->resp_size = mlink_json_pack(&handle_data->resp_data, "characteristics", characteristics_list);
    MDF_FREE(characteristics_list);

    return MDF_OK;
}

static mdf_err_t mlink_handle_get_status(mlink_handle_data_t *handle_data)
{
    MDF_ERROR_CHECK(!mlink_device_get_value, MDF_FAIL, "this device does not support get_status");

    mdf_err_t ret                     = MDF_OK;
    int cids_num                      = 0;
    handle_data->resp_data            = NULL;
    char *characteristics_list        = NULL;
    int cids[CHARACTERISTICS_MAX_NUM] = {0};
    characteristic_value_t value      = {0};

    ret = mlink_json_parse(handle_data->req_data, "cids", &cids_num);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "Parse the json formatted string");

    ret = mlink_json_parse(handle_data->req_data, "cids", cids);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "Parse the json formatted string");

    mdf_event_loop_send(MDF_EVENT_MLINK_GET_STATUS, NULL);

    for (int i = 0; i < cids_num; ++i) {
        char *tmp_str = NULL;

        switch (mlink_get_characteristics_format(cids[i])) {
            case CHARACTERISTIC_FORMAT_INT:
                ret = mlink_device_get_value(cids[i], &value.value_int);
                MDF_ERROR_BREAK(ret < 0, "<%s> mlink_device_get_value", mdf_err_to_name(ret));
                mlink_json_pack(&tmp_str, "cid", cids[i]);
                mlink_json_pack(&tmp_str, "value", value.value_int);
                break;

            case CHARACTERISTIC_FORMAT_DOUBLE:
                ret = mlink_device_get_value(cids[i], &value.value_double);
                MDF_ERROR_BREAK(ret < 0, "<%s> mlink_device_get_value", mdf_err_to_name(ret));
                mlink_json_pack(&tmp_str, "cid", cids[i]);
                mlink_json_pack_double(&tmp_str, "value", value.value_double);
                break;

            case CHARACTERISTIC_FORMAT_STRING:
                ret = mlink_device_get_value(cids[i], value.value_string);
                MDF_ERROR_BREAK(ret < 0, "<%s> mlink_device_get_value", mdf_err_to_name(ret));
                mlink_json_pack(&tmp_str, "cid", cids[i]);
                mlink_json_pack(&tmp_str, "value", value.value_string);
                break;

            default:
                MDF_LOGW("Data types in this format are not supported");
                break;

        }

        if (tmp_str) {
            mlink_json_pack(&characteristics_list, "[]", tmp_str);
            MDF_FREE(tmp_str);
        }
    }

    MDF_ERROR_CHECK(!characteristics_list, MDF_FAIL, "Create a json string");

    handle_data->resp_size = mlink_json_pack(&handle_data->resp_data,
                             "characteristics", characteristics_list);
    MDF_FREE(characteristics_list);

    return MDF_OK;
}

static mdf_err_t mlink_handle_set_status(mlink_handle_data_t *handle_data)
{
    MDF_ERROR_CHECK(!mlink_device_set_value, MDF_FAIL, "This device does not support set_status");

    int cid         = 0;
    mdf_err_t ret   = MDF_OK;
    size_t cids_num = 0;
    characteristic_value_t value = {0};
    char *characteristics_list[CHARACTERISTICS_MAX_NUM] = {NULL};

    ret = mlink_json_parse(handle_data->req_data, "characteristics", &cids_num);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "Parse the json formatted string");

    ret = mlink_json_parse(handle_data->req_data, "characteristics", characteristics_list);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "Parse the json formatted string");

    for (int i = 0; i < cids_num; ++i) {
        ret = mlink_json_parse(characteristics_list[i], "cid",  &cid);

        if (ret) {
            MDF_LOGW("<%s> Parse the json formatted string", mdf_err_to_name(ret));
            MDF_FREE(characteristics_list[i]);
            continue;
        }

        switch (mlink_get_characteristics_format(cid)) {
            case CHARACTERISTIC_FORMAT_INT:
                ret = mlink_json_parse(characteristics_list[i], "value", &value.value_int);
                MDF_ERROR_BREAK(ret != MDF_OK, "<%s> Parse the json formatted string", mdf_err_to_name(ret));
                ret = mlink_device_set_value(cid, &value.value_int);
                MDF_ERROR_BREAK(ret != MDF_OK, "<%s> Parse the json formatted string", mdf_err_to_name(ret));
                break;

            case CHARACTERISTIC_FORMAT_DOUBLE:
                ret = mlink_json_parse(characteristics_list[i], "value", &value.value_double);
                MDF_ERROR_BREAK(ret != MDF_OK, "<%s> Parse the json formatted string", mdf_err_to_name(ret));
                ret = mlink_device_set_value(cid, &value.value_double);
                MDF_ERROR_BREAK(ret != MDF_OK, "<%s> Parse the json formatted string", mdf_err_to_name(ret));
                break;

            case CHARACTERISTIC_FORMAT_STRING:
                ret = mlink_json_parse(characteristics_list[i], "value", value.value_string);
                MDF_ERROR_BREAK(ret != MDF_OK, "<%s> Parse the json formatted string", mdf_err_to_name(ret));
                ret = mlink_device_set_value(cid, value.value_string);
                MDF_ERROR_BREAK(ret != MDF_OK, "<%s> Parse the json formatted string", mdf_err_to_name(ret));
                break;

            default:
                MDF_LOGW("Data types in this format are not supported");
                break;
        }

        MDF_FREE(characteristics_list[i]);
    }

    mdf_event_loop_send(MDF_EVENT_MLINK_SET_STATUS, NULL);

    return MDF_OK;
}

static mdf_err_t mlink_handle_add_device(mlink_handle_data_t *handle_data)
{
    mdf_err_t ret                = MDF_FAIL;
    int whitelist_num            = 0;
    char **whitelist_json        = NULL;
    uint32_t duration_ms         = 30000;
    mconfig_data_t *mconfig_data = MDF_CALLOC(1, sizeof(mconfig_data_t));

    ret = mwifi_get_config(&mconfig_data->config);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "<%s> Get the configuration of the AP", mdf_err_to_name(ret));

    ret = mwifi_get_init_config(&mconfig_data->init_config);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "<%s> Get Mwifi init configuration", mdf_err_to_name(ret));

    ret = mlink_json_parse(handle_data->req_data, "whitelist", &whitelist_num);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "Parse the json formatted string: whitelist");

    whitelist_json = MDF_CALLOC(whitelist_num, sizeof(char *));
    ret = mlink_json_parse(handle_data->req_data, "whitelist", whitelist_json);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "Parse the json formatted string: whitelist");

    mconfig_data->whitelist_size = whitelist_num * sizeof(mconfig_whitelist_t);
    mconfig_data = MDF_REALLOC(mconfig_data, sizeof(mconfig_data_t) + mconfig_data->whitelist_size);

    for (int i = 0; i < whitelist_num && i < CONFIG_MWIFI_CAPACITY_NUM; ++i) {
        mlink_mac_str2hex(whitelist_json[i], (mconfig_data->whitelist_data + i)->addr);
    }

    mlink_json_parse(handle_data->req_data, "timeout", &duration_ms);
    ret = mconfig_chain_master(mconfig_data, duration_ms / portTICK_RATE_MS);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "<%s> Sending network configuration information to the devices",
                   mdf_err_to_name(ret));

EXIT:
    MDF_FREE(mconfig_data);
    MDF_FREE(whitelist_json);

    return ret;
}

static mdf_err_t mlink_handle_set_name(mlink_handle_data_t *handle_data)
{
    mdf_err_t ret = MDF_OK;
    char name[32] = {0};

    ret = mlink_json_parse(handle_data->req_data, "name", name);
    MDF_ERROR_CHECK(ret < 0, ret, "mlink_json_parse");

    ret = mlink_device_set_name(name);
    MDF_ERROR_CHECK(ret < 0, ret, "device_config_set");

    MDF_LOGD("Set device name: %s", name);

    return MDF_OK;
}

static mdf_err_t mlink_handle_set_position(mlink_handle_data_t *handle_data)
{
    mdf_err_t ret     = MDF_OK;
    char position[32] = {0};

    ret = mlink_json_parse(handle_data->req_data, "position", position);
    MDF_ERROR_CHECK(ret < 0, ret, "mlink_json_parse");

    ret = mlink_device_set_position(position);
    MDF_ERROR_CHECK(ret < 0, ret, "device_config_set");

    MDF_LOGD("Set device position: %s", position);

    return MDF_OK;
}

static mdf_err_t mlink_handle_get_ota_progress(mlink_handle_data_t *handle_data)
{
    mdf_err_t ret            = MDF_OK;
    mupgrade_status_t status = {0x0};

    ret = mupgrade_get_status(&status);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "mupgrade_get_status");

    mlink_json_pack(&handle_data->resp_data, "firmware_name", status.name);
    mlink_json_pack(&handle_data->resp_data, "total_size", status.total_size);
    mlink_json_pack(&handle_data->resp_data, "written_size", status.written_size);

    handle_data->resp_size = strlen(handle_data->resp_data);

    return MDF_OK;
}

static mdf_err_t mlink_handle_set_ota_fallback(mlink_handle_data_t *handle_data)
{
    mdf_err_t ret = MDF_OK;

    ret = mupgrade_version_fallback();
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "Upgrade error back to previous version");

    return mlink_handle_system_reboot(handle_data);
}

static mdf_err_t mlink_handle_get_config(mlink_handle_data_t *handle_data)
{
    mesh_addr_t mesh_id      = {0};
    mesh_addr_t parent_bssid = {0};
    uint8_t parent_mac[6]    = {0};
    char mac_str[17]         = {0};
    int interval_ms          = 0;
    mesh_assoc_t mesh_assoc  = {0x0};

    esp_mesh_get_id(&mesh_id);
    esp_wifi_vnd_mesh_get(&mesh_assoc);
    esp_mesh_get_beacon_interval(&interval_ms);
    esp_mesh_get_parent_bssid(&parent_bssid);

    if (esp_mesh_get_layer() == MESH_ROOT) {
        memcpy(parent_mac, parent_bssid.addr, 6);
    } else {
        mlink_mac_ap2sta(parent_bssid.addr, parent_mac);
    }

    mlink_json_pack(&handle_data->resp_data, "id", mlink_mac_hex2str(mesh_id.addr, mac_str));
    mlink_json_pack(&handle_data->resp_data, "max_layer", esp_mesh_get_max_layer());
    mlink_json_pack(&handle_data->resp_data, "max_connections", esp_mesh_get_ap_connections());
    mlink_json_pack(&handle_data->resp_data, "layer", esp_mesh_get_layer());
    mlink_json_pack(&handle_data->resp_data, "parent_mac",  mlink_mac_hex2str(parent_mac, mac_str));
    mlink_json_pack(&handle_data->resp_data, "type", esp_mesh_get_type());
    mlink_json_pack(&handle_data->resp_data, "prarent_rssi", mesh_assoc.rssi);
    mlink_json_pack(&handle_data->resp_data, "router_rssi", mesh_assoc.router_rssi);
    mlink_json_pack(&handle_data->resp_data, "beacon_interval", interval_ms);
    mlink_json_pack(&handle_data->resp_data, "assoc_expire", esp_mesh_get_ap_assoc_expire());
    mlink_json_pack(&handle_data->resp_data, "capacity_num", esp_mesh_get_capacity_num());
    mlink_json_pack(&handle_data->resp_data, "free_heap", esp_get_free_heap_size());
    mlink_json_pack(&handle_data->resp_data, "running_time", xTaskGetTickCount() * (1000 / configTICK_RATE_HZ));

    handle_data->resp_size = strlen(handle_data->resp_data);

    return MDF_OK;
}

static mdf_err_t mlink_handle_set_config(mlink_handle_data_t *handle_data)
{
    mdf_err_t ret = ESP_OK;
    int data      = 0;

    if (mlink_json_parse(handle_data->req_data, "beacon_interval", &data) == ESP_OK) {
        ret = esp_mesh_set_beacon_interval(data);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "esp_mesh_set_beacon_interval, ret: %d", ret);
        MDF_LOGI("ESP-MESH beacon interval: %d ms", data);
    }

    if (mlink_json_parse(handle_data->req_data, "log_level", &data) == ESP_OK) {
        esp_log_level_set("*", data);
        MDF_LOGI("Set log level: %d", data);
    }

    return ESP_OK;
}

static mlink_handle_t g_handles_list[MLINK_HANDLES_MAX_SIZE] = {
    {"reset",            mlink_handle_system_reset},
    {"reboot",           mlink_handle_system_reboot},
    {"get_device_info",  mlink_handle_get_info},
    {"get_status",       mlink_handle_get_status},
    {"set_status",       mlink_handle_set_status},
    {"add_device",       mlink_handle_add_device},
    {"rename_device",    mlink_handle_set_name},
    {"set_position",     mlink_handle_set_position},
    {"get_ota_progress", mlink_handle_get_ota_progress},
    {"set_ota_fallback", mlink_handle_set_ota_fallback},
    {"get_mesh_config",  mlink_handle_get_config},
    {"set_mesh_config",  mlink_handle_set_config},
    {NULL,              NULL},
};

mdf_err_t mlink_set_handle(const char *name, const mlink_handle_func_t func)
{
    MDF_PARAM_CHECK(name);
    MDF_PARAM_CHECK(func);

    int i = 0;

    for (i = 0; g_handles_list[i].name && i < MLINK_HANDLES_MAX_SIZE; i++) {
        if (!strcasecmp(g_handles_list[i].name, name)) {
            g_handles_list[i].func = (mlink_handle_func_t)func;
            return ESP_OK;
        }
    }

    MDF_ERROR_CHECK(i == MLINK_HANDLES_MAX_SIZE, MDF_FAIL, "Mlink handles list is full");

    g_handles_list[i].name = name;
    g_handles_list[i].func = (mlink_handle_func_t)func;

    return ESP_OK;
}

mdf_err_t mlink_handle(const uint8_t *src_addr, const mlink_httpd_type_t *type,
                       const void *data, size_t size)
{
    MDF_PARAM_CHECK(src_addr);
    MDF_PARAM_CHECK(type);
    MDF_PARAM_CHECK(data);
    MDF_PARAM_CHECK(size > 0);

    mdf_err_t ret                = MDF_FAIL;
    const uint8_t *dest_addr     = NULL;
    char func_name[32]           = {0x0};
    mlink_httpd_type_t resp_type = {0x0};
    mwifi_data_type_t data_type  = {
        .compression = true,
    };
    mlink_handle_data_t handle_data = {
        .req_data    = data,
        .req_size    = size,
        .req_fromat  = MLINK_HTTPD_FORMAT_JSON,
        .resp_data   = NULL,
        .resp_size   = 0,
        .resp_fromat = MLINK_HTTPD_FORMAT_JSON,
    };

    MDF_ERROR_GOTO(type->format != MLINK_HTTPD_FORMAT_JSON, EXIT,
                   "The current version only supports the json protocol");

    ret = mlink_json_parse(handle_data.req_data, "request", func_name);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "mlink_json_parse, ret: %d, key: %s, value: %.*s",
                   ret, func_name, handle_data.req_size, handle_data.req_data);

    ret = MDF_ERR_NOT_SUPPORTED;

    /**< If we can find this request from our list, we will handle this request */
    for (int i = 0; g_handles_list[i].func; i++) {
        if (!strcasecmp(func_name, g_handles_list[i].name)) {
            MDF_LOGD("Function: %s", func_name);
            ret = g_handles_list[i].func(&handle_data);
            break;
        }
    }

    /**< Check flag to decide whether reponse */
    if (!type->resp) {
        return MDF_OK;
    }

    if (type->from != MLINK_HTTPD_FROM_SERVER) {
        dest_addr = src_addr;
    }

EXIT:

    resp_type.sockfd = type->sockfd;
    resp_type.format = handle_data.resp_fromat;
    resp_type.from   = MLINK_HTTPD_FROM_DEVICE;
    resp_type.resp   = (ret == MDF_OK) ? true : false;
    data_type.protocol = MLINK_PROTO_HTTPD;
    memcpy(&data_type.custom, &resp_type, sizeof(mlink_httpd_type_t));

    if (handle_data.resp_fromat == MLINK_HTTPD_FORMAT_JSON) {
        mlink_json_pack(&handle_data.resp_data, "status_msg", mdf_err_to_name(ret));
        handle_data.resp_size = mlink_json_pack(&handle_data.resp_data, "status_code", -ret);
    }

    ret = mwifi_write(dest_addr, &data_type, handle_data.resp_data, handle_data.resp_size, true);
    MDF_LOGD("resp_size: %d, resp: %.*s", handle_data.resp_size, handle_data.resp_size, handle_data.resp_data);
    MDF_FREE(handle_data.resp_data);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "mdf_write");

    return MDF_OK;
}
