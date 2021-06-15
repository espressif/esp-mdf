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

#ifndef __MWIFI_H__
#define __MWIFI_H__

#include "mdf_common.h"
#include "esp_mesh.h"
#include "esp_mesh_internal.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

#define MWIFI_PAYLOAD_LEN       (1456) /**< Max payload size(in bytes) */

#define MWIFI_ADDR_LEN          (6) /**< Length of MAC address */
#define MWIFI_ADDR_NONE         {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}
#define MWIFI_ADDR_ROOT         {0xFF, 0x0, 0x0, 0x1, 0x0, 0x0}
#define MWIFI_ADDR_ANY          {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} /**< All node in the mesh network */
#define MWIFI_ADDR_BROADCAST    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0XFE} /**< Other node except the root */
#define MWIFI_ADDR_IS_EMPTY(addr) (((addr)[0] | (addr)[1] | (addr)[2] | (addr)[3] | (addr)[4] | (addr)[5]) == 0x0)
#define MWIFI_ADDR_IS_ANY(addr)   (((addr)[0] & (addr)[1] & (addr)[2] & (addr)[3] & (addr)[4] & (addr)[5]) == 0xFF)
#define MWIFI_ADDR_IS_BROADCAST(addr) (((addr)[0] & (addr)[1] & (addr)[2] & (addr)[3] & (addr)[4]) == 0xFF && (addr)[5] == 0xFE)

/**
 * @brief Mwifi error code definition
 */
#define MDF_ERR_MWIFI_NOT_INIT                  (MDF_ERR_MWIFI_BASE + 1)  /**< Mwifi isn't initialized */
#define MDF_ERR_MWIFI_INITED                    (MDF_ERR_MWIFI_BASE + 2)  /**< Mwifi has been initialized */
#define MDF_ERR_MWIFI_NOT_START                 (MDF_ERR_MWIFI_BASE + 3)  /**< Mwifi isn't started */
#define MDF_ERR_MWIFI_ARGUMENT                  (MDF_ERR_MWIFI_BASE + 4)  /**< Illegal argument */
#define MDF_ERR_MWIFI_EXCEED_PAYLOAD            (MDF_ERR_MWIFI_BASE + 5)  /**< Packet size exceeds payload */
#define MDF_ERR_MWIFI_TIMEOUT                   (MDF_ERR_MWIFI_BASE + 6)  /**< Timeout */
#define MDF_ERR_MWIFI_DISCONNECTED              (MDF_ERR_MWIFI_BASE + 7)  /**< Disconnected with parent on station interface */
#define MDF_ERR_MWIFI_NO_CONFIG                 (MDF_ERR_MWIFI_BASE + 8)  /**< Router not configured */
#define MDF_ERR_MWIFI_NO_FOUND                  (MDF_ERR_MWIFI_BASE + 9)  /**< Routes or devices not found */
#define MDF_ERR_MWIFI_NO_ROOT                   (MDF_ERR_MWIFI_BASE + 10)  /**< Routes or devices not found */

/**
 * @brief enumerated list of Mwifi event id
 */
#define MDF_EVENT_MWIFI_STARTED                 (MESH_EVENT_STARTED)            /**< Mwifi is started */
#define MDF_EVENT_MWIFI_STOPPED                 (MESH_EVENT_STOPPED)            /**< Mwifi is stopped */
#define MDF_EVENT_MWIFI_CHANNEL_SWITCH          (MESH_EVENT_CHANNEL_SWITCH)     /**< Channel switch */
#define MDF_EVENT_MWIFI_CHILD_CONNECTED         (MESH_EVENT_CHILD_CONNECTED)    /**< A child is connected on softAP interface */
#define MDF_EVENT_MWIFI_CHILD_DISCONNECTED      (MESH_EVENT_CHILD_DISCONNECTED) /**< A child is disconnected on softAP interface */
#define MDF_EVENT_MWIFI_ROUTING_TABLE_ADD       (MESH_EVENT_ROUTING_TABLE_ADD)  /**< Routing table is changed by adding newly joined children */
#define MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE    (MESH_EVENT_ROUTING_TABLE_REMOVE) /**< Routing table is changed by removing leave children */
#define MDF_EVENT_MWIFI_PARENT_CONNECTED        (MESH_EVENT_PARENT_CONNECTED)   /**< Parent is connected on station interface */
#define MDF_EVENT_MWIFI_PARENT_DISCONNECTED     (MESH_EVENT_PARENT_DISCONNECTED) /**< Parent is disconnected on station interface */
#define MDF_EVENT_MWIFI_NO_PARENT_FOUND         (MESH_EVENT_NO_PARENT_FOUND)    /**< No parent found */
#define MDF_EVENT_MWIFI_LAYER_CHANGE            (MESH_EVENT_LAYER_CHANGE)       /**< Layer changes over the Mwifi network */
#define MDF_EVENT_MWIFI_TODS_STATE              (MESH_EVENT_TODS_STATE)         /**< State represents if root is able to access external IP network */
#define MDF_EVENT_MWIFI_VOTE_STARTED            (MESH_EVENT_VOTE_STARTED)       /**< The process of voting a new root is started either by children or by root */
#define MDF_EVENT_MWIFI_VOTE_STOPPED            (MESH_EVENT_VOTE_STOPPED)       /**< The process of voting a new root is stopped */
#define MDF_EVENT_MWIFI_ROOT_ADDRESS            (MESH_EVENT_ROOT_ADDRESS)       /**< The root address is obtained. It is posted by Mwifi stack automatically. */
#define MDF_EVENT_MWIFI_ROOT_SWITCH_REQ         (MESH_EVENT_ROOT_SWITCH_REQ)    /**< Root switch request sent from a new voted root candidate */
#define MDF_EVENT_MWIFI_ROOT_SWITCH_ACK         (MESH_EVENT_ROOT_SWITCH_ACK)    /**< Root switch acknowledgment responds the above request sent from current root */
#define MDF_EVENT_MWIFI_ROOT_ASKED_YIELD        (MESH_EVENT_ROOT_ASKED_YIELD)   /**< Root is asked yield by a more powerful existing root. */

#define MDF_EVENT_MWIFI_SCAN_DONE               (MESH_EVENT_SCAN_DONE)          /**< if self-organized networking is disabled */
#define MDF_EVENT_MWIFI_NETWORK_STATE           (MESH_EVENT_NETWORK_STATE)      /**< network state, such as whether current mesh network has a root. */
#define MDF_EVENT_MWIFI_STOP_RECONNECTION       (MESH_EVENT_STOP_RECONNECTION)  /**< the root stops reconnecting to the router and non-root devices stop reconnecting to their parents. */
#define MDF_EVENT_MWIFI_FIND_NETWORK            (MESH_EVENT_FIND_NETWORK)       /**< when the channel field in mesh configuration is set to zero, mesh stack will perform a
                                                                                     full channel scan to find a mesh network that can join, and return the channel value
                                                                                     after finding it. */
#define MDF_EVENT_MWIFI_ROUTER_SWITCH           (MESH_EVENT_ROUTER_SWITCH)      /**< if users specify BSSID of the router in mesh configuration, when the root connects to another
                                                                                     router with the same SSID, this event will be posted and the new router information is attached. */

#define MDF_EVENT_MWIFI_CHANNEL_NO_FOUND        (64)                            /**< The router's channel is not set. */
#define MDF_EVENT_MWIFI_ROOT_GOT_IP             (64+1)
#define MDF_EVENT_MWIFI_ROOT_LOST_IP            (64+2)
#define MDF_EVENT_MWIFI_EXCEPTION               (64+3)                          /**< Some abnormal situations happen, eg. disconnected too many times */

/**
 * @brief  Mwifi initialization configuration
 */
typedef struct {
    /**
     * @brief Mesh root configuration
     */
    uint8_t vote_percentage;    /**< Vote percentage threshold above which the node becoms a root */
    uint8_t vote_max_count;     /**< Max multiple voting each device can have for the self-healing of a MESH network */
    int8_t backoff_rssi;        /**< RSSI threshold below which connections to the root node are not allowed */
    uint8_t scan_min_count;     /**< Minimum scan times before being a root */
    bool root_conflicts_enable; /**< Allow more than one root in one network */
    uint16_t root_healing_ms;   /**< Time lag between the moment a root node is disconnected from the network and the moment
                                     the devices start electing another root node  */

    /**
     * @brief Mesh network capacity configuration
     */
    uint16_t capacity_num;      /**< Network capacity, defining max number of devices allowed in the MESH network */
    uint8_t max_layer_deprecated; /**< Deprecated, shouldn't use it */
    uint8_t max_connection;     /**< Max number of MESH softAP connections */

    /**
     * @brief Mesh network stability configuration
     */
    uint16_t assoc_expire_ms;   /**< Period of time after which a MESH softAP breaks its association with inactive leaf nodes */
    uint16_t beacon_interval_ms;/**< Mesh softAP beacon interval */
    uint16_t passive_scan_ms;   /**< Mesh station passive scan duration */
    uint16_t monitor_duration_ms; /**<  Period (ms) for monitoring the parent's RSSI. If the signal stays weak throughout the
                                        period, the node will find another parent offering more stable connection */
    int8_t cnx_rssi;            /**< RSSI threshold above which the connection with a parent is considered strong */
    int8_t select_rssi;         /**< RSSI threshold for parent selection. Its value should be greater than switch_rssi */
    int8_t switch_rssi;         /**< RSSI threshold below which a node selects a parent with better RSSI */
    uint8_t attempt_count;      /**< Parent selection fail times, if the scan times reach this value,
                                     device will disconnect with associated children and join self-healing */
    uint8_t monitor_ie_count;   /**< Allowed number of changes a parent node can introduce into its information element (IE),
                                     before the leaf nodes must update their own IEs accordingly */

    /**
     * @brief Mesh network data transmission configuration
     */
    uint8_t xon_qsize;          /**< Number of MESH buffer queues */
    bool retransmit_enable;     /**< Enable a source node to retransmit data to the node from which it failed to receive ACK */
    bool data_drop_enable;      /**< If a root is changed, enable the new root to drop the previous packet */

    uint16_t max_layer;         /**< Max number of allowed layers */
    uint8_t topology;           /**< Topology of mesh network, can be tree or chain */
} mwifi_init_config_t;

#ifndef CONFIG_MWIFI_ROOT_CONFLICTS_ENABLE
#define CONFIG_MWIFI_ROOT_CONFLICTS_ENABLE false
#endif /**< CONFIG_MWIFI_ROOT_CONFLICTS_ENABLE */

#ifndef CONFIG_MWIFI_RETRANSMIT_ENABLE
#define CONFIG_MWIFI_RETRANSMIT_ENABLE false
#endif /**< CONFIG_MWIFI_RETRANSMIT_ENABLE */

#define MWIFI_INIT_CONFIG_DEFAULT() { \
        /**< .vote_percentage       =*/ CONFIG_MWIFI_VOTE_PERCENTAGE, \
        /**< .vote_max_count        =*/ CONFIG_MWIFI_VOTE_MAX_COUNT, \
        /**< .backoff_rssi          =*/ CONFIG_MWIFI_BACKOFF_RSSI, \
        /**< .scan_min_count        =*/ CONFIG_MWIFI_SCAN_MINI_COUNT, \
        /**< .root_conflicts_enable =*/ CONFIG_MWIFI_ROOT_CONFLICTS_ENABLE, \
        /**< .root_healing_ms       =*/ CONFIG_MWIFI_ROOT_HEALING_MS, \
        /**< .capacity_num          =*/ CONFIG_MWIFI_CAPACITY_NUM, \
        /**< .max_layer_deprecated  =*/ 0, \
        /**< .max_connection        =*/ CONFIG_MWIFI_MAX_CONNECTION, \
        /**< .assoc_expire_ms       =*/ CONFIG_MWIFI_ASSOC_EXPIRE_MS, \
        /**< .beacon_interval_ms    =*/ CONFIG_MWIFI_BEACON_INTERVAL_MS, \
        /**< .passive_scan_ms       =*/ CONFIG_MWIFI_PASSIVE_SCAN_MS, \
        /**< .monitor_duration_ms   =*/ CONFIG_MWIFI_MONITOR_DURATION_MS, \
        /**< .cnx_rssi              =*/ CONFIG_MWIFI_CNX_RSSI, \
        /**< .select_rssi           =*/ CONFIG_MWIFI_SELECT_RSSI, \
        /**< .switch_rssi           =*/ CONFIG_MWIFI_SWITCH_RSSI, \
        /**< .attempt_count         =*/ CONFIG_MWIFI_ATTEMPT_COUNT, \
        /**< .monitor_ie_count      =*/ CONFIG_MWIFI_MONITOR_IE_COUNT, \
        /**< .xon_qsize             =*/ CONFIG_MWIFI_XON_QSIZE, \
        /**< .retransmit_enable     =*/ CONFIG_MWIFI_RETRANSMIT_ENABLE, \
        /**< .data_drop_enable      =*/ CONFIG_MWIFI_DATA_DROP_ENABLE, \
        /**< .max_layer             =*/ CONFIG_MWIFI_MAX_LAYER, \
        /**< .topology              =*/ CONFIG_MWIFI_TOPOLOGY, \
    }

#define MWIFI_RSSI_THRESHOUD_DEFAULT() { \
        /*!< .high  =*/	 CONFIG_MWIFI_RSSI_THRESHOUD_HIGH, \
        /*!< medium =*/	 CONFIG_MWIFI_RSSI_THRESHOUD_MEDIUM, \
        /*!< low    =*/  CONFIG_MWIFI_RSSI_THRESHOUD_LOW, \
    }

/**
 * @brief Device type
 */
typedef enum {
    MWIFI_MESH_IDLE,    /**< hasn't joined the mesh network yet */
    MWIFI_MESH_ROOT,    /**< the only sink of the mesh network. Has the ability to access external IP network */
    MWIFI_MESH_NODE,    /**< intermediate device. Has the ability to forward packets over the mesh network */
    MWIFI_MESH_LEAF,    /**< has no forwarding ability */
    MWIFI_MESH_STA,     /**< connect to router with a standlone Wi-Fi station mode, no network expansion capability*/	
} node_type_t;

typedef uint8_t mwifi_node_type_t;

/**
 * @brief Mwifi AP configuration
 */
typedef struct {
    /**
     * @brief  Information about the router connected to the root.
     *         This item does not need to be set in the routerless scheme.
     */
    char router_ssid[32];     /**< SSID of the router */
    char router_password[64]; /**< Password of router */
    uint8_t router_bssid[6];  /**< BSSID, if this value is specified, users should also specify "router_switch_disable" */

    /**
     * @brief  Information about the mesh internal connected.
     */
    uint8_t mesh_id[6];     /**< Mesh network ID. Nodes sharing the same MESH ID can communicate with one another */
    char mesh_password[64]; /**< Password for secure communication between devices in a MESH network */
    mwifi_node_type_t mesh_type;      /**< Only MWIFI_MESH_IDLE, MWIFI_MESH_ROOT, MWIFI_MESH_NODE and MWIFI_MESH_LEAF device types are supported.
                                 Routerless solutions must be configured as MWIFI_MESH_ROOT or MWIFI_MESH_NODE
                                 MWIFI_MESH_IDLE: The node type is selected by MESH according to the network conditions.
                                 MWIFI_MESH_ROOT: The Device is the root node
                                 MWIFI_MESH_NODE: The device is a non-root node and contains intermediate nodes and leaf nodes.
                                 MWIFI_MESH_LEAF: The device is a leaf node, closes the softap, and is used in a low-power solutions.*/

    uint8_t channel;        /**< Channel, mesh and router will be on the same channel */
    uint8_t channel_switch_disable; /**< If this value is not set, when "attempt" (mwifi_init_config_t) times is reached, device will change to
                                         a full channel scan for a network that could join. */
    uint8_t router_switch_disable;  /**< If the BSSID is specified and this value is not also set, when the router of this specified BSSID
                                         fails to be found after "attempt" (mwifi_init_config_t) times, the whole network is allowed to switch
                                         to another router with the same SSID. The new router might also be on a different channel.
                                         There is a risk that if the password is different between the new switched router and the previous
                                         one, the mesh network could be established but the root will never connect to the new switched router. */
} mwifi_config_t;

/**
 * @brief Mesh network internal data transmission method.
 */
enum mwifi_communication_method {
    MWIFI_COMMUNICATE_UNICAST,   /**< Send data by unicast. */
    MWIFI_COMMUNICATE_MULTICAST, /**< Send data by multicast. */
    MWIFI_COMMUNICATE_BROADCAST, /**< Send data by broadcast. */
};

/**
 * @brief Mwifi packet type
 */
typedef struct {
    bool compression    : 1; /**< Enable data compression. If the data sent is json format or string,
                                 use data compression to increase the data transmission rate. */
    bool upgrade        : 1; /**< Upgrade packet flag */
    uint8_t communicate : 2; /**< Mesh data communication method, There are three types:
                                  MWIFI_COMMUNICATE_UNICAST, MWIFI_COMMUNICATE_MULTICAST, MWIFI_COMMUNICATE_BROADCAST*/
    bool group          : 1; /**< Send a package as a group */
    uint8_t reserved    : 1; /**< reserved */
    uint8_t protocol    : 2; /**< Type of transmitted application protocol */
    uint32_t custom;         /**< Type of transmitted application data */
} __attribute__((packed)) mwifi_data_type_t;

/**
 * @brief Buffer space when reading data
 */
typedef enum {
    MWIFI_DATA_MEMORY_MALLOC_INTERNAL = 1,  /**< Buffer space is requested by internal when reading data */
    MWIFI_DATA_MEMORY_MALLOC_EXTERNAL = 2,  /**< Buffer space is requested by external when reading data */
} mwifi_data_memory_t;

/**
 * @brief  Get mesh networking IE.
 *
 * @param  mesh_assoc pointer mesh networking IE.
 * @param  mesh_chain pointer mesh chain layer infomation.
 *
 * @return
 *    - ESP_OK
 *    - ESP_FAIL
 */
esp_err_t esp_wifi_vnd_mesh_get(mesh_assoc_t *mesh_assoc, mesh_chain_layer_t *mesh_chain);

/**
 * @brief  Mwifi initialization
 *
 * @attention 1. This API should be called after WiFi is started.
 * @attention 2. This API must be called before all other Mwifi API can be called.
 * @attention 3. Always use WIFI_INIT_CONFIG_DEFAULT macro to init the config to default values,
 *               this can guarantee all the fields got correct value when more fields
 *               are added into wifi_init_config_t in future release. If you want
 *               to set your owner initial values, overwrite the default values
 *               which are set by WIFI_INIT_CONFIG_DEFAULT.
 *
 * @param  config  Mwifi initialization configuration.
 *
 * @return
 *    - MDF_OK
 *    - MDF_ERR_MWIFI_NOT_INIT
 *    - MDF_ERR_MWIFI_INITED
 */
mdf_err_t mwifi_init(const mwifi_init_config_t *config);


/**
 * @brief  Deinit mwifi
 *         Free all resource allocated in mwifi_init and stop Mwifi task.
 *
 * @attention This API should be called if you want to remove Mwifi from the system.
 *
 * @return
 *    - MDF_OK
 *    - MDF_ERR_MWIFI_NOT_INIT
 */
mdf_err_t mwifi_deinit(void);

/**
 * @brief     Set Mwifi configuration
 *
 * @attention This API shall be called between esp_mesh_init() and esp_mesh_start().
 *
 * @param     init_config Use MWIFI_INIT_CONFIG_DEFAULT() to initialize the default values.
 *
 * @return
 *    - MDF_OK
 *    - MDF_ERR_MWIFI_ARGUMENT
 */
mdf_err_t mwifi_set_init_config(const mwifi_init_config_t *init_config);

/**
 * @brief  Get Mwifi init configuration.
 *
 * @param  init_config pointer to Mwifi init configuration.
 *
 * @return             [description]
 */
mdf_err_t mwifi_get_init_config(mwifi_init_config_t *init_config);

/**
 * @brief  Set the configuration of the AP
 *
 * @attention This API shall be called between esp_mesh_init() and esp_mesh_start().
 *
 * @param  config pointer to AP configuration
 *
 * @return
 *    - MDF_OK
 *    - MDF_ERR_MWIFI_ARGUMENT
 */
mdf_err_t mwifi_set_config(const mwifi_config_t *config);

/**
 * @brief  Get the configuration of the AP
 *
 * @param  config pointer to AP configuration
 *
 * @return
 *    - MDF_OK
 *    - MDF_ERR_MWIFI_ARGUMENT
 */
mdf_err_t mwifi_get_config(mwifi_config_t *config);

/**
 * @brief  Start Mwifi according to current configuration
 *
 * @return
 *    - MDF_OK
 *    - MDF_ERR_MWIFI_NOT_INIT
 */
mdf_err_t mwifi_start(void);

/**
 * @brief  Stop mwifi
 *
 * @return
 *    - MDF_OK
 *    - MDF_ERR_MWIFI_NOT_INIT
 */
mdf_err_t mwifi_stop(void);

/**
 * @brief  restart mwifi
 *
 * @attention This API should be called when the Mwifi configuration is modified.
 *
 * @return
 *    - MDF_OK
 *    - MDF_ERR_MWIFI_NOT_INIT
 */
mdf_err_t mwifi_restart();

/**
 * @brief  whether wifi_mesh is running
 *
 * @return
 *    - true
 *    - flase
 */
bool mwifi_is_started(void);

/**
 * @brief  Whether the node is already connected to the parent node
 *
 * @return
 *    - true
 *    - flase
 */
bool mwifi_is_connected(void);

/**
 * @brief  Print mesh configuration information
 */
void mwifi_print_config();

/**
 * @brief  Send a packet to any node in the mesh network.
 *
 * @attention 1. If data encryption is enabled, you must ensure that the task that
 *               calls this function is greater than 4KB.
 *            2. When sending data to the root node, if the destination address
 *               is NULL, the root node receives it using mwifi_root_read(). If
 *               the destination address is the mac address of the root node or MWIFI_ADDR_ROOT, the
 *               root node receives it using mwifi_read()
 *
 * @param  dest_addrs The address of the final destination of the packet
 *                    If the packet is to the root and "dest_addr" parameter is NULL
 * @param  data_type  The type of the data
 *                    If the default configuration is used, this parameter is NULL
 * @param  data       Pointer to a sending wifi mesh packet
 * @param  size       The length of the data
 * @param  block      Whether to block waiting for data transmission results
 *
 * @return
 *    - MDF_OK
 *    - MDF_ERR_MWIFI_NOT_START
 */
mdf_err_t mwifi_write(const uint8_t *dest_addrs, const mwifi_data_type_t *data_type,
                      const void *data, size_t size, bool block);

/**
 * @brief  Receive a packet targeted to self over the mesh network
 *
 * @attention Judging the allocation of buffers by the type of the parameter 'data'
 *            The memory of `data` is externally allocated, and the memory is larger than the total length expected to be received.
 *            The memory of `data` is allocated internally by `mwifi_read`, which needs to be released after the call.
 *
 *
 * @param  src_addr    The address of the original source of the packet
 * @param  data_type   The type of the data
 * @param  data        Pointer to the received mesh packet
 *                     To apply for buffer space externally, set the type of the data parameter to be (char *) or (uint8_t *)
 *                     To apply for buffer space internally, set the type of the data parameter to be (char **) or (uint8_t **)
 * @param  size        A non-zero pointer to the variable holding the length of out_value.
 *                     In case out_value is not zero, will be set to the actual length of the value written.
 * @param  wait_ticks  Wait time if a packet isn't immediately available
 * @param  type        Buffer space when reading data
 *
 * @return
 *    - MDF_OK
 *    - MDF_ERR_MWIFI_NOT_START
 *    - ESP_ERR_MESH_ARGUMENT
 *    - ESP_ERR_MESH_NOT_START
 *    - ESP_ERR_MESH_TIMEOUT
 *    - ESP_ERR_MESH_DISCARD
 */
mdf_err_t __mwifi_read(uint8_t *src_addr, mwifi_data_type_t *data_type, void *data, size_t *size, TickType_t wait_ticks, uint8_t type);
#define mwifi_read(src_addr, data_type, data, size, wait_ticks) \
    __mwifi_read(src_addr, data_type, (void *)data, size, wait_ticks, \
                 builtin_types_compatible_p(data, char *) * MWIFI_DATA_MEMORY_MALLOC_EXTERNAL \
                 + builtin_types_compatible_p(data, uint8_t *) * MWIFI_DATA_MEMORY_MALLOC_EXTERNAL \
                 + builtin_types_compatible_p(data, char []) * MWIFI_DATA_MEMORY_MALLOC_EXTERNAL \
                 + builtin_types_compatible_p(data, uint8_t []) * MWIFI_DATA_MEMORY_MALLOC_EXTERNAL \
                 + builtin_types_compatible_p(data, char **) * MWIFI_DATA_MEMORY_MALLOC_INTERNAL \
                 + builtin_types_compatible_p(data, uint8_t **) * MWIFI_DATA_MEMORY_MALLOC_INTERNAL)

/**
 * @brief  The root sends a packet to the device in the mesh.
 *
 * @attention 1. If data encryption is enabled, you must ensure that the task that
 *               calls this function is greater than 8KB
 *            2. This API is only used at the root node
 *            3. Can send data to multiple devices at the same time
 *            4. Packets are only transmitted down
 *
 * @param  dest_addrs     The address of the final destination of the packet
 * @param  dest_addrs_num Number of destination addresses
 * @param  data_type      The type of the data
 * @param  data           Pointer to a sending wifi mesh packet
 * @param  size           The length of the data
 * @param  block          Whether to block waiting for data transmission results
 *
 * @return
 *    - MDF_OK
 *    - MDF_ERR_MWIFI_NOT_START
 *    - ESP_ERR_MESH_ARGUMENT
 *    - ESP_ERR_MESH_NOT_START
 *    - ESP_ERR_MESH_DISCONNECTED
 *    - ESP_ERR_MESH_OPT_UNKNOWN
 *    - ESP_ERR_MESH_EXCEED_MTU
 *    - ESP_ERR_MESH_NO_MEMORY
 *    - ESP_ERR_MESH_TIMEOUT
 *    - ESP_ERR_MESH_QUEUE_FULL
 *    - ESP_ERR_MESH_NO_ROUTE_FOUND
 *    - ESP_ERR_MESH_DISCARD
 */
mdf_err_t mwifi_root_write(const uint8_t *dest_addrs, size_t dest_addrs_num,
                           const mwifi_data_type_t *data_type, const void *data,
                           size_t size, bool block);

/**
 * @brief  receive a packet targeted to external IP network
 *         root uses this API to receive packets destined to external IP network
 *         root forwards the received packets to the final destination via socket.
 *         This API is only used at the root node
 *
 * @attention 1. This API is only used at the root node
 *            2. Judging the allocation of buffers by the type of the parameter 'data'
 *               The memory of `data` is externally allocated, and the memory is larger than the total length expected to be received.
 *               The memory of `data` is allocated internally by `mwifi_read`, which needs to be released after the call.
 *
 * @param  src_addr    the address of the original source of the packet
 * @param  data_type   the type of the data
 * @param  data        Pointer to the received mesh packet
 *                     To apply for buffer space externally, set the type of the data parameter to be (char *) or (uint8_t *)
 *                     To apply for buffer space internally, set the type of the data parameter to be (char **) or (uint8_t **)
 * @param  size        The length of the data
 * @param  wait_ticks  wait time if a packet isn't immediately available(0:no wait, portMAX_DELAY:wait forever)
 * @param  type        Buffer space when reading data
 *
 * @return
 *    - MDF_OK
 *    - MDF_ERR_MWIFI_NOT_START
 *    - ESP_ERR_MESH_ARGUMENT
 *    - ESP_ERR_MESH_NOT_START
 *    - ESP_ERR_MESH_TIMEOUT
 *    - ESP_ERR_MESH_DISCARD
 */
mdf_err_t __mwifi_root_read(uint8_t *src_addr, mwifi_data_type_t *data_type,
                            void *data, size_t *size, TickType_t wait_ticks, uint8_t type);
#define mwifi_root_read(src_addr, data_type, data, size, wait_ticks) \
    __mwifi_root_read(src_addr, data_type, (void *)data, size, wait_ticks, \
                      builtin_types_compatible_p(data, char *) * MWIFI_DATA_MEMORY_MALLOC_EXTERNAL \
                      + builtin_types_compatible_p(data, uint8_t *) * MWIFI_DATA_MEMORY_MALLOC_EXTERNAL \
                      + builtin_types_compatible_p(data, char **) * MWIFI_DATA_MEMORY_MALLOC_INTERNAL \
                      + builtin_types_compatible_p(data, uint8_t **) * MWIFI_DATA_MEMORY_MALLOC_INTERNAL)

/**
 * @brief      Post the toDS state to the mesh stack, Usually used to notify the child node, whether the root is successfully connected to the server
 *
 * @attention  This API is only for the root.
 *
 * @param[in]  status  this state represents whether the root is able to access external IP network
 *
 * @return
 *    - ESP_OK
 *    - ESP_FAIL
 */
mdf_err_t mwifi_post_root_status(bool status);

/**
 * @brief      Get the toDS state from the mesh stack, Whether the root is successfully connected to the server
 *
 * @return
 *    - true
 *    - flase
 */
bool mwifi_get_root_status();

/**
 * @brief     Get the RSSI of the parent node
 *
 * @return    RSSI of the parent
 */
int8_t mwifi_get_parent_rssi();

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __MWIFI_H__ */
