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

#ifndef __ALIYUN_LIST_H__
#define __ALIYUN_LIST_H__

#include <sys/queue.h>
#include "aliyun_defs.h"
#include "mdf_err.h"

#define ALIYUN_SUB_STATUS_DEFAULT_TIMEOUT 10000
#define ALIYUN_SUB_STATUS_DELETE_TIMEOUT  (1000*120)

typedef enum {
    ALIYUN_DEVICE_TYPE_GATEWAY,
    ALIYUN_DEVICE_TYPE_SUBDEVICE,
} aliyun_device_type_t;

typedef enum {
    ALIYUN_LIST_TOPO_ADD_START,
    ALIYUN_LIST_TOPO_ADD_WAIT,
    ALIYUN_LIST_TOPO_ADD_SUCC,
    ALIYUN_LIST_TOPO_ADD_FAIL,
    ALIYUN_LIST_LOGIN_WAIT,
    ALIYUN_LIST_LOGIN_SUCC,
    ALIYUN_LIST_LOGIN_FAIL,
    ALIYUN_LIST_FINISH,
    ALIYUN_LIST_LOGOUT_START,
    ALIYUN_LIST_LOGOUT_WAIT,
    ALIYUN_LIST_LOGOUT_SUCC,
    ALIYUN_LIST_LOGOUT_FAIL,
    ALIYUN_LIST_TOPO_DELETE_WAIT,
    ALIYUN_LIST_TOPO_DELETE_SUCC,
    ALIYUN_LIST_TOPO_DELETE_FAIL,
    ALIYUN_LIST_DELETE_WAIT,
    ALIYUN_LIST_DELETE,
} aliyun_list_status_t;

typedef struct sub_info_list {
    int device_id;
    aliyun_device_meta_t meta;
    uint8_t device_type;
    uint8_t addrs[ALIYUN_SUBDEVICE_ADDRS_MAXLEN];
    uint8_t ota_type;
    uint8_t refresh;
    bool no_subscribe;
    aliyun_list_status_t status;
    uint32_t msg_id;
    int64_t timeout;
    SLIST_ENTRY(sub_info_list)
    next; //!< next command in the list
} sub_info_list_t;

/** linked list of command structures */

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

/**
 * @brief  Device list destroy.
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_list_destroy(void);

/**
 * @brief  Device list insertion information.
 *
 * @param  meta   Device triplet information.
 * @param  addrs  Device unique address.
 * @param  device_type  Device type, child device or gateway.
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_list_insert_meta(const aliyun_device_meta_t *meta, const uint8_t addrs[], aliyun_device_type_t device_type);

/**
 * @brief  Device list delete information.
 *
 * @param  meta   Device triplet information.
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_list_delete_meta(const aliyun_device_meta_t *meta);

/**
 * @brief  Device list select exist.
 *
 * @param  meta   Device triplet information.
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_list_select_exist(const aliyun_device_meta_t *meta);
mdf_err_t aliyun_list_get_by_meta(const aliyun_device_meta_t *meta, sub_info_list_t **sub_info);

/**
 * @brief  Device list select addrs.
 *
 * @param  meta   Device triplet information.
 * @param  addrs  Device unique address buf.
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_list_select_addrs(const aliyun_device_meta_t *meta, uint8_t addrs[]);

/**
 * @brief  get device by addr.
 *
 * @param  addrs  Device unique address.
 * @param  sub_info where to save device point.
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_list_get_by_addr(const uint8_t *addrs, sub_info_list_t **sub_info);

/**
 * @brief  Device list select meta.
 *
 * @param  addrs  Device unique address.
 * @param  meta   Device triplet information buf.
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_list_select_meta(const uint8_t *addrs, aliyun_device_meta_t *meta);

/**
 * @brief  Device list select status.
 *
 * @param  meta   Device triplet information.
 * @param  status Device status buf.
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_list_select_status(const aliyun_device_meta_t *meta, uint32_t *status);

/**
 * @brief  Device list select refresh.
 *
 * @param  routing_table Pointer to a routing table.
 * @param  table_size number of routing table .
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_list_select_refresh(uint8_t *routing_table, size_t table_size);

/**
 * @brief  Get device list through timeout status.
 *
 * @param  sub_list  where to save device poist list.
 * @param  sub_list_sz sub_list size.
 * @param  num  real number that device has been select.
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_list_get_by_timeout(sub_info_list_t **sub_list, size_t sub_list_sz, int *num);

/**
 * @brief  Get device list through unfinished status.
 *
 * @param  sub_list  where to save device point list.
 * @param  sub_list_sz sub_list size.
 * @param  num  real number that device has been select.
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_list_get_by_unfinished(sub_info_list_t **sub_list, size_t sub_list_sz, int *num);

/**
 * @brief  Device list update msg_id.
 *
 * @param  meta   Device triplet information.
 * @param  msg_id Device message.
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_list_update_msg_id(const aliyun_device_meta_t *meta, uint32_t msg_id);

/**
 * @brief  Device list update status.
 *
 * @param  meta   Device triplet information.
 * @param  status Device status.
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_list_update_status(const aliyun_device_meta_t *meta, uint32_t status, uint32_t timeout);

/**
 * @brief  Device list update subdevice reconnet.
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_list_update_subdevice_reconnet(void);

/**
 * @brief  Device list update message status.
 *
 * @param  meta   Device triplet information.
 * @param  status Device status.
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_list_update_msg_status(uint32_t msg_id, uint32_t status);

/**
 * @brief  Device list update ota type.
 *
 * @param  product_key  Device product key.
 * @param  device_name  Device name.
 * @param  type         Device ota type.
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
mdf_err_t aliyun_list_update_ota_type(const char *product_key, const char *device_name, aliyun_ota_type_t type);

/**
 * @brief  Device list select ota start addrs number
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
uint32_t aliyun_list_select_ota_start_addrs_num(void);

/**
 * @brief  Device list select ota start addrs list
 *
 * @param  addrs_list  addrs size
 * @param  size  Device name.
 *
 * @return
 *     - MDF_FAIL
 *     - MDF_OK
 */
uint32_t aliyun_list_select_ota_start_addrs_list(uint8_t *addrs_list, size_t size);

/**
 * @brief check if addr is in routing_table
 *
 * @param addr device addr(mac)
 * @return true in routing_table
 * @return false  not in routing_table
 */
bool aliyun_list_is_device_in_routing_table(const uint8_t *addr);

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __ALIYUN_LIST_H__ */
