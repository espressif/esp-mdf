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

#include "mwifi.h"
#include "miniz.h"

#define MWIFI_WAIVE_ROOT_INTERVAL  3 /**< When the root rssi is weak, MWIFI_WAIVE_ROOT_INTERVAL minutes will initiate a re root node selection */
#define MWIFI_EVET_INFO_SIZE 3

typedef struct {
    uint32_t magic;                   /**< Filter duplicate packets */
    struct {
        bool transmit_self      : 1;  /**< Whether the forwarded packet is for yourself */
        bool transmit_all       : 1;  /**< Whether to send packages to all devices */
        size_t transmit_num     : 10; /**< Number of destination devices forwarded */
        size_t total_size_low   : 12; /**< Total length of the packet */
        uint8_t packet_seq      : 3;  /**< Serial number of the packet */
        size_t total_size_hight : 1;  /**< Total length of the packet */
        uint8_t compress_rate   : 4;  /**< The ratio of the data to the original after compression */
    };
    mwifi_data_type_t type;           /**< The type of data */
} __attribute__((packed)) mwifi_data_head_t;

static const char *TAG           = "mwifi";
static bool g_mwifi_inited_flag  = false;
static bool mwifi_connected_flag = false;
static bool g_mwifi_started_flag = false;
static mwifi_config_t *g_ap_config        = NULL;
static mwifi_init_config_t *g_init_config = NULL;
static bool g_rootless_flag                       = false;
static mesh_event_toDS_state_t g_toDs_status_flag = false;
static xTimerHandle g_waive_root_timer;
static int g_waive_root_interval                  = MWIFI_WAIVE_ROOT_INTERVAL; /**< Avoid frequent triggers waive root*/

bool mwifi_is_started()
{
    return g_mwifi_started_flag;
}

bool mwifi_is_connected()
{
    return mwifi_connected_flag;
}

mdf_err_t mwifi_post_root_status(bool status)
{
    g_toDs_status_flag = status;
    return esp_mesh_post_toDS_state(status);
}

bool mwifi_get_root_status()
{
    if (!g_toDs_status_flag) {
        mesh_assoc_t mesh_assoc = {0x0};
        mesh_chain_layer_t mesh_chain;

        if (esp_wifi_vnd_mesh_get(&mesh_assoc, &mesh_chain) == MDF_OK) {
            g_toDs_status_flag = mesh_assoc.toDS;
        }
    }

    return g_toDs_status_flag && !g_rootless_flag;
}

#ifdef CONFIG_MWIFI_WAIVE_ROOT

static mdf_err_t mwifi_waive_root_timer_delete(void)
{
    mdf_err_t ret = MDF_OK;

    if (g_waive_root_timer) {
        if (xTimerDelete(g_waive_root_timer, 0) != pdPASS) {
            MDF_LOGE("Delete waive root timer failed");
            ret = MDF_FAIL;
        } else {
            g_waive_root_timer = NULL;
        }
    }

    return ret;
}

static void mwifi_waive_root_timercb(TimerHandle_t timer)
{
    if (!g_waive_root_timer) {
        MDF_LOGW("Timer has been deleted");
        return ;
    }

    static int s_waive_rssi_count = 0; /**< Avoid the effects of sudden changes in signal strength */

    if (esp_mesh_is_root() && esp_mesh_get_total_node_num() > 1
            && mwifi_get_parent_rssi() < CONFIG_MWIFI_WAIVE_ROOT_RSSI) {
        s_waive_rssi_count++;
    } else {
        s_waive_rssi_count = 0;
    }

    if (s_waive_rssi_count >= g_waive_root_interval) {
        s_waive_rssi_count = 0;
        esp_mesh_waive_root(NULL, MESH_VOTE_REASON_ROOT_INITIATED);
        g_waive_root_interval = (g_waive_root_interval < 60) ? g_waive_root_interval * 3 : 60;
        MDF_LOGI("Reselect root, waive_interval: %d, current_rssi: %d, waive_rssi: %d",
                 g_waive_root_interval, mwifi_get_parent_rssi(), CONFIG_MWIFI_WAIVE_ROOT_RSSI);
    }
}

static mdf_err_t mwifi_waive_root_timer_create(void)
{
    g_waive_root_timer = xTimerCreate("mwifi_waive_root", pdMS_TO_TICKS(60 * 1000), true, NULL, mwifi_waive_root_timercb);
    MDF_ERROR_CHECK(g_waive_root_timer == NULL, MDF_FAIL, "Create an esp_timer instance");

    if (xTimerStart(g_waive_root_timer, 0) != pdPASS) {
        MDF_LOGE("Start waive root timer failed");
        return MDF_FAIL;
    }

    return MDF_OK;
}

#endif /**< CONFIG_MWIFI_WAIVE_ROOT */

static void esp_ip_event_cb(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    MDF_LOGD("esp_ip_event_cb event_id: %d", event_id);
    static mesh_event_info_t s_event_info = { 0 };

    /**< Send event to the event handler */
    memcpy(&s_event_info, event_data, sizeof(mesh_event_info_t));

    switch (event_id) {
        case IP_EVENT_STA_LOST_IP: {
            MDF_LOGI("Root loses the IP address");
            esp_mesh_disconnect();
            mdf_event_loop_send(MDF_EVENT_MWIFI_ROOT_LOST_IP, &s_event_info);
            break;
        }

#ifdef CONFIG_MWIFI_WAIVE_ROOT

        case IP_EVENT_STA_GOT_IP: {
            mwifi_waive_root_timer_delete();
            mwifi_waive_root_timer_create();
            mdf_event_loop_send(MDF_EVENT_MWIFI_ROOT_GOT_IP, &s_event_info);
            break;
        }

#endif /**< CONFIG_MWIFI_WAIVE_ROOT */

        default:
            break;
    }
}

static void esp_mesh_event_cb(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    MDF_LOGD("esp_mesh_event_cb event_id: %d", event_id);
    static int s_disconnected_count = 0;
    static mesh_event_info_t s_evet_info[MWIFI_EVET_INFO_SIZE] = { 0 };
    static int evet_info_index = 0;

    switch (event_id) {
        case MESH_EVENT_PARENT_CONNECTED: {
            MDF_LOGI("Parent is connected");
            mwifi_connected_flag = true;
            s_disconnected_count = 0;

            if (esp_mesh_is_root()) {
                g_rootless_flag = false;
            }

            break;
        }

        case MESH_EVENT_PARENT_DISCONNECTED: {
            mesh_event_disconnected_t *disconnected = (mesh_event_disconnected_t *)event_data;
            MDF_LOGI("Parent is disconnected, reason: %d", disconnected->reason);
            mwifi_connected_flag = false;

            /**
                 * @brief The root node cannot find the router and only reports the disconnection.
                 */
            if (s_disconnected_count++ > 100) {
                s_disconnected_count = 0;
                mdf_event_loop_send(MDF_EVENT_MWIFI_EXCEPTION, NULL);

#ifdef CONFIG_MWIFI_WAIVE_ROOT

                if (esp_mesh_is_root()) {
                    esp_mesh_waive_root(NULL, MESH_VOTE_REASON_ROOT_INITIATED);
                    MDF_LOGW("WAIVE_ROOT: The root node cannot find the router and only reports the disconnection.");
                }

#endif /**< CONFIG_MWIFI_WAIVE_ROOT */
            }

            break;
        }

#ifdef CONFIG_MWIFI_WAIVE_ROOT

        case MESH_EVENT_LAYER_CHANGE: {
            if (!esp_mesh_is_root()) {
                mwifi_waive_root_timer_delete();
            }

            break;
        }

#endif /**< CONFIG_MWIFI_WAIVE_ROOT */

        case MESH_EVENT_STARTED: {
            MDF_LOGI("MESH is started");
            s_disconnected_count = 0;
            g_mwifi_started_flag = true;

            break;
        }

        case MESH_EVENT_STOPPED: {
            MDF_LOGI("MESH is stopped");
            g_mwifi_started_flag = false;
            mwifi_connected_flag = false;

            break;
        }

        case MESH_EVENT_ROUTING_TABLE_ADD: {
            mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
            MDF_LOGI("Routing table is changed by adding newly joined children add_num: %d, total_num: %d",
                     routing_table->rt_size_change,
                     routing_table->rt_size_new);
            g_waive_root_interval = MWIFI_WAIVE_ROOT_INTERVAL;
            break;
        }

        case MESH_EVENT_ROUTING_TABLE_REMOVE: {
            mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
            MDF_LOGI("Routing table is changed by removing leave children remove_num: %d, total_num: %d",
                     routing_table->rt_size_change,
                     routing_table->rt_size_new);
            break;
        }

        case MESH_EVENT_TODS_STATE: {
            mesh_event_toDS_state_t *tdos_state = (mesh_event_toDS_state_t *)event_data;
            MDF_LOGI("State represents: %d", g_toDs_status_flag);
            g_toDs_status_flag = *tdos_state;
            break;
        }

        case MESH_EVENT_NETWORK_STATE: {
            mesh_event_network_state_t *network_state = (mesh_event_network_state_t *)event_data;
            g_rootless_flag = network_state->is_rootless;
            MDF_LOGI("Network state: %s", g_rootless_flag ? "root_less" : "root_connect");

            memcpy(&s_evet_info[evet_info_index], event_data, sizeof(mesh_event_info_t));

            if (g_rootless_flag) {
                g_toDs_status_flag = s_evet_info[evet_info_index].toDS_state = MESH_TODS_UNREACHABLE;
                mdf_event_loop_send(MESH_EVENT_TODS_STATE, &s_evet_info[evet_info_index]);
            }

            evet_info_index = (evet_info_index + 1) % MWIFI_EVET_INFO_SIZE;

            break;
        }

        default:
            break;
    }

    /**< Send event to the event handler */
    if (event_data != NULL) {
        memcpy(&s_evet_info[evet_info_index], event_data, sizeof(mesh_event_info_t));
    }

    mdf_event_loop_send(event_id, &s_evet_info[evet_info_index]);
    evet_info_index = (evet_info_index + 1) % MWIFI_EVET_INFO_SIZE;
}

mdf_err_t mwifi_init(const mwifi_init_config_t *config)
{
    MDF_PARAM_CHECK(config);
    MDF_ERROR_CHECK(g_mwifi_inited_flag, MDF_ERR_MWIFI_INITED, "Mwifi has been initialized");

    MDF_LOGI("esp-mdf version: %s", mdf_get_version());

    if (!g_init_config) {
        g_init_config = MDF_CALLOC(1, sizeof(mwifi_init_config_t));
        MDF_ERROR_CHECK(!g_init_config, MDF_ERR_NO_MEM, "");
    }

    if (!g_ap_config) {
        g_ap_config = MDF_CALLOC(1, sizeof(mwifi_config_t));
        MDF_ERROR_CHECK(!g_ap_config, MDF_ERR_NO_MEM, "");
    }

    memcpy(g_init_config, config, sizeof(mwifi_init_config_t));
    g_mwifi_inited_flag = true;

    /**< Mesh initialization */
    return esp_mesh_init();
}

void mwifi_print_config()
{
    if (!g_mwifi_inited_flag) {
        MDF_LOGW("Mwifi is not initialized");
        return;
    }

    mesh_cfg_t cfg                     = {0};
    mesh_attempts_t attempts           = {0};
    int beacon_interval                = 0;
    mesh_switch_parent_t switch_parent = {0};
    mesh_rssi_threshold_t rssi_threshoud = {0};
    const char *bool_str[]             = {"false", "true"};
    const char *mesh_type_str[]        = {"idle", "root", "node", "leaf", "root_sta"};

    ESP_ERROR_CHECK(esp_mesh_get_config(&cfg));
    ESP_ERROR_CHECK(esp_mesh_get_attempts(&attempts));
    ESP_ERROR_CHECK(esp_mesh_get_switch_parent_paras(&switch_parent));
    ESP_ERROR_CHECK(esp_mesh_get_rssi_threshold(&rssi_threshoud));

    MDF_LOGI("**************** Root config ****************");
    MDF_LOGI("vote_percentage       : %0.2f", esp_mesh_get_vote_percentage());
    MDF_LOGI("vote_max_count        : %d", attempts.vote);
    MDF_LOGI("backoff_rssi          : %d", switch_parent.backoff_rssi);
    MDF_LOGI("scan_min_count        : %d", attempts.scan);
    MDF_LOGI("attempt_count         : %d", attempts.fail);
    MDF_LOGI("monitor_ie_count      : %d", attempts.monitor_ie);
    MDF_LOGI("root_healing_ms       : %d", esp_mesh_get_root_healing_delay());
    MDF_LOGI("root_conflicts_enable : %s", bool_str[esp_mesh_is_root_conflicts_allowed()]);
    MDF_LOGI("fix_root_enable       : %s", bool_str[esp_mesh_is_root_fixed()]);

    MDF_LOGI("****************  Capacity   ****************");
    MDF_LOGI("max_layer             : %d", esp_mesh_get_max_layer());
    MDF_LOGI("max_connection        : %d", cfg.mesh_ap.max_connection);
    MDF_LOGI("capacity_num          : %d", esp_mesh_get_capacity_num());
    MDF_LOGI("topology              : %d", esp_mesh_get_topology());

    MDF_LOGI("****************  Stability  ****************");
    MDF_LOGI("assoc_expire_ms       : %d", esp_mesh_get_ap_assoc_expire() * 1000);
    ESP_ERROR_CHECK(esp_mesh_get_beacon_interval(&beacon_interval));
    MDF_LOGI("beacon_interval_ms    : %d", beacon_interval);
    MDF_LOGI("passive_scan_ms       : %d", esp_mesh_get_passive_scan_time());
    MDF_LOGI("monitor_duration_ms   : %d", switch_parent.duration_ms);
    MDF_LOGI("cnx_rssi              : %d", switch_parent.cnx_rssi);
    MDF_LOGI("select_rssi           : %d", switch_parent.select_rssi);
    MDF_LOGI("switch_rssi           : %d", switch_parent.switch_rssi);
    MDF_LOGI("rssi_threshoud_high   : %d", rssi_threshoud.high);
    MDF_LOGI("rssi_threshoud_medium : %d", rssi_threshoud.medium);
    MDF_LOGI("rssi_threshoud_low    : %d", rssi_threshoud.low);

    MDF_LOGI("**************** Transmission ****************");
    MDF_LOGI("xon_qsize             : %d", esp_mesh_get_xon_qsize());
    MDF_LOGI("retransmit_enable     : %s", bool_str[g_init_config->retransmit_enable]);

    MDF_LOGI("**************** Router info  ****************");
    MDF_LOGI("ssid                  : %s", cfg.router.ssid);
    MDF_LOGI("password              : %s", cfg.router.password);
    MDF_LOGI("bssid                 : " MACSTR, MAC2STR(cfg.router.bssid));
    MDF_LOGI("mesh_type             : %s", mesh_type_str[g_ap_config->mesh_type]);
    MDF_LOGI("mesh_id               : " MACSTR, MAC2STR(cfg.mesh_id.addr));
    MDF_LOGI("mesh_password         : %s", cfg.mesh_ap.password);
    MDF_LOGI("mesh_authmode         : %d", esp_mesh_get_ap_authmode());
    MDF_LOGI("channel               : %d", cfg.channel);
    MDF_LOGI("allow_router_switch   : %s", bool_str[cfg.router.allow_router_switch]);
    MDF_LOGI("allow_channel_switch  : %s", bool_str[cfg.allow_channel_switch]);
}

mdf_err_t mwifi_start()
{
    if (mwifi_is_started()) {
        return MDF_OK;
    }

    MDF_ERROR_CHECK(!g_mwifi_inited_flag, MDF_ERR_MWIFI_NOT_INIT, "Mwifi isn't initialized");
    MDF_ERROR_CHECK(!g_ap_config, MDF_ERR_MWIFI_NO_CONFIG, "Config information is not set");

    mesh_cfg_t mesh_config             = MESH_INIT_CONFIG_DEFAULT();
    mwifi_config_t *ap_config          = g_ap_config;
    mwifi_init_config_t *init_config   = g_init_config;
    mesh_attempts_t attempts           = {0};
    mesh_switch_parent_t switch_parent = {0};
    mesh_rssi_threshold_t rssi_threshoud = MWIFI_RSSI_THRESHOUD_DEFAULT();

    /** Mesh initialization
     *  - Check whether Wi-Fi is started.
     *  - Initialize mesh global variables with default values.
     */
    ESP_ERROR_CHECK(esp_mesh_init());

    switch (ap_config->mesh_type) {
        case MESH_NODE:
            /** Enable network Fixed Root Setting
                 *  - Enabling fixed root disables automatic election of the root node via voting.
                 *  - All devices in the network shall use the same Fixed Root Setting (enabled or disabled).
                 *  - If Fixed Root is enabled, users should make sure a root node is designated for the network.
                 */
            ESP_ERROR_CHECK(esp_mesh_fix_root(true));
            break;

        case MESH_IDLE:
            break;

        default:
            /** Designate device type over the mesh network
                 *  - MESH_ROOT: designates the root node for a mesh network
                 *  - MESH_LEAF: designates a device as a standalone Wi-Fi station
                 */
            ESP_ERROR_CHECK(esp_mesh_set_type(ap_config->mesh_type));
            break;
    }

    /**< Set mesh event callback. */
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &esp_ip_event_cb, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &esp_mesh_event_cb, NULL));

    /**
     * @brief Mesh root configuration
     *  - Set attempts for mesh self-organized networking
     *  - Set vote percentage threshold for approval of being a root
     *      - During the networking, only obtaining vote percentage reaches this threshold, the device could be a root.
     *  - Set delay time before starting root healing
     *  - Set whether allow more than one root existing in one network
     */
    attempts.vote       = init_config->vote_max_count;
    attempts.scan       = init_config->scan_min_count;
    attempts.fail       = init_config->attempt_count;
    attempts.monitor_ie = init_config->monitor_ie_count;
    ESP_ERROR_CHECK(esp_mesh_set_attempts(&attempts));
    ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(init_config->vote_percentage / 100.0));
    ESP_ERROR_CHECK(esp_mesh_set_root_healing_delay(init_config->root_healing_ms));
    ESP_ERROR_CHECK(esp_mesh_allow_root_conflicts(init_config->root_conflicts_enable));

    /**
     * @brief Mesh network capacity configuration
     *  - Set network max layer value (max:25, default:25)
     *      - Network max layer limits the max hop count.
     *  - Set mesh network capacity (max:1000, default:300)
     */
    mesh_config.mesh_ap.max_connection = init_config->max_connection;
    ESP_ERROR_CHECK(esp_mesh_set_topology(init_config->topology));
    ESP_ERROR_CHECK(esp_mesh_set_max_layer(init_config->max_layer));
    ESP_ERROR_CHECK(esp_mesh_set_capacity_num(init_config->capacity_num));

    /**
     * @brief Mesh network stability configuration
     *  - Set parameters for parent switch
     *  - Set mesh softAP associate expired time (default:10 seconds)
     *      - If mesh softAP hasn't received any data from an associated child within this time,
     *              mesh softAP will take this child inactive and disassociate it.
     *      - If mesh softAP is encrypted, this value should be set a greater value, such as 30 seconds.
     *  - Set mesh softAP beacon interval
     *  - Set passive scan time
     */
    switch_parent.duration_ms  = init_config->monitor_duration_ms;
    switch_parent.backoff_rssi = init_config->backoff_rssi;
    switch_parent.cnx_rssi     = init_config->cnx_rssi;
    switch_parent.select_rssi  = init_config->select_rssi;
    switch_parent.switch_rssi  = init_config->switch_rssi;
    ESP_ERROR_CHECK(esp_mesh_set_switch_parent_paras(&switch_parent));
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(init_config->assoc_expire_ms / 1000));
    ESP_ERROR_CHECK(esp_mesh_set_beacon_interval(init_config->beacon_interval_ms));
    ESP_ERROR_CHECK(esp_mesh_set_passive_scan_time(init_config->passive_scan_ms));

    /**
     * @brief Mesh network data transmission configuration
     *  - Set the number of queue
     */
    ESP_ERROR_CHECK(esp_mesh_set_xon_qsize(init_config->xon_qsize));

#ifdef CONFIG_MWIFI_ENABLE_PS
    /* Enable mesh PS function */
    ESP_ERROR_CHECK(esp_mesh_enable_ps());
    /* better to increase the associate expired time, if a small duty cycle is set. */
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(60));
    /* better to increase the announce interval to avoid too much management traffic, if a small duty cycle is set. */
    ESP_ERROR_CHECK(esp_mesh_set_announce_interval(600, 3300));
#else
    /* Disable mesh PS function */
    ESP_ERROR_CHECK(esp_mesh_disable_ps());
#endif

    /**
     * @brief mwifi AP configuration
     *  - Set mesh softAP authentication mode
     *  - Set mesh stack configuration
     *      - Use MESH_INIT_CONFIG_DEFAULT() to initialize the default values, mesh IE is encrypted by default.
     *      - Mesh network is established on a fixed channel (1-14).
     *      - Mesh event callback is mandatory.
     *      - Mesh ID is an identifier of an MBSS. Nodes with the same mesh ID can communicate with each other.
     *      - Regarding to the router configuration, if the router is hidden, BSSID field is mandatory.
     */
    mesh_config.channel              = ap_config->channel;
    mesh_config.allow_channel_switch = !ap_config->channel_switch_disable;
    mesh_config.router.ssid_len = strlen(ap_config->router_ssid);
    mesh_config.router.allow_router_switch = !ap_config->router_switch_disable;
    memcpy(mesh_config.router.ssid, ap_config->router_ssid, mesh_config.router.ssid_len);
    memcpy(mesh_config.router.bssid, ap_config->router_bssid, MWIFI_ADDR_LEN);
    memcpy(mesh_config.router.password, ap_config->router_password,
           sizeof(mesh_config.router.password));
    memcpy(mesh_config.mesh_id.addr, ap_config->mesh_id, MWIFI_ADDR_LEN);

    if (strlen(ap_config->mesh_password)) {
        memcpy(mesh_config.mesh_ap.password, ap_config->mesh_password, sizeof(mesh_config.mesh_ap.password));
        ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(WIFI_AUTH_WPA_PSK));
    } else {
        ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(WIFI_AUTH_OPEN));
    }

    ESP_ERROR_CHECK(esp_mesh_set_rssi_threshold(&rssi_threshoud));

#ifndef CONFIG_MWIFI_MESH_IE_ENABLE
    mesh_config.crypto_funcs = NULL;
#endif /*< CONFIG_MWIFI_MESH_IE_ENABLE */

    ESP_ERROR_CHECK(esp_mesh_set_config(&mesh_config));

    /**< Start mesh network. */
    esp_err_t ret = esp_mesh_start();

#ifdef CONFIG_MWIFI_ENABLE_PS
    /* set the device active duty cycle. (default:10, MESH_PS_DEVICE_DUTY_REQUEST) */
    ESP_ERROR_CHECK(esp_mesh_set_active_duty_cycle(CONFIG_MWIFI_PS_DEV_DUTY, CONFIG_MWIFI_PS_DEV_DUTY_TYPE));
    /* set the network active duty cycle. (default:10, -1, MESH_PS_NETWORK_DUTY_APPLIED_ENTIRE) */
    ESP_ERROR_CHECK(esp_mesh_set_network_duty_cycle(CONFIG_MWIFI_PS_NWK_DUTY, CONFIG_MWIFI_PS_NWK_DUTY_DURATION, CONFIG_MWIFI_PS_NWK_DUTY_RULE));
#endif

    return ret;
}

mdf_err_t mwifi_stop()
{
    mdf_err_t ret = MDF_OK;

    if (!mwifi_is_started()) {
        return MDF_OK;
    }

    g_mwifi_started_flag = false;

    ret = esp_mesh_stop();
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "esp_mesh_stop");

    /**< To ensure that all tasks are properly exited */
    MDF_LOGD("vTaskDelay 50ms");
    vTaskDelay(50 / portTICK_RATE_MS);

    MDF_ERROR_ASSERT(esp_wifi_set_mode(WIFI_MODE_STA));

    return MDF_OK;
}

mdf_err_t mwifi_restart()
{
    mdf_err_t ret = MDF_OK;

    ret = mwifi_stop();
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "Mwifi stop");

    ret = mwifi_start();
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "Mwifi stop");

    return MDF_OK;
}

mdf_err_t mwifi_deinit()
{
    MDF_ERROR_CHECK(!g_mwifi_inited_flag, MDF_ERR_MWIFI_NOT_INIT, "Mwifi isn't initialized");
    g_mwifi_inited_flag = false;

    MDF_FREE(g_init_config);
    MDF_FREE(g_ap_config);

    ESP_ERROR_CHECK(esp_mesh_deinit());

    return MDF_OK;
}

mdf_err_t mwifi_set_init_config(const mwifi_init_config_t *init_config)
{
    MDF_PARAM_CHECK(init_config);
    MDF_ERROR_CHECK(!g_mwifi_inited_flag, MDF_ERR_MWIFI_NOT_INIT, "Mwifi isn't initialized");

    memcpy(g_init_config, init_config, sizeof(mwifi_init_config_t));

    return MDF_OK;
}

mdf_err_t mwifi_get_init_config(mwifi_init_config_t *init_config)
{
    MDF_PARAM_CHECK(init_config);
    MDF_ERROR_CHECK(!g_mwifi_inited_flag, MDF_ERR_MWIFI_NOT_INIT, "Mwifi isn't initialized");

    memcpy(init_config, g_init_config, sizeof(mwifi_init_config_t));

    return MDF_OK;
}

mdf_err_t mwifi_set_config(const mwifi_config_t *config)
{
    MDF_ERROR_CHECK(!g_mwifi_inited_flag, MDF_ERR_MWIFI_NOT_INIT, "Mwifi isn't initialized");
    MDF_PARAM_CHECK(config);
    MDF_PARAM_CHECK(!MWIFI_ADDR_IS_EMPTY(config->mesh_id));
    MDF_PARAM_CHECK(config->channel <= 14);
    MDF_PARAM_CHECK(strlen(config->router_ssid) || config->mesh_type);
    MDF_PARAM_CHECK(!strlen(config->router_password)
                    || (strlen(config->router_password) >= 8 && strlen(config->router_password) < 64));
    MDF_PARAM_CHECK(!strlen(config->mesh_password)
                    || (strlen(config->mesh_password) >= 8 && strlen(config->mesh_password) < 64));

    memcpy(g_ap_config, config, sizeof(mwifi_config_t));

    return MDF_OK;
}

mdf_err_t mwifi_get_config(mwifi_config_t *config)
{
    MDF_PARAM_CHECK(config);
    MDF_ERROR_CHECK(!g_ap_config, ESP_ERR_NOT_SUPPORTED, "config information is not set");

    memcpy(config, g_ap_config, sizeof(mwifi_config_t));

    return MDF_OK;
}

static bool addrs_remove(mesh_addr_t *addrs_list, size_t *addrs_num, const mesh_addr_t *addr)
{
    for (int i = 0; i < *addrs_num; i++) {
        if (!memcmp(addrs_list + i, addr, sizeof(mesh_addr_t))) {
            if (--(*addrs_num)) {
                memcpy(addrs_list + i, addrs_list + i + 1, (*addrs_num - i) * MWIFI_ADDR_LEN);
            }

            return true;
        }
    }

    return false;
}

/**
 * @brief Fragmenting packets for transmission
 */
static mdf_err_t mwifi_subcontract_write(const mesh_addr_t *dest_addr, const mesh_data_t *data,
        int flag, const mesh_opt_t *opt)
{
    mdf_err_t ret = MDF_OK;
    static SemaphoreHandle_t s_mwifi_send_lock = NULL;
    mwifi_data_head_t *data_head = (mwifi_data_head_t *)opt->val;
    mesh_data_t mesh_data        = {0x0};
    data_head->total_size_hight  = data->size >> 12;
    data_head->total_size_low    = data->size & 0xfff;
    data_head->packet_seq        = 0;
    TickType_t wait_ticks        = (flag & MESH_DATA_NONBLOCK) ? 0 : portMAX_DELAY;

    memcpy(&mesh_data, data, sizeof(mesh_data_t));

    if (!s_mwifi_send_lock) {
        s_mwifi_send_lock = xSemaphoreCreateMutex();
    }

    /** Fragmenting packets for transmission
     *  - The maximum length allowed for each ESP-WIFI-MESH packet is MWIFI_PAYLOAD_LEN
     */
    for (int unwritten_size = data->size; unwritten_size > 0;
            unwritten_size -= MWIFI_PAYLOAD_LEN) {
        data_head->magic = esp_random();
        mesh_data.size   = MIN(unwritten_size, MWIFI_PAYLOAD_LEN);

        /**< Wait for other tasks to be sent before send ESP-WIFI-MESH data */
        if (!xSemaphoreTake(s_mwifi_send_lock, wait_ticks)) {
            return MDF_ERR_TIMEOUT;
        }

        int retry_count = 3;

        do {
            /**< Send a packet over the mesh network */
            ret = esp_mesh_send(dest_addr, &mesh_data, flag, opt, 1);

            if (ret == ESP_ERR_MESH_NO_MEMORY) {
                MDF_LOGW("<%s> esp_mesh_send", mdf_err_to_name(ret));
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
        } while (ret == ESP_ERR_MESH_NO_MEMORY && --retry_count);

        /**< ESP-WIFI-MESH send completed, release send lock */
        xSemaphoreGive(s_mwifi_send_lock);
        MDF_ERROR_CHECK(ret != ESP_OK && !(flag & MESH_DATA_GROUP && ret == ESP_ERR_MESH_DISCARD), ret,
                        "Node failed to send packets, dest_addr: " MACSTR
                        ", flag: 0x%02x, opt->type: 0x%02x, opt->len: %d, data->tos: %d, data: %p, size: %d",
                        MAC2STR(dest_addr->addr), flag, opt->type, opt->len, mesh_data.tos, mesh_data.data, mesh_data.size);

        data_head->packet_seq++;
        mesh_data.data += MWIFI_PAYLOAD_LEN;
    }

    return MDF_OK;
}

/**
 * @brief Multicast forwarding
 */
static mdf_err_t mwifi_transmit_write(mesh_addr_t *addrs_list, size_t addrs_num, mesh_data_t *mesh_data,
                                      int data_flag, mesh_opt_t *mesh_opt)
{
    mdf_err_t ret          = MDF_OK;
    wifi_sta_list_t sta    = {0};
    uint8_t *transmit_data = NULL;
    mwifi_data_head_t *data_head = (mwifi_data_head_t *)mesh_opt->val;

    /**
     * @brief If the address designation is all nodes or other nodes except the root in the mesh network,
     *  transmit_all should be true to send data to all downstream nodes of the node.
     */
    if (MWIFI_ADDR_IS_ANY(addrs_list->addr) || MWIFI_ADDR_IS_BROADCAST(addrs_list->addr)) {
        data_head->transmit_all = true;
    }

    if (g_ap_config->mesh_type == MESH_LEAF || esp_wifi_ap_get_sta_list(&sta) != MDF_OK) {
        sta.num = 0;
    }

    /**
     * @brief Determine whether there is a destination device in the downstream node of the node.
     */
    for (int i = 0; i < sta.num && addrs_num > 0; ++i) {
        mesh_addr_t *child_addr = (mesh_addr_t *)&sta.sta[i].mac;
        MDF_LOGV("data_head->transmit_all: %d, child_addr: " MACSTR,
                 data_head->transmit_all, MAC2STR(child_addr->addr));

        /**
         * @brief If the packet not send to all nodes in the mesh network,
         *  find out if there is a destination device in the downstream node of the node and
         *  remove all downstream nodes from the address list.
         */
        if (!data_head->transmit_all) {
            int subnet_num           = 0;
            mesh_addr_t *subnet_addr = NULL;
            data_head->transmit_self = addrs_remove(addrs_list, &addrs_num, child_addr);
            data_head->transmit_num  = 0;

            /**< Get the number of nodes in the subnet of a specific child */
            ret = esp_mesh_get_subnet_nodes_num(child_addr, &subnet_num);
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> Get the number of nodes in the subnet of a specific child", mdf_err_to_name(ret));
            transmit_data = MDF_REALLOC_RETRY(transmit_data, mesh_data->size + subnet_num * MWIFI_ADDR_LEN);
            mesh_addr_t *transmit_addr = (mesh_addr_t *)transmit_data;
            MDF_LOGV("subnet_num: %d", subnet_num);

            if (subnet_num) {
                /**< Get nodes in the subnet of a specific child */
                subnet_addr = MDF_REALLOC_RETRY(NULL, subnet_num * sizeof(mesh_addr_t));
                ret = esp_mesh_get_subnet_nodes_list(child_addr, subnet_addr, subnet_num);
                MDF_ERROR_GOTO(ret != ESP_OK, MEM_FREE, "<%s> Get the subnet_node_list of nodes in the subnet of a specific child" MACSTR, mdf_err_to_name(ret), MAC2STR(child_addr->addr));
            }

            for (int j = 0; j < subnet_num && addrs_num > 0; ++j) {
                if (addrs_remove(addrs_list, &addrs_num, subnet_addr + j)) {
                    memcpy(transmit_addr + data_head->transmit_num, subnet_addr + j, sizeof(mesh_addr_t));
                    data_head->transmit_num++;
                    MDF_LOGV("count: %d, transmit_addr: " MACSTR,
                             data_head->transmit_num, MAC2STR(subnet_addr[j].addr));
                }
            }

MEM_FREE:
            MDF_FREE(subnet_addr);
        }

        /**
         * @brief Send data to child nodes.
         */
        if (data_head->transmit_num || data_head->transmit_self || data_head->transmit_all) {
            mesh_data_t tmp_data = {0x0};

            if (transmit_data) {
                memcpy(transmit_data + data_head->transmit_num * MWIFI_ADDR_LEN, mesh_data->data, mesh_data->size);
                tmp_data.size = mesh_data->size + data_head->transmit_num * MWIFI_ADDR_LEN;
                tmp_data.data = transmit_data;
            } else {
                tmp_data.size = mesh_data->size;
                tmp_data.data = mesh_data->data;
            }

            MDF_LOGV("mesh_data->size: %d, transmit_num: %d, child_addr: " MACSTR,
                     mesh_data->size, data_head->transmit_num, MAC2STR(child_addr->addr));

            /**< Fragmenting packets for transmission */
            ret = mwifi_subcontract_write(child_addr, &tmp_data, data_flag, mesh_opt);
            MDF_ERROR_BREAK(ret != ESP_OK, "<%s> Root node failed to send packets, dest_mac: "MACSTR,
                            mdf_err_to_name(ret), MAC2STR(child_addr->addr));
        }
    }

    MDF_FREE(transmit_data);

    /**
     * @brief Prevent topology changes during the process of sending packets,
     *        such as: the child node becomes the parent node and cannot be found.
     */
    if (addrs_num > 0) {
        data_flag = MESH_DATA_P2P | MESH_DATA_DROP;
        data_head->transmit_num = 0;

        /**
         * @brief When sending to all devices, the device need to send it to the root node itself.
         */
        if (MWIFI_ADDR_IS_ANY(addrs_list->addr) && data_head->transmit_all && esp_mesh_is_root()) {
            ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, (uint8_t *)addrs_list));
            data_head->transmit_all = false;
        }
    }

    for (int i = 0; i < addrs_num && !data_head->transmit_all; ++i) {
        data_head->transmit_self = true;

        /**< Fragmenting packets for transmission */
        ret = mwifi_subcontract_write(addrs_list + i, mesh_data, data_flag, mesh_opt);
        MDF_ERROR_CONTINUE(ret != ESP_OK, "<%s> Root node failed to send packets, dest_mac: "MACSTR,
                           mdf_err_to_name(ret), MAC2STR((addrs_list + i)->addr));
    }

    return ret;
}

mdf_err_t mwifi_write(const uint8_t *dest_addrs, const mwifi_data_type_t *data_type,
                      const void *data, size_t size, bool block)
{
    MDF_PARAM_CHECK(data_type);
    MDF_PARAM_CHECK(data);
    MDF_PARAM_CHECK(size > 0 && size < 8096);
    MDF_PARAM_CHECK(!dest_addrs || !MWIFI_ADDR_IS_EMPTY(dest_addrs));
    MDF_ERROR_CHECK(!mwifi_is_started(), MDF_ERR_MWIFI_NOT_START, "Mwifi isn't started");

    mdf_err_t ret          = MDF_OK;
    int data_flag          = 0;
    mz_ulong compress_size = 0;
    uint8_t *compress_data = NULL;
    uint8_t root_addr[]    = MWIFI_ADDR_ROOT;
    uint8_t empty_addr[]   = MWIFI_ADDR_NONE;
    uint8_t addr_any[]     = MWIFI_ADDR_ANY;
    uint8_t addr_broadcast[] = MWIFI_ADDR_BROADCAST;
    uint8_t self_addr[MWIFI_ADDR_LEN] = {0};

    /**
     * @brief  If the destination address is NULL, it is received by the mwifi_root_read of the root node.
     *         If the destination address is MWIFI_ADDR_ROOT, it is received by the mwifi_read of the root node.
     */
    bool to_root = !dest_addrs ? true : false;
    dest_addrs   = dest_addrs && !memcmp(dest_addrs, root_addr, MWIFI_ADDR_LEN) ? empty_addr : dest_addrs;
    dest_addrs   = to_root ? root_addr : dest_addrs;

    mwifi_data_head_t data_head = {0x0};
    mesh_data_t mesh_data       = {
        .tos   = data_type->communicate == MWIFI_COMMUNICATE_BROADCAST ? MESH_TOS_DEF : MESH_TOS_P2P,
        .data  = (uint8_t *)data,
        .size  = size,
    };
    mesh_opt_t mesh_opt   = {
        .len  = sizeof(mwifi_data_head_t),
        .val  = (void *) &data_head,
        .type = MESH_OPT_RECV_DS_ADDR,
    };

    /**< flag  bitmap for data sent */
    data_flag = (to_root) ? MESH_DATA_TODS : MESH_DATA_P2P;
    data_flag = (data_type->communicate == MWIFI_COMMUNICATE_BROADCAST && data_type->group) ? MESH_DATA_GROUP : data_flag;
    data_flag = (g_init_config->data_drop_enable) ? data_flag | MESH_DATA_DROP : data_flag;
    data_flag = (!block) ? data_flag | MESH_DATA_NONBLOCK : data_flag;
    data_head.transmit_self = true;
    memcpy(&data_head.type, data_type, sizeof(mwifi_data_type_t));
    MDF_ERROR_CHECK(to_root && g_rootless_flag, MDF_ERR_MWIFI_NO_ROOT, "Current network has no root");

    if (!data_type->group && data_type->communicate == MWIFI_COMMUNICATE_BROADCAST
            && !memcmp(dest_addrs, addr_broadcast, MWIFI_ADDR_LEN)) {
        dest_addrs = addr_any;
    }

    if (!to_root && data_type->communicate != MWIFI_COMMUNICATE_BROADCAST) {
        if (!data_head.type.group && (!memcmp(dest_addrs, addr_broadcast, MWIFI_ADDR_LEN)
                                      || !memcmp(dest_addrs, addr_any, MWIFI_ADDR_LEN))) {
            data_head.type.communicate = MWIFI_COMMUNICATE_MULTICAST;
            data_head.type.group = true;

            if (!memcmp(dest_addrs, addr_broadcast, MWIFI_ADDR_LEN)) {
                esp_wifi_get_mac(ESP_IF_WIFI_STA, self_addr);
                dest_addrs = self_addr;
            }
        } else {
            data_head.type.communicate = MWIFI_COMMUNICATE_UNICAST;
        }
    }

    MDF_LOGD("esp_mesh_send dest_addr: " MACSTR ", mesh_data.size: %d, data: %.*s",
             MAC2STR(dest_addrs), mesh_data.size, mesh_data.size, mesh_data.data);

    /**
     * @brief data compression
     */
    if (data_head.type.compression) {
        ret = MDF_ERR_NO_MEM;
        compress_size = compressBound(size);
        compress_data = MDF_MALLOC(compress_size);
        MDF_ERROR_GOTO(!compress_data, EXIT, "");

        ret = compress(compress_data, &compress_size, mesh_data.data, mesh_data.size);
        MDF_ERROR_GOTO(ret != MZ_OK, EXIT, "Compressed whitelist failed, ret: 0x%x", -ret);
        MDF_LOGD("compress, size: %zu, compress_size: %d, rate: %d%%",
                 size, (int)compress_size, (int)compress_size * 100 / size);

        if (compress_size > size) {
            data_head.type.compression = false;
        } else {
            data_head.compress_rate = (size / compress_size + 1) >= 15 ? 15 : (size / compress_size + 1);
            mesh_data.data = compress_data;
            mesh_data.size = compress_size;
        }
    }

    /**< Send a package as a group */
    if (!to_root && data_head.type.group && data_type->communicate != MWIFI_COMMUNICATE_BROADCAST) {
        ret = MDF_ERR_NO_MEM;
        uint8_t *group_data = MDF_MALLOC(mesh_data.size + MWIFI_ADDR_LEN);
        MDF_ERROR_GOTO(!group_data, EXIT, "");

        memcpy(group_data, dest_addrs, MWIFI_ADDR_LEN);
        memcpy(group_data + MWIFI_ADDR_LEN, mesh_data.data, mesh_data.size);
        mesh_data.tos  = MESH_TOS_P2P;
        mesh_data.size += MWIFI_ADDR_LEN;
        mesh_data.data = group_data;
        data_head.transmit_num = 1;
        data_head.transmit_all = true;
        dest_addrs = empty_addr;

        MDF_FREE(compress_data);
        compress_data = group_data;
    }

    data_head.total_size_hight = mesh_data.size >> 12;
    data_head.total_size_low   = mesh_data.size & 0xFFF;

    /**< Fragmenting packets for transmission */
    ret = mwifi_subcontract_write((mesh_addr_t *)dest_addrs, &mesh_data, data_flag, &mesh_opt);
    MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "<%s> Node failed to send packets, data_flag: 0x%x, dest_mac: " MACSTR,
                   mdf_err_to_name(ret), data_flag, MAC2STR(dest_addrs));

EXIT:
    MDF_FREE(compress_data);
    return ret;
}

mdf_err_t __mwifi_read(uint8_t *src_addr, mwifi_data_type_t *data_type,
                       void *data, size_t *size, TickType_t wait_ticks,
                       uint8_t type)
{
    MDF_PARAM_CHECK(src_addr);
    MDF_PARAM_CHECK(data_type);
    MDF_PARAM_CHECK(data);
    MDF_PARAM_CHECK(size);
    MDF_PARAM_CHECK(type == MWIFI_DATA_MEMORY_MALLOC_INTERNAL || *size > 0);
    MDF_ERROR_CHECK(!mwifi_is_started(), MDF_ERR_MWIFI_NOT_START, "Mwifi isn't started");
    MDF_ERROR_CHECK(type != MWIFI_DATA_MEMORY_MALLOC_EXTERNAL && type != MWIFI_DATA_MEMORY_MALLOC_INTERNAL, MDF_ERR_INVALID_ARG,
                    "To apply for buffer space externally, set the type of the data parameter to be (char *) or (uint8_t *)\n"
                    "To apply for buffer space internally, set the type of the data parameter to be (char **) or (uint8_t **)");

    mdf_err_t ret                = MDF_OK;
    uint8_t *recv_data           = NULL;
    size_t recv_size             = 0;
    size_t total_size            = 0;
    int data_flag                = 0;
    bool self_data_flag          = false;
    static uint32_t s_data_magic = 0;
    TickType_t start_ticks       = xTaskGetTickCount();
    mwifi_data_head_t data_head  = {0x0};
    mesh_data_t mesh_data        = {0x0};
    mesh_opt_t mesh_opt          = {
        .len  = sizeof(mwifi_data_head_t),
        .val  = (void *) &data_head,
        .type = MESH_OPT_RECV_DS_ADDR,
    };

    for (;;) {
        recv_size  = 0;
        total_size = 0;
        recv_data  = MDF_REALLOC_RETRY(recv_data, MWIFI_PAYLOAD_LEN);

        for (int expect_seq = 0, recv_ticks = 0; !recv_size || recv_size < total_size; expect_seq++) {
            mesh_data.size = recv_size ? total_size - recv_size : MWIFI_PAYLOAD_LEN;
            mesh_data.data = recv_data + recv_size;
            recv_ticks     = (wait_ticks == portMAX_DELAY) ? portMAX_DELAY :
                             xTaskGetTickCount() - start_ticks < wait_ticks ?
                             wait_ticks - (xTaskGetTickCount() - start_ticks) : 0;

            MDF_LOGV("wait_ticks: %d, start_ticks: %d, recv_ticks: %d", wait_ticks, start_ticks, recv_ticks);

            /**< Receive a packet targeted to self over the mesh network */
            ret = esp_mesh_recv((mesh_addr_t *)src_addr, &mesh_data, recv_ticks * portTICK_RATE_MS,
                                &data_flag, &mesh_opt, 1);
            MDF_LOGV("esp_mesh_recv, src_addr: " MACSTR ", size: %d, data: %.*s",
                     MAC2STR(src_addr), mesh_data.size, mesh_data.size, mesh_data.data);

            if (ret == ESP_ERR_MESH_NOT_START) {
                MDF_LOGW("<ESP_ERR_MESH_NOT_START> Node failed to receive packets");
                vTaskDelay(100 / portTICK_RATE_MS);
                continue;
            } else if (ret == ESP_ERR_MESH_TIMEOUT) {
                MDF_LOGD("<MDF_ERR_MWIFI_TIMEOUT> Node failed to receive packets");
                goto EXIT;
            } else if (ret != ESP_OK) {
                MDF_LOGW("<%s> Node failed to receive packets", mdf_err_to_name(ret));
                goto EXIT;
            }

            /**
             * @brief Discard this packet if there is a packet loss in the middle
             */
            if (data_head.packet_seq != expect_seq) {
                MDF_LOGW("Part of the packet is lost, expect_seq: %d, recv_seq: %d",
                         expect_seq, data_head.packet_seq);

                recv_size  = 0;
                expect_seq = -1;

                if (data_head.packet_seq != 0) {
                    continue;
                }

                memcpy(recv_data, mesh_data.data, mesh_data.size);
            }

            /**
             * @brief Filter retransmitted packets
             */
            if (data_head.magic == s_data_magic) {
                expect_seq--;
                MDF_LOGD("Received duplicate packets, sequence: %d", s_data_magic);
                continue;
            }

            s_data_magic = data_head.magic;
            total_size   = (data_head.total_size_hight << 12) + data_head.total_size_low;
            recv_size   += mesh_data.size;
            recv_data    = MDF_REALLOC_RETRY(recv_data, total_size);
        }

        self_data_flag = data_head.transmit_self;
        mesh_data.data = recv_data;
        mesh_data.size = recv_size;

        /**
         * @brief Existing data needs to be forwarded
         */
        if (data_head.transmit_num || data_head.transmit_all) {
            mesh_addr_t *transmit_addr       = NULL;
            size_t transmit_num              = data_head.transmit_num;
            uint8_t addr_any[MWIFI_ADDR_LEN] = MWIFI_ADDR_BROADCAST;
            data_flag = MESH_DATA_P2P;

            if (data_head.transmit_all) {
                transmit_num  = 1;
                transmit_addr = (mesh_addr_t *)addr_any;
            } else {
                transmit_addr  = (mesh_addr_t *)recv_data;
                mesh_data.data = recv_data + data_head.transmit_num * MWIFI_ADDR_LEN;
                mesh_data.size = recv_size - data_head.transmit_num * MWIFI_ADDR_LEN;
            }

            MDF_LOGV("Data forwarding, size: %d, recv_size: %d, flag: %d, transmit_num: %d, data: %.*s",
                     mesh_data.size, recv_size, data_flag, data_head.transmit_num, mesh_data.size, mesh_data.data);

            /**< Multicast forwarding */
            ret = mwifi_transmit_write(transmit_addr, transmit_num, &mesh_data,
                                       data_flag, &mesh_opt);
            MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "<%s> mwifi_root_write, size: %d",
                           mdf_err_to_name(ret), mesh_data.size);
        }

        /**
         * @brief If this data typde is group and my is in this destination group, receive this data.
         */
        if (data_head.type.group && data_head.type.communicate != MWIFI_COMMUNICATE_BROADCAST) {
            uint8_t self_addr[MWIFI_ADDR_LEN] = {0};
            esp_wifi_get_mac(ESP_IF_WIFI_STA, self_addr);

            if ((data_head.type.communicate == MWIFI_COMMUNICATE_UNICAST
                    && esp_mesh_is_my_group((mesh_addr_t *)mesh_data.data))
                    || (data_head.type.communicate == MWIFI_COMMUNICATE_MULTICAST
                        && memcmp(mesh_data.data, self_addr, MWIFI_ADDR_LEN))) {
                mesh_data.data += MWIFI_ADDR_LEN;
                mesh_data.size -= MWIFI_ADDR_LEN;
            } else {
                self_data_flag = false;
            }
        }

        if (self_data_flag) {
            break;
        }
    }

    memcpy(data_type, &data_head.type, sizeof(mwifi_data_type_t));

    if (data_type->compression) {
        int mz_ret  = MZ_OK;
        int mz_rate = data_head.compress_rate;

        if (type == MWIFI_DATA_MEMORY_MALLOC_INTERNAL) {
            do {
                mz_rate = (!mz_rate) ? 5 : mz_rate;
                *size = mesh_data.size * mz_rate;
                *((uint8_t **)data) = MDF_REALLOC_RETRY(*((uint8_t **)data), *size);
                mz_ret = uncompress(*((uint8_t **)data), (mz_ulong *)size, mesh_data.data, mesh_data.size);
                mz_rate += 2;
                ret = MDF_FAIL;
            } while (mz_ret == MZ_BUF_ERROR);

            if (mz_ret != MZ_OK) {
                ret = MDF_FAIL;
                MDF_LOGW("<%s> Uncompress, size: %d", mz_error(mz_ret), mesh_data.size);
                MDF_FREE(*((uint8_t **)data));
                goto EXIT;
            }
        } else {
            mz_ret = uncompress((uint8_t *)data, (mz_ulong *)size, mesh_data.data, mesh_data.size);
            ret = (mz_ret == MZ_BUF_ERROR) ? MDF_ERR_BUF : MDF_FAIL;
            MDF_ERROR_GOTO(mz_ret != MZ_OK, EXIT, "<%s> Uncompress, size: %d", mz_error(mz_ret), mesh_data.size);
        }
    } else {
        if (type == MWIFI_DATA_MEMORY_MALLOC_INTERNAL) {
            *size = mesh_data.size;
            *((uint8_t **)data) = MDF_REALLOC_RETRY(NULL, mesh_data.size);
            memcpy(*((uint8_t **)data), mesh_data.data, mesh_data.size);
        } else {
            ret = (*size < mesh_data.size) ? MDF_ERR_BUF : MDF_FAIL;
            MDF_ERROR_GOTO(*size < mesh_data.size, EXIT,
                           "Buffer is too small, size: %d, the expected size is: %d", *size, mesh_data.size);
            *size = mesh_data.size;
            memcpy(data, mesh_data.data, mesh_data.size);
        }
    }

    ret = MDF_OK;
    MDF_LOGD("esp_mesh_recv, src_addr: " MACSTR ", size: %d, data: %.*s",
             MAC2STR(src_addr), *size, *size, (type == MWIFI_DATA_MEMORY_MALLOC_INTERNAL) ? * ((char **)data) : (char *)data);

EXIT:
    MDF_FREE(recv_data);
    return ret;
}

mdf_err_t mwifi_root_write(const uint8_t *addrs_list, size_t addrs_num,
                           const mwifi_data_type_t *data_type, const void *data,
                           size_t size, bool block)
{
    MDF_PARAM_CHECK(addrs_list);
    MDF_PARAM_CHECK(addrs_num > 0);
    MDF_PARAM_CHECK(data_type);
    MDF_PARAM_CHECK(data);
    MDF_PARAM_CHECK(!MWIFI_ADDR_IS_EMPTY(addrs_list));
    MDF_PARAM_CHECK(size > 0 && size < 8096);
    MDF_ERROR_CHECK(!mwifi_is_started(), MDF_ERR_MWIFI_NOT_START, "Mwifi isn't started");

    mdf_err_t ret = MDF_OK;
    int data_flag = MESH_DATA_FROMDS;
    uint8_t *compress_data = NULL;
    uint8_t *tmp_addrs = NULL;
    mwifi_data_head_t data_head = {
        .transmit_self = true,
    };
    mesh_data_t mesh_data = {
        .tos   = !block || !g_init_config->retransmit_enable ? MESH_TOS_DEF : MESH_TOS_P2P,
        .data  = (uint8_t *)data,
        .size  = size,
    };
    mesh_opt_t mesh_opt   = {
        .len  = sizeof(mwifi_data_head_t),
        .val  = (void *) &data_head,
        .type = MESH_OPT_RECV_DS_ADDR,
    };

    data_flag = (!block) ? data_flag | MESH_DATA_NONBLOCK : data_flag;
    memcpy(&data_head.type, data_type, sizeof(mwifi_data_type_t));

    /**
     * @brief If destination adress is group address, will forward to each device in address list.
     */
    if (data_head.type.group && data_type->communicate != MWIFI_COMMUNICATE_BROADCAST) {
        for (int i = 0; i < addrs_num; ++i) {
            MDF_LOGD("count: %d, dest_addr: " MACSTR ", mesh_data.size: %d, data: %.*s",
                     i, MAC2STR(addrs_list + 6 * i), mesh_data.size, mesh_data.size, mesh_data.data);
            ret = mwifi_write(addrs_list + 6 * i, data_type, data, size, block);
            MDF_ERROR_BREAK(ret != ESP_OK, "<%s> Root node failed to send packets, dest_mac: " MACSTR,
                            mdf_err_to_name(ret), MAC2STR(addrs_list));
        }

        return ret;
    }

    /**
     * @brief data compression
     */
    if (data_head.type.compression) {
        ret = MDF_ERR_NO_MEM;
        mz_ulong compress_size = compressBound(size);
        compress_data = MDF_MALLOC(compress_size);
        MDF_ERROR_GOTO(!compress_data, EXIT, "");

        ret = compress(compress_data, &compress_size, (uint8_t *)data, size);
        MDF_ERROR_GOTO(ret != MZ_OK, EXIT, "Compressed whitelist failed, ret: 0x%x", -ret);

        MDF_LOGD("compress, size: %d, compress_size: %d, rate: %d%%",
                 size, (int)compress_size, (int)compress_size * 100 / size);

        if (compress_size > size) {
            data_head.type.compression = false;
        } else {
            mesh_data.data = compress_data;
            mesh_data.size = compress_size;
            data_head.compress_rate = (size / compress_size + 1) >= 15 ? 15 : (size / compress_size + 1);
        }
    }

    if (data_type->communicate == MWIFI_COMMUNICATE_UNICAST) {
        /**
         * @brief If destination adress is any device or other device expect root, get all nodes adress.
         */
        if (MWIFI_ADDR_IS_ANY(addrs_list) || MWIFI_ADDR_IS_BROADCAST(addrs_list)) {
            ret = MDF_ERR_NO_MEM;
            addrs_num  = esp_mesh_get_routing_table_size();
            tmp_addrs = MDF_MALLOC(addrs_num * sizeof(mesh_addr_t));
            MDF_ERROR_GOTO(!tmp_addrs, EXIT, "");
            ESP_ERROR_CHECK(esp_mesh_get_routing_table((mesh_addr_t *)tmp_addrs,
                            addrs_num * sizeof(mesh_addr_t), (int *)&addrs_num));

            if (MWIFI_ADDR_IS_BROADCAST(addrs_list)) {
                uint8_t root_mac[6] = {0x0};
                ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, root_mac));
                addrs_remove((mesh_addr_t *)tmp_addrs, &addrs_num, (mesh_addr_t *)root_mac);
                MDF_ERROR_GOTO(addrs_num > 2048 || addrs_num <= 0, EXIT, "dest_addrs_num: %d", addrs_num);
            }

            addrs_list = tmp_addrs;
        }

        /**
         * @brief Send each device by p2p
         */
        for (int i = 0; i < addrs_num; ++i) {
            MDF_LOGD("count: %d, dest_addr: " MACSTR" mesh_data.size: %d, data: %.*s",
                     i, MAC2STR(addrs_list + 6 * i), mesh_data.size, mesh_data.size, mesh_data.data);

            /**< Fragmenting packets for transmission */
            ret = mwifi_subcontract_write((mesh_addr_t *)addrs_list + i, &mesh_data, data_flag, &mesh_opt);
            MDF_ERROR_BREAK(ret != ESP_OK, "<%s> Root node failed to send packets, dest_mac: "MACSTR,
                            mdf_err_to_name(ret), MAC2STR(addrs_list));
        }
    } else if (data_type->communicate == MWIFI_COMMUNICATE_MULTICAST) {
        ret = MDF_ERR_NO_MEM;
        tmp_addrs = MDF_MALLOC(addrs_num * sizeof(mesh_addr_t));
        MDF_ERROR_GOTO(!tmp_addrs, EXIT, "");
        memcpy(tmp_addrs, addrs_list, addrs_num * sizeof(mesh_addr_t));
        MDF_LOGD("addrs_num: %d, addrs_list: " MACSTR ", mesh_data.size: %d",
                 addrs_num, MAC2STR(tmp_addrs), mesh_data.size);

        /**< Multicast forwarding */
        ret = mwifi_transmit_write((mesh_addr_t *)tmp_addrs, addrs_num, &mesh_data,
                                   data_flag, &mesh_opt);
        MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "Mwifi_transmit_write");
    } else if (data_type->communicate == MWIFI_COMMUNICATE_BROADCAST && addrs_num == 1) {

        /**< Fragmenting packets for transmission */
        ret = mwifi_subcontract_write((mesh_addr_t *)addrs_list, &mesh_data, data_flag, &mesh_opt);
        MDF_ERROR_GOTO(ret != ESP_OK, EXIT, "<%s> Root node failed to send packets, dest_mac: " MACSTR,
                       mdf_err_to_name(ret), MAC2STR(addrs_list));
    } else {
        MDF_LOGE("<MDF_ERR_NOT_SUPPORTED> The current version does not support broadcasting.");
        return MDF_ERR_NOT_SUPPORTED;
    }

EXIT:
    MDF_FREE(tmp_addrs);
    MDF_FREE(compress_data);
    return ret;
}

mdf_err_t __mwifi_root_read(uint8_t *src_addr, mwifi_data_type_t *data_type,
                            void *data, size_t *size, TickType_t wait_ticks, uint8_t type)
{
    MDF_PARAM_CHECK(src_addr);
    MDF_PARAM_CHECK(data_type);
    MDF_PARAM_CHECK(data);
    MDF_PARAM_CHECK(size);
    MDF_PARAM_CHECK(type == MWIFI_DATA_MEMORY_MALLOC_INTERNAL || *size > 0);
    MDF_ERROR_CHECK(!mwifi_is_started(), MDF_ERR_MWIFI_NOT_START, "Mwifi isn't started");
    MDF_ERROR_CHECK(type != MWIFI_DATA_MEMORY_MALLOC_EXTERNAL && type != MWIFI_DATA_MEMORY_MALLOC_INTERNAL, MDF_ERR_INVALID_ARG,
                    "To apply for buffer space externally, set the type of the data parameter to be (char *) or (uint8_t *)\n"
                    "To apply for buffer space internally, set the type of the data parameter to be (char **) or (uint8_t **)");

    mdf_err_t ret               = MDF_OK;
    int data_flag               = 0;
    mesh_addr_t dest_addr       = {0};
    mwifi_data_head_t data_head = {0x0};
    TickType_t start_ticks      = xTaskGetTickCount();
    ssize_t recv_size           = 0;
    uint8_t *recv_data          = MDF_REALLOC_RETRY(NULL, MWIFI_PAYLOAD_LEN);
    size_t total_size           = 0;

    mesh_data_t mesh_data = {0x0};
    mesh_opt_t mesh_opt   = {
        .len  = sizeof(mwifi_data_head_t),
        .val  = (void *) &data_head,
        .type = MESH_OPT_RECV_DS_ADDR,
    };

    for (int expect_seq = 0, recv_ticks = 0; !expect_seq || recv_size < total_size; expect_seq++) {
        mesh_data.size = !expect_seq ? MWIFI_PAYLOAD_LEN : total_size - recv_size;
        mesh_data.data = recv_data + recv_size;
        recv_ticks     = (wait_ticks == portMAX_DELAY) ? portMAX_DELAY :
                         xTaskGetTickCount() - start_ticks < wait_ticks ?
                         wait_ticks - (xTaskGetTickCount() - start_ticks) : 0;

        MDF_LOGV("wait_ticks: %d, start_ticks: %d, recv_ticks: %d", wait_ticks, start_ticks, recv_ticks);

        /**< Receive a packet targeted to external IP network */
        ret = esp_mesh_recv_toDS((mesh_addr_t *)src_addr, &dest_addr,
                                 &mesh_data, recv_ticks * portTICK_RATE_MS, &data_flag, &mesh_opt, 1);

        if (ret == ESP_ERR_MESH_NOT_START) {
            MDF_LOGW("<ESP_ERR_MESH_NOT_START> Node failed to receive packets");
            vTaskDelay(100 / portTICK_RATE_MS);
            continue;
        } else if (ret == ESP_ERR_MESH_TIMEOUT) {
            MDF_LOGD("<MDF_ERR_MWIFI_TIMEOUT> Node failed to receive packets");
            goto EXIT;
        }

        MDF_ERROR_GOTO(ret != ESP_OK || mesh_data.size <= 0, EXIT, "<%s> Node failed to receive packets", mdf_err_to_name(ret));

        /**
         * @brief Discard this packet if there is a packet loss in the middle
         */
        if (data_head.packet_seq != expect_seq) {
            MDF_LOGW("Part of the packet is lost, expect_seq: %d, recv_seq: %d",
                     expect_seq, data_head.packet_seq);

            recv_size  = 0;
            expect_seq = -1;

            if (data_head.packet_seq != 0) {
                continue;
            }

            memcpy(recv_data, mesh_data.data, mesh_data.size);
        }

        total_size = (data_head.total_size_hight << 12) + data_head.total_size_low;
        recv_size += mesh_data.size;
        recv_data  = MDF_REALLOC_RETRY(recv_data, total_size);
    }

    memcpy(data_type, &data_head.type, sizeof(mwifi_data_type_t));

    if (data_type->compression) {
        int mz_ret  = MZ_OK;
        int mz_rate = data_head.compress_rate;

        if (type == MWIFI_DATA_MEMORY_MALLOC_INTERNAL) {
            do {
                mz_rate = (!mz_rate) ? 5 : mz_rate;
                *size = recv_size * mz_rate;
                *((uint8_t **)data) = MDF_REALLOC_RETRY(*((uint8_t **)data), *size);
                mz_ret = uncompress(*((uint8_t **)data), (mz_ulong *)size, recv_data, recv_size);
                mz_rate += 2;
                ret = MDF_FAIL;
            } while (mz_ret == MZ_BUF_ERROR);

            if (mz_ret != MZ_OK) {
                ret = MDF_FAIL;
                MDF_LOGW("<%s> Uncompress, size: %d", mz_error(mz_ret), *size);
                MDF_FREE(*((uint8_t **)data));
                goto EXIT;
            }
        } else {
            mz_ret = uncompress((uint8_t *)data, (mz_ulong *)size, recv_data, recv_size);
            ret = (mz_ret == MZ_BUF_ERROR) ? MDF_ERR_BUF : MDF_FAIL;
            MDF_ERROR_GOTO(mz_ret != MZ_OK, EXIT, "<%s> Uncompress, size: %d", mz_error(mz_ret), recv_size);
        }
    } else {
        if (type == MWIFI_DATA_MEMORY_MALLOC_INTERNAL) {
            *size = recv_size;
            *((uint8_t **)data) = MDF_REALLOC_RETRY(NULL, recv_size);
            memcpy(*((uint8_t **)data), recv_data, recv_size);
        } else {
            ret = (*size < recv_size) ? MDF_ERR_BUF : MDF_FAIL;
            MDF_ERROR_GOTO(*size < recv_size, EXIT,
                           "Buffer is too small, size: %d, the expected size is: %d", *size, recv_size);
            *size = recv_size;
            memcpy(data, recv_data, recv_size);
        }
    }

    ret = MDF_OK;
    MDF_LOGD("esp_mesh_recv_toDS, src_addr: " MACSTR ", size: %d, data: %.*s",
             MAC2STR(src_addr), *size, *size, (type == MWIFI_DATA_MEMORY_MALLOC_INTERNAL) ? * ((char **)data) : (char *)data);

EXIT:
    MDF_FREE(recv_data);
    return ret;
}

int8_t mwifi_get_parent_rssi()
{
    wifi_ap_record_t ap_info = {0};
    return !esp_wifi_sta_get_ap_info(&ap_info) && ap_info.rssi != 0 ? ap_info.rssi : -120;
}
