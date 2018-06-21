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

#ifndef __MDF_WIFI_MESH_H__
#define __MDF_WIFI_MESH_H__

#include "esp_mesh.h"
#include "esp_mesh_internal.h"

#include "mdf_socket.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

#ifndef CONFIG_WIFI_MESH_MAX_LAYER
#define CONFIG_WIFI_MESH_MAX_LAYER 4
#endif /**< CONFIG_WIFI_MESH_MAX_LAYER */
#define WIFI_MESH_MAX_LAYER CONFIG_WIFI_MESH_MAX_LAYER

#ifndef CONFIG_WIFI_MESH_AP_CONNECTIONS
#define CONFIG_WIFI_MESH_AP_CONNECTIONS 6
#endif /**< CONFIG_WIFI_MESH_AP_CONNECTIONS */
#define WIFI_MESH_AP_CONNECTIONS CONFIG_WIFI_MESH_AP_CONNECTIONS

#ifndef CONFIG_WIFI_MESH_VOTE_PERCENTAGE
#define CONFIG_WIFI_MESH_VOTE_PERCENTAGE 100
#endif /**< CONFIG_WIFI_MESH_VOTE_PERCENTAGE */
#define WIFI_MESH_VOTE_PERCENTAGE CONFIG_WIFI_MESH_VOTE_PERCENTAGE

#ifndef CONFIG_WIFI_MESH_AP_ASSOC_EXPIRE
#define CONFIG_WIFI_MESH_AP_ASSOC_EXPIRE 10
#endif /**< CONFIG_WIFI_MESH_AP_ASSOC_EXPIRE */
#define WIFI_MESH_AP_ASSOC_EXPIRE CONFIG_WIFI_MESH_AP_ASSOC_EXPIRE

#ifndef CONFIG_WIFI_MESH_ATTEMPT_SCAN_TIME
#define CONFIG_WIFI_MESH_ATTEMPT_SCAN_TIME 10
#endif /**< CONFIG_WIFI_MESH_ATTEMPT_SCAN_TIME */
#define WIFI_MESH_ATTEMPT_SCAN_TIME CONFIG_WIFI_MESH_ATTEMPT_SCAN_TIME

#ifndef CONFIG_WIFI_WIFI_MESH_AUTH_MODE
#define CONFIG_WIFI_WIFI_MESH_AUTH_MODE WIFI_AUTH_OPEN
#endif /**< CONFIG_WIFI_WIFI_MESH_AUTH_MODE */
#define WIFI_WIFI_MESH_AUTH_MODE CONFIG_WIFI_WIFI_MESH_AUTH_MODE

#define WIFI_MESH_XON_QSIZE       105
#define WIFI_MESH_PACKET_MAX_SIZE 1440

/**
 * @brief wifi mesh packet type
 */
typedef union {
    struct {
        uint32_t encrypt     : 1; /**< data encrypted(Unimplemented) */
        uint32_t to_server   : 1; /**< identify this packet is target to external IP network */
        uint32_t block       : 1; /**< esp_mesh_send() non-block */
        uint32_t group       : 1; /**< identify this packet is target to a group */
        uint32_t no_response : 1; /**< no need to response */
        uint32_t ota         : 1; /**< data content is upgrade data */
        uint32_t notice      : 1; /**< notification */
        uint32_t proto       : 2; /**< protocol of transmitted application data */
        uint32_t layer       : 8; /**< layer value in mesh network */
    };
    uint32_t val;                 /**< collective of all device types */
} wifi_mesh_data_type_t;

typedef struct {
    uint8_t eid;
    uint8_t len;
    uint8_t oui[3];
    uint8_t type;
    uint8_t version;
    uint8_t mdf_type;
    uint8_t mdf_id[6];
    uint8_t layer_cap;
    uint8_t layer;
    uint8_t assoc_cap;
    uint8_t assoc;
    uint8_t leaf_cap;
    uint8_t leaf_assoc;
    uint16_t root_cap;
    uint16_t self_assoc;
    uint16_t layer2_cap;
    uint16_t scan_ap_num;
    int8_t rssi;
    int8_t router_rssi;
    uint8_t flag;
    uint8_t rc_addr[6];
    int8_t rc_rssi;
    uint8_t vote_addr[6];
    int8_t vote_rssi;
    uint8_t vote_ttl;
    uint16_t votes;
    uint16_t my_votes;
    uint8_t reason;
    uint8_t child[6];
    uint8_t toDS;
} __attribute__((packed)) wifi_vnd_mdf_assoc_t;

esp_err_t esp_wifi_vnd_mesh_get(wifi_vnd_mdf_assoc_t *mdf_assoc);

/**
 * @brief wifi mesh address
 */
typedef sockaddr_t wifi_mesh_addr_t;

/**
 * @brief data carried by the head of the wifi mesh packet
 */
typedef struct {
    uint8_t seq;                /**< packet sending for large packets */
    uint16_t size;              /**< packet sending for large packets */
    int id;                     /**< filter duplicate packets*/
    wifi_mesh_data_type_t type; /**< the type of data */
    wifi_mesh_addr_t addr;      /**< external network address */
} wifi_mesh_head_data_t;

/**
 * @brief enumerated list of mdf event id
 */
enum {
    MDF_EVENT_STARTED,                   /**< mesh is started */
    MDF_EVENT_STOPPED,                   /**< mesh is stopped */
    MDF_EVENT_CHANNEL_SWITCH,            /**< channel switch */
    MDF_EVENT_CHILD_CONNECTED,           /**< a child is connected on softAP interface */
    MDF_EVENT_CHILD_DISCONNECTED,        /**< a child is disconnected on softAP interface */
    MDF_EVENT_ROUTING_TABLE_ADD,         /**< routing table is changed by adding newly joined children */
    MDF_EVENT_ROUTING_TABLE_REMOVE,      /**< routing table is changed by removing leave children */
    MDF_EVENT_PARENT_CONNECTED,          /**< parent is connected on station interface */
    MDF_EVENT_PARENT_DISCONNECTED,       /**< parent is disconnected on station interface */
    MDF_EVENT_NO_PARENT_FOUND,           /**< no parent found */
    MDF_EVENT_LAYER_CHANGE,              /**< layer changes over the mesh network */
    MDF_EVENT_TODS_STATE,                /**< state represents if root is able to access external IP network */
    MDF_EVENT_VOTE_STARTED,              /**< the process of voting a new root is started either by children or by root */
    MDF_EVENT_VOTE_STOPPED,              /**< the process of voting a new root is stopped */
    MDF_EVENT_ROOT_ADDRESS,              /**< the root address is obtained. It is posted by mesh stack automatically. */
    MDF_EVENT_ROOT_SWITCH_REQ,           /**< root switch request sent from a new voted root candidate */
    MDF_EVENT_ROOT_SWITCH_ACK,           /**< root switch acknowledgment responds the above request sent from current root */
    MDF_EVENT_ROOT_GOT_IP,               /**< root obtains the IP address. It is posted by LwIP stack automatically */
    MDF_EVENT_ROOT_LOST_IP,              /**< root loses the IP address. It is posted by LwIP stack automatically */
    MDF_EVENT_ROOT_ASKED_YIELD,          /**< root is asked yield by a more powerful existing root. If self organized is disabled
                                             and this device is specified to be a root by users, users should set a new parent
                                             for this device. if self organized is enabled, this device will find a new parent
                                             by itself, users could ignore this event. */
};

/**
 * @brief broadcast address
 */
extern const uint8_t WIFI_MESH_BROADCAST_ADDR[];
/**
 * @brief root node address
 */
extern const uint8_t WIFI_MESH_ROOT_DEFAULT_ADDR[];

/**
 * @brief wifi mesh initialization configuration
 */
typedef struct {
    char ssid[32];              /**< ssid */
    char password[64];          /**< password */
    uint8_t bssid[6];           /**< bssid */
    uint8_t channel;            /**< channel, the mesh network on */
    wifi_mesh_addr_t mesh_id;   /**< mesh network identification */
} wifi_mesh_config_t;

/**
 * @brief  wifi mesh initialization
 *
 * @attention This API should be called after WiFi is started.
 *
 * @param  config  wifi mesh initialization configuration
 *
 * @return
 *    - ESP_OK
 *    - ESP_FAIL
 */
esp_err_t mdf_wifi_mesh_init(const wifi_mesh_config_t *config);

/**
 * @brief     wifi mesh de-initialization
 *            release resources and stop the wifi mesh
 *
 * @return
 *    - ESP_OK
 *    - ESP_FAIL
 */
esp_err_t mdf_wifi_mesh_deinit(void);

/**
 * @brief  wether wifi_mesh is running
 *
 * @return
 *    - true
 *    - flase
 */
bool mdf_wifi_mesh_is_running(void);

/**
 * @brief  wether wifi_mesh is disconnected
 *
 * @return
 *    - true
 *    - flase
 */
bool mdf_wifi_mesh_is_connect(void);

/**
 * @brief  send a packet over the mesh network
 *         send a packet to any node in the mesh network.
 *         send a packet to external IP network.
 *
 * @param  dest_addr the address of the final destination of the packet
 * @param  data_type the length of the data
 * @param  data      pointer to a sending wifi mesh packet
 * @param  size      The length of the data
 *
 * @return
 *    - ESP_FAIL
 *    - the length of the sent data
 */
ssize_t mdf_wifi_mesh_send(const wifi_mesh_addr_t *dest_addr,
                           const wifi_mesh_data_type_t *data_type,
                           const void *data, ssize_t size);

/**
 * @brief  receive a packet targeted to self over the mesh network
 *
 * @param  src_addr    the address of the original source of the packet
 * @param  dest_addr   the address of the final destination of the packet
 * @param  data_type   the length of the data
 * @param  data        data  pointer to the received mesh packet
 * @param  size        The length of the data
 * @param  timeout_ms  wait time if a packet isn't immediately available(0:no wait, portMAX_DELAY:wait forever)
 *
 * @return
 *    - ESP_FAIL
 *    - the length of the recv data
 */
ssize_t mdf_wifi_mesh_recv(wifi_mesh_addr_t *src_addr, wifi_mesh_data_type_t *data_type,
                           void *data, ssize_t size, int timeout_ms);

/**
 * @brief  send a packet targeted from external IP network
 *         root uses this API to receive packets destined from external IP network
 *
 * @param  src_addr    the address of the original source of the packet
 * @param  dest_addr   the address contains remote IP address and port(IPv4:PORT)
 * @param  data_type   the length of the data
 * @param  data        pointer to a sending wifi mesh packet
 * @param  size        The length of the data
 *
 * @return
 *    - ESP_FAIL
 *    - the length of the sent data
 */
ssize_t mdf_wifi_mesh_root_send(wifi_mesh_addr_t *src_addr, wifi_mesh_addr_t *dest_addr,
                                const wifi_mesh_data_type_t *data_type,
                                const void *data, ssize_t size);

/**
 * @brief  receive a packet targeted to external IP network
 *         root uses this API to receive packets destined to external IP network
 *         root forwards the received packets to the final destination via socket.
 *
 * @param  src_addr    the address of the original source of the packet
 * @param  dest_addr   the address contains remote IP address and port(IPv4:PORT)
 * @param  data_type   the length of the data
 * @param  data        data  pointer to the received mesh packet
 * @param  size        The length of the data
 * @param  timeout_ms  wait time if a packet isn't immediately available(0:no wait, portMAX_DELAY:wait forever)
 *
 * @return
 *    - ESP_FAIL
 *    - the length of the recv data
 */
ssize_t mdf_wifi_mesh_root_recv(wifi_mesh_addr_t *src_addr, wifi_mesh_addr_t *dest_addr,
                                wifi_mesh_data_type_t *data_type,
                                void *data, ssize_t size, int timeout_ms);

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __MDF_WIFI_MESH_H__ */
