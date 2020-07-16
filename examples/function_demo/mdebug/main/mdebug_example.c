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

#include "mdf_common.h"
#include "mwifi.h"
#include "mdebug.h"

#define MLINK_PROTO_LOG  (2)

static const char *TAG = "mdebug_example";

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

/**
 * @brief http_client task creation
 *      You can modify the http address according to your needs,
 *      parameters CONFIG_FLASH_LOG_POST_URL if change.and parameters
 *      transport_type if change.
 */
static void root_read_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    char *data    = NULL;
    size_t size   = 0;
    char mac_str[18] = {0};
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};
    mwifi_data_type_t mwifi_type     = {0};
    esp_http_client_handle_t client  = NULL;
    esp_http_client_config_t config  = {
        .url            = CONFIG_FLASH_LOG_POST_URL,
        .transport_type = HTTP_TRANSPORT_UNKNOWN,
    };

    MDF_LOGI("root_write_task is running");

    while (mwifi_is_connected() && esp_mesh_get_layer() == MESH_ROOT) {
        /**< Receive a packet targeted to external IP network */
        ret = mwifi_root_read(src_addr, &mwifi_type, &data, &size, portMAX_DELAY);
        MDF_ERROR_GOTO(ret != MDF_OK, FREE_MEM, "<%s> mwifi_root_read", mdf_err_to_name(ret));
        printf("Root receive, addr: " MACSTR ", size: %d, data: %.*s\n",
               MAC2STR(src_addr), size, size, data);

        switch (mwifi_type.protocol) {
            case MLINK_PROTO_LOG: {  /**< Use http-client protocol */
                MDF_LOGI("Open HTTP connection: %s, send to log src_addr: " MACSTR ", size: %d",
                         CONFIG_FLASH_LOG_POST_URL, MAC2STR(src_addr), size);
                sprintf(mac_str, MACSTR, MAC2STR(src_addr));

                client = esp_http_client_init(&config);
                MDF_ERROR_GOTO(!client, FREE_MEM, "Initialise HTTP connection");

                esp_http_client_set_header(client, "Node-Mac", mac_str);
                esp_http_client_set_method(client, HTTP_METHOD_POST);

                while (esp_http_client_open(client, size) != MDF_OK) {
                    MDF_LOGW("<%s> Connection service failed", mdf_err_to_name(ret));
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }

                esp_http_client_write(client, data, size);
                esp_http_client_close(client);
                break;
            }

            default:
                MDF_LOGW("Does not support the protocol: %d", mwifi_type.protocol);
                break;
        }

FREE_MEM:
        MDF_FREE(data);
    }

    MDF_FREE(data);
    MDF_LOGW("root_write_task is exit");
    esp_http_client_cleanup(client);
    vTaskDelete(NULL);
}

static void node_write_task(void *arg)
{
    mdf_err_t ret  = MDF_OK;
    char *log_data = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    mwifi_data_type_t mwifi_type = {
        .protocol = MLINK_PROTO_LOG,
    };

    MDF_LOGI("Node write task is running");

    for (;;) {
        if (!mwifi_is_connected() || mdebug_flash_size() < MWIFI_PAYLOAD_LEN) {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }

        /**
         * @brief Read the log data and transfer the data out via http server
         */
        for (size_t log_size = mdebug_flash_size(), size = MIN(MWIFI_PAYLOAD_LEN, log_size);
                size > 0 && mdebug_flash_read(log_data, &size) == MDF_OK && ret == MDF_OK;
                log_size -= size, size = MIN(MWIFI_PAYLOAD_LEN, log_size)) {
            ret = mwifi_write(NULL, &mwifi_type, log_data, size, true);
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mwifi_write", mdf_err_to_name(ret));
        }
    }

    MDF_LOGW("Node write task is exit");

    MDF_FREE(log_data);
    vTaskDelete(NULL);
}

/**@{*/
/**
 * @brief Register a console command that outputs system information
 */
static struct {
    struct arg_lit *output;
    struct arg_end *end;
} console_system_info_args;

static int console_system_info(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void **) &console_system_info_args) != ESP_OK) {
        arg_print_errors(stderr, console_system_info_args.end, argv[0]);
        return MDF_FAIL;
    }

    if (console_system_info_args.output->count) {
        uint8_t primary                 = 0;
        wifi_second_chan_t second       = 0;
        mesh_addr_t parent_bssid        = {0};
        uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
        wifi_sta_list_t wifi_sta_list   = {0x0};

        esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
        esp_wifi_ap_get_sta_list(&wifi_sta_list);
        esp_wifi_get_channel(&primary, &second);
        esp_mesh_get_parent_bssid(&parent_bssid);

        MDF_LOGI("System information, channel: %d, layer: %d, self mac: " MACSTR ", parent bssid: " MACSTR
                 ", parent rssi: %d, node num: %d, free heap: %u", primary,
                 esp_mesh_get_layer(), MAC2STR(sta_mac), MAC2STR(parent_bssid.addr),
                 mwifi_get_parent_rssi(), esp_mesh_get_total_node_num(), esp_get_free_heap_size());

        for (int i = 0; i < wifi_sta_list.num; i++) {
            MDF_LOGI("Child mac: " MACSTR, MAC2STR(wifi_sta_list.sta[i].mac));
        }
    }

    return MDF_OK;
}

static void console_register_system_info()
{
    console_system_info_args.output = arg_lit0("i", "info", "Print the system information");
    console_system_info_args.end    = arg_end(1);

    const esp_console_cmd_t cmd = {
        .command = "system_info",
        .help = "Print the system information",
        .hint = NULL,
        .func = console_system_info,
        .argtable = &console_system_info_args,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/**@{*/

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
    MDF_LOGI("event_loop_cb, event: %d", event);

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

        case MDF_EVENT_MWIFI_ROUTING_TABLE_ADD:
        case MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE:
            MDF_LOGI("total_num: %d", esp_mesh_get_total_node_num());
            break;

        case MDF_EVENT_MWIFI_ROOT_GOT_IP: {
            MDF_LOGI("Root obtains the IP address. It is posted by LwIP stack automatically");

            /**
             * @brief Data transfer between wifi mesh devices
             */
            xTaskCreate(root_read_task, "root_read_task", 4 * 1024,
                        NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
            break;
        }

        case MDF_EVENT_MDEBUG_FLASH_FULL: {
            MDF_LOGI("The flash partition that stores the log is full, size: %d", mdebug_flash_size());
            break;
        }

        default:
            break;
    }

    return MDF_OK;
}

void app_main()
{
    mwifi_init_config_t cfg = MWIFI_INIT_CONFIG_DEFAULT();
    mwifi_config_t config   = {
        .router_ssid     = CONFIG_ROUTER_SSID,
        .router_password = CONFIG_ROUTER_PASSWORD,
        .mesh_id         = CONFIG_MESH_ID,
        .mesh_password   = CONFIG_MESH_PASSWORD,
    };
    mdebug_log_config_t log_config = {
        .log_uart_enable   = true,
        .log_flash_enable  = true,
        .log_espnow_enable = true,
    };

    /**
     * @brief Set the log level for serial port printing.
     */
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    /**
     * @brief Initialize wifi mesh.
     */
    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));
    MDF_ERROR_ASSERT(wifi_init());
    MDF_ERROR_ASSERT(mwifi_init(&cfg));
    MDF_ERROR_ASSERT(mwifi_set_config(&config));
    MDF_ERROR_ASSERT(mwifi_start());

    /**
    * @brief Add debug function, you can use serial command and wireless debugging.
    *      1. Initialize console module
    */
    MDF_ERROR_ASSERT(mdebug_console_init());
    MDF_ERROR_ASSERT(mdebug_log_init());
    MDF_ERROR_ASSERT(mdebug_log_set_config(&log_config));
    mdebug_cmd_register_common();

    /**
     * @brief Register a console command that outputs system information
     */
    console_register_system_info();

    MDF_LOGI(" ==================================================");
    MDF_LOGI(" |             Steps to test ESP-info             |");
    MDF_LOGI(" |                                                |");
    MDF_LOGI(" |  1. Print 'help' to gain overview of commands  |");
    MDF_LOGI(" |     mdf> help                                  |");
    MDF_LOGI(" |  2. System information                         |");
    MDF_LOGI(" |      mdf> system_info -i                       |");
    MDF_LOGI(" |                                                |");
    MDF_LOGI(" ==================================================\n");

    /**
     * @brief Set the timestamp before the log
     *        Set timezone to China Standard Time
     *        1572084468 >> 2019-10-26 18:07:17
     */
    setenv("TZ", "CST-8", 1);
    tzset();

    struct timeval now = { .tv_sec = 1572084468 };
    settimeofday(&now, NULL);

    xTaskCreate(node_write_task, "node_write_task", 4 * 1024,
                NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
}
