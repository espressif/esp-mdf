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

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"

#include "lwip/inet.h"
#include "esp_event_loop.h"

#include "mdf_json.h"
#include "mdf_http.h"
#include "mdf_espnow_debug.h"
#include "mdf_console_debug.h"
#include "mdf_event_loop.h"
#include "mdf_upgrade.h"
#include "mdf_info_store.h"
#include "mdf_network_config.h"
#include "mdf_device_handle.h"
#include "mdf_espnow.h"
#include "mdf_espnow_debug.h"
#include "mdf_low_power.h"
#include "mdf_notice.h"

#include "mdf_reboot_event.h"
#include "mdf_trigger.h"

#define RESTART_DELAY_TIME                5000
#define CHARACTERISTICS_MAX_NUM           30

typedef struct {
    const char *func_name;
    device_func_t func;
} device_handle_t;

device_operate_t mdf_device_get_value        = NULL;
static device_operate_t mdf_device_set_value = NULL;

static device_config_t *g_device_config = NULL;
static const char *TAG                  = "mdf_device_handle";

/*************************** debug ***************************/
static void mdf_show_sysinfo_timercb(void *timer)
{
    uint8_t sta_mac[6] = {0};
    esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);

    mesh_addr_t par_mac = {0};
    esp_mesh_get_parent_bssid(&par_mac);

    mesh_assoc_t mesh_assoc = {0x0};
    esp_wifi_vnd_mesh_get(&mesh_assoc);

    MDF_LOGI("parent: "MACSTR", mac:"MACSTR", layer: %d, prarent_rssi: %d, router_rssi: %d, free heap: %u, compile time: %s %s",
             MAC2STR(par_mac.addr), MAC2STR(sta_mac), esp_mesh_get_layer(), mesh_assoc.rssi, mesh_assoc.router_rssi,  esp_get_free_heap_size(), __DATE__, __TIME__);

    wifi_sta_list_t wifi_sta_list = {0};
    esp_wifi_ap_get_sta_list(&wifi_sta_list);

    if (wifi_sta_list.num > 0) {
        for (int i = 0; i < wifi_sta_list.num; i++) {
            MDF_LOGI("child id: %d, mac: " MACSTR, i, MAC2STR(wifi_sta_list.sta[i].mac));
        }
    }
}

static esp_err_t mdf_wifi_init()
{
    static bool mdf_wifi_init_flag = false;

    if (!mdf_wifi_init_flag) {

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

        tcpip_adapter_init();
        tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
        tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP);

        ESP_ERROR_CHECK(esp_event_loop_init(NULL, NULL));
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

        ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
        ESP_ERROR_CHECK(esp_wifi_start());

        mdf_wifi_init_flag = true;
    }

    return ESP_OK;
}

esp_err_t mdf_reconfig_network(network_config_t *network_config)
{
    MDF_PARAM_CHECK(network_config);

    esp_err_t ret                    = ESP_OK;
    size_t addr_size                 = 0;
    wifi_mesh_addr_t *dest_addrs     = NULL;
    int dest_addrs_num               = 0;
    char *reconfig_network_data      = mdf_calloc(1, WIFI_MESH_PACKET_MAX_SIZE);
    const char *reconfig_network_fmt = "{\"request\":\"set_network\",\"require_resp\":0,"
                                       "\"delay\":5000,\"ssid\":\"%s\",\"password\":\"%s\","
                                       "\"channel\":%d,\"mesh_id\":\"%02x%02x%02x%02x%02x%02x\"}";
    wifi_mesh_data_type_t data_type  = {
        .no_response = true,
        .proto       = MDF_PROTO_JSON,
        .to_server   = true,
    };

    sprintf(reconfig_network_data, reconfig_network_fmt, network_config->ssid,
            network_config->password, network_config->channel, MAC2STR(network_config->mesh_id));

    addr_size = esp_mesh_get_routing_table_size() * 6;
    dest_addrs = mdf_calloc(1, addr_size);
    ESP_ERROR_CHECK(esp_mesh_get_routing_table((mesh_addr_t *)dest_addrs, addr_size, &dest_addrs_num));

    for (int i = 0; i < dest_addrs_num; ++i) {
        ret = mdf_wifi_mesh_root_send(dest_addrs, dest_addrs + i, &data_type,
                                      reconfig_network_data, strlen(reconfig_network_data));
        MDF_ERROR_CONTINUE(ret <= 0, "mdf_wifi_mesh_root_send, ret: %d", ret);
    }

    mdf_free(dest_addrs);
    mdf_free(reconfig_network_data);

    return ret < 0 ? ESP_FAIL : ESP_OK;
}

esp_err_t mdf_device_init_config(uint16_t tid, const char name[32], const char version[16])
{
    esp_err_t ret = ESP_OK;

    g_device_config = mdf_calloc(1, sizeof(device_config_t));

    if (mdf_info_load("device_name", g_device_config->name, sizeof(g_device_config->name)) < 0) {
        ret = mdf_wifi_init();
        MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "mdf_wifi_init, ret: %x", ret);

        uint8_t mac[6] = {0};
        ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, mac));
        snprintf(g_device_config->name, sizeof(g_device_config->name) - 1,
                 "%s_%02x%02x%02x", name, mac[3], mac[4], mac[5]);
    }

    g_device_config->tid = tid;
    strncpy(g_device_config->version, version, sizeof(g_device_config->version) - 1);

    g_device_config->characteristics_num = 0;
    g_device_config->characteristics     = NULL;

    return ESP_OK;
}

esp_err_t mdf_device_set_name(const char name[32])
{
    MDF_PARAM_CHECK(g_device_config);

    esp_err_t ret = ESP_OK;

    strncpy(g_device_config->name, name, sizeof(g_device_config->name) - 1);

    ret = mdf_info_save("device_name", g_device_config->name, sizeof(g_device_config->name));
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_save, ret: %d", ret);

    return ESP_OK;
}

const char *mdf_device_get_name()
{
    MDF_ASSERT(g_device_config);

    return g_device_config->name;
}

const char *mdf_device_get_version()
{
    MDF_ASSERT(g_device_config);

    return g_device_config->version;
}

uint16_t mdf_device_get_tid()
{
    MDF_PARAM_CHECK(g_device_config);

    return g_device_config->tid;
}

esp_err_t mdf_device_add_characteristics(uint16_t cid,  const char name[32],
        uint8_t perms, int min, int max, int step)
{
    MDF_PARAM_CHECK(g_device_config);
    MDF_ERROR_CHECK(g_device_config->characteristics_num > DEVICE_CHARACTERISTICS_MAX_SIZE,
                    ESP_FAIL, "characteristics cannot exceed %d", DEVICE_CHARACTERISTICS_MAX_SIZE);

    device_characteristics_t *characteristics = NULL;

    if (g_device_config->characteristics_num) {
        device_characteristics_t *characteristics_tmp =
            mdf_malloc((g_device_config->characteristics_num + 1) * sizeof(device_characteristics_t));
        memcpy(characteristics_tmp, g_device_config->characteristics,
               sizeof(device_characteristics_t) * g_device_config->characteristics_num);
        mdf_free(g_device_config->characteristics);
        g_device_config->characteristics = characteristics_tmp;
        characteristics = g_device_config->characteristics + g_device_config->characteristics_num;
    } else {
        g_device_config->characteristics = mdf_malloc(sizeof(device_characteristics_t));
        characteristics = g_device_config->characteristics;
    }

    characteristics->cid   = cid;
    strncpy(characteristics->name, name, sizeof(characteristics->name));
    characteristics->perms = perms;
    characteristics->min   = min;
    characteristics->max   = max;
    characteristics->step  = step;

    g_device_config->characteristics_num++;

    return ESP_OK;
}

/*************************** event ***************************/
static esp_err_t mdf_device_event_loop_cb(mdf_event_loop_t event, void *ctx)
{
    esp_err_t ret = ESP_OK;

    switch (event) {
        case MDF_EVENT_SYSTEM_RESET:
            MDF_LOGI("***********************************");
            MDF_LOGI("*       RESTORE FACTORY RESET     *");
            MDF_LOGI("***********************************");
            ret = mdf_info_erase(MDF_SPACE_NAME);
            MDF_ERROR_CHECK(ret != 0, ESP_FAIL, "erase mdf config");
            esp_restart();
            break;

        case MDF_EVENT_SYSTEM_REBOOT:
            MDF_LOGI("***********************************");
            MDF_LOGI("*       RESTORE SYSTEM REBOOT     *");
            MDF_LOGI("***********************************");
            esp_restart();
            break;

        default:
            break;
    }

    return ESP_OK;
}

static esp_err_t mdf_device_config_network(device_data_t *device_data)
{
    esp_err_t ret = ESP_OK;
    int delay_time = RESTART_DELAY_TIME;
    mdf_json_parse(device_data->request, "delay", &delay_time);

    ret = mdf_network_clear_config();
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_network_clear_config, ret: %d", ret);

    ret = mdf_event_loop_delay_send(MDF_EVENT_SYSTEM_REBOOT, NULL, delay_time);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_event_loop_delay_send, ret: %d", ret);

    MDF_LOGW("the system will be restarted after %d milliseconds", delay_time);

    return ESP_OK;
}

static esp_err_t mdf_device_set_network(device_data_t *device_data)
{
    int delay_time                  = RESTART_DELAY_TIME;
    esp_err_t ret                   = ESP_OK;
    network_config_t network_config = {0};
    char mac_str[16]                = {0};

    ret = mdf_json_parse(device_data->request, "ssid", network_config.ssid);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_parse, %d", ret);

    ret = mdf_json_parse(device_data->request, "password", network_config.password);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_parse, %d", ret);

    ret = mdf_json_parse(device_data->request, "channel", &network_config.channel);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_parse, %d", ret);

    ret = mdf_json_parse(device_data->request, "mesh_id", mac_str);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_parse, %d", ret);
    str2mac(mac_str, network_config.mesh_id);

    ret = mdf_network_set_config(&network_config);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_network_set_config, ret: %d", ret);

    mdf_json_parse(device_data->request, "delay", &delay_time);
    ret = mdf_event_loop_delay_send(MDF_EVENT_SYSTEM_REBOOT, NULL, delay_time);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_event_loop_delay_send, ret: %d", ret);

    MDF_LOGW("the system will be restarted after %d milliseconds", delay_time);

    return ESP_OK;
}

static esp_err_t mdf_device_reboot(device_data_t *device_data)
{
    int delay_time = RESTART_DELAY_TIME;
    mdf_json_parse(device_data->request, "delay", &delay_time);

    esp_err_t ret = mdf_event_loop_delay_send(MDF_EVENT_SYSTEM_REBOOT, NULL, delay_time);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_event_loop_delay_send, ret: %d", ret);

    MDF_LOGW("the system will be restarted after %d milliseconds", delay_time);

    return ESP_OK;
}

static esp_err_t mdf_device_reset(device_data_t *device_data)
{
    esp_err_t ret  = 0;
    int delay_time = RESTART_DELAY_TIME;
    mdf_json_parse(device_data->request, "delay", &delay_time);

    ret = mdf_coredump_erase();
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_coredump_erase, ret: %d", ret);

    MDF_LOGW("the system will be restarted after %d milliseconds", delay_time);
    ret = mdf_event_loop_delay_send(MDF_EVENT_SYSTEM_RESET, NULL, delay_time);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_event_loop_delay_send, ret: %d", ret);

    return ESP_OK;
}

static esp_err_t mdf_device_ota_status(device_data_t *device_data)
{
    esp_err_t ret                = 0;
    ssize_t ota_bin_len          = 0;
    ssize_t ota_package_len      = 0;
    char ota_bin_version[64]     = {0};
    uint8_t *ota_progress_array  = NULL;
    ssize_t ota_package_num      = 0;
    ssize_t ota_write_num        = 0;
    char *ota_progress_array_str = NULL;
    uint8_t loss_package_flag    = false;

    ret = mdf_json_parse(device_data->request, "ota_bin_version", ota_bin_version);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_parse ota_bin_version, ret: %d", ret);

    ret = mdf_json_parse(device_data->request, "ota_bin_len", &ota_bin_len);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_parse ota_bin_len, ret: %d", ret);

    ret = mdf_json_parse(device_data->request, "package_length", &ota_package_len);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_parse package_length, ret: %d", ret);

    ret = mdf_upgrade_init(ota_bin_len, ota_package_len, ota_bin_version);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_upgrade_init, ret: %d", ret);

    ret = mdf_upgrade_status(&ota_progress_array, &ota_write_num, &ota_package_num, &ota_package_len);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_ota_get_status, ret: %d", ret);

    MDF_LOGD("ota_progress_array: %p, ota_package_num: %d, ota_bin_version: %s, ota_write_num: %d",
             ota_progress_array, ota_package_num, ota_bin_version, ota_write_num);

    ota_progress_array_str = mdf_malloc(WIFI_MESH_PACKET_MAX_SIZE);

    if (!ota_write_num) {
        loss_package_flag = true;
        strcpy(ota_progress_array_str, "[0, -1]");
    } else {
        for (int i = 0; i < ota_package_num; ++i) {
            if (ota_progress_array[i] == true) {
                continue;
            }

            loss_package_flag = true;

            if (mdf_json_pack(ota_progress_array_str, "[]", i) > WIFI_MESH_PACKET_MAX_SIZE - 64) {
                MDF_LOGV("ota_progress_array_str: %s", ota_progress_array_str);
                break;
            }
        }
    }

    if (loss_package_flag) {
        device_data->response_size = mdf_json_pack(device_data->response, "package_length", ota_package_len);
        device_data->response_size = mdf_json_pack(device_data->response, "package_sequence", ota_progress_array_str);
    }

    MDF_LOGD("ota_progress_array_str: %s", ota_progress_array_str);

    mdf_free(ota_progress_array_str);

    return loss_package_flag;
}

static esp_err_t mdf_device_ota_reboot(device_data_t *device_data)
{
    esp_err_t ret = ESP_OK;
    ret = mdf_upgrade_deinit();
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_upgrade_deinit, ret: %d", ret);

    return mdf_device_reboot(device_data);
}

static esp_err_t mdf_device_get_info(device_data_t *device_data)
{
    esp_err_t ret                            = ESP_OK;
    mesh_addr_t mesh_id                      = {0};
    char tmp_str[17]                         = {0};
    device_characteristics_t *characteristic = g_device_config->characteristics;
    const char *characteristic_format_str    = "[%d,\"%s\",\"int\",%d,%d,%d,%d,%d]";
    char position[16]                        = {0x0};
    char tid_str[5]                          = {0};
    sprintf(tid_str, "%d", g_device_config->tid);

    if (mdf_info_load("position", position, sizeof(position)) > 0) {
        ret = mdf_json_pack(device_data->response, "position", position);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);
    }

    ret = mdf_json_pack(device_data->response, "protocol_version", 1);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);

    ret = mdf_json_pack(device_data->response, "tid", tid_str);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);

    ret = mdf_json_pack(device_data->response, "name", g_device_config->name);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);

    ESP_ERROR_CHECK(esp_mesh_get_id(&mesh_id));
    mac2str(mesh_id.addr, tmp_str);
    ret = mdf_json_pack(device_data->response, "mesh_id", tmp_str);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);

    device_data->response_size = mdf_json_pack(device_data->response, "version", g_device_config->version);
    MDF_ERROR_CHECK(device_data->response_size < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);

    if (!g_device_config->characteristics) {
        return ESP_OK;
    }

    char *characteristics_json_str = device_data->request;
    characteristics_json_str[0]    = '\0';
    char *characteristic_json_str  = mdf_malloc(128);

    for (int i = 0; i < g_device_config->characteristics_num; ++i) {
        int value = 0;
        mdf_device_get_value(characteristic[i].cid, &value);

        sprintf(characteristic_json_str, characteristic_format_str,
                characteristic[i].cid, characteristic[i].name,
                characteristic[i].perms, value,
                characteristic[i].min, characteristic[i].max, characteristic[i].step);

        mdf_json_pack(characteristics_json_str, "[]", characteristic_json_str);
    }

    mdf_free(characteristic_json_str);

    device_data->response_size = mdf_json_pack(device_data->response,
                                 "characteristics", characteristics_json_str);
    MDF_ERROR_CHECK(device_data->response_size < 0, ESP_FAIL,
                    "mdf_json_pack, device_data->response_size: %d", device_data->response_size);

    return ESP_OK;
}

static esp_err_t mdf_device_get_status(device_data_t *device_data)
{
    MDF_ERROR_CHECK(!mdf_device_get_value, ESP_FAIL, "this device does not support get_status");

    esp_err_t ret                    = ESP_OK;
    wifi_mesh_addr_t mesh_id          = {0};
    char mesh_id_str[32]              = {0};
    int value                         = 0;
    int cids[CHARACTERISTICS_MAX_NUM] = {0};
    int cids_num                      = mdf_json_parse(device_data->request, "cids", cids);
    device_data->request[0]           = '\0';
    device_data->response[0]          = '\0';
    char *characteristics             = device_data->request;
    char *characteristics_value       = device_data->response;

    ESP_ERROR_CHECK(esp_mesh_get_id((mesh_addr_t *)&mesh_id));
    sprintf(mesh_id_str, MACSTR, MAC2STR(mesh_id.mac));

    for (int i = 0; i < cids_num; ++i) {
        ret = mdf_device_get_value(cids[i], &value);
        MDF_ERROR_CONTINUE(ret < 0, "mdf_device_get_value, ret: %d", ret);

        *characteristics_value = '\0';
        ret = mdf_json_pack(characteristics_value, "cid", cids[i]);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);

        ret = mdf_json_pack(characteristics_value, "value", value);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);

        ret = mdf_json_pack(characteristics, "[]", characteristics_value);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);
    }

    *(device_data->response)   = '\0';
    device_data->response_size = mdf_json_pack(device_data->response,
                                 "characteristics", characteristics);
    return ESP_OK;
}

static esp_err_t mdf_device_set_status(device_data_t *device_data)
{
    MDF_ERROR_CHECK(!mdf_device_set_value, ESP_FAIL, "This device does not support set_status");

    esp_err_t ret = ESP_OK;
    char *characteristics[CHARACTERISTICS_MAX_NUM];

    int cids_num = mdf_json_parse(device_data->request, "characteristics", characteristics);
    int cid      = 0;
    int value    = 0;

    for (int i = 0; i < cids_num; ++i) {
        ret = mdf_json_parse(characteristics[i], "cid",  &cid);
        MDF_ERROR_CONTINUE(ret < 0, "mdf_json_parse cid, ret: %d", ret);

        ret = mdf_json_parse(characteristics[i], "value", &value);
        MDF_ERROR_CONTINUE(ret < 0, "mdf_json_parse cid, ret: %d", ret);

        ret = mdf_device_set_value(cid, (void *)value);
        MDF_ERROR_CONTINUE(ret < 0, "mdf_device_set_value, ret: %d", ret);

        mdf_free(characteristics[i]);
    }

    mdf_trigger_isr();

    return ESP_OK;
}

static esp_err_t mdf_device_add(device_data_t *device_data)
{
    esp_err_t ret          = ESP_OK;
    int timeout             = MDF_AUTO_NETWORK_TIMEROUT;
    char **whitelist_json  = mdf_malloc(100 * sizeof(char *));
    int whitelist_num      = 0;

    mdf_json_parse(device_data->request, "timeout", &timeout);

    whitelist_num = mdf_json_parse(device_data->request, "whitelist", whitelist_json);

    if (whitelist_num <= 0) {
        mdf_free(whitelist_json);
        MDF_LOGW("mdf_json_parse, whitelist, whitelist_num: %d", whitelist_num);
        return ESP_FAIL;
    }

    uint8_t *whitelist = mdf_calloc(whitelist_num, 6);

    for (int i = 0; i < whitelist_num; ++i) {
        str2mac(whitelist_json[i], whitelist + i * 6);
    }

    mdf_network_add_whitelist(whitelist, whitelist_num);

    mdf_free(whitelist);
    mdf_free(whitelist_json);

    ret = mdf_network_enable_auto(timeout);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_network_enable_auto, ret: %d", ret);

    return ESP_OK;
}

static esp_err_t mdf_device_config_debug(device_data_t *device_data)
{
    esp_err_t ret   = 0;
    char mac_str[16] = {0};
    mdf_espnow_debug_pkt_t *mdf_espnow_pkt = mdf_calloc(1,
            sizeof(mdf_espnow_debug_pkt_t) + 6);

    ret = mdf_json_parse(device_data->request, "type", &mdf_espnow_pkt->type);
    MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "mdf_json_parse type, ret: %d", ret);

    ret = mdf_json_parse(device_data->request, "oprt", &mdf_espnow_pkt->oprt);
    MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "mdf_json_parse oprt, ret: %d", ret);

    ret = mdf_json_parse(device_data->request, "params", &mdf_espnow_pkt->params);
    MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "mdf_json_parse params, ret: %d", ret);

    ret = mdf_json_parse(device_data->request, "recv_mac", mac_str);
    MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "mdf_json_parse recv_mac, ret: %d", ret);

    mdf_espnow_pkt->size = 6;
    str2mac(mac_str, mdf_espnow_pkt->data);
    ret = mdf_espnow_set_config(mdf_espnow_pkt);
    MDF_ERROR_GOTO(ret < 0, ERR_EXIT, "mdf_espnow_set_config, ret: %d", ret);

    MDF_LOGI("espnow_debug type: %d, oprt: %d, params: %d, recv_mac: "MACSTR,
             mdf_espnow_pkt->type, mdf_espnow_pkt->oprt,
             mdf_espnow_pkt->params, MAC2STR(mdf_espnow_pkt->data));

    mdf_free(mdf_espnow_pkt);
    return ESP_OK;

ERR_EXIT:
    mdf_free(mdf_espnow_pkt);
    return ESP_FAIL;
}

static esp_err_t mdf_device_rename(device_data_t *device_data)
{
    esp_err_t ret = ESP_OK;
    char name[32] = {0};

    MDF_LOGV("mdf_device_rename: %s", device_data->request);

    ret = mdf_json_parse(device_data->request, "name", name);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_parse, ret: %d", ret);

    ret = mdf_device_set_name(name);;
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "device_config_set, ret: %d", ret);

    return ESP_OK;
}

static esp_err_t mdf_device_get_wifi_mesh(device_data_t *device_data)
{
    esp_err_t ret       = 0;
    mesh_addr_t mesh_id = {0};
    uint8_t bssid[6]    = {0};
    char tmp_str[17]    = {0};
    int interval_ms     = 0;
    mesh_assoc_t mesh_assoc = {0x0};

    ret = mdf_json_pack(device_data->response, "free_heap", esp_get_free_heap_size());
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);

    ret = mdf_json_pack(device_data->response, "running_time",
                        xTaskGetTickCount() * (1000 / configTICK_RATE_HZ));
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);

    ESP_ERROR_CHECK(esp_mesh_get_id(&mesh_id));
    mac2str(mesh_id.addr, tmp_str);
    ret = mdf_json_pack(device_data->response, "id", tmp_str);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);

    ret = mdf_json_pack(device_data->response, "max_layer", esp_mesh_get_max_layer());
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);

    ret = mdf_json_pack(device_data->response, "max_connections", esp_mesh_get_ap_connections());
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);

    ret = mdf_json_pack(device_data->response, "layer", esp_mesh_get_layer());
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);

    ESP_ERROR_CHECK(esp_mesh_get_parent_bssid((mesh_addr_t *)bssid));
    ADDR_AP2STA(bssid);
    mac2str(bssid, tmp_str);
    ret = mdf_json_pack(device_data->response, "parent_mac", tmp_str);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);

    ret = mdf_json_pack(device_data->response, "type", esp_mesh_get_type());
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);

    esp_wifi_vnd_mesh_get(&mesh_assoc);
    ret = mdf_json_pack(device_data->response, "prarent_rssi", mesh_assoc.rssi);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);

    ret = mdf_json_pack(device_data->response, "router_rssi", mesh_assoc.router_rssi);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);

    ret = mdf_json_pack(device_data->response, "rc_rssi", mesh_assoc.rc_rssi);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);

    ESP_ERROR_CHECK(esp_mesh_get_beacon_interval(&interval_ms));
    ret = mdf_json_pack(device_data->response, "beacon_interval", interval_ms);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);

    ret = mdf_json_pack(device_data->response, "assoc_expire", esp_mesh_get_ap_assoc_expire());
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);

    ret = mdf_json_pack(device_data->response, "capacity_num", esp_mesh_get_capacity_num());
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);

    device_data->response_size = ret;

    MDF_LOGI("wifi mesh config: %s", device_data->response);

    return ESP_OK;
}

static esp_err_t mdf_device_set_wifi_mesh(device_data_t *device_data)
{
    esp_err_t ret                     = ESP_OK;
    int timer                         = 0;
    uint32_t max_layer                = 0;
    uint32_t max_connection           = 0;
    mesh_switch_parent_t switch_paras = {0};

    if (mdf_json_parse(device_data->request, "beacon_interval", &timer) == ESP_OK) {
        ret = esp_mesh_set_beacon_interval(timer);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "esp_mesh_set_beacon_interval, ret: %d", ret);
        MDF_LOGI("wifi mesh beacon interval: %d ms", timer);
    }

    if (mdf_json_parse(device_data->request, "passive_scan", &timer) == ESP_OK) {
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "esp_mesh_set_passive_scan_time, ret: %d", ret);
        mdf_info_save("passive_scan", &timer, sizeof(uint32_t));
        MDF_LOGI("wifi mesh passive scan time: %d ms", timer);
    }

    if (mdf_json_parse(device_data->request, "assoc_expire", &timer) == ESP_OK) {
        mdf_info_save("assoc_expire", &timer, sizeof(uint32_t));
        MDF_LOGI("wifi mesh assc expire time: %d ms", timer);
    }

    if (mdf_json_parse(device_data->request, "backoff_rssi", &switch_paras.backoff_rssi) == ESP_OK) {
        mdf_info_save("backoff_rssi", &switch_paras.backoff_rssi, sizeof(int));
        MDF_LOGI("wifi backoff rssi: %d", switch_paras.backoff_rssi);
    }

    if (mdf_json_parse(device_data->request, "select_rssi", &switch_paras.select_rssi) == ESP_OK) {
        mdf_info_save("select_rssi", &switch_paras.select_rssi, sizeof(int));
        MDF_LOGI("wifi select rssi: %d", switch_paras.select_rssi);
    }

    if (mdf_json_parse(device_data->request, "cnx_rssi", &switch_paras.cnx_rssi) == ESP_OK) {
        mdf_info_save("cnx_rssi", &switch_paras.cnx_rssi, sizeof(int));
        MDF_LOGI("wifi cnx rssi: %d", switch_paras.cnx_rssi);
    }

    if (mdf_json_parse(device_data->request, "switch_rssi", &switch_paras.switch_rssi) == ESP_OK) {
        mdf_info_save("switch_rssi", &switch_paras.switch_rssi, sizeof(int));
        MDF_LOGI("wifi switch rssi: %d", switch_paras.switch_rssi);
    }

    if (mdf_json_parse(device_data->request, "max_layer", &max_layer) == ESP_OK) {
        mdf_info_save("max_layer", &max_layer, sizeof(uint32_t));
        MDF_LOGI("wifi mesh max layer: %d", max_layer);
    }

    if (mdf_json_parse(device_data->request, "max_connection", &max_connection) == ESP_OK) {
        mdf_info_save("max_connection", &max_connection, sizeof(uint32_t));
        MDF_LOGI("wifi mesh max connections: %d", max_connection);
    }

    uint32_t rate_enable = false;

    if (mdf_json_parse(device_data->request, "6m_rate", &rate_enable) == ESP_OK) {
        mdf_info_save("6m_rate", &rate_enable, sizeof(uint32_t));

        MDF_LOGI("wifi mesh 6m rate enable: %d", rate_enable);
    }

    return ESP_OK;
}

static esp_err_t mdf_device_set_position(device_data_t *device_data)
{
    esp_err_t ret     = ESP_OK;
    char position[16] = {0};

    if (mdf_json_parse(device_data->request, "position", position) < 0) {
        return ESP_FAIL;
    }

    ret = mdf_info_save("position", position, sizeof(position));
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_save, ret: %d", ret);

    MDF_LOGI("set device position: %s", position);

    return ESP_OK;
}

static device_handle_t g_device_handle_list[DEVICE_HANDLE_SIZE] = {
    {"reset",             mdf_device_reset},
    {"reboot",            mdf_device_reboot},
    {"config_network",    mdf_device_config_network},
    {"set_network",       mdf_device_set_network},
    {"ota_status",        mdf_device_ota_status},
    {"ota_reboot",        mdf_device_ota_reboot},
    {"get_device_info",   mdf_device_get_info},
    {"get_status",        mdf_device_get_status},
    {"set_status",        mdf_device_set_status},
    {"add_device",        mdf_device_add},
    {"rename_device",     mdf_device_rename},
    {"get_mesh_config",   mdf_device_get_wifi_mesh},
    {"set_mesh_config",   mdf_device_set_wifi_mesh},
    {"config_debug",      mdf_device_config_debug},
    {"set_position",      mdf_device_set_position},
    {NULL,                NULL},
};

/*************************** device request handle ***************************/
void mdf_device_request_task(void *arg)
{
    esp_err_t ret                   = ESP_OK;
    char func_name[32]              = {0};
    int status_code                 = 0;
    wifi_mesh_data_type_t data_type = {0};
    wifi_mesh_addr_t src_addr       = {0};
    wifi_mesh_addr_t dest_addr      = {0};

    device_data_t device_data = {
        .request       = mdf_malloc(WIFI_MESH_PACKET_MAX_SIZE),
        .request_size  = 0,
        .response      = mdf_malloc(WIFI_MESH_PACKET_MAX_SIZE),
        .response_size = 0,
    };

    for (;;) {
        status_code                = ESP_FAIL;
        device_data.response_size  = WIFI_MESH_PACKET_MAX_SIZE;
        device_data.response_proto = MDF_PROTO_JSON;

        memset(device_data.response, 0, WIFI_MESH_PACKET_MAX_SIZE);
        memset(device_data.request, 0, WIFI_MESH_PACKET_MAX_SIZE);

        device_data.request_size = mdf_wifi_mesh_recv(&src_addr, &data_type, device_data.request,
                                   WIFI_MESH_PACKET_MAX_SIZE, portMAX_DELAY);
        MDF_ERROR_CONTINUE(device_data.request_size < 0 || data_type.proto != MDF_PROTO_JSON,
                           "mdf_wifi_mesh_recv, ret: %d, proto: %d", device_data.request_size, data_type.proto);
        ret = mdf_json_parse(device_data.request, "request", func_name);
        MDF_ERROR_CONTINUE(ret < 0, "mdf_json_parse, ret: %d, key: %s, value: %s",
                           ret, func_name, device_data.request);

        /**< if we can find this HTTP PATH from our list, we will handle this request. */
        for (int i = 0; g_device_handle_list[i].func; i++) {
            if (!strncmp(func_name, g_device_handle_list[i].func_name, strlen(g_device_handle_list[i].func_name))) {
                status_code = g_device_handle_list[i].func(&device_data);
                MDF_LOGV("status_code: %d, func: %s, type: %x", status_code, func_name, data_type.val);
                break;
            }
        }

        /**< check flag to decide whether reponse */
        if (data_type.no_response) {
            continue;
        }

        if (data_type.to_server) {
            /**< reponse data in bin or json format */
            memset(&data_type, 0, sizeof(wifi_mesh_data_type_t));
            data_type.to_server   = true;
            data_type.proto       = device_data.response_proto;
            memcpy(&dest_addr, &src_addr, sizeof(wifi_mesh_addr_t));

            if (data_type.proto == MDF_PROTO_JSON) {
                device_data.response_size = mdf_json_pack(device_data.response, "status_code", status_code);
                MDF_ERROR_CONTINUE(device_data.response_size <= 0,
                                   "mdf_json_pack, device_data.response_size: %d", device_data.response_size);
            }
        } else {
            /**< notice to server of update state */
            data_type.to_server = true;
            data_type.notice    = true;
            data_type.proto     = MDF_PROTO_BIN;

            device_data.response[0]   = NOTICE_STATUS;
            device_data.response_size = 1;

            memcpy(&dest_addr, WIFI_MESH_ROOT_DEFAULT_ADDR, sizeof(wifi_mesh_addr_t));
        }

        ret = mdf_wifi_mesh_send(&dest_addr, &data_type, device_data.response, device_data.response_size);
        MDF_LOGV("response_size: %d, response: %s", device_data.response_size, device_data.response);
        MDF_ERROR_CONTINUE(ret <= 0, "mdf_write, ret: %d", ret);
    }

    mdf_free(device_data.request);
    mdf_free(device_data.response);
    vTaskDelete(NULL);
}

esp_err_t mdf_device_add_handle(const char *func_name, void *func)
{
    MDF_PARAM_CHECK(func_name);
    MDF_PARAM_CHECK(func);

    int i = 0;

    for (i = 0; g_device_handle_list[i].func_name && i < DEVICE_HANDLE_SIZE; i++) {
        if (!strncasecmp(g_device_handle_list[i].func_name, func_name,
                         strlen(g_device_handle_list[i].func_name))) {
            g_device_handle_list[i].func = (device_func_t)func;
            return ESP_OK;
        }
    }

    MDF_ERROR_CHECK(i == DEVICE_HANDLE_SIZE, ESP_FAIL, "device processing list is full");

    g_device_handle_list[i].func_name = func_name;
    g_device_handle_list[i].func = (device_func_t)func;

    return ESP_OK;
}

esp_err_t mdf_device_init_handle(mdf_event_loop_cb_t event_cb,
                                 const void *get_value, const void *set_value)
{
    esp_err_t ret                   = ESP_OK;
    esp_chip_info_t chip_info       = {0};
    TimerHandle_t timer             = NULL;
    mdf_device_get_value            = get_value;
    mdf_device_set_value            = set_value;
    network_config_t network_config = {0};
    wifi_mesh_config_t mesh_config  = {0};

    event_cb = event_cb ? event_cb : mdf_device_event_loop_cb;
    ret = mdf_event_loop_init(event_cb);
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "mdf_event_loop_init, ret: %d", ret);

    ret = mdf_reboot_event_init();
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "mdf_reboot_event_init, ret: %d", ret);

    /**< pint system information */
    esp_chip_info(&chip_info);
    MDF_LOGI("******************* SYSTEM INFO *******************");
    MDF_LOGI("idf version      : %s", esp_get_idf_version());
    // MDF_LOGI("mdf version     : %s", MDF_VERSION);
    MDF_LOGI("device version   : %s-%s-%d",
             mdf_device_get_name(), mdf_device_get_version(), mdf_device_get_tid());
    MDF_LOGI("compile time     : %s %s", __DATE__, __TIME__);
    MDF_LOGI("free heap        : %d B", esp_get_free_heap_size());
    MDF_LOGI("CPU cores        : %d", chip_info.cores);
    MDF_LOGI("function         : WiFi%s%s",
             (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
             (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    MDF_LOGI("silicon revision : %d", chip_info.revision);
    MDF_LOGI("flash            : %d MB %s", spi_flash_get_chip_size() / (1024 * 1024),
             (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    MDF_LOGI("***************************************************");

    ret = mdf_wifi_init();
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "mdf_wifi_init, ret: %x", ret);

    ret = mdf_espnow_init();
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "mdf_espnow_init, ret: %d", ret);

    /**< initilize mdf console module */
    mdf_console_init();

    /**< initilize espnow debug module */
    ret = mdf_espnow_debug_init();
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "mdf_espnow_debug_init, ret: %d", ret);

    /**< initilize low power */
    ret = mdf_low_power_init();
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "mdf_espnow_debug_init, ret: %d", ret);

    if (mdf_get_running_mode() & TRANS_WIFI_MESH) {
        ret = mdf_network_get_config(&network_config);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_get_network_config, ret: %d", ret);
        MDF_LOGD("ssid: %s, password: %s, bssid: "MACSTR,
                 network_config.ssid, network_config.password, MAC2STR(network_config.bssid));
        MDF_LOGD("mesh channel: %d, mesh_id: "MACSTR,
                 network_config.channel, MAC2STR(network_config.mesh_id));

        mesh_config.channel = network_config.channel;
        strncpy(mesh_config.ssid, network_config.ssid, sizeof(mesh_config.ssid));
        strncpy(mesh_config.password, network_config.password, sizeof(mesh_config.password));
        memcpy(mesh_config.bssid, network_config.bssid, sizeof(mesh_config.bssid));
        memcpy(&mesh_config.mesh_id, network_config.mesh_id, sizeof(mesh_config.mesh_id));

        ret = mdf_wifi_mesh_init(&mesh_config);
        MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "mdf_wifi_mesh_init, ret: %d", ret);

        xTaskCreate(mdf_device_request_task, "mdf_device_request", 1024 * 3,
                    NULL, MDF_TASK_DEFAULT_PRIOTY, NULL);
    }

    timer = xTimerCreate("mdf_show_sysinfo_timercb", 5000 / portTICK_RATE_MS,
                         true, NULL, mdf_show_sysinfo_timercb);
    xTimerStart(timer, 0);

    return ESP_OK;
}
