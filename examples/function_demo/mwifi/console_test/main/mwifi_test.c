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

#include "rom/uart.h"

#include "mwifi.h"
#include "mdf_common.h"
#include "mespnow.h"
#include "mdebug_console.h"
#include "mdebug_log.h"

#include "esp_console.h"
#include "argtable3/argtable3.h"

enum {
    IPERF_BANDWIDTH,
    IPERF_BANDWIDTH_STOP,
    IPERF_PING,
};

struct mesh_iperf_cfg {
    bool finish;
    uint16_t packet_len;
    uint16_t transmit_time;
    uint32_t ping_count;
    uint16_t report_interval;
    uint8_t addr[6];
} g_mesh_iperf_cfg = {
    .finish = true,
};

static const char *TAG = "mwifi_test";

static bool mac_str2hex(const char *mac_str, uint8_t *mac_hex)
{
    MDF_ERROR_ASSERT(!mac_str);
    MDF_ERROR_ASSERT(!mac_hex);

    uint32_t mac_data[6] = {0};

    int ret = sscanf(mac_str, MACSTR, mac_data, mac_data + 1, mac_data + 2,
                     mac_data + 3, mac_data + 4, mac_data + 5);

    for (int i = 0; i < 6; i++) {
        mac_hex[i] = mac_data[i];
    }

    return ret == 6 ? true : false;
}

static struct {
    struct arg_str *ssid;
    struct arg_str *password;
    struct arg_str *bssid;
    struct arg_int *channel;
    struct arg_str *mesh_id;
    struct arg_str *mesh_type;
    struct arg_str *mesh_password;
    struct arg_int *max_connection;
    struct arg_int *max_layer;
    struct arg_lit *output;
    struct arg_lit *save;
    struct arg_lit *erase;
    struct arg_end *end;
} mesh_config_args;

static int mesh_config_func(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void **) &mesh_config_args) != ESP_OK) {
        arg_print_errors(stderr, mesh_config_args.end, argv[0]);
        return MDF_FAIL;
    }

    mdf_err_t ret            = MDF_OK;
    mwifi_config_t ap_config = {0};
    mwifi_init_config_t networking_config = MWIFI_INIT_CONFIG_DEFAULT();
    MDF_ERROR_CHECK(esp_wifi_get_max_tx_power((int8_t *)&ret), ESP_ERR_NOT_SUPPORTED,
                    "Run mesh_config after initializing ESP-WIFI-MESH");

    mwifi_get_config(&ap_config);
    mwifi_get_init_config(&networking_config);

    if (mesh_config_args.output->count) {
        mwifi_print_config();
        return MDF_OK;
    }

    if (mesh_config_args.ssid->count) {
        strncpy(ap_config.router_ssid, mesh_config_args.ssid->sval[0], sizeof(ap_config.router_ssid));
    }

    if (mesh_config_args.password->count) {
        strncpy(ap_config.router_password, mesh_config_args.password->sval[0], sizeof(ap_config.router_password));
    }

    if (mesh_config_args.bssid->count) {
        ret = mac_str2hex(mesh_config_args.bssid->sval[0], ap_config.router_bssid);
        MDF_ERROR_CHECK(ret != true, ESP_ERR_INVALID_ARG,
                        "The format of the address is incorrect. Please enter the format as xx:xx:xx:xx:xx:xx");
    }

    if (mesh_config_args.channel->count) {
        ap_config.channel = mesh_config_args.channel->ival[0];
    }

    if (mesh_config_args.mesh_id->count) {
        ret = mac_str2hex(mesh_config_args.mesh_id->sval[0], ap_config.mesh_id);
        MDF_ERROR_CHECK(ret != true, ESP_ERR_INVALID_ARG,
                        "The format of the mesh_id is incorrect. Please enter the format as xx:xx:xx:xx:xx:xx");
    }

    if (mesh_config_args.mesh_type->count) {
        const char *type_str[] = {"idle", "root", "node", "leaf"};

        for (int i = 0; i < 4; ++i) {
            if (!strcasecmp(type_str[i], mesh_config_args.mesh_type->sval[0])) {
                ap_config.mesh_type = i;
                break;
            }
        }
    }

    if (mesh_config_args.mesh_password->count) {
        strncpy(ap_config.mesh_password, mesh_config_args.mesh_password->sval[0], sizeof(ap_config.mesh_password));
    }

    if (mesh_config_args.max_connection->count) {
        networking_config.max_connection = mesh_config_args.max_connection->ival[0];
    }

    if (mesh_config_args.max_layer->count) {
        networking_config.max_layer = mesh_config_args.max_layer->ival[0];
    }

    ret = mwifi_set_init_config(&networking_config);
    MDF_ERROR_CHECK(ret != MDF_OK, ESP_ERR_INVALID_ARG, "ESP-WIFI-MESH networking parameter error");

    ret = mwifi_set_config(&ap_config);
    MDF_ERROR_CHECK(ret != MDF_OK, ESP_ERR_INVALID_ARG, "ESP-WIFI-MESH's AP configuration parameter error");

    ret = mwifi_restart();
    MDF_ERROR_CHECK(ret != MDF_OK, ESP_ERR_INVALID_ARG, "restart ESP-WIFI-MESH");

    if (mesh_config_args.save->count) {
        mdf_info_save("init_config", &networking_config, sizeof(mwifi_init_config_t));
        mdf_info_save("ap_config", &ap_config, sizeof(mwifi_config_t));
    }

    if (mesh_config_args.erase->count) {
        mdf_info_erase("init_config");
        mdf_info_erase("ap_config");
    }

    mwifi_print_config();

    return ESP_OK;
}

static void register_mesh_config()
{
    mesh_config_args.ssid           = arg_str0("s", "ssid", "<ssid>", "SSID of router");
    mesh_config_args.password       = arg_str0("p", "password", "<password>", "Password of router");
    mesh_config_args.bssid          = arg_str0("b", "bssid", "<bssid (xx:xx:xx:xx:xx:xx)>", "BSSID of router");
    mesh_config_args.channel        = arg_int0("c", "channel", "<channel (1 ~ 13)>", "Channel of ESP-WIFI-MESH and router");
    mesh_config_args.mesh_id        = arg_str0("i", "mesh_id", "<mesh_id (6 Bytes)>", "Mesh network ID");
    mesh_config_args.mesh_type      = arg_str0("t", "mesh_type", "<mesh_type ('idle' or 'root' or 'node' or 'leaf')>", "Device type in ESP-WIFI-MESH");
    mesh_config_args.mesh_password  = arg_str0("P", "mesh_password", "<mesh_password>", "Password for secure communication between devices in ESP-WIFI-MESH");
    mesh_config_args.max_connection = arg_int0("n", "max_connection", "<max_connection (1 ~ 10)>", "Max number of ESP-WIFI-MESH softAP connections");
    mesh_config_args.max_layer      = arg_int0("l", "max_layer", "<max_layer (1 ~ 32)>", "Max number of allowed layers in ESP-WIFI-MESH");
    mesh_config_args.output         = arg_lit0("o", "output", "Print all configuration information");
    mesh_config_args.save           = arg_lit0("S", "save", "Save mesh configuration information");
    mesh_config_args.erase           = arg_lit0("E", "erase", "Erase mesh configuration information");
    mesh_config_args.end            = arg_end(9);

    const esp_console_cmd_t cmd = {
        .command = "mesh_config",
        .help = "ESP-WIFI-MESH configuration",
        .hint = NULL,
        .func = &mesh_config_func,
        .argtable = &mesh_config_args,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}


static struct {
    struct arg_lit *start;
    struct arg_lit *stop;
    struct arg_lit *output;
    struct arg_end *end;
} mesh_status_args;

static int mesh_status_func(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void **) &mesh_status_args) != ESP_OK) {
        arg_print_errors(stderr, mesh_status_args.end, argv[0]);
        return MDF_FAIL;
    }

    mdf_err_t ret = MDF_OK;

    if (mesh_status_args.output->count) {
        uint8_t primary                 = 0;
        wifi_second_chan_t second       = 0;
        mesh_addr_t parent_bssid        = {0};
        uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
        mesh_assoc_t mesh_assoc         = {0x0};
        wifi_sta_list_t wifi_sta_list   = {0x0};

        esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
        esp_wifi_ap_get_sta_list(&wifi_sta_list);
        esp_wifi_get_channel(&primary, &second);
        esp_wifi_vnd_mesh_get(&mesh_assoc);
        esp_mesh_get_parent_bssid(&parent_bssid);

        MDF_LOGI("mac: " MACSTR ", channel: %d, layer: %d, parent bssid: " MACSTR ", parent rssi: %d, node num: %d",
                 MAC2STR(sta_mac), primary, esp_mesh_get_layer(), MAC2STR(parent_bssid.addr),
                 mesh_assoc.rssi, esp_mesh_get_total_node_num());

        for (int i = 0; i < wifi_sta_list.num; i++) {
            MDF_LOGI("Child mac: " MACSTR, MAC2STR(wifi_sta_list.sta[i].mac));
        }
    }

    if (mesh_status_args.start->count) {
        ret = mwifi_restart();
        MDF_ERROR_CHECK(ret != MDF_OK, ret, "Start ESP-WIFI-MESH");
    }

    if (mesh_status_args.stop->count) {
        MDF_LOGD("task, name: %s, HWM: %d", pcTaskGetTaskName(NULL), uxTaskGetStackHighWaterMark(NULL));
        ret = mwifi_stop();
        MDF_ERROR_CHECK(ret != MDF_OK, ret, "Stop ESP-WIFI-MESH");
    }

    return ESP_OK;
}

static void register_mesh_status()
{
    mesh_status_args.start  = arg_lit0("s", "start", "Start ESP-WIFI-MESH, Start mesh network management service");
    mesh_status_args.stop   = arg_lit0("p", "stop", "Stop ESP-WIFI-MESH, Stop mesh network management service");
    mesh_status_args.output = arg_lit0("o", "output", "Print all status information");
    mesh_status_args.end    = arg_end(3);

    const esp_console_cmd_t cmd = {
        .command = "mesh_status",
        .help = "ESP-WIFI-MESH status",
        .hint = NULL,
        .func = &mesh_status_func,
        .argtable = &mesh_status_args,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static struct {
    struct arg_int *rssi;
    struct arg_str *ssid;
    struct arg_str *bssid;
    struct arg_str *type;
    struct arg_int *passive;
    struct arg_end *end;
} mesh_scan_args;

static esp_err_t mesh_scan_func(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void **) &mesh_scan_args) != ESP_OK) {
        arg_print_errors(stderr, mesh_scan_args.end, argv[0]);
        return ESP_FAIL;
    }

    mdf_err_t ret      = MDF_OK;
    int ie_len         = 0;
    int8_t filter_rssi = -120;
    bool filter_router = false;
    bool filter_mesh   = false;
    mesh_assoc_t assoc = {0x0};
    uint16_t ap_number = 0;
    wifi_ap_record_t ap_record     = {0x0};
    uint8_t bssid[6]   = {0x0};
    wifi_scan_config_t scan_config = {
        .show_hidden = true,
        .scan_type   = WIFI_SCAN_TYPE_ACTIVE,
    };

    if (mesh_scan_args.passive->count) {
        scan_config.scan_type = WIFI_SCAN_TYPE_PASSIVE;
        scan_config.scan_time.passive = mesh_scan_args.passive->ival[0];
    }

    if (mesh_scan_args.ssid->count) {
        scan_config.ssid = (uint8_t *)mesh_scan_args.ssid->sval[0];
    }

    if (mesh_scan_args.bssid->count) {
        ret = mac_str2hex(mesh_scan_args.bssid->sval[0], bssid);
        MDF_ERROR_CHECK(ret != true, ESP_ERR_INVALID_ARG,
                        "The format of the address is incorrect. Please enter the format as xx:xx:xx:xx:xx:xx");
        scan_config.bssid = bssid;
    }

    if (mesh_scan_args.rssi->count) {
        filter_rssi = mesh_scan_args.rssi->ival[0];
        MDF_LOGW("filter_rssi: %d", filter_rssi);
    }

    if (mesh_scan_args.type->count) {
        if (!strcasecmp(mesh_scan_args.type->sval[0], "mesh")) {
            filter_mesh = true;
        } else if (!strcasecmp(mesh_scan_args.type->sval[0], "router")) {
            filter_router = true;
        } else {
            MDF_LOGW("Type (%s) not supported, please enter 'mesh' or 'router'",
                     mesh_scan_args.type->sval[0]);
            return ESP_ERR_INVALID_ARG;
        }
    }

    ESP_ERROR_CHECK(esp_mesh_set_self_organized(false, false));
    esp_wifi_scan_stop();

    int retry_count = 3;

    do {
        esp_wifi_scan_start(&scan_config, true);
        esp_wifi_scan_get_ap_num(&ap_number);
    } while (ap_number <= 0 && --retry_count);

    MDF_ERROR_CHECK(ap_number <= 0, ESP_FAIL, "esp_wifi_scan_get_ap_num");

    MDF_LOGI("Get number of APs found, number: %d", ap_number);

    for (int i = 0; i < ap_number; i++) {
        ie_len = 0;
        memset(&ap_record, 0, sizeof(wifi_ap_record_t));
        memset(&assoc, 0, sizeof(mesh_assoc_t));

        esp_mesh_scan_get_ap_ie_len(&ie_len);
        esp_mesh_scan_get_ap_record(&ap_record, &assoc);

        if (ap_record.rssi < filter_rssi) {
            continue;
        }

        if (ie_len == sizeof(mesh_assoc_t) && !filter_mesh) {
            MDF_LOGI("Mesh, ssid: %s, bssid: " MACSTR ", channel: %d, rssi: %d, "
                     "parent_rssi: %d, router_rssi: %d, id: " MACSTR ", type: %d, "
                     "layer: %d/%d, capacity: %d/%d",
                     ap_record.ssid, MAC2STR(ap_record.bssid), ap_record.primary, ap_record.rssi,
                     assoc.rssi, assoc.router_rssi, MAC2STR(assoc.mesh_id), assoc.mesh_type,
                     assoc.layer, assoc.layer_cap, assoc.assoc, assoc.assoc_cap);
        } else if (!filter_router) {
            MDF_LOGI("Router, ssid: %s, bssid: " MACSTR ", channel: %u, rssi: %d",
                     ap_record.ssid, MAC2STR(ap_record.bssid),
                     ap_record.primary, ap_record.rssi);
        }
    }

    mwifi_config_t ap_config = {0};
    mwifi_get_config(&ap_config);

    if (ap_config.mesh_type != MESH_IDLE) {
        ESP_ERROR_CHECK(esp_mesh_set_type(ap_config.mesh_type));
    } else {
        ESP_ERROR_CHECK(esp_mesh_set_self_organized(true, false));
    }

    return ESP_OK;
}

static void register_mesh_scan()
{
    mesh_scan_args.rssi    = arg_int0("r", "rssi", "<rssi (-120 ~ 0)>", "Filter device uses RSSI");
    mesh_scan_args.ssid    = arg_str0("s", "ssid", "<ssid (xx:xx:xx:xx:xx:xx)>", "Filter device uses SSID");
    mesh_scan_args.bssid   = arg_str0("b", "bssid", "<bssid (xx:xx:xx:xx:xx:xx)>", "Filter device uses AP's MAC");
    mesh_scan_args.type    = arg_str0("t", "type", "<type ('router' or 'mesh')>", "Filter device uses type");
    mesh_scan_args.passive = arg_int0("p", "passive", "<time>", "Passive scan time per channel");
    mesh_scan_args.end     = arg_end(5);

    const esp_console_cmd_t cmd = {
        .command  = "mesh_scan",
        .help     = "ESP-WIFI-MESH scan",
        .hint     = NULL,
        .func     = &mesh_scan_func,
        .argtable = &mesh_scan_args,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static void mesh_iperf_client_task(void *arg)
{
    mdf_err_t ret   = MDF_OK;
    uint8_t *buffer = MDF_MALLOC(g_mesh_iperf_cfg.packet_len);

    mwifi_data_type_t data_type = {
        .protocol = IPERF_BANDWIDTH,
    };

    TickType_t start_ticks = xTaskGetTickCount();
    TickType_t end_ticks   = start_ticks + g_mesh_iperf_cfg.transmit_time * 1000 * portTICK_RATE_MS;
    uint32_t total_count   = 0;

    MDF_LOGI("[    Server MAC   ] Interval       Transfer     Bandwidth");

    for (uint32_t report_ticks = start_ticks + g_mesh_iperf_cfg.report_interval * 1000 / portTICK_RATE_MS, report_count = 0;
            xTaskGetTickCount() < end_ticks && !g_mesh_iperf_cfg.finish; ++total_count) {
        ret = mwifi_write(g_mesh_iperf_cfg.addr, &data_type, buffer,
                          g_mesh_iperf_cfg.packet_len, true);
        MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mwifi_write", mdf_err_to_name(ret));
        data_type.custom++;

        if (xTaskGetTickCount() >= report_ticks) {
            uint32_t report_timer = (report_ticks - start_ticks) * portTICK_RATE_MS / 1000;
            double report_size    = (data_type.custom - report_count) * g_mesh_iperf_cfg.packet_len / 1e6;

            MDF_LOGI("["MACSTR"]  %2d-%2d sec  %2.2f MBytes  %0.2f Mbits/sec",
                     MAC2STR(g_mesh_iperf_cfg.addr), report_timer - g_mesh_iperf_cfg.report_interval, report_timer,
                     report_size, report_size * 8 / g_mesh_iperf_cfg.report_interval);

            report_ticks = xTaskGetTickCount() + g_mesh_iperf_cfg.report_interval * 1000 / portTICK_RATE_MS;
            report_count = data_type.custom;
        }
    }

    data_type.protocol  = IPERF_BANDWIDTH_STOP;
    int retry_count     = 3;
    uint32_t spend_time = (xTaskGetTickCount() - start_ticks) * portTICK_RATE_MS;

    do {
        ret = mwifi_write(g_mesh_iperf_cfg.addr, &data_type, buffer, 1, true);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_write", mdf_err_to_name(ret));

        size_t buffer_len = g_mesh_iperf_cfg.packet_len;
        ret = mwifi_read(g_mesh_iperf_cfg.addr, &data_type, buffer, &buffer_len, 3000 / portTICK_RATE_MS);
    } while (ret != MDF_OK && retry_count-- > 0);

    if (ret != MDF_OK) {
        MDF_LOGW("<%s> Receive server response failed", mdf_err_to_name(ret));
    }

    uint32_t write_count = data_type.custom;
    uint32_t lost_count  = total_count - write_count;
    double total_len     = (total_count * g_mesh_iperf_cfg.packet_len) / 1e6;

    if (total_count && write_count && spend_time) {
        MDF_LOGI("client Report:");
        MDF_LOGI("[ ID] Interval      Transfer       Bandwidth      Jitter   Lost/Total Datagrams");
        MDF_LOGI("[000] %2d-%2d sec    %2.2f MBytes    %0.2f Mbits/sec    %d ms    %d/%d (%d%%)",
                 0, spend_time / 1000, total_len, total_len * 8 * 1000 / spend_time, spend_time / write_count,
                 lost_count, total_count, lost_count * 100 / total_count);
    }

    MDF_FREE(buffer);
    g_mesh_iperf_cfg.finish = true;

    vTaskDelete(NULL);
}

static void mesh_iperf_server_task(void *arg)
{
    mdf_err_t ret     = MDF_OK;
    uint8_t *buffer   = NULL;
    size_t buffer_len = 0;
    mwifi_data_type_t data_type = {0x0};
    TickType_t start_ticks = 0;
    uint32_t recv_count    = 0;

    MDF_LOGI("[    Client MAC   ] Interval       Transfer     Bandwidth");

    for (uint32_t report_ticks = 0, report_count = 0;
            !g_mesh_iperf_cfg.finish;) {
        ret = mwifi_read(g_mesh_iperf_cfg.addr, &data_type, &buffer,
                         &buffer_len, 100 / portTICK_RATE_MS);

        if (ret == MDF_ERR_MWIFI_TIMEOUT || ret == ESP_ERR_MESH_TIMEOUT) {
            continue;
        } else if (ret != MDF_OK) {
            g_mesh_iperf_cfg.finish = true;
            MDF_LOGW("<%s> mwifi_read", mdf_err_to_name(ret));
            goto FREE_MEM;
        }

        recv_count++;

        if (data_type.custom == 0) {
            recv_count   = 0;
            start_ticks  = xTaskGetTickCount();
            report_ticks = start_ticks + g_mesh_iperf_cfg.report_interval * 1000 / portTICK_RATE_MS;
        }

        if (data_type.protocol == IPERF_BANDWIDTH && xTaskGetTickCount() >= report_ticks) {
            uint32_t report_timer = (report_ticks - start_ticks) * portTICK_RATE_MS / 1000;
            double report_size    = (data_type.custom - report_count) * g_mesh_iperf_cfg.packet_len / 1e6;
            MDF_LOGI("["MACSTR"]  %2d-%2d sec  %2.2f MBytes  %0.2f Mbits/sec",
                     MAC2STR(g_mesh_iperf_cfg.addr), report_timer - g_mesh_iperf_cfg.report_interval, report_timer,
                     report_size, report_size * 8 / g_mesh_iperf_cfg.report_interval);

            report_ticks = xTaskGetTickCount() + g_mesh_iperf_cfg.report_interval * 1000 / portTICK_RATE_MS;
            report_count = data_type.custom;
        } else if (data_type.protocol == IPERF_PING) {
            MDF_LOGV("recv IPERF_PING, seq: %d, recv_count: %d", data_type.custom, recv_count);
            buffer[0] = esp_mesh_get_layer();
            ret = mwifi_write(g_mesh_iperf_cfg.addr, &data_type, buffer, buffer_len, true);
            MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_write", mdf_err_to_name(ret));
        } else if (data_type.protocol == IPERF_BANDWIDTH_STOP) {
            uint32_t total_count = data_type.custom;
            uint32_t lost_count  = total_count - recv_count;
            double total_len     = (total_count * g_mesh_iperf_cfg.packet_len) / 1e6;
            uint32_t spend_time  = (xTaskGetTickCount() - start_ticks) * portTICK_RATE_MS;

            MDF_LOGI("[ ID] Interval      Transfer       Bandwidth      Jitter   Lost/Total Datagrams");
            MDF_LOGI("[000] %2d-%2d sec    %2.2f MBytes    %0.2f Mbits/sec    %d ms    %d/%d (%d%%)",
                     0, spend_time / 1000, total_len, total_len * 8 * 1000 / spend_time, spend_time / recv_count,
                     lost_count, total_count, lost_count * 100 / total_count);

            data_type.custom = recv_count;
            MDF_LOGD("data_type.custom: %d",  data_type.custom);
            ret = mwifi_write(g_mesh_iperf_cfg.addr, &data_type, &ret, 1, true);
            MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_write", mdf_err_to_name(ret));
        }

FREE_MEM:
        MDF_FREE(buffer);
    }

    vTaskDelete(NULL);
}

static void mesh_iperf_ping_task(void *arg)
{
    mdf_err_t ret     = MDF_OK;
    size_t buffer_len = g_mesh_iperf_cfg.packet_len;
    uint8_t *buffer   = MDF_MALLOC(buffer_len);

    size_t send_count = 0;
    size_t recv_count = 0;
    size_t spend_time = 0;

    size_t recv_size      = 0;
    uint8_t *recv_data    = NULL;
    uint16_t server_layer = 0;
    uint32_t max_time     = 0;
    uint32_t min_time     = -1;
    uint16_t res_time     = 0;
    struct timeval start, end, res;
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0};

    for (int seq = 0; seq < g_mesh_iperf_cfg.ping_count && !g_mesh_iperf_cfg.finish; ++seq) {
        mwifi_data_type_t data_type = {
            .protocol = IPERF_PING,
            .custom   = seq,
        };

        send_count++;
        gettimeofday(&start, NULL);
        ret = mwifi_write(g_mesh_iperf_cfg.addr, &data_type, buffer, buffer_len, true);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_write", mdf_err_to_name(ret));

        do {
            ret = mwifi_read(src_addr, &data_type, &recv_data, &recv_size, 20000 / portTICK_RATE_MS);

            if (ret == MDF_OK && (data_type.protocol != IPERF_PING || data_type.custom != seq)) {
                MDF_LOGW("data_size: %d", recv_size);
                MDF_FREE(recv_data);
            }
        } while (ret == MDF_OK && (data_type.protocol != IPERF_PING || data_type.custom != seq));

        gettimeofday(&end, NULL);
        timersub(&end, &start, &res);
        res_time = res.tv_sec * 1000 + res.tv_usec / 1000;
        spend_time += res_time;
        max_time =  MAX(max_time, res_time);
        min_time =  MIN(min_time, res_time);

        if (ret == MDF_ERR_MWIFI_TIMEOUT || ret == ESP_ERR_MESH_TIMEOUT) {
            MDF_LOGW("seq=%d Destination Host Unreachable", data_type.custom);
            continue;
        }

        recv_count++;
        server_layer = recv_data[0];
        MDF_LOGI("%d bytes from " MACSTR ": seq=%d self_layer=%d server_layer=%d time=%ld ms",
                 recv_size, MAC2STR(g_mesh_iperf_cfg.addr), data_type.custom,
                 esp_mesh_get_layer(), server_layer, res.tv_sec * 1000 + res.tv_usec / 1000);

        MDF_FREE(recv_data);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_read", mdf_err_to_name(ret));
    }

    MDF_LOGI("client ping report:");
    MDF_LOGI("ping statistics for " MACSTR, MAC2STR(g_mesh_iperf_cfg.addr));
    MDF_LOGI("%d packets transmitted, %d received, %d packet loss, time: total %d ms, max: %d, min: %d, average %d ms",
             send_count, recv_count, (send_count - recv_count) * 100 / send_count,
             spend_time, max_time, min_time, spend_time / send_count);

    g_mesh_iperf_cfg.finish = true;
    vTaskDelete(NULL);
}

static struct {
    struct arg_str *client;
    struct arg_lit *server;
    struct arg_lit *ping;
    struct arg_int *interval;
    struct arg_int *len;
    struct arg_int *time;
    struct arg_lit *abort;
    struct arg_end *end;
} mesh_iperf_args;

static esp_err_t mesh_iperf_func(int argc, char **argv)
{
    mdf_err_t ret                    = MDF_OK;
    g_mesh_iperf_cfg.packet_len      = MWIFI_PAYLOAD_LEN;
    g_mesh_iperf_cfg.transmit_time   = 60;
    g_mesh_iperf_cfg.report_interval = 3;
    g_mesh_iperf_cfg.ping_count      = 64;

    if (arg_parse(argc, argv, (void **) &mesh_iperf_args) != ESP_OK) {
        arg_print_errors(stderr, mesh_iperf_args.end, argv[0]);
        return ESP_FAIL;
    }

    if (mesh_iperf_args.abort->count) {
        g_mesh_iperf_cfg.finish = true;
        return MDF_OK;
    }

    if (!g_mesh_iperf_cfg.finish) {
        MDF_LOGW("Mesh iperf is running");
        return ESP_ERR_NOT_SUPPORTED;
    }

    if ((mesh_iperf_args.client->count && mesh_iperf_args.server->count)
            || (!mesh_iperf_args.client->count && !mesh_iperf_args.server->count)) {
        MDF_LOGW("Should specific client/server mode");
        return ESP_ERR_INVALID_ARG;
    }

    if (mesh_iperf_args.len->count) {
        g_mesh_iperf_cfg.packet_len = mesh_iperf_args.len->ival[0];
    }

    if (mesh_iperf_args.interval->count) {
        g_mesh_iperf_cfg.report_interval = mesh_iperf_args.interval->ival[0];
    }

    if (mesh_iperf_args.time->count) {
        g_mesh_iperf_cfg.transmit_time = mesh_iperf_args.time->ival[0];
    }

    uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
    esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);

    g_mesh_iperf_cfg.finish = false;

    if (mesh_iperf_args.client->count) {
        ret = mac_str2hex(mesh_iperf_args.client->sval[0], g_mesh_iperf_cfg.addr);
        MDF_ERROR_CHECK(ret == false, ESP_ERR_INVALID_ARG,
                        "The format of the address is incorrect. Please enter the format as xx:xx:xx:xx:xx:xx");

        MDF_LOGI("------------------------------------------------------------");
        MDF_LOGI("client " MACSTR " send to " MACSTR, MAC2STR(sta_mac), MAC2STR(g_mesh_iperf_cfg.addr));
        MDF_LOGI("ESP-WIFI-MESH layer: %d", esp_mesh_get_layer());
        MDF_LOGI("------------------------------------------------------------");
        MDF_LOGI("time: %d, interval: %d, len: %d",
                 g_mesh_iperf_cfg.transmit_time, g_mesh_iperf_cfg.report_interval,
                 g_mesh_iperf_cfg.packet_len);

        if (mesh_iperf_args.ping->count) {
            xTaskCreatePinnedToCore(mesh_iperf_ping_task, "mesh_iperf_ping", 4 * 1024,
                                    NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY,
                                    NULL, CONFIG_MDF_TASK_PINNED_TO_CORE);
        } else {
            xTaskCreatePinnedToCore(mesh_iperf_client_task, "mesh_iperf_client", 4 * 1024,
                                    NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY,
                                    NULL, CONFIG_MDF_TASK_PINNED_TO_CORE);
        }
    }

    if (mesh_iperf_args.server->count) {
        MDF_LOGI("------------------------------------------------------------");
        MDF_LOGI("Server " MACSTR " listening", MAC2STR(sta_mac));
        MDF_LOGI("ESP-WIFI-MESH window size: 8195 bytes");
        MDF_LOGI("ESP-WIFI-MESH layer: %d", esp_mesh_get_layer());
        MDF_LOGI("------------------------------------------------------------");

        xTaskCreatePinnedToCore(mesh_iperf_server_task, "mesh_iperf_server", 4 * 1024,
                                NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY,
                                NULL, CONFIG_MDF_TASK_PINNED_TO_CORE);
    }

    return ESP_OK;
}

static void register_mesh_iperf()
{
    mesh_iperf_args.client   = arg_str0("c", "client", "<host (xx:xx:xx:xx:xx:xx)>", "run in client mode, ping to <host>");
    mesh_iperf_args.server   = arg_lit0("s", "server", "run in server mode, receive from throughput or ping");
    mesh_iperf_args.ping     = arg_lit0("p", "ping", "run in ping mode, send to <host>");
    mesh_iperf_args.interval = arg_int0("i", "interval", "<interval (sec)>", "seconds between periodic bandwidth reports (default 3 secs)");
    mesh_iperf_args.time     = arg_int0("t", "time", "<time (sec)>", "time in seconds to transmit for (default 10 secs)");
    mesh_iperf_args.len      = arg_int0("l", "len", "<len (Bytes)>", "length of buffer in bytes to read or write (Defaults: 1456 Bytes)");
    mesh_iperf_args.abort    = arg_lit0("a", "abort", "abort running mesh-iperf");
    mesh_iperf_args.end      = arg_end(6);

    const esp_console_cmd_t cmd = {
        .command  = "mesh_iperf",
        .help     = "ESP-WIFI-MESH iperf",
        .hint     = NULL,
        .func     = &mesh_iperf_func,
        .argtable = &mesh_iperf_args,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static mdf_err_t wifi_init()
{
    mdf_err_t ret          = nvs_flash_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        MDF_ERROR_ASSERT(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    MDF_ERROR_ASSERT(ret);

    tcpip_adapter_init();
    MDF_ERROR_ASSERT(esp_event_loop_init(NULL, NULL));
    MDF_ERROR_ASSERT(esp_wifi_init(&cfg));
    MDF_ERROR_ASSERT(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    MDF_ERROR_ASSERT(esp_wifi_set_mode(WIFI_MODE_STA));
    MDF_ERROR_ASSERT(esp_wifi_set_ps(WIFI_PS_NONE));
    MDF_ERROR_ASSERT(esp_mesh_set_6m_rate(false));
    MDF_ERROR_ASSERT(esp_wifi_start());

    return MDF_OK;
}

static mdf_err_t event_loop_cb(mdf_event_loop_t event, void *ctx)
{
    mesh_event_info_t *event_info = (mesh_event_info_t *)ctx;

    switch (event) {
        case MDF_EVENT_MWIFI_STARTED:
            MDF_LOGI("MESH is started");
            break;

        case MDF_EVENT_MWIFI_PARENT_CONNECTED:
            MDF_LOGI("Parent is connected on station interface");
            break;

        case MDF_EVENT_MWIFI_PARENT_DISCONNECTED:
            MDF_LOGI("Parent is disconnected on station interface");
            break;

        case MDF_EVENT_MWIFI_CHILD_DISCONNECTED:
            MDF_LOGI("disconnected child: " MACSTR, MAC2STR(event_info->child_disconnected.mac));
            break;

        case MDF_EVENT_MWIFI_ROUTING_TABLE_ADD:
        case MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE:
            MDF_LOGI("total_num: %d, changed: %d", esp_mesh_get_total_node_num(), event_info->routing_table.rt_size_change);
            break;

        case MDF_EVENT_MWIFI_ROOT_GOT_IP: {
            MDF_LOGI("Root obtains the IP address. It is posted by LwIP stack automatically");
            MDF_LOGI("sta ip: " IPSTR ", mask: " IPSTR ", gw: " IPSTR,
                     IP2STR(&event_info->got_ip.ip_info.ip),
                     IP2STR(&event_info->got_ip.ip_info.netmask),
                     IP2STR(&event_info->got_ip.ip_info.gw));
            break;
        }

        default:
            break;
    }

    return MDF_OK;
}

void app_main()
{
    mwifi_config_t ap_config = {0};
    mwifi_init_config_t networking_config = MWIFI_INIT_CONFIG_DEFAULT();
    mdebug_log_config_t log_config = {
        .log_uart_enable = true,
        .log_espnow_enable = true,
    };

    /**
    * @brief Set the log level for serial port printing.
    */
    esp_log_level_set("*", ESP_LOG_INFO);
    // esp_log_level_set(TAG, ESP_LOG_DEBUG);

    /**
     * @brief Initialize wifi mesh.
     */
    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));
    MDF_ERROR_ASSERT(wifi_init());
    MDF_ERROR_ASSERT(mespnow_init());
    MDF_ERROR_ASSERT(mwifi_init(&networking_config));

    if (mdf_info_load("init_config", &networking_config, sizeof(mwifi_init_config_t)) == MDF_OK
            && mdf_info_load("ap_config", &ap_config, sizeof(mwifi_config_t)) == MDF_OK) {
        MDF_ERROR_ASSERT(mwifi_set_config(&ap_config));
        MDF_ERROR_ASSERT(mwifi_set_init_config(&networking_config));
        MDF_ERROR_ASSERT(mwifi_start());
    }

    /**
     * @brief Add debug function, you can use serial command and wireless debugging.
     *      1. Initialize console module
     */
    MDF_ERROR_ASSERT(mdebug_console_init());
    MDF_ERROR_ASSERT(mdebug_log_set_config(&log_config));
    mdebug_cmd_register_common();

    /**
     * @brief register command.
     */
    register_mesh_config();
    register_mesh_status();
    register_mesh_scan();
    register_mesh_iperf();

    printf("\n");
    MDF_LOGI(" ==================================================");
    MDF_LOGI(" |             Steps to test ESP-WIFI-MESH             |");
    MDF_LOGI(" |                                                |");
    MDF_LOGI(" |  1. Print 'help' to gain overview of commands  |");
    MDF_LOGI(" |     mdf> help                                  |");
    MDF_LOGI(" |  2. Configure ESP-WIFI-MESH                         |");
    MDF_LOGI(" |     mdf> mesh_config -i <id> -s <ssid> -p <pwd> |");
    MDF_LOGI(" |  3. Setup ESP-WIFI-MESH                             |");
    MDF_LOGI(" |     mdf> mesh_status -s                        |");
    MDF_LOGI(" |  4. Run iperf to test throughput               |");
    MDF_LOGI(" |     mdf> mesh_iperf -s                         |");
    MDF_LOGI(" |     mdf> mesh_iperf -c <host>                  |");
    MDF_LOGI(" |  5. Run ping to test Delay                     |");
    MDF_LOGI(" |     mdf> mesh_iperf -s                         |");
    MDF_LOGI(" |     mdf> mesh_iperf -p -c <host>               |");
    MDF_LOGI(" |  6. Run scan to get the network environment    |");
    MDF_LOGI(" |     mdf> mesh_scan                             |");
    MDF_LOGI(" |                                                |");
    MDF_LOGI(" ==================================================\n");
}
