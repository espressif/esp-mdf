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
#include "mlink.h"
#include "mupgrade.h"

#include "mespnow.h"
#include "mconfig_blufi.h"
#include "mconfig_chain.h"
#include "mdebug_console.h"
#include "mdebug_espnow.h"

#include "mesh_utils.h"

#include "app_peripheral.h"
#include "buddy_cmd.h"

#define BUDDY_TID                     (1)
#define BUDDY_NAME                    "buddy"
#define BUDDY_RESTART_COUNT_FALLBACK  (10)
#define BUDDY_RESTART_COUNT_RESET     (3)

static const char *TAG  = "buddy_example";
esp_netif_t *sta_netif;

typedef struct {
    bool finish;
    uint16_t packet_len;
    uint16_t transmit_time;
    uint32_t ping_count;
    uint16_t report_interval;
    uint8_t addr[6];
} mesh_iperf_cfg;

typedef struct mac_list_ {
    uint8_t mac_addr[MWIFI_ADDR_LEN];
    uint32_t recv_count;
    uint32_t report_ticks;
    uint32_t report_count;
    TickType_t start_ticks;
    SLIST_ENTRY(mac_list_) next;    //!< next command in the list
} mac_list_t;

oled_context_t oled_context = {0};

mesh_iperf_cfg g_mesh_iperf_cfg;

// system
static QueueHandle_t oled_queue = NULL;

static TaskHandle_t iperf_server_task = NULL;
static TaskHandle_t iperf_client_task = NULL;

static void mesh_iperf_server_task(void *arg)
{
    g_mesh_iperf_cfg.packet_len      = MWIFI_PAYLOAD_LEN;
    g_mesh_iperf_cfg.transmit_time   = 60;
    g_mesh_iperf_cfg.report_interval = 3;
    g_mesh_iperf_cfg.ping_count      = 64;

    oled_context.iperf_running = true;

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

    oled_context.iperf_running = false;

    iperf_server_task = NULL;
    vTaskDelete(NULL);
}

static void mesh_iperf_client_task(void *arg)
{
    g_mesh_iperf_cfg.packet_len      = MWIFI_PAYLOAD_LEN;
    g_mesh_iperf_cfg.transmit_time   = 60;
    g_mesh_iperf_cfg.report_interval = 3;
    g_mesh_iperf_cfg.ping_count      = 64;

    oled_context.iperf_running = true;

    mdf_err_t ret   = MDF_OK;
    uint8_t *buffer = MDF_MALLOC(g_mesh_iperf_cfg.packet_len);

    mwifi_data_type_t data_type = {
        .protocol = IPERF_BANDWIDTH,
    };

    TickType_t start_ticks = xTaskGetTickCount();
    TickType_t end_ticks   = start_ticks + g_mesh_iperf_cfg.transmit_time * 1000 * portTICK_RATE_MS;
    uint32_t total_count   = 0;

    MDF_LOGI("[    Server MAC   ] Interval       Transfer     Bandwidth");

    uint8_t root_addr[]    = MWIFI_ADDR_ROOT;
    memcpy(g_mesh_iperf_cfg.addr, root_addr, 6);
    MDF_LOGI(" send to " MACSTR, MAC2STR(g_mesh_iperf_cfg.addr));

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
                    
            oled_context.iperf_result = report_size * 8 / g_mesh_iperf_cfg.report_interval;

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

    oled_context.iperf_result = total_len * 8 * 1000 / spend_time;

    MDF_FREE(buffer);
    g_mesh_iperf_cfg.finish = true;
    oled_context.iperf_running = false;

    iperf_client_task = NULL;
    vTaskDelete(NULL);
}

void oled_next_page()
{
    oled_evt_t evt;
    evt.type = OLED_EVT_PAGE_NEXT;
    xQueueSend(oled_queue, &evt, portMAX_DELAY);
}

void oled_prev_page()
{
    oled_evt_t evt;
    evt.type = OLED_EVT_PAGE_PREV;
    xQueueSend(oled_queue, &evt, portMAX_DELAY);
}

static void oled_task(void* arg)
{
    portBASE_TYPE ret;
    oled_evt_t evt;

    oled_context.current_page_num = PAGE_STATUS;
    oled_show_page(oled_context.current_page_num, &oled_context);

    if (oled_queue == NULL) {
        oled_queue = xQueueCreate(10, sizeof(oled_evt_t));
    }
    
    while (1) {
        ret = xQueueReceive(oled_queue, &evt, 500 / portTICK_PERIOD_MS);
        if (ret == pdTRUE) {
            if (evt.type == OLED_EVT_PAGE_NEXT) {
                oled_context.current_page_num += (PAGE_MAX + 1);
                oled_context.current_page_num %= PAGE_MAX;
            } else if (evt.type == OLED_EVT_PAGE_PREV) {
                oled_context.current_page_num += (PAGE_MAX - 1);
                oled_context.current_page_num %= PAGE_MAX;
            }
            oled_show_page(oled_context.current_page_num, &oled_context);
        } else {
            oled_show_page(oled_context.current_page_num, &oled_context);
        }
    }
}

/**
 * @brief All module events will be sent to this task in esp-mdf
 *
 * @Note:
 *     1. Do not block or lengthy operations in the callback function.
 *     2. Do not consume a lot of memory in the callback function.
 *        The task memory of the callback function is only 4KB.
 */
static mdf_err_t event_loop_cb(mdf_event_loop_t event, void *ctx)
{
    MDF_LOGI("event_loop_cb, event: 0x%x", event);
    mdf_err_t ret = MDF_OK;

    switch (event) {
        case MDF_EVENT_MWIFI_STARTED:
            MDF_LOGI("MESH is started");
            break;

        case MDF_EVENT_MWIFI_PARENT_CONNECTED:
            MDF_LOGI("Parent is connected on station interface");
            if(esp_mesh_is_root()){
                esp_netif_dhcpc_start(sta_netif);
            }

            oled_context.g_mesh_build_tick = xTaskGetTickCount() - oled_context.g_mesh_start_tick;
            ws2812b_green();
            break;

        case MDF_EVENT_MWIFI_PARENT_DISCONNECTED:
            MDF_LOGI("Parent is disconnected on station interface");

            ws2812b_red();

            if (!esp_mesh_is_root()) {
                break;
            }

            ret = mwifi_post_root_status(false);
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mwifi_post_root_status", mdf_err_to_name(ret));

            break;

        case MDF_EVENT_MWIFI_FIND_NETWORK: {
        MDF_LOGI("the root connects to another router with the same SSID");
        break;
    }

        case MDF_EVENT_MWIFI_ROUTING_TABLE_ADD:
        case MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE: {
        MDF_LOGI("total_num: %d", esp_mesh_get_total_node_num());

        oled_context.g_mesh_build_tick = xTaskGetTickCount() - oled_context.g_mesh_start_tick;

        break;
    }

        case MDF_EVENT_MWIFI_ROOT_GOT_IP: {
        MDF_LOGI("Root obtains the IP address");

        ret = mwifi_post_root_status(true);
        MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mwifi_post_root_status", mdf_err_to_name(ret));

        break;
    }

        case MDF_EVENT_MCONFIG_BLUFI_STA_DISCONNECTED:
            break;

        case MDF_EVENT_MCONFIG_BLUFI_STA_CONNECTED:
            break;

        case MDF_EVENT_MCONFIG_BLUFI_FINISH:
        case MDF_EVENT_MCONFIG_CHAIN_FINISH:
            break;

        case MDF_EVENT_MUPGRADE_STARTED:
            MDF_LOGI("Enter upgrade mode");
            vTaskDelay(pdMS_TO_TICKS(3000));
            break;

        case MDF_EVENT_MUPGRADE_STATUS: {
        MDF_LOGI("The upgrade progress is: %d%%", (int)ctx);
        mwifi_data_type_t mwifi_type = {
            .protocol = MLINK_PROTO_NOTICE,
        };
        ret = mwifi_write(NULL, &mwifi_type, "ota_status", strlen("ota_status"), true);
        MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mwifi_write", esp_err_to_name(ret));
        break;
    }

        case MDF_EVENT_MUPGRADE_FINISH:
            MDF_LOGI("Upgrade completed waiting for restart");
            break;

        case MDF_EVENT_MLINK_SYSTEM_RESET:
            MDF_LOGW("Erase information saved in flash and system restart");

            ret = mdf_info_erase(MDF_SPACE_NAME);
            MDF_ERROR_BREAK(ret != 0, "Erase the information");

            esp_restart();
            break;

        case MDF_EVENT_MLINK_SYSTEM_REBOOT:
            MDF_LOGW("Restart PRO and APP CPUs");
            esp_restart();
            break;

        case MDF_EVENT_MLINK_SET_STATUS:
            /**
             * @brief Waiting for adjacent packets to be processed, avoiding loops
             */
            vTaskDelay(pdMS_TO_TICKS(50));

            /**
             * @brief Trigger handler
             */
            ret = mlink_trigger_handle(MLINK_COMMUNICATE_MESH);
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_trigger_handle", mdf_err_to_name(ret));

            break;

        case MDF_EVENT_MLINK_BUFFER_FULL: {
        MDF_LOGI("Receive data from sniffer");

        /**
         * @brief Notify the APP to actively request the sniffer data received by the device
         */
        mwifi_data_type_t mwifi_type = {
            .protocol = MLINK_PROTO_NOTICE,
        };
        ret = mwifi_write(NULL, &mwifi_type, "sniffer", strlen("sniffer"), true);
        MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mwifi_write", esp_err_to_name(ret));
        break;
    }

        case MDF_EVENT_BUTTON_KEY_SHORT_RELEASE:
            MDF_LOGI("MDF_EVENT_BUTTON_KEY_SHORT_RELEASE");
            switch (*(uint32_t *)ctx) {
                case PAGE_BUTTON_IO_NUM:
                    oled_next_page();
                break;
                case SET_BUTTON_IO_NUM:
                    switch (oled_context.current_page_num)
                    {
                    case PAGE_IPERF:
                        if (esp_mesh_is_root()) {
                            if (iperf_server_task == NULL) {
                                g_mesh_iperf_cfg.finish = false;
                                oled_context.iperf_running = true;
                                xTaskCreatePinnedToCore(mesh_iperf_server_task, "mesh_iperf_server", 4 * 1024,
                                                        NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY,
                                                        &iperf_server_task, CONFIG_MDF_TASK_PINNED_TO_CORE);
                            } else {
                                g_mesh_iperf_cfg.finish = true;
                                oled_context.iperf_running = false;
                            }
                        } else if (iperf_client_task == NULL) {
                            g_mesh_iperf_cfg.finish = false;
                                oled_context.iperf_running = true;
                            xTaskCreatePinnedToCore(mesh_iperf_client_task, "mesh_iperf_client", 4 * 1024,
                                            NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY,
                                            &iperf_client_task, CONFIG_MDF_TASK_PINNED_TO_CORE);
                        } else {
                            g_mesh_iperf_cfg.finish = true;
                            oled_context.iperf_running = false;
                        }
                        
                        break;
                    
                    default:
                        break;
                    }
                break;
            }
            break;

        case MDF_EVENT_BUTTON_KEY_MIDDLE_RELEASE:
            MDF_LOGI("MDF_EVENT_BUTTON_KEY_MIDDLE_RELEASE");
            switch (*(uint32_t *)ctx) {
                case PAGE_BUTTON_IO_NUM:
                    oled_prev_page();
                break;
                case SET_BUTTON_IO_NUM:

                break;
            }
            break;

        case MDF_EVENT_BUTTON_KEY_LONG_RELEASE:
            MDF_LOGI("MDF_EVENT_BUTTON_KEY_LONG_RELEASE");
            switch (*(uint32_t *)ctx) {
                case PAGE_BUTTON_IO_NUM:

                break;
                case SET_BUTTON_IO_NUM: {
                mwifi_config_t ap_config = {0x0};
                mwifi_get_config(&ap_config);
                ap_config.mesh_type = (ap_config.mesh_type == MWIFI_MESH_ROOT) ? MWIFI_MESH_NODE : MWIFI_MESH_ROOT;
                (ap_config.mesh_type == MWIFI_MESH_ROOT) ? ws2812b_blue() : ws2812b_red();
                MDF_ERROR_ASSERT(mwifi_set_config(&ap_config));
                MDF_ERROR_ASSERT(mwifi_restart());
            }
            break;
            }
            break;

        default:
            break;
    }

    return MDF_OK;
}

void app_main()
{
    mwifi_config_t ap_config        = {0x0};
    mwifi_init_config_t init_config = MWIFI_INIT_CONFIG_DEFAULT();
    
    oled_context.g_mesh_start_tick = xTaskGetTickCount();

    /**
     * @brief Set the log level for serial port printing.
     */
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    peripherals_init();
    xTaskCreate(oled_task, "oled_task", 1024 * 8, NULL, 12, NULL);

    /**
     * @brief   1.Initialize event loop, receive event
     *          2.Initialize wifi with station mode
     *          3.Initialize espnow(ESP-NOW is a kind of connectionless WiFi communication protocol)
     */
    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));
    MDF_ERROR_ASSERT(esp_netif_init());
    MDF_ERROR_ASSERT(esp_event_loop_create_default());
    MDF_ERROR_ASSERT(esp_netif_create_default_wifi_mesh_netifs(&sta_netif,NULL));
    MDF_ERROR_ASSERT(wifi_init());
    MDF_ERROR_ASSERT(mespnow_init());
    MDF_ERROR_ASSERT(mwifi_init(&init_config));

    /**
     * @brief Add debug function, you can use serial command and wireless debugging.
     *      1. Initialize console module
     */
    MDF_ERROR_ASSERT(mdebug_console_init());
    MDF_ERROR_ASSERT(mdebug_espnow_init());
    // mdebug_cmd_register_common();

    register_wifi_config();
    register_wifi_scan();
    register_log();
    register_coredump();
    register_command();
    register_mesh_iperf();
    register_mesh_config();
    register_mesh_status();
    register_attenuator();

    MDF_LOGI(" ==================================================");
    MDF_LOGI(" |          Steps to ESP-Buddy test               |");
    MDF_LOGI(" |                                                |");
    MDF_LOGI(" |  1. Print 'help' to gain overview of commands  |");
    MDF_LOGI(" |     mdf> help                                  |");
    MDF_LOGI(" |  2. Configure Wi-Fi                            |");
    MDF_LOGI(" |     mdf> wifi_config -c <channel> or wifi_config -s <ssid> -p <password> |");
    MDF_LOGI(" |  3. Add devices to monitor                     |");
    MDF_LOGI(" |     mdf> log -a <xx:xx:xx:xx:xx:xx>            |");
    MDF_LOGI(" |  4. Use other commands to get the status       |");
    MDF_LOGI(" |                                                |");
    MDF_LOGI(" ==================================================\n");

    if (mdf_info_load("init_config", &init_config, sizeof(mwifi_init_config_t)) != MDF_OK
            && mdf_info_load("ap_config", &ap_config, sizeof(mwifi_config_t)) != MDF_OK) {
        MDF_LOGW("Wifi is not configured, Use default settings");
        ap_config.channel = 13;
        memset(ap_config.mesh_id, 0x12, 6);
        ap_config.mesh_type = MWIFI_MESH_NODE;
    }
    memset(ap_config.mesh_id, 0x12, 6);
    MDF_ERROR_ASSERT(mwifi_set_config(&ap_config));
    MDF_ERROR_ASSERT(mwifi_set_init_config(&init_config));
    MDF_ERROR_ASSERT(mwifi_start());

    /**
     * @brief Periodically print system information.
     */
    // TimerHandle_t timer = xTimerCreate("show_system_info", pdMS_TO_TICKS(10000),
    //                                    true, NULL, show_system_info_timercb);
    // xTimerStart(timer, 0);
}
