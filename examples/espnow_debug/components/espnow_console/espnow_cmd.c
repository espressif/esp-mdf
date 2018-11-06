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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "esp_log.h"
#include "esp_system.h"
#include "driver/rtc_io.h"
#include "argtable3/argtable3.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_wifi.h"
#include "esp_log.h"

#include "esp_console.h"
#include "mdf_common.h"
#include "mdf_info_store.h"
#include "mdf_espnow.h"
#include "mdf_espnow_debug.h"

#include "espnow_sdcard.h"
#include "espnow_console.h"

static const char *TAG = "espnow_cmd";

/**< Arguments used by 'add' function */
static struct {
    struct arg_str *mac; /**< -m mac*/
    struct arg_end *end;
} add_args;

/**< Arguments used by 'del' function */
static struct {
    struct arg_str *mac; /**< -m mac */
    struct arg_end *end;
} del_args;

/**< Arguments used by 'log' function */
static struct {
    struct arg_int *level; /**< -l log level */
    struct arg_end *end;
} log_args;

static struct {
    struct arg_int *level;
    struct arg_str *tag;
    struct arg_end *end;
} local_log_args;

static struct {
    struct arg_str *ssid;
    struct arg_str *password;
    struct arg_end *end;
} join_args;

static struct {
    struct arg_int *primary_channel;
    struct arg_str *secondary_channel;
    struct arg_end *end;
} channel_args;

typedef struct {
    char ssid[32];
    char password[64];
} wifi_connect_t;

/**< file operation cmd args, including: ls, rm, read */
typedef struct {
    struct arg_str *type;
    struct arg_end *end;
} file_oprt_args;
file_oprt_args ls_args = { 0 };
file_oprt_args rm_args = { 0 };
file_oprt_args read_args = { 0 };

/**< Arguments used by 'manual' function */
static struct {
    struct arg_int *type; /**< -t manual type */
    struct arg_end *end;
} manual_args;

/**< Arguments used by 'erase_nvs' function */
static struct {
    struct arg_str *key; /**< -k key */
    struct arg_end *end;
} erase_nvs_args;

/* the remote peer could be in scanning or espnow network config state
 * this means it will switch channel for every 100 ms
 * the espnow terminal tries to send multiple times to make sure they can receive */
#define ESPNOW_SEND_WAIT_COUNT     20
#define ESPNOW_SEND_BACKOFF_MS     70

#define MAX_DEV_NUM  ESP_NOW_MAX_TOTAL_PEER_NUM
/* defines the max devices can be operated in one command (add/del) */
#define MAX_DEV_IN_ONE_CMD   5

/**< global argument of added devices, num and mac */
static uint8_t g_dev_num = 0;
static uint8_t g_dev_mac[MAX_DEV_NUM][6]    = { { 0 }, };
static bool    g_dev_mac_index[MAX_DEV_NUM] = { false }; /**< for del cmd operation */
static wifi_connect_t g_wifi_connect        = { 0 };

/**< declaration of functions */
static void register_free();
static void register_restart();

static void register_add();
static void register_del();
static void register_delr();
static void register_list();
static void register_log();
static void register_dumpreq();
static void register_dumperase();
static void register_wifi();
static void file_oprt();
static void register_manual();
static void register_erase_nvs();

void register_cmds()
{
    /**< systems */
    register_free();
    register_restart();

    /* wifi */
    register_wifi();

    /**< espnow */
    register_add();
    register_del();
    register_delr();
    register_list();
    register_log();
    register_dumpreq();
    register_dumperase();

    /**< file operation cmd, including: ls, rm, read */
    file_oprt();

    register_manual();
    register_erase_nvs();
}

static inline bool str_to_mac(const char *str, uint8_t *dest)
{
    uint8_t loop = 0;
    uint8_t tmp = 0;
    const char *src_p = str;

    if (strlen(src_p) != 17) { /**< must be like 12:34:56:78:90:AB */
        return false;
    }

    for (loop = 0; loop < 17 ; loop++) {
        if (loop % 3 == 2) {
            if (src_p[loop] != ':') {
                return false;
            }

            continue;
        }

        if ((src_p[loop] >= '0') && (src_p[loop] <= '9')) {
            tmp = tmp * 16 + src_p[loop] - '0';
        } else if ((src_p[loop] >= 'A') && (src_p[loop] <= 'F')) {
            tmp = tmp * 16 + src_p[loop] - 'A' + 10;
        } else if ((src_p[loop] >= 'a') && (src_p[loop] <= 'f')) {
            tmp = tmp * 16 + src_p[loop] - 'a' + 10;
        } else {
            return false;
        }

        if (loop % 3 == 1) {
            *dest++ = tmp;
            tmp = 0;
        }
    }

    return true;
}
/**< 'free' command prints available heap memory */
static int free_mem(int argc, char **argv)
{
    MDF_LOGI("free heap: %d", esp_get_free_heap_size());
    return 0;
}
static void register_free()
{
    const esp_console_cmd_t cmd = {
        .command = "free",
        .help = "Get the total size of heap memory available",
        .hint = NULL,
        .func = &free_mem,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/**< 'restart' command restarts the program */
static int restart(int argc, char **argv)
{
    MDF_LOGI("Restarting ...");
    esp_restart();
    return 0;
}
static void register_restart()
{
    const esp_console_cmd_t cmd = {
        .command = "restart",
        .help = "Restart the program",
        .hint = NULL,
        .func = &restart,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/**< ------------------- espnow related functions ------------------- */
static bool espnow_send_command_internal(bool retry, void *send_pkt, size_t len, const uint8_t *dst_mac)
{
    bool send_ret = false;
    uint8_t i;
    uint8_t retry_count = retry ? ESPNOW_SEND_WAIT_COUNT : 1;
    int ret;

    MDF_ERROR_CHECK(!send_pkt || len <= 0, false,
                    "espnow_send_command, send_pkt: %p, len: %d", send_pkt, len);

    for (i = 0; i < retry_count; i++) {
        ret = mdf_espnow_write(MDF_ESPNOW_DEBUG, dst_mac, send_pkt, len, 0);

        if (ret <= 0) {
            vTaskDelay(ESPNOW_SEND_BACKOFF_MS / portTICK_PERIOD_MS);
            continue;
        }

        send_ret = true;
        break;
    }

    if (!send_ret) {
        MDF_LOGE("espnow_send command to mac: "MACSTR" failed", MAC2STR(dst_mac));
    }

    return send_ret;
}

#define espnow_send_command(send_pkt, len, dst_mac) espnow_send_command_internal(true, send_pkt, len, dst_mac)

static bool espnow_send_to_all(void* send_pkt, size_t len, bool is_positive_send)
{
    bool send_ret = true;

    MDF_ERROR_CHECK(!send_pkt || len <= 0, false,
                    "espnow_send_to_all, send_pkt: %p, len: %d", send_pkt, len);

    for (int i = 0; i < g_dev_num; ++i) {
        if (g_dev_mac_index[i] != is_positive_send) {
            continue;
        }
        send_ret &= espnow_send_command_internal(is_positive_send, send_pkt, len, g_dev_mac[i]);
    }

    return send_ret;
}

static esp_err_t espnow_save_dev(void)
{
    esp_err_t ret = ESP_OK;

    ret = mdf_info_save("dev_num", &g_dev_num, sizeof(uint8_t));
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_save dev_num, ret: %d", ret);

    ret = mdf_info_save("dev_mac", g_dev_mac, MAX_DEV_NUM * 6);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_save dev_mac, ret: %d", ret);

    ret = mdf_info_save("dev_mac_i", g_dev_mac_index, MAX_DEV_NUM * sizeof(bool));
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_save dev_mac_i, ret: %d", ret);

    return ESP_OK;
}

esp_err_t espnow_load_dev(void)
{
    esp_err_t ret = ESP_OK;

#ifndef CONFIG_AUTO_CONNECT_TO_DEFAULT_AP
    wifi_config_t wifi_config = { 0 };

    ret = mdf_info_load("wifi_connect", &g_wifi_connect, sizeof(wifi_connect_t));
    if(ret != ESP_FAIL){
        MDF_LOGI("wifi sta connect to %s", g_wifi_connect.ssid);
        strncpy((char*) wifi_config.sta.ssid, g_wifi_connect.ssid, sizeof(g_wifi_connect.ssid));
        strncpy((char*) wifi_config.sta.password, g_wifi_connect.password, sizeof(g_wifi_connect.password));

        ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
        ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
        ESP_ERROR_CHECK( esp_wifi_disconnect() );
        ESP_ERROR_CHECK( esp_wifi_connect() );
    }
#endif

    ret = mdf_info_load("dev_num", &g_dev_num, sizeof(uint8_t));
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_load dev_num, ret: %d", ret);

    ret = mdf_info_load("dev_mac", g_dev_mac, MAX_DEV_NUM * 6);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_load dev_mac, ret: %d", ret);

    ret = mdf_info_load("dev_mac_i", g_dev_mac_index, MAX_DEV_NUM * sizeof(bool));
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_load dev_mac_i, ret: %d", ret);

    for (int i = 0; i < g_dev_num; i++) {
        ret = mdf_espnow_add_peer_no_encrypt(g_dev_mac[i]);
        MDF_ERROR_CONTINUE(ret < 0, "mdf_espnow_add_peer g_dev_num: %d, mac:"
                           MACSTR, g_dev_num, MAC2STR(g_dev_mac[g_dev_num]));
    }

    return ESP_OK;
}

static bool dev_exist(uint8_t mac[6])
{
    for(int i = 0; i < g_dev_num; i++){
        if(!memcmp(mac, g_dev_mac[i], 6)){
            return true;
        }
    }

    return false;
}

static int add(int argc, char** argv)
{
    int ret       = ESP_OK;
    int nerrors   = 0;
    uint8_t tmp_mac[6];
    bool send_ret = true;
    mdf_espnow_debug_pkt_t *espnow_pkt = NULL;

    nerrors = arg_parse(argc, argv, (void**) &add_args);
    if (nerrors) {
        arg_print_errors(stderr, add_args.end, argv[0]);
        return 1;
    }

    espnow_pkt = mdf_calloc(1, sizeof(mdf_espnow_debug_pkt_t) + 6);
    espnow_pkt->type   = MDF_ESPNOW_DEBUG_CONFIG;
    espnow_pkt->oprt   = MDF_ESPNOW_CONFIG_ALL;
    espnow_pkt->params = MDF_ESPNOW_LOG_INFO;
    espnow_pkt->size   = 6;
    ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, espnow_pkt->data));

    for (int i = 0; i < add_args.mac->count; i++) {
        if (str_to_mac(add_args.mac->sval[i], tmp_mac)) {
            if(dev_exist(tmp_mac)){
                MDF_LOGI("mac address already exist: %s", add_args.mac->sval[i]);
                continue;
            }

            ret = mdf_espnow_add_peer_no_encrypt(tmp_mac);
            MDF_ERROR_CONTINUE(ret < 0, "mdf_espnow_add_peer_no_encryptm g_dev_num: %d, mac: %s",
                               g_dev_num, add_args.mac->sval[i]);
            send_ret = espnow_send_command(espnow_pkt, sizeof(mdf_espnow_debug_pkt_t) + 6, tmp_mac);
            if (!send_ret) {
                continue;
            }
            memcpy(g_dev_mac[g_dev_num], tmp_mac, 6);
            g_dev_mac_index[g_dev_num] = true;
            g_dev_num++;
            MDF_LOGI("mac address added: %s", add_args.mac->sval[i]);
        } else {
            MDF_LOGI("incorrect mac address format: %s", add_args.mac->sval[i]);
        }
    }
    mdf_free(espnow_pkt);
    ESP_ERROR_CHECK(espnow_save_dev());

    return 0;
}
static void register_add()
{
    add_args.mac = arg_strn("m", "mac", "<m>", 1, MAX_DEV_IN_ONE_CMD, "mac of espnow devices");
    add_args.end = arg_end(10);

    const esp_console_cmd_t add_cmd = {
        .command = "add",
        .help = "add espnow devices",
        .hint = NULL,
        .func = &add,
        .argtable = &add_args
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&add_cmd) );
}

static int del(int argc, char** argv)
{
    int nerrors     = 0;
    uint8_t i, j;
    uint8_t removed_mac = 0;
    uint8_t tmp_mac[6];
    bool send_ret   = true;
    mdf_espnow_debug_pkt_t *espnow_pkt = NULL;

    nerrors = arg_parse(argc, argv, (void**) &del_args);
    if (nerrors) {
        arg_print_errors(stderr, del_args.end, argv[0]);
        return 1;
    }
    /**< del devices pkt */
    espnow_pkt = mdf_calloc(1, sizeof(mdf_espnow_debug_pkt_t) + 6);
    espnow_pkt->type   = MDF_ESPNOW_DEBUG_CONFIG;
    espnow_pkt->oprt   = MDF_ESPNOW_CONFIG_ALL;
    espnow_pkt->params = MDF_ESPNOW_LOG_NONE;
    espnow_pkt->size   = 6;

    if (del_args.mac->count == 0) {
        /* delete all */
        for (int i = 0; i < g_dev_num; i++) {
            if (g_dev_mac_index[i] == true) {
                send_ret = espnow_send_command(espnow_pkt, sizeof(mdf_espnow_debug_pkt_t) + 6, g_dev_mac[i]);
                removed_mac++;
                g_dev_mac_index[i] = false;
                if (send_ret) {
                    MDF_LOGI("remove mac success: "MACSTR, MAC2STR(g_dev_mac[i]));
                } else {
                    MDF_LOGE("remove mac failed, force to remove: "MACSTR, MAC2STR(g_dev_mac[i]));
                }
            }
        }
    } else {
        for (i = 0; i < del_args.mac->count; i++) {
            if (str_to_mac(del_args.mac->sval[i], tmp_mac)) {
                for (j = 0; j < g_dev_num; j++) {
                    if (!memcmp(g_dev_mac[j], tmp_mac, 6)) {
                        send_ret = espnow_send_command(espnow_pkt, sizeof(mdf_espnow_debug_pkt_t) + 6, tmp_mac);
                        g_dev_mac_index[j] = false;
                        removed_mac++;
                        if (send_ret) {
                            MDF_LOGI("remove mac success: %s", del_args.mac->sval[i]);
                        } else {
                            MDF_LOGE("remove mac failed, force to remove: %s", del_args.mac->sval[i]);
                        }
                        break;
                    }
                }
            } else {
                MDF_LOGI("incorrect mac address format: %s", del_args.mac->sval[i]);
            }
        }
    }
    mdf_free(espnow_pkt);
    if (removed_mac > 0) {
        /**< update dev mac table, copy all mac to the beginning of the list */
        for (i = j = 0; i < g_dev_num; i++) {
            if (g_dev_mac_index[i] && i != j) {
                memcpy(g_dev_mac[j], g_dev_mac[i], 6);
                g_dev_mac_index[i] = false;
                g_dev_mac_index[j] = true;
                j++;
                continue;
            }
        }
        g_dev_num -= removed_mac;
        ESP_ERROR_CHECK(espnow_save_dev());
    }
    return 0;
}

static void register_del()
{
    del_args.mac = arg_strn("m", "mac", "<m>", 0, MAX_DEV_IN_ONE_CMD, "mac of espnow device");
    del_args.end = arg_end(10);

    const esp_console_cmd_t del_cmd = {
        .command = "del",
        .help = "delete espnow devices\n\tuse command \"del\""
                "without any param will delete all devices.",
        .hint = NULL,
        .func = &del,
        .argtable = &del_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&del_cmd) );
}

/**< 'delr' command delete router information */
static int delr(int argc, char **argv)
{
    MDF_LOGI("del router info, ssid: %s", g_wifi_connect.ssid);

    esp_err_t ret = mdf_info_erase("wifi_connect");
    MDF_ERROR_CHECK(ret != ESP_OK, 1, "mdf_info_erase wifi_connect, ret: %d", ret);

    return 0;
}

static void register_delr()
{
    const esp_console_cmd_t cmd = {
        .command = "delr",
        .help = "Delete router info",
        .hint = NULL,
        .func = &delr,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/**< 'list' command print all added devices mac */
static int list(int argc, char** argv)
{
    MDF_LOGI("g_dev_num: %d, MAX_DEV_NUM: %d", g_dev_num, MAX_DEV_NUM);

    for (int i = 0; i < g_dev_num; i++) {
        MDF_LOGI("[%u] added device: "MACSTR", status: %s",
                 i, MAC2STR(g_dev_mac[i]), g_dev_mac_index[i] ? "true" : "false");
    }

    return 0;
}
static void register_list()
{
    const esp_console_cmd_t list_cmd = {
        .command = "list",
        .help = "Print all added devices mac",
        .hint = NULL,
        .func = &list,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&list_cmd) );
}

/* 'loglevel' command set device log level */
static int loglevel(int argc, char** argv)
{
    int nerrors   = 0;
    bool send_ret = true;
    mdf_espnow_debug_pkt_t *espnow_pkt = NULL;

    nerrors = arg_parse(argc, argv, (void**) &log_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, log_args.end, argv[0]);
        return 1;
    }

    espnow_pkt = mdf_calloc(1, sizeof(mdf_espnow_debug_pkt_t));
    espnow_pkt->type   = MDF_ESPNOW_DEBUG_CONFIG;
    espnow_pkt->oprt   = MDF_ESPNOW_CONFIG_LOG;

    if (log_args.level->count == 0) {
        espnow_pkt->params = MDF_ESPNOW_LOG_NONE;
    } else {
        espnow_pkt->params = log_args.level->ival[0];
    }
    MDF_LOGI("espnow set log level: %d", espnow_pkt->params);

    send_ret = espnow_send_to_all(espnow_pkt, sizeof(mdf_espnow_debug_pkt_t), true);
    mdf_free(espnow_pkt);
    MDF_ERROR_CHECK(!send_ret, 1, "not all devices send ok");

    return 0;
}

static int local_loglevel(int argc, char** argv)
{
    int nerrors   = 0;
    uint8_t log_level = ESP_LOG_INFO;
    const char * log_tag = "*";

    nerrors = arg_parse(argc, argv, (void**) &local_log_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, local_log_args.end, argv[0]);
        return 1;
    }

    if (local_log_args.level->count > 0) {
        log_level = local_log_args.level->ival[0];
    }
    if (local_log_args.tag->count > 0) {
        log_tag = local_log_args.tag->sval[0];
    }
    esp_log_level_set((const char *) log_tag, log_level);

    return 0;
}

static void register_log()
{
    log_args.level = arg_int0("l", "level", "<l>", "espnow log level");
    log_args.end = arg_end(1);

    const esp_console_cmd_t log_cmd = {
        .command = "log",
        .help = "espnow set devices log level: \n"
        "\tNONE, ERR, WARN, INFO, DEBUG, VER [0 ~ 5]\n",
        .hint = NULL,
        .func = &loglevel,
        .argtable = &log_args,
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&log_cmd) );

    local_log_args.level = arg_int0("l", "level", "<l>", "local log level");
    local_log_args.tag = arg_str0("t", "tag", "<s>", "local log tag");
    local_log_args.end = arg_end(5);

    const esp_console_cmd_t local_log_cmd = {
        .command = "llog",
        .help = "set local log level: \n"
        "\tNONE, ERR, WARN, INFO, DEBUG, VER [0 ~ 5]\n",
        .hint = NULL,
        .func = &local_loglevel,
        .argtable = &local_log_args,
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&local_log_cmd) );
}

static int dumpreq(int argc, char** argv)
{
    bool send_ret = true;
    mdf_espnow_debug_pkt_t *espnow_pkt = NULL;

    espnow_pkt = mdf_calloc(1, sizeof(mdf_espnow_debug_pkt_t));
    espnow_pkt->type = MDF_ESPNOW_DEBUG_COREDUMP;
    espnow_pkt->oprt = MDF_ESPNOW_COREDUMP_DETECT;

    send_ret = espnow_send_to_all(espnow_pkt, sizeof(mdf_espnow_debug_pkt_t), true);
    mdf_free(espnow_pkt);
    MDF_ERROR_CHECK(!send_ret, 1, "not all devices send ok");

    return 0;
}
static void register_dumpreq()
{
    const esp_console_cmd_t dumpreq_cmd = {
        .command = "dumpreq",
        .help = "espnow coredump request",
        .hint = NULL,
        .func = &dumpreq,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&dumpreq_cmd) );
}

static int dumperase(int argc, char** argv)
{
    bool send_ret = true;
    mdf_espnow_debug_pkt_t *espnow_pkt = NULL;

    espnow_pkt = mdf_calloc(1, sizeof(mdf_espnow_debug_pkt_t));
    espnow_pkt->type = MDF_ESPNOW_DEBUG_COREDUMP;
    espnow_pkt->oprt = MDF_ESPNOW_COREDUMP_ERASE;
    send_ret = espnow_send_to_all(espnow_pkt, sizeof(mdf_espnow_debug_pkt_t), true);
    mdf_free(espnow_pkt);
    MDF_ERROR_CHECK(!send_ret, 1, "not all devices send ok");

    return 0;
}
static void register_dumperase()
{
    const esp_console_cmd_t dumperase_cmd = {
        .command = "dumperase",
        .help = "espnow coredump erase",
        .hint = NULL,
        .func = &dumperase,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&dumperase_cmd) );
}

static int join(int argc, char **argv)
{
    wifi_config_t wifi_config = { 0 };

    int nerrors = arg_parse(argc, argv, (void**) &join_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, join_args.end, argv[0]);
        return 1;
    }
    if (join_args.ssid->sval[0] != NULL) {
        MDF_LOGI("wifi sta connect to %s", join_args.ssid->sval[0]);
        strncpy((char*) wifi_config.sta.ssid,
                join_args.ssid->sval[0], sizeof(wifi_config.sta.ssid));
        if (join_args.password->sval[0]) {
            strncpy((char*) wifi_config.sta.password,
                    join_args.password->sval[0], sizeof(wifi_config.sta.password));
        }
        ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
        ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
        ESP_ERROR_CHECK( esp_wifi_disconnect() );
        ESP_ERROR_CHECK( esp_wifi_connect() );

        memcpy(g_wifi_connect.ssid, wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid));
        memcpy(g_wifi_connect.password, wifi_config.sta.password, sizeof(wifi_config.sta.password));

        esp_err_t ret = mdf_info_save("wifi_connect", &g_wifi_connect, sizeof(wifi_connect_t));
        MDF_ERROR_CHECK(ret < 0, 1, "mdf_info_save wifi_connect ret: %d", ret);
    }
    return 0;
}

static int channel(int argc, char **argv)
{
    uint8_t primary_channel;
    wifi_second_chan_t secondary_channel;

    int nerrors = arg_parse(argc, argv, (void**) &channel_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, channel_args.end, argv[0]);
        return 1;
    }
    ESP_ERROR_CHECK( esp_wifi_get_channel(&primary_channel, &secondary_channel) );
    if (channel_args.primary_channel->count > 0) {
        primary_channel = channel_args.primary_channel->ival[0];
    }
    if (channel_args.secondary_channel->count > 0) {
        if (memcmp(channel_args.secondary_channel->sval[0], "none", 4) == 0) {
            secondary_channel = WIFI_SECOND_CHAN_NONE;
        } else if (memcmp(channel_args.secondary_channel->sval[0], "above", 5) == 0) {
            secondary_channel = WIFI_SECOND_CHAN_ABOVE;
        } else if (memcmp(channel_args.secondary_channel->sval[0], "below", 5) == 0) {
            secondary_channel = WIFI_SECOND_CHAN_BELOW;
        } else {
            MDF_LOGE("incorrect secondary channel: %s",
                     channel_args.secondary_channel->sval[0]);
        }
    }
    MDF_LOGI("wifi sta set channel");
    esp_wifi_set_channel(primary_channel, secondary_channel);
    return 0;
}

static void register_wifi(void)
{
    join_args.ssid = arg_str1(NULL, NULL, "<s>", "ssid to connect");
    join_args.password = arg_str0(NULL, NULL, "<s>", "AP password");
    join_args.end = arg_end(5);

    const esp_console_cmd_t join_cmd = {
        .command = "join",
        .help = "wifi sta join ap",
        .hint = NULL,
        .func = &join,
        .argtable = &join_args,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&join_cmd) );

    channel_args.primary_channel = arg_int1(NULL, NULL, "<i>", "primary channel");
    channel_args.secondary_channel = arg_str1(NULL, NULL, "<s>",
                                     "secondary channel: \"none\" or \"above\" or \"below\"");
    channel_args.end = arg_end(5);

    const esp_console_cmd_t channel_cmd = {
        .command = "channel",
        .help = "wifi sta set channel",
        .hint = NULL,
        .func = &channel,
        .argtable = &channel_args,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&channel_cmd) );
}

static int ls_file(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void**) &ls_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, ls_args.end, argv[0]);
        return 1;
    }

    MDF_LOGI("list file type: %s", ls_args.type->sval[0]);
    if (!ls_args.type->sval[0][0] || !strcmp("all", ls_args.type->sval[0])){
        espnow_sd_ls("ALL");
    } else if (!strcmp("log", ls_args.type->sval[0])){
        espnow_sd_ls("LOG");
    } else if (!strcmp("dmp", ls_args.type->sval[0])){
        espnow_sd_ls("DMP");
    } else {
        MDF_LOGW("unsupported file type: %s", ls_args.type->sval[0]);
        return 1;
    }
    return 0;
}

static int rm_file(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void**) &rm_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, rm_args.end, argv[0]);
        return 1;
    }
    if(rm_args.type->sval[0] == NULL){
        return 1;
    }
    MDF_LOGI("file: %s", rm_args.type->sval[0]);
    espnow_sd_rm(rm_args.type->sval[0]);
    return 0;
}

static int read_file(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void**) &read_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, read_args.end, argv[0]);
        return 1;
    }
    if (read_args.type->sval[0] == NULL) {
        return 1;
    }
    MDF_LOGI("read file: %s", read_args.type->sval[0]);
    espnow_sd_read(read_args.type->sval[0]);
    return 0;
}

static void file_oprt()
{
    ls_args.type = arg_str0(NULL, NULL, "<type>", "ls file");
    ls_args.end = arg_end(1);
    const esp_console_cmd_t ls_cmd = {
        .command = "ls",
        .help = "list files with given file type([all], log, dump)",
        .hint = NULL,
        .func = &ls_file,
        .argtable = &ls_args,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&ls_cmd) );

    rm_args.type = arg_str1(NULL, NULL, "<type>", "rm file");
    rm_args.end = arg_end(1);
    const esp_console_cmd_t rm_cmd = {
        .command = "rm",
        .help = "remove files with given file type or name",
        .hint = NULL,
        .func = &rm_file,
        .argtable = &rm_args,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&rm_cmd) );

    read_args.type = arg_str1(NULL, NULL, "<type>", "read file");
    read_args.end = arg_end(1);
    const esp_console_cmd_t read_cmd = {
        .command = "read",
        .help = "read files with given file name(curedump only)",
        .hint = NULL,
        .func = &read_file,
        .argtable = &read_args,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&read_cmd) );
}

/* 'manual' command control device */
static int manual(int argc, char** argv)
{
    int nerrors   = 0;
    bool send_ret = true;
    mdf_espnow_debug_pkt_t *espnow_pkt = NULL;

    nerrors = arg_parse(argc, argv, (void**) &manual_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, manual_args.end, argv[0]);
        return 1;
    }

    if (manual_args.type->count == 0) {
        MDF_LOGE("manual no parameter");
        return 1;
    }

    espnow_pkt = mdf_calloc(1, sizeof(mdf_espnow_debug_pkt_t));
    espnow_pkt->type = MDF_ESPNOW_DEBUG_MANUAL;
    espnow_pkt->oprt = manual_args.type->ival[0];
    MDF_LOGI("espnow manual control: %d", espnow_pkt->oprt);

    send_ret = espnow_send_to_all(espnow_pkt, sizeof(mdf_espnow_debug_pkt_t), true);
    mdf_free(espnow_pkt);
    MDF_ERROR_CHECK(!send_ret, 1, "not all devices send ok");

    return 0;
}
static void register_manual()
{
    manual_args.type = arg_int0("t", "type", "<type>", "espnow manual");
    manual_args.end = arg_end(1);

    const esp_console_cmd_t manual_cmd = {
        .command = "manual",
        .help = "espnow manual control device: reboot, reset, config [0,1,2]",
        .hint = NULL,
        .func = &manual,
        .argtable = &manual_args,
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&manual_cmd) );
}

static int erase_nvs(int argc, char **argv)
{
    int nerrors = 0;
    bool send_ret = true;
    mdf_espnow_debug_pkt_t *espnow_pkt = NULL;

    nerrors = arg_parse(argc, argv, (void**) &erase_nvs_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, erase_nvs_args.end, argv[0]);
        return 1;
    }

    if (erase_nvs_args.key->sval[0] == NULL) {
        MDF_LOGE("erase nvs no key");
        return 1;
    }

    MDF_LOGI("erase_nvs key: %s", erase_nvs_args.key->sval[0]);

    espnow_pkt = mdf_calloc(1, sizeof(mdf_espnow_debug_pkt_t) + strlen(erase_nvs_args.key->sval[0]));
    espnow_pkt->type = MDF_ESPNOW_DEBUG_MANUAL;
    espnow_pkt->oprt = MDF_ESPNOW_MANUAL_ERASE;
    espnow_pkt->size = strlen(erase_nvs_args.key->sval[0]);
    memcpy(espnow_pkt->data, erase_nvs_args.key->sval[0], espnow_pkt->size);
    send_ret = espnow_send_to_all(espnow_pkt, (sizeof(mdf_espnow_debug_pkt_t) + espnow_pkt->size), true);
    mdf_free(espnow_pkt);
    MDF_ERROR_CHECK(!send_ret, 1, "not all devices send ok");

    return 0;
}

static void register_erase_nvs()
{
    erase_nvs_args.key = arg_str1("k", "key", "<key>", "nvs key to erase");
    erase_nvs_args.end = arg_end(1);

    const esp_console_cmd_t erase_nvs_cmd = {
        .command = "erase_nvs",
        .help = "erase nvs by given key",
        .hint = NULL,
        .func = &erase_nvs,
        .argtable = &erase_nvs_args,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&erase_nvs_cmd) );
}
