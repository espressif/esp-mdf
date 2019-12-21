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
#include "mesh_mqtt_handle.h"

// #define MEMORY_DEBUG

typedef struct {
    size_t last_num;
    uint8_t *last_list;
    size_t change_num;
    uint8_t *change_list;
} node_list_t;

static const char *TAG = "mqtt_examples";

static bool addrs_remove(uint8_t *addrs_list, size_t *addrs_num, const uint8_t *addr)
{
    for (int i = 0; i < *addrs_num; i++, addrs_list += MWIFI_ADDR_LEN) {
        if (!memcmp(addrs_list, addr, MWIFI_ADDR_LEN)) {
            if (--(*addrs_num)) {
                memcpy(addrs_list, addrs_list + MWIFI_ADDR_LEN, (*addrs_num - i) * MWIFI_ADDR_LEN);
            }

            return true;
        }
    }

    return false;
}

void root_write_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    char *data    = NULL;
    size_t size   = MWIFI_PAYLOAD_LEN;
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};
    mwifi_data_type_t data_type      = {0x0};

    MDF_LOGI("Root write task is running");

    while (mwifi_is_connected() && esp_mesh_get_layer() == MESH_ROOT) {
        if (!mesh_mqtt_is_connect()) {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }

        /**
         * @brief Recv data from node, and forward to mqtt server.
         */
        ret = mwifi_root_read(src_addr, &data_type, &data, &size, portMAX_DELAY);
        MDF_ERROR_GOTO(ret != MDF_OK, MEM_FREE, "<%s> mwifi_root_read", mdf_err_to_name(ret));

        ret = mesh_mqtt_write(src_addr, data, size);
        MDF_ERROR_GOTO(ret != MDF_OK, MEM_FREE, "<%s> mesh_mqtt_publish", mdf_err_to_name(ret));

MEM_FREE:
        MDF_FREE(data);
    }

    MDF_LOGW("Root write task is exit");
    mesh_mqtt_stop();
    vTaskDelete(NULL);
}

void root_read_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    char *data    = NULL;
    size_t size   = MWIFI_PAYLOAD_LEN;
    uint8_t dest_addr[MWIFI_ADDR_LEN] = {0x0};
    mwifi_data_type_t data_type       = {0x0};

    MDF_LOGI("Root read task is running");

    while (mwifi_is_connected() && esp_mesh_get_layer() == MESH_ROOT) {
        if (!mesh_mqtt_is_connect()) {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }

        /**
         * @brief Recv data from mqtt data queue, and forward to special device.
         */
        ret = mesh_mqtt_read(dest_addr, (void **)&data, &size, 500 / portTICK_PERIOD_MS);
        MDF_ERROR_GOTO(ret != MDF_OK, MEM_FREE, "");

        ret = mwifi_root_write(dest_addr, 1, &data_type, data, size, true);
        MDF_ERROR_GOTO(ret != MDF_OK, MEM_FREE, "<%s> mwifi_root_write", mdf_err_to_name(ret));

MEM_FREE:
        MDF_FREE(data);
    }

    MDF_LOGW("Root read task is exit");
    mesh_mqtt_stop();
    vTaskDelete(NULL);
}

static void node_read_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    char *data    = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    size_t size   = MWIFI_PAYLOAD_LEN;
    mwifi_data_type_t data_type      = {0x0};
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};

    MDF_LOGI("Node read task is running");

    for (;;) {
        if (!mwifi_is_connected()) {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }

        size = MWIFI_PAYLOAD_LEN;
        memset(data, 0, MWIFI_PAYLOAD_LEN);
        ret = mwifi_read(src_addr, &data_type, data, &size, portMAX_DELAY);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_read", mdf_err_to_name(ret));
        MDF_LOGD("Node receive: " MACSTR ", size: %d, data: %s", MAC2STR(src_addr), size, data);
    }

    MDF_LOGW("Node read task is exit");
    MDF_FREE(data);
    vTaskDelete(NULL);
}

static void node_write_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    int count     = 0;
    size_t size   = 0;
    char *data    = NULL;
    mwifi_data_type_t data_type     = {0x0};
    uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};

    MDF_LOGI("Node task is running");

    esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);

    for (;;) {
        if (!mwifi_is_connected() || !mwifi_get_root_status()) {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }

        /**
         * @brief Send device information to mqtt server throught root node.
         */
        size = asprintf(&data, "{\"mac\": \"%02x%02x%02x%02x%02x%02x\", \"seq\":%d,\"layer\":%d}",
                        MAC2STR(sta_mac), count++, esp_mesh_get_layer());

        MDF_LOGD("Node send, size: %d, data: %s", size, data);
        ret = mwifi_write(NULL, &data_type, data, size, true);
        MDF_FREE(data);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_write", mdf_err_to_name(ret));

        vTaskDelay(3000 / portTICK_RATE_MS);
    }

    MDF_LOGW("Node task is exit");
    vTaskDelete(NULL);
}

/**
 * @brief Timed printing system information
 */
static void print_system_info_timercb(void *timer)
{
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

    MDF_LOGI("System information, channel: %d, layer: %d, self mac: " MACSTR ", parent bssid: " MACSTR
             ", parent rssi: %d, node num: %d, free heap: %u", primary,
             esp_mesh_get_layer(), MAC2STR(sta_mac), MAC2STR(parent_bssid.addr),
             mesh_assoc.rssi, esp_mesh_get_total_node_num(), esp_get_free_heap_size());

    for (int i = 0; i < wifi_sta_list.num; i++) {
        MDF_LOGI("Child mac: " MACSTR, MAC2STR(wifi_sta_list.sta[i].mac));
    }

#ifdef MEMORY_DEBUG

    if (!heap_caps_check_integrity_all(true)) {
        MDF_LOGE("At least one heap is corrupt");
    }

    mdf_mem_print_heap();
    mdf_mem_print_record();
    mdf_mem_print_task();
#endif /**< MEMORY_DEBUG */
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
    static node_list_t node_list = {0x0};

    switch (event) {
        case MDF_EVENT_MWIFI_STARTED:
            MDF_LOGI("MESH is started");
            break;

        case MDF_EVENT_MWIFI_PARENT_CONNECTED:
            MDF_LOGI("Parent is connected on station interface");
            break;

        case MDF_EVENT_MWIFI_PARENT_DISCONNECTED:
            MDF_LOGI("Parent is disconnected on station interface");

            if (esp_mesh_is_root()) {
                mesh_mqtt_stop();
            }

            break;

        case MDF_EVENT_MWIFI_ROUTING_TABLE_ADD:
            MDF_LOGI("MDF_EVENT_MWIFI_ROUTING_TABLE_ADD, total_num: %d", esp_mesh_get_total_node_num());

            if (esp_mesh_is_root()) {

                /**
                 * @brief find new add device.
                 */
                node_list.change_num  = esp_mesh_get_routing_table_size();
                node_list.change_list = MDF_MALLOC(node_list.change_num * sizeof(mesh_addr_t));
                ESP_ERROR_CHECK(esp_mesh_get_routing_table((mesh_addr_t *)node_list.change_list,
                                node_list.change_num * sizeof(mesh_addr_t), (int *)&node_list.change_num));

                for (int i = 0; i < node_list.last_num; ++i) {
                    addrs_remove(node_list.change_list, &node_list.change_num, node_list.last_list + i * MWIFI_ADDR_LEN);
                }

                node_list.last_list = MDF_REALLOC(node_list.last_list,
                                                  (node_list.change_num + node_list.last_num) * MWIFI_ADDR_LEN);
                memcpy(node_list.last_list + node_list.last_num * MWIFI_ADDR_LEN,
                       node_list.change_list, node_list.change_num * MWIFI_ADDR_LEN);
                node_list.last_num += node_list.change_num;

                /**
                 * @brief subscribe topic for new node
                 */
                MDF_LOGI("total_num: %d, add_num: %d", node_list.last_num, node_list.change_num);
                mesh_mqtt_subscribe(node_list.change_list, node_list.change_num);
                MDF_FREE(node_list.change_list);
            }

            break;

        case MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE:
            MDF_LOGI("MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE, total_num: %d", esp_mesh_get_total_node_num());

            if (esp_mesh_is_root()) {
                /**
                 * @brief find removed device.
                 */
                size_t table_size      = esp_mesh_get_routing_table_size();
                uint8_t *routing_table = MDF_MALLOC(table_size * sizeof(mesh_addr_t));
                ESP_ERROR_CHECK(esp_mesh_get_routing_table((mesh_addr_t *)routing_table,
                                table_size * sizeof(mesh_addr_t), (int *)&table_size));

                for (int i = 0; i < table_size; ++i) {
                    addrs_remove(node_list.last_list, &node_list.last_num, routing_table + i * MWIFI_ADDR_LEN);
                }

                node_list.change_num  = node_list.last_num;
                node_list.change_list = MDF_MALLOC(node_list.last_num * MWIFI_ADDR_LEN);
                memcpy(node_list.change_list, node_list.last_list, node_list.change_num * MWIFI_ADDR_LEN);

                node_list.last_num  = table_size;
                memcpy(node_list.last_list, routing_table, table_size * MWIFI_ADDR_LEN);
                MDF_FREE(routing_table);

                /**
                 * @brief unsubscribe topic for leaved node
                 */
                MDF_LOGI("total_num: %d, add_num: %d", node_list.last_num, node_list.change_num);
                mesh_mqtt_unsubscribe(node_list.change_list, node_list.change_num);
                MDF_FREE(node_list.change_list);
            }

            break;

        case MDF_EVENT_MWIFI_ROOT_GOT_IP: {
            MDF_LOGI("Root obtains the IP address. It is posted by LwIP stack automatically");

            mesh_mqtt_start(CONFIG_MQTT_URL);

            /**
             * @brief subscribe topic for all subnode
             */
            size_t table_size      = esp_mesh_get_routing_table_size();
            uint8_t *routing_table = MDF_MALLOC(table_size * sizeof(mesh_addr_t));
            ESP_ERROR_CHECK(esp_mesh_get_routing_table((mesh_addr_t *)routing_table,
                            table_size * sizeof(mesh_addr_t), (int *)&table_size));

            node_list.last_num  = table_size;
            node_list.last_list = MDF_REALLOC(node_list.last_list,
                                              node_list.last_num * MWIFI_ADDR_LEN);
            memcpy(node_list.last_list, routing_table, table_size * MWIFI_ADDR_LEN);
            MDF_FREE(routing_table);

            MDF_LOGI("subscribe %d node", node_list.last_num);
            mesh_mqtt_subscribe(node_list.last_list, node_list.last_num);
            MDF_FREE(node_list.change_list);

            xTaskCreate(root_write_task, "root_write", 4 * 1024,
                        NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
            xTaskCreate(root_read_task, "root_read", 4 * 1024,
                        NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
            break;
        }

        case MDF_EVENT_CUSTOM_MQTT_CONNECT:
            MDF_LOGI("MQTT connect");
            mwifi_post_root_status(true);
            break;

        case MDF_EVENT_CUSTOM_MQTT_DISCONNECT:
            MDF_LOGI("MQTT disconnected");
            mwifi_post_root_status(false);
            break;

        default:
            break;
    }

    return MDF_OK;
}

void app_main()
{
    mwifi_init_config_t cfg   = MWIFI_INIT_CONFIG_DEFAULT();
    mwifi_config_t config     = {
        .router_ssid     = CONFIG_ROUTER_SSID,
        .router_password = CONFIG_ROUTER_PASSWORD,
        .mesh_id         = CONFIG_MESH_ID,
        .mesh_password   = CONFIG_MESH_PASSWORD,
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
     * @brief Create node handler
     */
    xTaskCreate(node_write_task, "node_write_task", 4 * 1024,
                NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
    xTaskCreate(node_read_task, "node_read_task", 4 * 1024,
                NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);

    TimerHandle_t timer = xTimerCreate("print_system_info", 10000 / portTICK_RATE_MS,
                                       true, NULL, print_system_info_timercb);
    xTimerStart(timer, 0);
}
