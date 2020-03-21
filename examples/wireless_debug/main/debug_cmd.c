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
#include "wpa2/utils/base64.h"

#include "sdcard.h"
#include "debug_recv.h"
#include "debug_cmd.h"

#include "mwifi.h"
#include "mdebug_espnow.h"
#include "mdf_common.h"

#define SDCARD_MOUNT_POINT "/sdcard"

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
    } else if (log_receiver_args.remove->count) {
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
    } else if (wifi_config.sta.channel > 0 && wifi_config.sta.channel <= 14) {
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
    wifi_config_args.channel  = arg_int0("c", "channel", "<channel (1 ~ 13)>", "Channel of ESP-WIFI-MESH and router");
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

    esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config);
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
        MDF_ERROR_CHECK(ret != MDF_OK, ESP_ERR_INVALID_ARG, "ESP-WIFI-MESH networking parameter error");

        ret = mwifi_set_config(&ap_config);
        MDF_ERROR_CHECK(ret != MDF_OK, ESP_ERR_INVALID_ARG, "ESP-WIFI-MESH's AP configuration parameter error");

        ret = mwifi_restart();
        MDF_ERROR_CHECK(ret != MDF_OK, ESP_ERR_INVALID_ARG, "restart ESP-WIFI-MESH");

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
                     "layer: %d/%d, capacity: %d/%d/%d/%d, root_mac: " MACSTR,
                     ap_record.ssid, MAC2STR(ap_record.bssid), ap_record.primary, ap_record.rssi,
                     assoc.rssi, assoc.router_rssi, MAC2STR(assoc.mesh_id), assoc.mesh_type,
                     assoc.layer, assoc.layer_cap, assoc.assoc, assoc.assoc_cap,
                     assoc.root_cap, assoc.self_cap, MAC2STR(assoc.rc_addr));
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

    if (strlen((char *)wifi_config.sta.ssid)) {
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_connect());
    } else if (wifi_config.sta.channel) {
        ESP_ERROR_CHECK(esp_wifi_set_channel(wifi_config.sta.channel, WIFI_SECOND_CHAN_NONE));
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
    wifi_scan_args.mesh_password  = arg_str0("P", "mesh_password", "<mesh_password>", "Password for secure communication between devices in ESP-WIFI-MESH");
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

static struct {
    struct arg_str *list;
    struct arg_str *remove;
    struct arg_str *output;
    struct arg_str *type;
    struct arg_end *end;
} sdcard_args;

/**
 * @brief  A function which implements sdcard command.
 */
static int sdcard_func(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void **) &sdcard_args) != ESP_OK) {
        arg_print_errors(stderr, sdcard_args.end, argv[0]);
        return ESP_FAIL;
    }

    if (sdcard_args.list->count) {
        sdcard_list_file(sdcard_args.list->sval[0]);
    }

    if (sdcard_args.remove->count) {
        sdcard_remove_file(sdcard_args.remove->sval[0]);
    }

    if (sdcard_args.output->count) {
        file_format_t type = FILE_TYPE_STRING;

        if (sdcard_args.type->count) {
            if (!strcasecmp(sdcard_args.type->sval[0], "string")) {
                type = FILE_TYPE_STRING;
            } else if (!strcasecmp(sdcard_args.type->sval[0], "hex")) {
                type = FILE_TYPE_HEX;
            } else if (!strcasecmp(sdcard_args.type->sval[0], "base64")) {
                type = FILE_TYPE_BASE64;
            }  else if (!strcasecmp(sdcard_args.type->sval[0], "bin")) {
                type = FILE_TYPE_BIN;
            } else {
                type = FILE_TYPE_NONE;
            }
        }

        sdcard_print_file(sdcard_args.output->sval[0], type, INT32_MAX);
    }

    return MDF_OK;
}

/**
 * @brief  Register sdcard command.
 */
void register_sdcard()
{
    sdcard_args.list   = arg_str0("l", "list", "<file_name>", "List all matched FILE(s)");
    sdcard_args.remove = arg_str0("r", "remove", "<file_name>", "Remove designation FILE(s)");
    sdcard_args.output = arg_str0("o", "output", "<file_name>", "Concatenate FILE(s) to standard output");
    sdcard_args.type   = arg_str0("t", "type", "<type (hex, string, base64)>", "FILE(s) output type");
    sdcard_args.end    = arg_end(3);

    const esp_console_cmd_t cmd = {
        .command = "sdcard",
        .help = "SD-Card operation",
        .hint = NULL,
        .func = &sdcard_func,
        .argtable = &sdcard_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
