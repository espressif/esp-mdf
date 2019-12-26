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

#include "mdf_common.h"
#include "mwifi.h"
#include "mespnow.h"

/*Include desired font here*/
#define PROGMEM
#include "mdebug_espnow.h"
#include "mdebug_console.h"
#include "mdebug_espnow.h"

#define MDEBUG_LOG_MAX_SIZE 1460

typedef struct log_record_ {
    size_t total;
    size_t info;
    size_t warning;
    size_t error;
    size_t restart;
    size_t coredump;
    uint32_t timestamp;
    uint8_t mac[6];
    SLIST_ENTRY(log_record_) next;    //!< next command in the list
} log_record_t;

static const char *TAG = "debug_recv";

static SLIST_HEAD(log_record_list_, log_record_) s_log_record_list;

extern "C" {
    unsigned char *base64_encode(const unsigned char *src, size_t len,
                                 size_t *out_len);
}

static esp_err_t log_analysis(const uint8_t *mac, const uint8_t *data)
{
    uint32_t timestamp     = 0;
    log_record_t *log_record = NULL;

    SLIST_FOREACH(log_record, &s_log_record_list, next) {
        if (!memcmp(log_record->mac, mac, 6)) {
            break;
        }
    }

    if (!log_record) {
        log_record = (log_record_t *)calloc(1, sizeof(log_record_t));
        memcpy(log_record->mac, mac, 6);
        SLIST_INSERT_HEAD(&s_log_record_list, log_record, next);
    }

    /**< get log tag */
    uint8_t log_tag_index = (data[0] == 0x1b) ? 7 : 0;

    switch (data[log_tag_index]) {
        case 'I':
            log_record->info++;
            break;

        case 'W':
            log_record->warning++;
            break;

        case 'E':
            log_record->error++;
            break;

        default:
            break;
    }

    log_record->total++;

    /**< run time tag */
    if (sscanf((char *)data, "%*[^(](%d", &timestamp)) {
        if (timestamp  + 1000 < log_record->timestamp) {
            log_record->restart++;
        }

        log_record->timestamp = timestamp;
    }

    return ESP_OK;
}

void print_system_info_timercb(void *timer)
{
    char *buffer = NULL;
    int count = 0;
    log_record_t *log_record = NULL;

    asprintf(&buffer, "   ADDR     DMP  RES  ERR  WARN  INFO");
    free(buffer);

    SLIST_FOREACH(log_record, &s_log_record_list, next) {
        asprintf(&buffer, "[%02x:%02x:%02x] %3d  %3d  %3d  %3d   %4d",
                 log_record->mac[3], log_record->mac[4], log_record->mac[5], log_record->coredump,
                 log_record->restart, log_record->error, log_record->warning, log_record->info);
        free(buffer);
        count++;
    }
}

static void espnow_recv_task(void *arg)
{
    uint8_t src_addr[6]  = { 0 };
    uint8_t *recv_data   = (uint8_t *)malloc(MDEBUG_LOG_MAX_SIZE);
    size_t recv_size     = MDEBUG_LOG_MAX_SIZE;
    mdebug_espnow_t type = MDEBUG_ESPNOW_COREDUMP;

    for (;;) {
        recv_size = MDEBUG_LOG_MAX_SIZE;
        memset(recv_data, 0, MDEBUG_LOG_MAX_SIZE);

        /**
         * @brief read data from controller
         */
        if (mdebug_espnow_read(src_addr, recv_data, &recv_size, &type, portMAX_DELAY) == MDF_OK) {
            switch (type) {
                case MDEBUG_ESPNOW_COREDUMP: {
                    static int expect_seq = 0;
                    static uint8_t expect_addr[6] = {0};
                    static size_t written_size = 0;

                    mdebug_coredump_packet_t *packet = (mdebug_coredump_packet_t *)recv_data;
                    MDF_ERROR_BREAK(packet->size <= 0, "Core dump length: %d Bytes", packet->size);
                    MDF_LOGV("Core dump, %d, length: %d, seq: %d", packet->type, packet->size, packet->seq);

                    switch (packet->type) {
                        case MDEBUG_COREDUMP_BEGIN: {
                            MDF_LOGI("Core dump recv being");
                            expect_seq = 0;
                            written_size = 0;
                            memcpy(expect_addr, src_addr, 6);

                            const esp_partition_t *coredump_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                                                    ESP_PARTITION_SUBTYPE_DATA_COREDUMP, NULL);
                            esp_partition_erase_range(coredump_part, 0, coredump_part->size);

                            break;
                        }

                        case MDEBUG_COREDUMP_DATA: {
                            if (memcmp(expect_addr, src_addr, 6) || packet->seq != expect_seq) {
                                break;
                            }

                            MDF_LOGI("Core dump recving, packet->seq: %d", packet->seq);
                            expect_seq++;

                            uint8_t *b64_buf = base64_encode(packet->data, packet->size, NULL);

                            const esp_partition_t *coredump_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                                                    ESP_PARTITION_SUBTYPE_DATA_COREDUMP, NULL);
                            esp_partition_write(coredump_part, written_size, b64_buf, strlen((char *)b64_buf));

                            written_size += strlen((char *)b64_buf);
                            free(b64_buf);
                            break;
                        }

                        case MDEBUG_COREDUMP_END: {
                            if (memcmp(expect_addr, src_addr, 6) || packet->seq != expect_seq) {
                                break;
                            }

                            char file_name[32] = {0x0};


                            MDF_LOGI("================= CORE DUMP START =================");

                            for (int offset = 0; offset < written_size; offset += recv_size) {
                                recv_size = MIN(MDEBUG_LOG_MAX_SIZE, written_size - offset);
                                const esp_partition_t *coredump_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                                                        ESP_PARTITION_SUBTYPE_DATA_COREDUMP, NULL);
                                esp_partition_read(coredump_part, offset, recv_data, recv_size);

                                printf("%.*s", recv_size, recv_data);
                            }

                            MDF_LOGI("================= CORE DUMP END ===================");
                            MDF_LOGI("1. Save core dump text body to some file manually");
                            MDF_LOGI("2. Run the following command: python $MDF_PATH/esp-idf/components/espcoredump/espcoredump.py "
                                     "info_corefile -t b64 -c </path/to/saved/base64/text> </path/to/program/elf/file>");

                            log_record_t *log_record = NULL;

                            SLIST_FOREACH(log_record, &s_log_record_list, next) {
                                if (!memcmp(log_record->mac, expect_addr, 6)) {
                                    log_record->coredump++;
                                    break;
                                }
                            }

                            break;
                        }

                        default:
                            break;
                    }

                    break;
                }

                case MDEBUG_ESPNOW_LOG: {
                    printf("[" MACSTR "] %.*s\n", MAC2STR(src_addr), recv_size, recv_data);
                    log_analysis(src_addr, recv_data);

                    break;
                }

                default:
                    break;
            }
        }
    }

    free(recv_data);
    vTaskDelete(NULL);
}

extern "C" void log_record_print(uint8_t *addr)
{
    log_record_t *log_record = NULL;

    SLIST_FOREACH(log_record, &s_log_record_list, next) {
        if (MWIFI_ADDR_IS_ANY(addr) || !memcmp(addr, log_record->mac, 6)) {
            MDF_LOGI("mac: " MACSTR ", total: %d, info: %d, warning: %d, error: %d, restart: %d, coredump: %d, timestamp: %d",
                     MAC2STR(log_record->mac), log_record->total, log_record->info, log_record->warning,
                     log_record->error, log_record->restart, log_record->coredump, log_record->timestamp);
        }
    }
}

extern "C" mdf_err_t debug_recv_init()
{
    xTaskCreatePinnedToCore(espnow_recv_task, "mdebug_espnow_recv", 3 * 1024,
                            NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY,
                            NULL, CONFIG_MDF_TASK_PINNED_TO_CORE);

    TimerHandle_t timer = xTimerCreate("print_system_info", 1000 / portTICK_RATE_MS,
                                       true, NULL, print_system_info_timercb);
    xTimerStart(timer, 0);

    return MDF_OK;
}
