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

#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "mbedtls/base64.h"

#include "mdf_common.h"
#include "mdebug.h"
#include "mupgrade.h"

#define MDEBUG_LOG_MAX_SIZE (MESPNOW_PAYLOAD_LEN * 2 - 2) /**< Set log length size */

static const char *TAG = "mdebug_cmd";

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

/**
 * @brief  A function which implements version command.
 */
static int version_func(int argc, char **argv)
{
    esp_chip_info_t chip_info = {0};

    /**< Pint system information */
    esp_chip_info(&chip_info);
    MDF_LOGI("ESP-IDF version  : %s", esp_get_idf_version());
    MDF_LOGI("ESP-MDF version  : %s", mdf_get_version());
    MDF_LOGI("compile time     : %s %s", __DATE__, __TIME__);
    MDF_LOGI("free heap        : %d Bytes", esp_get_free_heap_size());
    MDF_LOGI("CPU cores        : %d", chip_info.cores);
    MDF_LOGI("silicon revision : %d", chip_info.revision);
    MDF_LOGI("feature          : %s%s%s%s%d%s",
             chip_info.features & CHIP_FEATURE_WIFI_BGN ? "/802.11bgn" : "",
             chip_info.features & CHIP_FEATURE_BLE ? "/BLE" : "",
             chip_info.features & CHIP_FEATURE_BT ? "/BT" : "",
             chip_info.features & CHIP_FEATURE_EMB_FLASH ? "/Embedded-Flash:" : "/External-Flash:",
             spi_flash_get_chip_size() / (1024 * 1024), " MB");

    return ESP_OK;
}

/**
 * @brief  Register version command.
 */
static void register_version()
{
    const esp_console_cmd_t cmd = {
        .command = "version",
        .help = "Get version of chip and SDK",
        .hint = NULL,
        .func = &version_func,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static struct {
    struct arg_str *tag;
    struct arg_str *level;
    struct arg_str *send;
    struct arg_str *enable_type;
    struct arg_str *disable_type;
    struct arg_lit *output_type;
    struct arg_lit *read;
    struct arg_end *end;
} log_args;

/**
 * @brief  A function which implements log command.
 */
static int log_func(int argc, char **argv)
{
    mdf_err_t ret = MDF_OK;
    mdebug_log_config_t log_config = {0};
    const char *level_str[6] = {"NONE", "ERR", "WARN", "INFO", "DEBUG", "VER"};

    mdebug_log_get_config(&log_config);

    if (arg_parse(argc, argv, (void **)&log_args) != ESP_OK) {
        arg_print_errors(stderr, log_args.end, argv[0]);
        return MDF_FAIL;
    }

    for (int i = 0; log_args.level->count && i < sizeof(level_str) / sizeof(char *); ++i) {
        if (!strcasecmp(level_str[i], log_args.level->sval[0])) {
            const char *tag = log_args.tag->count ? log_args.tag->sval[0] : "*";
            esp_log_level_set(tag, i);
        }
    }

    if (log_args.send->count) {
        ret = mac_str2hex(log_args.send->sval[0], log_config.dest_addr);
        MDF_ERROR_CHECK(ret == false, ESP_ERR_INVALID_ARG,
                        "The format of the address is incorrect. Please enter the format as xx:xx:xx:xx:xx:xx");
    }

    if (log_args.enable_type->count) { /**< Enable write to flash memory */
        if (!strcasecmp(log_args.enable_type->sval[0], "flash")) {
            log_config.log_flash_enable = true;
        } else if (!strcasecmp(log_args.enable_type->sval[0], "uart")) {
            log_config.log_uart_enable = true;
        } else if (!strcasecmp(log_args.enable_type->sval[0], "espnow")) {
            log_config.log_espnow_enable = true;
        }
    }

    if (log_args.disable_type->count) { /**< Disable write to flash memory */
        if (!strcasecmp(log_args.enable_type->sval[0], "flash")) {
            log_config.log_flash_enable = false;
        } else if (!strcasecmp(log_args.enable_type->sval[0], "uart")) {
            log_config.log_uart_enable = false;
        } else if (!strcasecmp(log_args.enable_type->sval[0], "espnow")) {
            log_config.log_espnow_enable = false;
        }
    }

    if (log_args.output_type->count) { /**< Output enable type */
        MDF_LOGI("Enable type: %s%s%s", log_config.log_uart_enable ? "uart" : "",
                 log_config.log_flash_enable ? "/flash" : "", log_config.log_espnow_enable ? "/espuart" : "");
    }

    if (log_args.read->count) {  /**< read to the flash of log data */
        int log_size   = mdebug_flash_size();
        char *log_data = MDF_MALLOC(MDEBUG_LOG_MAX_SIZE - 17);

        MDF_LOGI("The flash partition that stores the log size: %d", log_size);

        if (log_config.log_flash_enable) {
            for (size_t size = MIN(MDEBUG_LOG_MAX_SIZE - 17, log_size);
                    size > 0 && mdebug_flash_read(log_data, &size) == MDF_OK;
                    log_size -= size, size = MIN(MDEBUG_LOG_MAX_SIZE - 17, log_size)) {
                MDF_LOGI("mdebug_log_data: %.*s", size, log_data);
            }
        }

        MDF_FREE(log_data);
    }

    mdebug_log_set_config(&log_config);

    return MDF_OK;
}

/**
 * @brief  Register log command.
 */
static void register_log()
{
    log_args.tag         = arg_str0("t", "tag", "<tag>", "Tag of the log entries to enable, '*' resets log level for all tags to the given value");
    log_args.level       = arg_str0("l", "level", "<level>", "Selects log level to enable (NONE, ERR, WARN, INFO, DEBUG, VER)");
    log_args.enable_type = arg_str0("e", "enable_type", "<enable_type('uart' or 'flash' or 'espnow')>", "Selects mdebug log to enable (uart,flash,espnow)");
    log_args.disable_type = arg_str0("d", "disable_type", "<disable_type('uart' or 'flash' or 'espnow')>", "Selects mdebug log to disable (uart,flash,espnow)");
    log_args.output_type  = arg_lit0("o", "output_type", "Output enable type");
    log_args.read        = arg_lit0("r", "read", "Read to the flash of mdebug log information");
    log_args.send        = arg_str0("s", "send", "<addr (xx:xx:xx:xx:xx:xx)>", "Configure the address of the ESP-NOW log receiver");
    log_args.end         = arg_end(8);

    const esp_console_cmd_t cmd = {
        .command = "log",
        .help = "Set log level for given tag",
        .hint = NULL,
        .func = &log_func,
        .argtable = &log_args,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/**
 * @brief  A function which implements restart command.
 */
static int restart_func(int argc, char **argv)
{
    ESP_LOGI(TAG, "Restarting");
    esp_restart();
}

/**
 * @brief  Register restart command.
 */
static void register_restart()
{
    const esp_console_cmd_t cmd = {
        .command = "restart",
        .help    = "Software reset of the chip",
        .hint    = NULL,
        .func    = &restart_func,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/**
 * @brief  A function which implements reset command.
 */
static int reset_func(int argc, char **argv)
{
    mdf_err_t ret = MDF_OK;
    esp_partition_iterator_t part_itra = NULL;
    const esp_partition_t *nvs_part = NULL;

    MDF_LOGI("Erase part of the nvs partition");

    part_itra = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "nvs");
    MDF_ERROR_CHECK(!part_itra, MDF_ERR_NOT_SUPPORTED, "partition no find, subtype: 0x%x, label: %s",
                    ESP_PARTITION_SUBTYPE_ANY, "nvs");

    nvs_part = esp_partition_get(part_itra);
    MDF_ERROR_CHECK(!nvs_part, MDF_ERR_NOT_SUPPORTED, "esp_partition_get");

    ret = esp_partition_erase_range(nvs_part, 0, nvs_part->size);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "Erase part of the nvs partition");

    ESP_LOGI(TAG, "Restarting");
    esp_restart();
}

/**
 * @brief  Register reset command.
 */
static void register_reset()
{
    const esp_console_cmd_t cmd = {
        .command = "reset",
        .help = "Clear device configuration information",
        .hint = NULL,
        .func = &reset_func,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/**
 * @brief  A function which implements reset command.
 */
static int fallback_func(int argc, char **argv)
{
    return mupgrade_version_fallback();
}

/**
 * @brief  Register fallback command.
 */
static void register_fallback()
{
    const esp_console_cmd_t cmd = {
        .command = "fallback",
        .help = "Upgrade error back to previous version",
        .hint = NULL,
        .func = &fallback_func,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/**
 * @brief  A function which implements heap command.
 */
static int heap_func(int argc, char **argv)
{
    mdf_mem_print_record();
    mdf_mem_print_heap();
    mdf_mem_print_task();

    if (!heap_caps_check_integrity_all(true)) {
        MDF_LOGE("At least one heap is corrupt");
    }

    return MDF_OK;
}

/**
 * @brief  Register heap command.
 */
static void register_heap()
{
    const esp_console_cmd_t cmd = {
        .command = "heap",
        .help = "Get the current size of free heap memory",
        .hint = NULL,
        .func = &heap_func,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static struct {
    struct arg_lit *length;
    struct arg_str *send_length;
    struct arg_lit *output;
    struct arg_lit *erase;
    struct arg_int *seq;
    struct arg_str *send;
    struct arg_end *end;
} coredump_args;

/**
 * @brief  A function which implements coredump command.
 */
static int coredump_func(int argc, char **argv)
{
    mdf_err_t ret        = MDF_OK;
    ssize_t coredump_size = 0;
    const esp_partition_t *coredump_part = NULL;

    if (arg_parse(argc, argv, (void **)&coredump_args) != ESP_OK) {
        arg_print_errors(stderr, coredump_args.end, argv[0]);
        return MDF_FAIL;
    }

    coredump_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                    ESP_PARTITION_SUBTYPE_DATA_COREDUMP, NULL);
    MDF_ERROR_CHECK(coredump_part == NULL, MDF_ERR_NOT_SUPPORTED, "No core dump partition found!");

    ret = esp_partition_read(coredump_part, 4, &coredump_size, sizeof(size_t));
    MDF_ERROR_CHECK(coredump_part == NULL, MDF_ERR_NOT_SUPPORTED, "Core dump read length!");

    if (coredump_args.length->count) {
        MDF_LOGI("Core dump is length: %d Bytes", coredump_size);
    }

    if (coredump_args.send_length->count) {
        uint8_t dest_addr[6] = {0x0};
        MDF_LOGI("Core dump is length: %d Bytes", coredump_size);

        if (mac_str2hex(coredump_args.send_length->sval[0], dest_addr)) {
            char *log_str = NULL;
            asprintf(&log_str, "Core dump is length: %d Bytes", coredump_size);
            mdebug_espnow_write(dest_addr, log_str, strlen(log_str), MDEBUG_ESPNOW_LOG, portMAX_DELAY);
            MDF_FREE(log_str);
        }
    }

    if (coredump_args.output->count && coredump_size > 0) {
#define COREDUMP_BUFFER_SIZE 1024
        uint8_t *buffer = MDF_REALLOC_RETRY(NULL, COREDUMP_BUFFER_SIZE);
        MDF_LOGI("\n================= CORE DUMP START =================\n");

        for (int offset = 4; offset < coredump_size; offset += COREDUMP_BUFFER_SIZE) {
            size_t size = MIN(COREDUMP_BUFFER_SIZE, coredump_size - offset);
            esp_partition_read(coredump_part, offset, buffer, size);
            size_t dlen = (size + 2) / 3 * 4; //base64 encode maximum length = ⌈ n / 3 ⌉ * 4
            size_t olen = 0;
            uint8_t *b64_buf = MDF_MALLOC(dlen);
            mbedtls_base64_encode(b64_buf, dlen, &olen, buffer, size);
            printf("%s", b64_buf);
            MDF_FREE(b64_buf);
        }

        MDF_LOGI("================= CORE DUMP END ===================\n");

        MDF_LOGI("1. Save core dump text body to some file manually");
        MDF_LOGI("2. Run the following command: \n"
                 "python $MDF_PATH/esp-idf/components/espcoredump/espcoredump.py info_corefile -t b64 -c </path/to/saved/base64/text> </path/to/program/elf/file>");
        MDF_FREE(buffer);
    }

    if (coredump_args.send->count) {
        uint8_t dest_addr[6] = {0x0};
        mdebug_coredump_packet_t *packet = NULL;

        ret = mac_str2hex(coredump_args.send->sval[0], dest_addr);
        MDF_ERROR_CHECK(ret == false, ESP_ERR_INVALID_ARG,
                        "The format of the address is incorrect. Please enter the format as xx:xx:xx:xx:xx:xx");

        packet = MDF_REALLOC_RETRY(NULL, sizeof(mdebug_coredump_packet_t));

        if (coredump_args.seq->count) {
            packet->seq = coredump_args.seq->ival[0];
        }

        if (packet->seq == 0) {
            packet->type = MDEBUG_COREDUMP_BEGIN;
            packet->size = coredump_size;
            ret = mdebug_espnow_write(dest_addr, packet, 4, MDEBUG_ESPNOW_COREDUMP, portMAX_DELAY);
            MDF_ERROR_CHECK(ret != MDF_OK, ret, "mdebug_espnow_write, seq: %d", packet->seq);
        }

        packet->type = MDEBUG_COREDUMP_DATA;

        for (; packet->seq * sizeof(packet->data) < coredump_size; packet->seq++) {
            packet->size = MIN(coredump_size - packet->seq * sizeof(packet->data), sizeof(packet->data));

            esp_partition_read(coredump_part, 4 + packet->seq * sizeof(packet->data),
                               packet->data, sizeof(packet->data));

            for (int i = 0; i < 5; ++i) {
                ret = mdebug_espnow_write(dest_addr, packet, sizeof(mdebug_coredump_packet_t),
                                          MDEBUG_ESPNOW_COREDUMP, portMAX_DELAY);

                if (ret == MDF_OK) {
                    break;
                }

                vTaskDelay(100 / portTICK_RATE_MS);
            }

            MDF_ERROR_BREAK(ret != MDF_OK, "mdebug_espnow_write, seq: %d", packet->seq);

            /**
             * @brief TODO Since espnow is now an unreliable transmission,
             *             sending too fast will result in packet loss.
             */
            vTaskDelay(20 / portTICK_RATE_MS);
        }

        packet->type = MDEBUG_COREDUMP_END;
        packet->seq  = coredump_size / sizeof(packet->data) + (coredump_size % sizeof(packet->data) ? 1 : 0);
        packet->size = coredump_size;
        ret = mdebug_espnow_write(dest_addr, packet, 4, MDEBUG_ESPNOW_COREDUMP, portMAX_DELAY);
        MDF_ERROR_CHECK(ret != MDF_OK, ret, "mdebug_espnow_write, seq: %d", packet->seq);

        MDF_FREE(packet);
    }

    if (coredump_args.erase->count) {
        ret = esp_partition_erase_range(coredump_part, 0, coredump_part->size);
        MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "Core dump erase fail");
        MDF_LOGI("Core dump erase successful");
    }

    return MDF_OK;
}

/**
 * @brief  Register coredump command.
 */
static void register_coredump()
{
    coredump_args.length = arg_lit0("l", "length", "Get coredump data length");
    coredump_args.send_length = arg_str0(NULL, "sendlength", "Send coredump data length", "Configure the address of the coredump data length receiver");
    coredump_args.output = arg_lit0("o", "output", "Read the coredump data of the device");
    coredump_args.erase  = arg_lit0("e", "erase", "Erase the coredump data of the device");
    coredump_args.seq    = arg_int0("q", "sequence", "<seq>", "Sequence");
    coredump_args.send   = arg_str0("s", "send", "<addr (xx:xx:xx:xx:xx:xx)>", "Configure the address of the ESP-NOW log receiver");
    coredump_args.end    = arg_end(5);

    const esp_console_cmd_t cmd = {
        .command = "coredump",
        .help = "Get core dump information",
        .hint = NULL,
        .func = &coredump_func,
        .argtable = &coredump_args,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void mdebug_cmd_register_common()
{
    register_version();
    register_heap();
    register_restart();
    register_reset();
    register_fallback();
    register_log();
    register_coredump();
}
