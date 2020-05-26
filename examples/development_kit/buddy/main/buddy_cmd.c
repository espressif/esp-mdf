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

#include "esp_console.h"
#include "argtable3/argtable3.h"

#include "mwifi.h"
#include "mdebug_espnow.h"
#include "mdf_common.h"

#include "buddy_cmd.h"
#include "buddy_recv.h"
#include "attenuator.h"

static const char *TAG = "debug_cmd";

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
    struct arg_str *addr;
    struct arg_str *command;
    struct arg_end *end;
} command_args;

/**
 * @brief  A function which implements `command` command.
 */
static int command_func(int argc, char **argv)
{
    mdf_err_t ret = MDF_OK;
    uint8_t dest_addr[6] = {0x0};

    if (arg_parse(argc, argv, (void **) &command_args) != ESP_OK) {
        arg_print_errors(stderr, command_args.end, argv[0]);
        return ESP_FAIL;
    }

    ret = mac_str2hex(command_args.addr->sval[0], dest_addr);
    MDF_ERROR_CHECK(ret == false, ESP_ERR_INVALID_ARG,
                    "The format of the address is incorrect. Please enter the format as xx:xx:xx:xx:xx:xx");

    ret = mdebug_espnow_write(dest_addr, command_args.command->sval[0],
                              strlen(command_args.command->sval[0]), MDEBUG_ESPNOW_CONSOLE, portMAX_DELAY);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "mespnow_write");

    return MDF_OK;
}

/**
 * @brief  Register `command` command.
 */
void register_command()
{
    command_args.addr    = arg_str1(NULL, NULL, "<addr ((xx:xx:xx:xx:xx:xx))>", "MAC of the monitored device");
    command_args.command = arg_str1(NULL, NULL, "<\"command\">", "Console command for the monitored device");
    command_args.end     = arg_end(2);

    const esp_console_cmd_t cmd = {
        .command = "command",
        .help = "Let the console command run on the monitoring device",
        .hint = NULL,
        .func = &command_func,
        .argtable = &command_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static struct {
    struct arg_str *addr;
    struct arg_lit *add;
    struct arg_lit *remove;
    struct arg_str *tag;
    struct arg_str *level;
    struct arg_lit *info;
    struct arg_end *end;
} log_receiver_args;

/**
 * @brief  A function which implements log command.
 */
static int log_receiver_func(int argc, char **argv)
{
    mdf_err_t ret = MDF_OK;
    uint8_t dest_addr[6] = {0x0};
    char *command_str    = NULL;
    uint8_t self_mac[6]  = {0x0};

    if (arg_parse(argc, argv, (void **) &log_receiver_args) != ESP_OK) {
        arg_print_errors(stderr, log_receiver_args.end, argv[0]);
        return ESP_FAIL;
    }

    ret = mac_str2hex(log_receiver_args.addr->sval[0], dest_addr);
    MDF_ERROR_CHECK(ret == false, ESP_ERR_INVALID_ARG,
                    "The format of the address is incorrect. Please enter the format as xx:xx:xx:xx:xx:xx");

    if (log_receiver_args.info->count) {
        log_record_print(dest_addr);
        return MDF_OK;
    }

    if (log_receiver_args.add->count) {
        ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, self_mac));
        asprintf(&command_str, "log -s " MACSTR, MAC2STR(self_mac));
    }

    if (log_receiver_args.remove->count) {
        asprintf(&command_str, "log -s 00:00:00:00:00:00");
    }

    if (log_receiver_args.tag->count && log_receiver_args.level->count) {
        asprintf(&command_str, "log -t %s -l %s", log_receiver_args.tag->sval[0], log_receiver_args.level->sval[0]);
    }

    mdebug_espnow_write(dest_addr, command_str, strlen(command_str), MDEBUG_ESPNOW_CONSOLE, portMAX_DELAY);
    MDF_FREE(command_str);

    return MDF_OK;
}

/**
 * @brief  Register log command.
 */
void register_log()
{
    log_receiver_args.addr   = arg_str1(NULL, NULL, "<mac (xx:xx:xx:xx:xx:xx)>", "MAC of the monitored device");
    log_receiver_args.add    = arg_lit0("a", "add", "Add a monitored device");
    log_receiver_args.remove = arg_lit0("r", "remove", "Remove devices");
    log_receiver_args.tag    = arg_str0("t", "tag", "<tag>", "Tag of the log entries to enable, '*' resets log level for all tags to the given value");
    log_receiver_args.level  = arg_str0("l", "level", "<level>", "Selects log level to enable (NONE, ERR, WARN, INFO, DEBUG, VER)");
    log_receiver_args.info   = arg_lit0("i", "info", "Output the received log statistics");
    log_receiver_args.end    = arg_end(6);

    const esp_console_cmd_t cmd = {
        .command = "log",
        .help = "Get the log of the monitored devices",
        .hint = NULL,
        .func = &log_receiver_func,
        .argtable = &log_receiver_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static struct {
    struct arg_str *addr;
    struct arg_lit *length;
    struct arg_lit *recv;
    struct arg_lit *erase;
    struct arg_int *seq;
    struct arg_end *end;
} coredump_args;

/**
 * @brief  A function which implements coredump command.
 */
static int coredump_func(int argc, char **argv)
{
    mdf_err_t ret = MDF_OK;
    uint8_t dest_addr[6] = {0x0};
    char *command_str    = NULL;
    uint8_t self_mac[6]  = {0x0};

    if (arg_parse(argc, argv, (void **) &coredump_args) != ESP_OK) {
        arg_print_errors(stderr, coredump_args.end, argv[0]);
        return ESP_FAIL;
    }

    ret = mac_str2hex(coredump_args.addr->sval[0], dest_addr);
    MDF_ERROR_CHECK(ret == false, ESP_ERR_INVALID_ARG,
                    "The format of the address is incorrect. Please enter the format as xx:xx:xx:xx:xx:xx");

    if (coredump_args.length->count) {
        ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, self_mac));
        asprintf(&command_str, "coredump --sendlength " MACSTR, MAC2STR(self_mac));
    }

    if (coredump_args.recv->count) {
        int seq = 0;

        if (coredump_args.seq->count) {
            seq = coredump_args.seq->ival[0];
        }

        ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, self_mac));
        asprintf(&command_str, "coredump -s " MACSTR " -q %d", MAC2STR(self_mac), seq);
    }

    if (coredump_args.erase->count) {
        asprintf(&command_str, "coredump -e");
    }

    mdebug_espnow_write(dest_addr, command_str, strlen(command_str), MDEBUG_ESPNOW_CONSOLE, portMAX_DELAY);
    MDF_FREE(command_str);

    return MDF_OK;
}

/**
 * @brief  Register coredump command.
 */
void register_coredump()
{
    coredump_args.addr   = arg_str1(NULL, NULL, "<mac (xx:xx:xx:xx:xx:xx)>", "MAC of the monitored device");
    coredump_args.length = arg_lit0("f", "find", "Find out if the surrounding devices have coredump");
    coredump_args.recv   = arg_lit0("r", "recv", "Recv the coredump data of the device");
    coredump_args.erase  = arg_lit0("e", "erase", "Erase the coredump data of the device");
    coredump_args.seq    = arg_int0("q", "sequence", "<seq>", "Receive device coredump data with the specified sequence");
    coredump_args.end    = arg_end(4);

    const esp_console_cmd_t cmd = {
        .command = "coredump",
        .help = "Get the core dump of the monitored devices",
        .hint = NULL,
        .func = &coredump_func,
        .argtable = &coredump_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static struct {
    struct arg_int *channel;
    struct arg_str *ssid;
    struct arg_str *bssid;
    struct arg_str *password;
    struct arg_lit *save;
    struct arg_lit *erase;
    struct arg_end *end;
} wifi_config_args;

/**
 * @brief  A function which implements wifi config command.
 */
static int wifi_config_func(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void **) &wifi_config_args) != ESP_OK) {
        arg_print_errors(stderr, wifi_config_args.end, argv[0]);
        return ESP_FAIL;
    }

    wifi_config_t wifi_config = {0x0};

    if (wifi_config_args.ssid->count) {
        strncpy((char *)wifi_config.sta.ssid, wifi_config_args.ssid->sval[0], sizeof(wifi_config.sta.ssid));
    }

    if (wifi_config_args.password->count) {
        strncpy((char *)wifi_config.sta.password, wifi_config_args.password->sval[0], sizeof(wifi_config.sta.password));
    }

    if (wifi_config_args.bssid->count) {
        mdf_err_t ret = mac_str2hex(wifi_config_args.bssid->sval[0], wifi_config.sta.bssid);
        MDF_ERROR_CHECK(ret != true, ESP_ERR_INVALID_ARG,
                        "The format of the address is incorrect. Please enter the format as xx:xx:xx:xx:xx:xx");
    }

    if (wifi_config_args.channel->count) {
        wifi_config.sta.channel = wifi_config_args.channel->ival[0];
    }

    if (strlen((char *)wifi_config.sta.ssid)) {
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_connect());
    } else {
        ESP_ERROR_CHECK(esp_wifi_set_channel(wifi_config.sta.channel, WIFI_SECOND_CHAN_NONE));
        MDF_LOGI("Set primary/secondary channel of ESP32, channel: %d", wifi_config.sta.channel);
    }

    if (wifi_config_args.save->count) {
        mdf_info_save("wifi_config", &wifi_config, sizeof(wifi_config_t));
    }

    if (wifi_config_args.erase->count) {
        mdf_info_erase("wifi_config");
    }

    return MDF_OK;
}

/**
 * @brief  Register wifi config command.
 */
void register_wifi_config()
{
    wifi_config_args.ssid     = arg_str0("s", "ssid", "<ssid>", "SSID of router");
    wifi_config_args.password = arg_str0("p", "password", "<password>", "Password of router");
    wifi_config_args.bssid    = arg_str0("b", "bssid", "<bssid (xx:xx:xx:xx:xx:xx)>", "BSSID of router");
    wifi_config_args.channel  = arg_int0("c", "channel", "<channel (1 ~ 13)>", "Channel of ESP-MESH and router");
    wifi_config_args.save     = arg_lit0("S", "save", "Save Wi-Fi configuration information");
    wifi_config_args.erase    = arg_lit0("E", "Erase", "Erase Wi-Fi configuration information");
    wifi_config_args.end      = arg_end(4);

    const esp_console_cmd_t cmd = {
        .command = "wifi_config",
        .help = "Set the configuration of the ESP32 STA",
        .hint = NULL,
        .func = &wifi_config_func,
        .argtable = &wifi_config_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static struct {
    struct arg_int *rssi;
    struct arg_str *ssid;
    struct arg_str *bssid;
    struct arg_int *passive;
    struct arg_lit *mesh;
    struct arg_str *mesh_id;
    struct arg_str *mesh_password;
    struct arg_end *end;
} wifi_scan_args;

/**
 * @brief  A function which implements wifi scan command.
 */
static esp_err_t wifi_scan_func(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void **) &wifi_scan_args) != ESP_OK) {
        arg_print_errors(stderr, wifi_scan_args.end, argv[0]);
        return ESP_FAIL;
    }

    mdf_err_t ret      = MDF_OK;
    int ie_len         = 0;
    bool filter_router = false;
    int8_t filter_rssi = -120;
    mesh_assoc_t assoc = {0x0};
    uint16_t ap_number = 0;
    uint8_t bssid[6]   = {0x0};
    wifi_config_t wifi_config      = {0x0};
    wifi_ap_record_t ap_record     = {0x0};
    wifi_scan_config_t scan_config = {
        .show_hidden = true,
        .scan_type   = WIFI_SCAN_TYPE_ACTIVE,
    };

    ESP_ERROR_CHECK(esp_wifi_disconnect());

    if (wifi_scan_args.passive->count) {
        scan_config.scan_type = WIFI_SCAN_TYPE_PASSIVE;
        scan_config.scan_time.passive = wifi_scan_args.passive->ival[0];
    }

    if (wifi_scan_args.ssid->count) {
        scan_config.ssid = (uint8_t *)wifi_scan_args.ssid->sval[0];
    }

    if (wifi_scan_args.bssid->count) {
        ret = mac_str2hex(wifi_scan_args.bssid->sval[0], bssid);
        MDF_ERROR_CHECK(ret != true, ESP_ERR_INVALID_ARG,
                        "The format of the address is incorrect. Please enter the format as xx:xx:xx:xx:xx:xx");
        scan_config.bssid = bssid;
    }

    if (wifi_scan_args.rssi->count) {
        filter_rssi = wifi_scan_args.rssi->ival[0];
        MDF_LOGW("filter_rssi: %d", filter_rssi);
    }

    if (wifi_scan_args.mesh->count) {
        mdf_err_t ret            = MDF_OK;
        mwifi_config_t ap_config = {
            .mesh_type = MESH_ROOT,
            .mesh_id = "123456",
        };
        mwifi_init_config_t networking_config = MWIFI_INIT_CONFIG_DEFAULT();
        networking_config.max_layer      = 1;
        networking_config.max_connection = 1;
        networking_config.capacity_num   = 1;

        if (wifi_scan_args.mesh_id->count) {
            ret = mac_str2hex(wifi_scan_args.mesh_id->sval[0], ap_config.mesh_id);
            MDF_ERROR_CHECK(ret != true, ESP_ERR_INVALID_ARG,
                            "The format of the mesh_id is incorrect. Please enter the format as xx:xx:xx:xx:xx:xx");
        }

        if (wifi_scan_args.mesh_password->count) {
            strncpy(ap_config.mesh_password, wifi_scan_args.mesh_password->sval[0], sizeof(ap_config.mesh_password));
        }

        ret = mwifi_set_init_config(&networking_config);
        MDF_ERROR_CHECK(ret != MDF_OK, ESP_ERR_INVALID_ARG, "ESP-MESH networking parameter error");

        ret = mwifi_set_config(&ap_config);
        MDF_ERROR_CHECK(ret != MDF_OK, ESP_ERR_INVALID_ARG, "ESP-MESH's AP configuration parameter error");

        ret = mwifi_restart();
        MDF_ERROR_CHECK(ret != MDF_OK, ESP_ERR_INVALID_ARG, "restart ESP-MESH");

        ESP_ERROR_CHECK(esp_mesh_set_self_organized(false, false));

        filter_router = true;
    }

    esp_wifi_scan_stop();

    int retry_count = 20;

    do {
        esp_wifi_disconnect();
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

        if (ie_len == sizeof(mesh_assoc_t)) {
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

    if (wifi_scan_args.mesh->count) {
        mwifi_stop();
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    }

    if (mdf_info_load("wifi_config", &wifi_config, sizeof(wifi_config_t)) == MDF_OK) {
        if (strlen((char *)wifi_config.sta.ssid)) {
            ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
            ESP_ERROR_CHECK(esp_wifi_connect());
        } else {
            ESP_ERROR_CHECK(esp_wifi_set_channel(wifi_config.sta.channel, WIFI_SECOND_CHAN_NONE));
        }
    }

    return ESP_OK;
}

/**
 * @brief  Register wifi scan command.
 */
void register_wifi_scan()
{
    wifi_scan_args.rssi    = arg_int0("r", "rssi", "<rssi (-120 ~ 0)>", "Filter device uses RSSI");
    wifi_scan_args.ssid    = arg_str0("s", "ssid", "<ssid>", "Filter device uses SSID");
    wifi_scan_args.bssid   = arg_str0("b", "bssid", "<bssid (xx:xx:xx:xx:xx:xx)>", "Filter device uses AP's MAC");
    wifi_scan_args.passive = arg_int0("p", "passive", "<time (ms)>", "Passive scan time per channel");
    wifi_scan_args.mesh    = arg_lit0("m", "mesh", "Scanning mesh device");
    wifi_scan_args.mesh_id        = arg_str0("i", "mesh_id", "<mesh_id (xx:xx:xx:xx:xx:xx)>", "Mesh network ID");
    wifi_scan_args.mesh_password  = arg_str0("P", "mesh_password", "<mesh_password>", "Password for secure communication between devices in ESP-MESH");
    wifi_scan_args.end     = arg_end(5);

    const esp_console_cmd_t cmd = {
        .command  = "wifi_scan",
        .help     = "Wi-Fi is station mode, start scan ap",
        .hint     = NULL,
        .func     = &wifi_scan_func,
        .argtable = &wifi_scan_args,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

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
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_write", mdf_err_to_name(ret));
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

    buffer[0] = esp_mesh_get_layer();

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

    MDF_LOGI("client Report:");
    MDF_LOGI("[Layer] Interval      Transfer       Bandwidth      Jitter   Lost/Total Datagrams");
    MDF_LOGI("[  %d  ] %2d-%2d sec    %2.2f MBytes    %0.2f Mbits/sec    %d ms    %d/%d (%d%%)", esp_mesh_get_layer(),
             0, spend_time / 1000, total_len, total_len * 8 * 1000 / spend_time, spend_time / write_count,
             lost_count, total_count, lost_count * 100 / total_count);

    MDF_FREE(buffer);
    g_mesh_iperf_cfg.finish = true;

    vTaskDelete(NULL);
}

typedef struct mac_list_ {
    uint8_t mac_addr[MWIFI_ADDR_LEN];
    uint32_t recv_count;
    uint32_t report_ticks;
    uint32_t report_count;
    TickType_t start_ticks;
    SLIST_ENTRY(mac_list_) next;    //!< next command in the list
} mac_list_t;

static void mesh_iperf_server_task(void *arg)
{
    mdf_err_t ret     = MDF_OK;
    uint8_t *buffer   = NULL;
    size_t buffer_len = 0;
    mwifi_data_type_t data_type = {0x0};
    uint8_t mac_addr[MWIFI_ADDR_LEN];

    SLIST_HEAD(test_list_, mac_list_) g_mac_list;
    SLIST_INIT(&g_mac_list);

    MDF_LOGD("[    Client MAC   ] Interval       Transfer     Bandwidth");

    for (; !g_mesh_iperf_cfg.finish;) {
        ret = mwifi_read(mac_addr, &data_type, &buffer,
                         &buffer_len, 100 / portTICK_RATE_MS);

        if (ret == MDF_ERR_MWIFI_TIMEOUT || ret == ESP_ERR_MESH_TIMEOUT) {
            continue;
        }

        MDF_ERROR_GOTO(ret != MDF_OK, FREE_MEM, "<%s> mwifi_read", mdf_err_to_name(ret));

        mac_list_t *it, *tmp;
        SLIST_FOREACH_SAFE(it, &g_mac_list, next, tmp) {
            if (!memcmp(mac_addr, it->mac_addr, MWIFI_ADDR_LEN)) {
                break;
            }
        }

        if (it == NULL) {
            mac_list_t *item = (mac_list_t *) MDF_CALLOC(1, sizeof(mac_list_t));
            memcpy(item->mac_addr, mac_addr, MWIFI_ADDR_LEN);
            SLIST_INSERT_HEAD(&g_mac_list, item, next);
            it = item;
        }

        it->recv_count++;

        if (data_type.custom == 0) {
            it->recv_count   = 0;
            it->report_count   = 0;
            it->start_ticks  = xTaskGetTickCount();
            it->report_ticks = it->start_ticks + g_mesh_iperf_cfg.report_interval * 1000 / portTICK_RATE_MS;
        }

        if (data_type.protocol == IPERF_BANDWIDTH && xTaskGetTickCount() >= it->report_ticks) {
            uint32_t report_timer = (it->report_ticks - it->start_ticks) * portTICK_RATE_MS / 1000;
            double report_size    = (data_type.custom - it->report_count) * g_mesh_iperf_cfg.packet_len / 1e6;
            MDF_LOGD("["MACSTR"]  %2d-%2d sec  %2.2f MBytes  %0.2f Mbits/sec",
                     MAC2STR(mac_addr), report_timer - g_mesh_iperf_cfg.report_interval, report_timer,
                     report_size, report_size * 8 / g_mesh_iperf_cfg.report_interval);

            it->report_ticks = xTaskGetTickCount() + g_mesh_iperf_cfg.report_interval * 1000 / portTICK_RATE_MS;
            it->report_count = data_type.custom;
        } else if (data_type.protocol == IPERF_PING) {
            MDF_LOGV("recv IPERF_PING, seq: %d, recv_count: %d", data_type.custom, it->recv_count);
            buffer[0] = esp_mesh_get_layer();
            ret = mwifi_write(mac_addr, &data_type, buffer, buffer_len, true);
            MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_write", mdf_err_to_name(ret));
        } else if (data_type.protocol == IPERF_BANDWIDTH_STOP) {
            uint32_t total_count = data_type.custom;
            uint32_t lost_count  = total_count - it->recv_count;
            double total_len     = (total_count * g_mesh_iperf_cfg.packet_len) / 1e6;
            uint32_t spend_time  = (xTaskGetTickCount() - it->start_ticks) * portTICK_RATE_MS;

            MDF_LOGI("[    Client MAC   ],\tLayer,\tInterval,\tTransfer,\tBandwidth,\tJitter,\tLost/Total Datagrams");
            MDF_LOGI("["MACSTR"],\t%d,\t%02d-%02d sec,\t%2.2f MBytes,\t%0.2f Mbits/sec,\t%d ms,\t%d/%d (%d%%)",
                     MAC2STR(mac_addr), buffer[0], 0, spend_time / 1000, total_len, total_len * 8 * 1000 / spend_time, spend_time / it->recv_count,
                     lost_count, total_count, lost_count * 100 / total_count);

            data_type.custom = it->recv_count;
            MDF_LOGD("data_type.custom: %d",  data_type.custom);
            ret = mwifi_write(mac_addr, &data_type, &ret, 1, true);
            MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_write", mdf_err_to_name(ret));
        }

FREE_MEM:
        MDF_FREE(buffer);
    }

    mac_list_t *it, *tmp;
    SLIST_FOREACH_SAFE(it, &g_mac_list, next, tmp) {
        SLIST_REMOVE(&g_mac_list, it, mac_list_, next);
        MDF_FREE(it);
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

    if (arg_parse(argc, argv, (void**) &mesh_iperf_args) != ESP_OK) {
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
        MDF_LOGI("ESP-MESH layer: %d", esp_mesh_get_layer());
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
        MDF_LOGI("ESP-MESH window size: 8195 bytes");
        MDF_LOGI("ESP-MESH layer: %d", esp_mesh_get_layer());
        MDF_LOGI("------------------------------------------------------------");

        xTaskCreatePinnedToCore(mesh_iperf_server_task, "mesh_iperf_server", 4 * 1024,
                                NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY,
                                NULL, CONFIG_MDF_TASK_PINNED_TO_CORE);
    }

    return ESP_OK;
}

void register_mesh_iperf()
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
        .help     = "ESP-MESH iperf",
        .hint     = NULL,
        .func     = &mesh_iperf_func,
        .argtable = &mesh_iperf_args,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
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
    if (arg_parse(argc, argv, (void**) &mesh_config_args) != ESP_OK) {
        arg_print_errors(stderr, mesh_config_args.end, argv[0]);
        return MDF_FAIL;
    }

    mdf_err_t ret            = MDF_OK;
    mwifi_config_t ap_config = {0};
    mwifi_init_config_t networking_config = MWIFI_INIT_CONFIG_DEFAULT();
    MDF_ERROR_CHECK(esp_wifi_get_max_tx_power((int8_t *)&ret), ESP_ERR_NOT_SUPPORTED,
                    "Run mesh_config after initializing ESP-MESH");

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
    MDF_ERROR_CHECK(ret != MDF_OK, ESP_ERR_INVALID_ARG, "ESP-MESH networking parameter error");

    ret = mwifi_set_config(&ap_config);
    MDF_ERROR_CHECK(ret != MDF_OK, ESP_ERR_INVALID_ARG, "ESP-MESH's AP configuration parameter error");

    ret = mwifi_restart();
    MDF_ERROR_CHECK(ret != MDF_OK, ESP_ERR_INVALID_ARG, "restart ESP-MESH");

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

void register_mesh_config()
{
    mesh_config_args.ssid           = arg_str0("s", "ssid", "<ssid>", "SSID of router");
    mesh_config_args.password       = arg_str0("p", "password", "<password>", "Password of router");
    mesh_config_args.bssid          = arg_str0("b", "bssid", "<bssid (xx:xx:xx:xx:xx:xx)>", "BSSID of router");
    mesh_config_args.channel        = arg_int0("c", "channel", "<channel (1 ~ 13)>", "Channel of ESP-MESH and router");
    mesh_config_args.mesh_id        = arg_str0("i", "mesh_id", "<mesh_id (6 Bytes)>", "Mesh network ID");
    mesh_config_args.mesh_type      = arg_str0("t", "mesh_type", "<mesh_type ('idle' or 'root' or 'node' or 'leaf')>", "Device type in ESP-MESH");
    mesh_config_args.mesh_password  = arg_str0("P", "mesh_password", "<mesh_password>", "Password for secure communication between devices in ESP-MESH");
    mesh_config_args.max_connection = arg_int0("n", "max_connection", "<max_connection (1 ~ 10)>", "Max number of ESP-MESH softAP connections");
    mesh_config_args.max_layer      = arg_int0("l", "max_layer", "<max_layer (1 ~ 32)>", "Max number of allowed layers in ESP-MESH");
    mesh_config_args.output         = arg_lit0("o", "output", "Print all configuration information");
    mesh_config_args.save           = arg_lit0("S", "save", "Save mesh configuration information");
    mesh_config_args.erase           = arg_lit0("E", "erase", "Erase mesh configuration information");
    mesh_config_args.end            = arg_end(9);

    const esp_console_cmd_t cmd = {
        .command = "mesh_config",
        .help = "ESP-MESH configuration",
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
    if (arg_parse(argc, argv, (void**) &mesh_status_args) != ESP_OK) {
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
        MDF_ERROR_CHECK(ret != MDF_OK, ret, "Start ESP-MESH");
    }

    if (mesh_status_args.stop->count) {
        MDF_LOGD("task, name: %s, HWM: %d", pcTaskGetTaskName(NULL), uxTaskGetStackHighWaterMark(NULL));
        ret = mwifi_stop();
        MDF_ERROR_CHECK(ret != MDF_OK, ret, "Stop ESP-MESH");
    }

    return ESP_OK;
}

void register_mesh_status()
{
    mesh_status_args.start  = arg_lit0("s", "start", "Start ESP-MESH, Start mesh network management service");
    mesh_status_args.stop   = arg_lit0("p", "stop", "Stop ESP-MESH, Stop mesh network management service");
    mesh_status_args.output = arg_lit0("o", "output", "Print all status information");
    mesh_status_args.end    = arg_end(3);

    const esp_console_cmd_t cmd = {
        .command = "mesh_status",
        .help = "ESP-MESH status",
        .hint = NULL,
        .func = &mesh_status_func,
        .argtable = &mesh_status_args,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static struct {
    struct arg_int *attenuator_data;
    struct arg_end *end;
} attenuator_args;

static int attenuator_func(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void**) &attenuator_args) != ESP_OK) {
        arg_print_errors(stderr, attenuator_args.end, argv[0]);
        return MDF_FAIL;
    }

    if (attenuator_args.attenuator_data->count) {
        attenuator_set(attenuator_args.attenuator_data->ival[0]);
    }

    return MDF_OK;
}

void register_attenuator()
{
    attenuator_args.attenuator_data  = arg_int0("d", "data", "<int>", "setting attenuation data");
    attenuator_args.end    = arg_end(3);

    const esp_console_cmd_t cmd = {
        .command = "attenuator",
        .help = "attenuator",
        .hint = NULL,
        .func = &attenuator_func,
        .argtable = &attenuator_args,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
