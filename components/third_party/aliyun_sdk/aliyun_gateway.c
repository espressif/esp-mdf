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

#include "cJSON.h"
#include "esp_mesh.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "mdf_mem.h"
#include "aliyun_gateway.h"

#include "aliyun_kv.h"

#include "aliyun_coap.h"
#include "aliyun_sign.h"
#include "aliyun_url.h"

#include "aliyun_list.h"
#include "aliyun_pack.h"
#include "aliyun_parse.h"

#include "aliyun_mqtt.h"
#include "aliyun_publish.h"
#include "aliyun_subscribe.h"
#include "aliyun_unsubscribe.h"

#include "aliyun_platform.h"

#define ROUTING_TABLE_MARGIN (10)
#define TASK_RUNNING 1
#define TASK_STOP 0

static const char *TAG = "aliyun_gateway";

static int *g_aliyun_read_task_status = NULL;
static int *g_aliyun_write_task_status = NULL;

#ifdef CONFIG_ALIYUN_PLATFORM_MDF
static SemaphoreHandle_t g_refresh_device_list_sepr = NULL;
#endif

const char ntp_response_template[] = "{\"deviceSendTime\":%lld,\"serverRecvTime\":%lld,\"serverSendTime\":%lld}";

static mdf_err_t aliyun_gateway_add_subdevice_reply(const aliyun_device_meta_t *sub_meta, int status)
{
    mdf_err_t ret = MDF_OK;
    aliyun_device_reply_t reply_info = { 0 };
    uint8_t dest_addrs[ALIYUN_SUBDEVICE_ADDRS_MAXLEN] = { 0 };

    ret = aliyun_list_select_addrs(sub_meta, dest_addrs);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_list_select_addrs is NULL");

    strncpy(reply_info.product_key, sub_meta->product_key, ALIYUN_PRODUCT_KEY_MAXLEN - 1);
    strncpy(reply_info.device_name, sub_meta->device_name, ALIYUN_DEVICE_NAME_MAXLEN - 1);
    reply_info.status = status;

    return aliyun_platform_gateway_write(dest_addrs, ALIYUN_GATEWAY_ADD_SUBDEVICE_REPLY, &reply_info, sizeof(aliyun_device_reply_t));
}

static mdf_err_t aliyun_gateway_delete_subdevice_reply(const aliyun_device_meta_t *sub_meta, int status)
{
    mdf_err_t ret = MDF_OK;
    aliyun_device_reply_t reply_info = { 0 };
    uint8_t dest_addrs[ALIYUN_SUBDEVICE_ADDRS_MAXLEN] = { 0 };

    ret = aliyun_list_select_addrs(sub_meta, dest_addrs);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_list_select_addrs is NULL");

    strncpy(reply_info.product_key, sub_meta->product_key, ALIYUN_PRODUCT_KEY_MAXLEN - 1);
    strncpy(reply_info.device_name, sub_meta->device_name, ALIYUN_DEVICE_NAME_MAXLEN - 1);
    reply_info.status = status;

    return aliyun_platform_gateway_write(dest_addrs, ALIYUN_GATEWAY_DELETE_SUBDEVICE_REPLY, &reply_info, sizeof(aliyun_device_reply_t));
}

static mdf_err_t aliyun_gateway_heartbeat_subdevice_reply(const uint8_t *dest_addrs, int status)
{
    return aliyun_platform_gateway_write(dest_addrs, ALIYUN_SUBDEVICE_HEARTBEAT_REPLY, &status, sizeof(int));
}

static mdf_err_t aliyun_gateway_thing_sub_topo_add(const aliyun_device_meta_t *gateway_meta, const aliyun_device_meta_t *sub_meta, aliyun_buffer_t *buffer)
{
    mdf_err_t ret = MDF_FAIL;

    uint32_t msg_id = aliyun_platform_get_msg_id();

    char *topo_add_buf = aliyun_pack_thing_sub_topo_add(msg_id, sub_meta);
    MDF_ERROR_CHECK(topo_add_buf == NULL, MDF_FAIL, "aliyun_pack_thing_sub_topo_add is NULL");
    MDF_ERROR_CHECK(strlen(topo_add_buf) > CONFIG_ALIYUN_PAYLOAD_SIZE, MDF_FAIL, "topo add payload big");

    strncpy((char *)buffer->payload, topo_add_buf, strlen(topo_add_buf));
    buffer->payload_len = strlen(topo_add_buf);
    MDF_FREE(topo_add_buf);

    ret = aliyun_list_update_msg_id(sub_meta, msg_id);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_list_update_msg_id error");

    ret = aliyun_list_update_status(sub_meta, ALIYUN_LIST_TOPO_ADD_WAIT, ALIYUN_SUB_STATUS_DEFAULT_TIMEOUT);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_list_update_status error");

    ret = aliyun_publish_sub_topo_add(gateway_meta->product_key, gateway_meta->device_name, buffer);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_publish_sub_topo_add error");

    return MDF_OK;
}

static mdf_err_t aliyun_gateway_thing_sub_topo_delete(const aliyun_device_meta_t *gateway_meta, const aliyun_device_meta_t *sub_meta, aliyun_buffer_t *buffer)
{
    mdf_err_t ret = MDF_FAIL;

    uint32_t msg_id = aliyun_platform_get_msg_id();

    char *topo_delete_buf = aliyun_pack_thing_sub_topo_delete(msg_id, sub_meta);
    MDF_ERROR_CHECK(topo_delete_buf == NULL, MDF_FAIL, "aliyun_pack_thing_sub_topo_add is NULL");

    MDF_ERROR_CHECK(strlen(topo_delete_buf) > CONFIG_ALIYUN_PAYLOAD_SIZE, MDF_FAIL, "aliyun_pack_thing_sub_topo_delete payload big");
    strncpy((char *)buffer->payload, topo_delete_buf, strlen(topo_delete_buf));
    buffer->payload_len = strlen(topo_delete_buf);
    MDF_FREE(topo_delete_buf);

    ret = aliyun_list_update_msg_id(sub_meta, msg_id);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_list_update_msg_id error");

    ret = aliyun_list_update_status(sub_meta, ALIYUN_LIST_TOPO_DELETE_WAIT, ALIYUN_SUB_STATUS_DEFAULT_TIMEOUT);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_list_update_status error");

    ret = aliyun_publish_sub_topo_delete(gateway_meta->product_key, gateway_meta->device_name, buffer);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_publish_sub_topo_add error");

    return MDF_OK;
}

static mdf_err_t aliyun_gateway_thing_sub_combine_login(const aliyun_device_meta_t *gateway_meta, const aliyun_device_meta_t *sub_meta, aliyun_buffer_t *buffer)
{
    mdf_err_t ret = MDF_FAIL;

    uint32_t msg_id = aliyun_platform_get_msg_id();

    char *combine_login_buf = aliyun_pack_thing_sub_combine_login(msg_id, sub_meta);
    MDF_ERROR_CHECK(combine_login_buf == NULL, MDF_FAIL, "aliyun_pack_thing_sub_topo_add is NULL");

    MDF_ERROR_CHECK(strlen(combine_login_buf) > CONFIG_ALIYUN_PAYLOAD_SIZE, MDF_FAIL, "aliyun_pack_thing_sub_combine_login payload big");
    strncpy((char *)buffer->payload, combine_login_buf, strlen(combine_login_buf));
    buffer->payload_len = strlen(combine_login_buf);
    MDF_FREE(combine_login_buf);

    ret = aliyun_list_update_msg_id(sub_meta, msg_id);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_list_update_msg_id error");

    ret = aliyun_list_update_status(sub_meta, ALIYUN_LIST_LOGIN_WAIT, ALIYUN_SUB_STATUS_DEFAULT_TIMEOUT);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_list_update_status error");

    ret = aliyun_publish_sub_combine_login(gateway_meta->product_key, gateway_meta->device_name, buffer);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_publish_sub_topo_add error");

    return MDF_OK;
}

#ifdef CONFIG_ALIYUN_PLATFORM_MDF
static mdf_err_t aliyun_gateway_refresh_subdevice_internal(void)
{
    int table_size = aliyun_platform_get_routing_table_size() + ROUTING_TABLE_MARGIN;

    uint8_t *routing_table = MDF_MALLOC(table_size * ALIYUN_SUBDEVICE_ADDRS_MAXLEN);
    aliyun_platform_get_routing_table(routing_table, &table_size);
    MDF_LOGI("aliyun_platform_get_routing_table_size:%d", table_size);

    mdf_err_t ret = aliyun_list_select_refresh(routing_table, table_size);

    MDF_FREE(routing_table);
    return ret;
}
#endif

mdf_err_t aliyun_gateway_thing_sub_combine_logout(const aliyun_device_meta_t *gateway_meta, const aliyun_device_meta_t *sub_meta, aliyun_buffer_t *buffer)
{
    mdf_err_t ret = MDF_FAIL;

    uint32_t msg_id = aliyun_platform_get_msg_id();

    char *combine_logout_buf = aliyun_pack_thing_sub_combine_logout(msg_id, sub_meta);
    MDF_ERROR_CHECK(combine_logout_buf == NULL, MDF_FAIL, "aliyun_pack_thing_sub_topo_add is NULL");

    MDF_ERROR_CHECK(strlen(combine_logout_buf) > CONFIG_ALIYUN_PAYLOAD_SIZE, MDF_FAIL, "aliyun_pack_thing_sub_combine_logout payload big");
    strncpy((char *)buffer->payload, combine_logout_buf, strlen(combine_logout_buf));
    buffer->payload_len = strlen(combine_logout_buf);
    MDF_FREE(combine_logout_buf);

    ret = aliyun_list_update_msg_id(sub_meta, msg_id);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_list_update_msg_id error");

    ret = aliyun_list_update_status(sub_meta, ALIYUN_LIST_LOGOUT_WAIT, ALIYUN_SUB_STATUS_DEFAULT_TIMEOUT);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_list_update_status error");

    ret = aliyun_publish_sub_combine_logout(gateway_meta->product_key, gateway_meta->device_name, buffer);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_publish_sub_topo_add error");

    return MDF_OK;
}

static mdf_err_t aliyun_gateway_loop_process_timeout(const aliyun_device_meta_t *gateway_meta, aliyun_buffer_t *buffer)
{
    mdf_err_t ret = MDF_OK;
    /* Because delete list member is execute in one task, so it is safe to use reference rather than value*/
    static sub_info_list_t *subdevice_list[ALIYUN_PROCESS_SIZE];
    int unhandled_num = 0;

    ret = aliyun_list_get_by_timeout(subdevice_list, ALIYUN_PROCESS_SIZE, &unhandled_num);

    if (ret != MDF_OK) {
        return ret;
    }

    for (int i = 0; i < unhandled_num; i++) {
        sub_info_list_t *subdevice = subdevice_list[i];

        switch (subdevice->status) {
            case ALIYUN_LIST_TOPO_ADD_WAIT:
                MDF_LOGI("ALIYUN_LIST_TOPO_ADD_WAIT, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);
                ret = aliyun_gateway_thing_sub_topo_add(gateway_meta, &subdevice->meta, buffer);
                MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "ALIYUN_LIST_TOPO_ADD_WAIT, product_key:%s, device_name:%s, status :%d, ret:%d",
                                subdevice->meta.product_key, subdevice->meta.device_name, subdevice->status, ret);
                break;

            case ALIYUN_LIST_TOPO_DELETE_WAIT:
                MDF_LOGI("ALIYUN_LIST_TOPO_DELETE_WAIT, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);
                ret = aliyun_gateway_thing_sub_topo_delete(gateway_meta, &subdevice->meta, buffer);
                MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "ALIYUN_LIST_TOPO_DELETE_WAIT, product_key:%s, device_name:%s, status :%d, ret:%d",
                                subdevice->meta.product_key, subdevice->meta.device_name, subdevice->status, ret);
                break;

            case ALIYUN_LIST_LOGIN_WAIT:
                MDF_LOGI("ALIYUN_LIST_LOGIN_WAIT, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);
                ret = aliyun_gateway_thing_sub_combine_login(gateway_meta, &subdevice->meta, buffer);
                MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "ALIYUN_LIST_LOGIN_WAIT, product_key:%s, device_name:%s, status :%d, ret:%d",
                                subdevice->meta.product_key, subdevice->meta.device_name, subdevice->status, ret);
                break;

            case ALIYUN_LIST_LOGOUT_WAIT:
                MDF_LOGI("ALIYUN_LIST_LOGOUT_WAIT, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);
                ret = aliyun_gateway_thing_sub_combine_logout(gateway_meta, &subdevice->meta, buffer);
                MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "ALIYUN_LIST_LOGOUT_WAIT, product_key:%s, device_name:%s, status :%d, ret:%d",
                                subdevice->meta.product_key, subdevice->meta.device_name, subdevice->status, ret);
                break;

            case ALIYUN_LIST_DELETE_WAIT:
                MDF_LOGI("ALIYUN_LIST_DELETE_WAIT, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);
                ret = aliyun_unsubscribe_subdevice_all_topic(&subdevice->meta, buffer);
                MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_unsubscribe_subdevice_all_topic error, product_key:%s, device_name:%s",
                                subdevice->meta.product_key, subdevice->meta.device_name);
                ret = aliyun_list_delete_meta(&subdevice->meta);
                MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_list_delete_meta error, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);
                break;

            default:
                MDF_LOGV("type is not find (%d)", subdevice->status);
                break;
        }
    }

    return ret;
}

static mdf_err_t aliyun_gateway_loop_process_unfinished(const aliyun_device_meta_t *gateway_meta, aliyun_buffer_t *buffer)
{
    mdf_err_t ret = MDF_OK;
    /* Because delete list member is execute in one task, so it is safe to use reference rather than  value*/
    static sub_info_list_t *subdevice_list[ALIYUN_PROCESS_SIZE];
    int unhandled_num = 0;

    ret = aliyun_list_get_by_unfinished(subdevice_list, ALIYUN_PROCESS_SIZE, &unhandled_num);

    if (ret != MDF_OK) {
        return ret;
    }

    for (int i = 0; i < unhandled_num; i++) {
        sub_info_list_t *subdevice = subdevice_list[i];

        switch (subdevice->status) {
            case ALIYUN_LIST_TOPO_ADD_START:
                MDF_LOGI("ALIYUN_LIST_TOPO_ADD_START, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);
                ret = aliyun_gateway_thing_sub_topo_add(gateway_meta, &subdevice->meta, buffer);
                MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_gateway_thing_sub_topo_add error, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);
                break;

            case ALIYUN_LIST_TOPO_ADD_SUCC:
                MDF_LOGI("ALIYUN_LIST_TOPO_ADD_SUCC, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);
                ret = aliyun_gateway_thing_sub_combine_login(gateway_meta, &subdevice->meta, buffer);
                MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_gateway_thing_sub_combine_login error, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);
                break;

            case ALIYUN_LIST_TOPO_ADD_FAIL:
                MDF_LOGW("ALIYUN_LIST_TOPO_ADD_FAIL, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);
                ret = aliyun_gateway_add_subdevice_reply(&subdevice->meta, MDF_FAIL);

                if (ret != MDF_OK) {
                    MDF_LOGW("aliyun_gateway_add_subdevice_reply error, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);
                }

                ret = aliyun_list_delete_meta(&subdevice->meta);
                MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_list_delete_meta error, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);
                break;

            case ALIYUN_LIST_LOGIN_SUCC:
                MDF_LOGI("ALIYUN_LIST_LOGIN_SUCC, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);

                if (subdevice->no_subscribe != true) {
                    ret = aliyun_subscribe_subdevice_all_topic(&subdevice->meta, buffer);
                    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "Subdevice subscribe error, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);
                }

                ret = aliyun_gateway_add_subdevice_reply(&subdevice->meta, MDF_OK);

                if (ret != MDF_OK) {
                    MDF_LOGW("Gateway add reply error, product_key:%s, device_name:%s, reason:%s", subdevice->meta.product_key, subdevice->meta.device_name, mdf_err_to_name(ret));
                }

                ret = aliyun_list_update_status(&subdevice->meta, ALIYUN_LIST_FINISH, ALIYUN_SUB_STATUS_DEFAULT_TIMEOUT);
                MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_list_update_status error, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);
                subdevice->no_subscribe = false;
                break;

            case ALIYUN_LIST_LOGIN_FAIL:
                MDF_LOGW("ALIYUN_LIST_LOGIN_FAIL, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);
                ret = aliyun_gateway_add_subdevice_reply(&subdevice->meta, MDF_FAIL);

                if (ret != MDF_OK) {
                    MDF_LOGW("Gateway add reply error, product_key:%s, device_name:%s, reason:%s", subdevice->meta.product_key, subdevice->meta.device_name, mdf_err_to_name(ret));
                }

                ret = aliyun_list_delete_meta(&subdevice->meta);
                MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_list_delete_meta error, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);
                break;

            case ALIYUN_LIST_LOGOUT_START:
                MDF_LOGI("ALIYUN_LIST_LOGOUT_START, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);
                ret = aliyun_gateway_thing_sub_combine_logout(gateway_meta, &subdevice->meta, buffer);
                MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_gateway_thing_sub_combine_logout error, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);
                break;

            case ALIYUN_LIST_LOGOUT_SUCC:
                MDF_LOGI("ALIYUN_LIST_LOGOUT_SUCC, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);
                ret = aliyun_gateway_thing_sub_topo_delete(gateway_meta, &subdevice->meta, buffer);
                MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_gateway_thing_sub_topo_delete error, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);
                break;

            case ALIYUN_LIST_LOGOUT_FAIL:
                MDF_LOGW("ALIYUN_LIST_LOGOUT_FAIL, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);

                if (aliyun_list_is_device_in_routing_table(subdevice->addrs) == true) {
                    MDF_LOGW("Subdevice is online, Relogin");
                    aliyun_list_update_status(&subdevice->meta, ALIYUN_LIST_TOPO_ADD_SUCC, ALIYUN_SUB_STATUS_DELETE_TIMEOUT);
                } else {
                    MDF_LOGW("Subdevice is offline, start topo delete");
                    aliyun_list_update_status(&subdevice->meta, ALIYUN_LIST_LOGOUT_SUCC, ALIYUN_SUB_STATUS_DEFAULT_TIMEOUT);
                }

                break;

            case ALIYUN_LIST_TOPO_DELETE_SUCC:
                MDF_LOGI("ALIYUN_LIST_TOPO_DELETE_SUCC, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name); //ret = aliyun_unsubscribe_subdevice_all_topic(&sub_meta, buffer);
                ret = aliyun_gateway_delete_subdevice_reply(&subdevice->meta, MDF_OK);

                if (ret != MDF_OK) {
                    MDF_LOGW("Gateway delete reply error, pruduct_key:%s, device_name:%s, reason:%s", subdevice->meta.product_key, subdevice->meta.device_name, mdf_err_to_name(ret));
                }

                ret = aliyun_list_update_status(&subdevice->meta, ALIYUN_LIST_DELETE_WAIT, ALIYUN_SUB_STATUS_DELETE_TIMEOUT);
                MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_list_update_status error, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);
                break;

            case ALIYUN_LIST_TOPO_DELETE_FAIL:
                MDF_LOGW("ALIYUN_LIST_DELETE_FAIL, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);

                if (aliyun_list_is_device_in_routing_table(subdevice->addrs) == true) {
                    MDF_LOGW("Subdevice is online, re-add");
                    aliyun_list_update_status(&subdevice->meta, ALIYUN_LIST_TOPO_ADD_START, ALIYUN_SUB_STATUS_DEFAULT_TIMEOUT);
                } else {
                    MDF_LOGW("Subdevice is offline, wait for delete");
                    aliyun_list_update_status(&subdevice->meta, ALIYUN_LIST_DELETE_WAIT, ALIYUN_SUB_STATUS_DELETE_TIMEOUT);
                }

                break;

            case ALIYUN_LIST_DELETE:
                MDF_LOGI("ALIYUN_LIST_DELETE, product_key:%s, device_name:%s", subdevice->meta.product_key, subdevice->meta.device_name);
                ret = aliyun_unsubscribe_subdevice_all_topic(&subdevice->meta, buffer);
                MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_unsubscribe_subdevice_all_topic error, product_key:%s, device_name:%s",
                                subdevice->meta.product_key, subdevice->meta.device_name);
                ret = aliyun_gateway_thing_sub_combine_logout(gateway_meta, &subdevice->meta, buffer);
                MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_gateway_thing_sub_combine_logout error, product_key:%s, device_name:%s",
                                subdevice->meta.product_key, subdevice->meta.device_name);
                ret = aliyun_gateway_thing_sub_topo_delete(gateway_meta, &subdevice->meta, buffer);
                MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_gateway_thing_sub_topo_delete error, product_key:%s, device_name:%s",
                                subdevice->meta.product_key, subdevice->meta.device_name);
                aliyun_gateway_delete_subdevice_reply(&subdevice->meta, MDF_OK);
                ret = aliyun_list_delete_meta(&subdevice->meta);
                MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_list_delete_meta error, product_key:%s, device_name:%s",
                                subdevice->meta.product_key, subdevice->meta.device_name);
                break;

            default:
                MDF_LOGV("type is not find: %d", subdevice->status);
                break;
        }
    }

    return ret;
}

static mdf_err_t aliyun_gateway_loop_process(const aliyun_device_meta_t *gateway_meta, aliyun_buffer_t *buffer)
{
    aliyun_gateway_loop_process_unfinished(gateway_meta, buffer);
    aliyun_gateway_loop_process_timeout(gateway_meta, buffer);
    return MDF_OK;
}

static mdf_err_t aliyun_gateway_subdevice_process(int type, const uint8_t *src_addr, aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(src_addr);
    MDF_PARAM_CHECK(buffer);

    mdf_err_t ret = MDF_OK;

    switch (type) {
        case ALIYUN_GATEWAY_ADD_SUBDEVICE: {
            aliyun_device_meta_t *sub_meta = (aliyun_device_meta_t *)buffer->payload;
            sub_info_list_t *subdevice;
            MDF_LOGI("Gateway add: product_key=%s, device_name=%s, mac=" MACSTR, sub_meta->product_key, sub_meta->device_name, MAC2STR(src_addr));

            if (aliyun_list_get_by_meta(sub_meta, &subdevice) == MDF_OK) {
                if (subdevice->status == ALIYUN_LIST_FINISH) {
                    ret = aliyun_gateway_add_subdevice_reply(sub_meta, MDF_OK);

                    if (ret != MDF_OK) {
                        MDF_LOGW("Gateway add reply error, reason %s", mdf_err_to_name(ret));
                    }
                } else if (subdevice->status == ALIYUN_LIST_DELETE_WAIT) {
                    ret = aliyun_list_update_status(sub_meta, ALIYUN_LIST_TOPO_ADD_START, ALIYUN_SUB_STATUS_DEFAULT_TIMEOUT);
                    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_list_update_status error, product_key=%s, device_name=%s, mac=" MACSTR, sub_meta->product_key, sub_meta->device_name, MAC2STR(src_addr));
                }
            } else {
                ret = aliyun_list_insert_meta(sub_meta, src_addr, ALIYUN_DEVICE_TYPE_SUBDEVICE);
                MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_list_insert_meta error, product_key=%s, device_name=%s, mac=" MACSTR, sub_meta->product_key, sub_meta->device_name, MAC2STR(src_addr));
            }
        }
        break;

        case ALIYUN_GATEWAY_DELETE_SUBDEVICE: {
            aliyun_device_meta_t *sub_meta = (aliyun_device_meta_t *)buffer->payload;
            sub_info_list_t *subdevice;
            MDF_LOGI("Gateway delete: product_key=%s, device_name=%s, mac=" MACSTR, sub_meta->product_key, sub_meta->device_name, MAC2STR(src_addr));

            if (aliyun_list_get_by_meta(sub_meta, &subdevice) == MDF_OK) {
                if (subdevice->status == ALIYUN_LIST_FINISH) {
                    /* if subdevice delete itself, then directly delete */
                    ret = aliyun_list_update_status(sub_meta, ALIYUN_LIST_DELETE, ALIYUN_SUB_STATUS_DEFAULT_TIMEOUT);
                    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_list_update_status error, product_key=%s, device_name=%s, mac=" MACSTR, sub_meta->product_key, sub_meta->device_name, MAC2STR(src_addr));
                } else {
                    /* if subdevice delete itself, but subdevice has not finish add process, return FAIL */
                    ret = aliyun_gateway_delete_subdevice_reply(sub_meta, MDF_FAIL);

                    if (ret != MDF_OK) {
                        MDF_LOGW("Gateway delete reply error, reason %s", mdf_err_to_name(ret));
                    }
                }
            } else {
                ret = aliyun_gateway_delete_subdevice_reply(sub_meta, MDF_OK);

                if (ret != MDF_OK) {
                    MDF_LOGW("Gateway delete reply error, reason %s", mdf_err_to_name(ret));
                }
            }
        }
        break;

        case ALIYUN_SUBDEVICE_HEARTBEAT: {
            MDF_LOGD("ALIYUN_SUBDEVICE_HEARTBEAT from" MACSTR, MAC2STR(src_addr));
            aliyun_device_meta_t sub_meta = { 0 };
            int status = MDF_OK;
            ret = aliyun_list_select_meta(src_addr, &sub_meta);

            if (ret != MDF_OK) {
                status = MDF_FAIL;
            }

            ret = aliyun_mqtt_get_connet_status();

            if (ret != MDF_OK) {
                status = MDF_FAIL;
            }

            ret = aliyun_gateway_heartbeat_subdevice_reply(src_addr, status);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_gateway_heartbeat_subdevice_reply error");
        }
        break;

        default:
            MDF_LOGE("type is not find :%d", type);
            break;
    }

    return ret;
}

static mdf_err_t aliyun_gateway_publish_process(int type, const uint8_t *src_addr, aliyun_buffer_t *buffer)
{
    MDF_PARAM_CHECK(src_addr);
    MDF_PARAM_CHECK(buffer);

    mdf_err_t ret = MDF_OK;
    sub_info_list_t *subdevice;

    ret = aliyun_list_get_by_addr(src_addr, &subdevice);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_list_get_by_addr src_addr not find, type=%d, src_addr=" MACSTR,
                    type, MAC2STR(src_addr));

    if (subdevice->status != ALIYUN_LIST_FINISH) {
        return MDF_FAIL;
    }

    switch (type) {

        case ALIYUN_COAP_SYS_NOTIFY_DEVICE_INFO:
            MDF_LOGD("ALIYUN_COAP_SYS_NOTIFY_DEVICE_INFO");
            ret = aliyun_coap_send_app((char *)ALIYUN_URL_SYS_DEVICE_INFO_NOTIFY, buffer->payload, buffer->payload_len);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_coap_send_app info notify: %s", buffer->payload);
            break;

        case ALIYUN_COAP_SYS_EVENT_CONNECTAP_NOTIFY:
            MDF_LOGD("ALIYUN_COAP_SYS_EVENT_CONNECTAP_NOTIFY");
            ret = aliyun_coap_send_app((char *)ALIYUN_URL_SYS_AWSS_EVENT_CONNECTAP_NOTIFY, buffer->payload, buffer->payload_len);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_coap_send_app connectap notify: %s", buffer->payload);
            break;

        case ALIYUN_MQTT_PUBLISH_AWSS_ENROLLEE_MATCH:
            MDF_LOGD("ALIYUN_MQTT_PUBLISH_AWSS_ENROLLEE_MATCH");
            ret = aliyun_publish_thing_awss_enrollee_match(subdevice->meta.product_key, subdevice->meta.device_name, buffer);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_publish_thing_awss_enrollee_match: %s", buffer->payload);
            break;

        case ALIYUN_MQTT_PUBLISH_RESET:
            MDF_LOGD("ALIYUN_MQTT_PUBLISH_RESET");
            ret = aliyun_publish_thing_reset(subdevice->meta.product_key, subdevice->meta.device_name, buffer);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_publish_thing_reset: %s", buffer->payload);
            break;

        case ALIYUN_MQTT_PUBLISH_CIPHER_GET:
            MDF_LOGD("ALIYUN_MQTT_PUBLISH_CIPHER_GET");
            ret = aliyun_publish_thing_cipher_get(subdevice->meta.product_key, subdevice->meta.device_name, buffer);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_publish_thing_cipher_get: %s", buffer->payload);
            break;

        case ALIYUN_MQTT_PUBLISH_EVENT_PROPERTY_POST:
            MDF_LOGD("ALIYUN_MQTT_PUBLISH_EVENT_PROPERTY_POST");
            ret = aliyun_publish_thing_event_property_post(subdevice->meta.product_key, subdevice->meta.device_name, buffer);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_publish_thing_event_property_post: %s", buffer->payload);
            break;

        case ALIYUN_MQTT_PUBLISH_SET_SERVICE_PROPERTY_REPLY:
            MDF_LOGD("ALIYUN_MQTT_PUBLISH_SET_SERVICE_PROPERTY_REPLY");
            ret = aliyun_publish_thing_service_property_set_reply(subdevice->meta.product_key, subdevice->meta.device_name, buffer);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_publish_thing_service_property_set_reply: %s", buffer->payload);
            break;

        case ALIYUN_MQTT_PUBLISH_EVENT_POST:
            MDF_LOGD("ALIYUN_MQTT_PUBLISH_EVENT_POST");
            ret = aliyun_publish_thing_event_post(subdevice->meta.product_key, subdevice->meta.device_name, buffer);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_publish_thing_event_post: %s", buffer->payload);
            break;

        case ALIYUN_MQTT_PUBLISH_RESPONSE_SERVICE:
            MDF_LOGD("ALIYUN_MQTT_PUBLISH_RESPONSE_SERVICE");
            ret = aliyun_publish_thing_service_response(subdevice->meta.product_key, subdevice->meta.device_name, buffer);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_publish_thing_service_response: %s", buffer->payload);
            break;

        case ALIYUN_MQTT_PUBLISH_UPDATE_DEVICEINFO:
            MDF_LOGD("ALIYUN_MQTT_PUBLISH_UPDATE_DEVICEINFO");
            ret = aliyun_publish_thing_deviceinfo_update(subdevice->meta.product_key, subdevice->meta.device_name, buffer);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_publish_thing_deviceinfo_update: %s", buffer->payload);
            break;

        case ALIYUN_MQTT_PUBLISH_DELETE_DEVICEINFO:
            MDF_LOGD("ALIYUN_MQTT_PUBLISH_DELETE_DEVICEINFO");
            ret = aliyun_publish_thing_deviceinfo_delete(subdevice->meta.product_key, subdevice->meta.device_name, buffer);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_publish_thing_deviceinfo_delete: %s", buffer->payload);
            break;

        case ALIYUN_MQTT_PUBLISH_GET_CONFIG:
            MDF_LOGD("ALIYUN_MQTT_PUBLISH_GET_CONFIG");
            ret = aliyun_publish_thing_config_get(subdevice->meta.product_key, subdevice->meta.device_name, buffer);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_publish_thing_config_get: %s", buffer->payload);
            break;

        case ALIYUN_MQTT_PUBLISH_CONFIG_PUSH_REPLY:
            MDF_LOGD("ALIYUN_MQTT_PUBLISH_CONFIG_PUSH_REPLY");
            ret = aliyun_publish_thing_config_push_reply(subdevice->meta.product_key, subdevice->meta.device_name, buffer);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_publish_thing_config_push_reply: %s", buffer->payload);
            break;

        case ALIYUN_MQTT_PUBLISH_OTA_DEVICE_INFORM:
            MDF_LOGD("ALIYUN_MQTT_PUBLISH_OTA_DEVICE_INFORM");
            ret = aliyun_publish_ota_device_inform(subdevice->meta.product_key, subdevice->meta.device_name, buffer);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_publish_thing_config_push_reply: %s", buffer->payload);
            break;

        case ALIYUN_MQTT_PUBLISH_OTA_DEVICE_PROGRESS:
            MDF_LOGD("ALIYUN_MQTT_PUBLISH_OTA_DEVICE_PROGRESS");
            ret = aliyun_publish_ota_device_progress(subdevice->meta.product_key, subdevice->meta.device_name, buffer);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_publish_ota_device_progress: %s", buffer->payload);
            break;

        case ALIYUN_MQTT_PUBLISH_OTA_DEVICE_REQUEST:
            MDF_LOGD("ALIYUN_MQTT_PUBLISH_OTA_DEVICE_REQUEST");
            ret = aliyun_publish_ota_device_request(subdevice->meta.product_key, subdevice->meta.device_name, buffer);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_publish_ota_device_request: %s", buffer->payload);
            break;

        case ALIYUN_MQTT_PUBLISH_NTP_REQUEST: {
            MDF_LOGD("ALIYUN_MQTT_PUBLISH_NTP_REQUEST");
            char local_device_name[ALIYUN_DEVICE_NAME_MAXLEN];
            aliyun_get_device_name(local_device_name, sizeof(local_device_name));

            if (strncmp(local_device_name, subdevice->meta.device_name, sizeof(local_device_name)) == 0) {
                ret = aliyun_publish_ntp_request(subdevice->meta.product_key, subdevice->meta.device_name, buffer);
                MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_publish_ntp_request: %s", buffer->payload);
            } else {
                /* Cloud does not accept subdevice send ntp request through gateway. Once gateway receive
                        ntp request from subdevice, gateway handle it by itself */
                char str[128];
                int64_t device_send_time;
                cJSON *request = cJSON_Parse((char *)buffer->payload);
                assert(request != NULL); //TODO cjson exception handle
                cJSON *item = cJSON_GetObjectItem(request, "deviceSendTime");
                assert(item != NULL);
                device_send_time = (int64_t)item->valuedouble;
                cJSON_Delete(request);
                int data_len = snprintf(str, sizeof(str), ntp_response_template, device_send_time, aliyun_platform_get_utc_ms(), aliyun_platform_get_utc_ms());
                ret = aliyun_platform_gateway_write(subdevice->addrs, ALIYUN_MQTT_REQUEST_NTP_RESPONSE, str, data_len);
            }

            break;
        }

        case ALIYUN_MQTT_PUBLISH_GET_DSLTEMPLATE:
            MDF_LOGD("ALIYUN_MQTT_PUBLISH_GET_DSLTEMPLATE");
            ret = aliyun_publish_thing_dsltemplate_get(subdevice->meta.product_key, subdevice->meta.device_name, buffer);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_publish_thing_dsltemplate_get: %s", buffer->payload);
            break;

        case ALIYUN_MQTT_PUBLISH_GET_DESIRED_PROPERTY:
            MDF_LOGD("ALIYUN_MQTT_PUBLISH_GET_DESIRED_PROPERTY");
            ret = aliyun_publish_thing_property_desired_get(subdevice->meta.product_key, subdevice->meta.device_name, buffer);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_publish_thing_property_desired_get: %s", buffer->payload);
            break;

        case ALIYUN_MQTT_PUBLISH_DELETE_DESIRED_PROPERTY:
            MDF_LOGD("ALIYUN_MQTT_PUBLISH_DELETE_DESIRED_PROPERTY");
            ret = aliyun_publish_thing_property_desired_delete(subdevice->meta.product_key, subdevice->meta.device_name, buffer);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_publish_thing_property_desired_delete: %s", buffer->payload);
            break;

        case ALIYUN_MQTT_PUBLISH_POST_LOG:
            MDF_LOGD("ALIYUN_MQTT_PUBLISH_POST_LOG");
            ret = aliyun_publish_thing_log_post(subdevice->meta.product_key, subdevice->meta.device_name, buffer);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_publish_thing_log_post: %s", buffer->payload);
            break;

        default:
            MDF_LOGV("type is not find :%d", type);
            break;
    }

    return ret;
}

static mdf_err_t aliyun_gateway_process(int type, uint8_t *src_addr, aliyun_buffer_t *buffer)
{
    switch (type) {
        case ALIYUN_GATEWAY_ADD_SUBDEVICE:
        case ALIYUN_GATEWAY_DELETE_SUBDEVICE:
        case ALIYUN_SUBDEVICE_HEARTBEAT:
            return aliyun_gateway_subdevice_process(type, src_addr, buffer);

        case ALIYUN_COAP_SYS_NOTIFY_DEVICE_INFO:
        case ALIYUN_COAP_SYS_EVENT_CONNECTAP_NOTIFY:
        case ALIYUN_MQTT_PUBLISH_AWSS_ENROLLEE_MATCH:
        case ALIYUN_MQTT_PUBLISH_RESET:
        case ALIYUN_MQTT_PUBLISH_CIPHER_GET:
        case ALIYUN_MQTT_PUBLISH_EVENT_PROPERTY_POST:
        case ALIYUN_MQTT_PUBLISH_SET_SERVICE_PROPERTY_REPLY:
        case ALIYUN_MQTT_PUBLISH_EVENT_POST:
        case ALIYUN_MQTT_PUBLISH_RESPONSE_SERVICE:
        case ALIYUN_MQTT_PUBLISH_UPDATE_DEVICEINFO:
        case ALIYUN_MQTT_PUBLISH_DELETE_DEVICEINFO:
        case ALIYUN_MQTT_PUBLISH_GET_CONFIG:
        case ALIYUN_MQTT_PUBLISH_CONFIG_PUSH_REPLY:
        case ALIYUN_MQTT_PUBLISH_OTA_DEVICE_INFORM:
        case ALIYUN_MQTT_PUBLISH_OTA_DEVICE_PROGRESS:
        case ALIYUN_MQTT_PUBLISH_OTA_DEVICE_REQUEST:
        case ALIYUN_MQTT_PUBLISH_NTP_REQUEST:
        case ALIYUN_MQTT_PUBLISH_GET_DSLTEMPLATE:
        case ALIYUN_MQTT_PUBLISH_GET_DESIRED_PROPERTY:
        case ALIYUN_MQTT_PUBLISH_DELETE_DESIRED_PROPERTY:
        case ALIYUN_MQTT_PUBLISH_POST_LOG:
            return aliyun_gateway_publish_process(type, src_addr, buffer);

        default:
            MDF_LOGV("type is not find :%d", type);
            break;
    }

    return MDF_OK;
}

#ifdef CONFIG_ALIYUN_PLATFORM_MDF
static void delay_refresh_handler(xTimerHandle xtimer)
{
    xSemaphoreGive(g_refresh_device_list_sepr);
}
#endif

static void aliyun_gateway_read_task(void *arg)
{
    int *aliyun_gateway_read_task_status = (int *)arg;
    mdf_err_t ret = MDF_OK;
    bool is_login = false;

    uint8_t src_addr[ALIYUN_SUBDEVICE_ADDRS_MAXLEN] = { 0 };
    uint8_t gateway_addr[ALIYUN_SUBDEVICE_ADDRS_MAXLEN] = { 0 };

    aliyun_msg_type_t type = ALIYUN_MSG_TYPE_MAX;
    aliyun_device_meta_t gateway_meta = { 0 };
    aliyun_mqtt_config_t mqtt_config = { 0 };
    aliyun_buffer_t *buffer = MDF_MALLOC(sizeof(aliyun_buffer_t) + CONFIG_ALIYUN_TOPIC_SIZE + CONFIG_ALIYUN_PAYLOAD_SIZE + 2);
    buffer->topic = (char *)buffer->data;
    buffer->payload = buffer->data + CONFIG_ALIYUN_TOPIC_SIZE + 1;

#ifdef CONFIG_ALIYUN_PLATFORM_MDF
    xTimerHandle delay_refresh_timer = xTimerCreate("delay_refresh", pdMS_TO_TICKS(1000), false, NULL, delay_refresh_handler);
#endif

    MDF_LOGI("aliyun_gateway_read_task is running");

    aliyun_get_meta(&gateway_meta);

    aliyun_platform_get_device_mac(gateway_addr);

    MDF_LOGI("gateway gateway_addr: " MACSTR, MAC2STR(gateway_addr));

    aliyun_list_insert_meta(&gateway_meta, gateway_addr, ALIYUN_DEVICE_TYPE_GATEWAY);
    aliyun_list_update_status(&gateway_meta, ALIYUN_LIST_FINISH, ALIYUN_SUB_STATUS_DEFAULT_TIMEOUT);

    aliyun_sign_mqtt_config(ALIYUN_MQTT_REGION_SHANGHAI, &gateway_meta, &mqtt_config);

    aliyun_mqtt_connect(&mqtt_config);

    esp_log_level_set("mwifi", ESP_LOG_ERROR);

    while (*aliyun_gateway_read_task_status == TASK_RUNNING) {
        memset(buffer->topic, 0, CONFIG_ALIYUN_TOPIC_SIZE + 1);
        memset(buffer->payload, 0, CONFIG_ALIYUN_PAYLOAD_SIZE + 1);

        if (aliyun_mqtt_get_connet_status() != MDF_OK) { // if mqtt is disconnect, then ignore all data from subdevice
            is_login = false;
            buffer->payload_len = CONFIG_ALIYUN_PAYLOAD_SIZE;
            aliyun_platform_gateway_read(src_addr, &type, buffer->payload, &buffer->payload_len, CONFIG_ALIYUN_READ_TIMROUT_MS / portTICK_RATE_MS);
            continue;
        }

        if (!aliyun_platform_get_cloud_connect_status() || !is_login) {
            aliyun_subscribe_gateway_all_topic(&gateway_meta, buffer);
            aliyun_list_update_subdevice_reconnet();
            aliyun_platform_set_cloud_connect_status(true);
            is_login = true;
        }

        buffer->payload_len = CONFIG_ALIYUN_PAYLOAD_SIZE;
        ret = aliyun_platform_gateway_read(src_addr, &type, buffer->payload, &buffer->payload_len, CONFIG_ALIYUN_READ_TIMROUT_MS / portTICK_RATE_MS);

        if (ret == MDF_OK) {
            aliyun_gateway_process(type, src_addr, buffer);
        } else if (ret == MDF_ERR_TIMEOUT || ret == ESP_ERR_MESH_TIMEOUT) {
            aliyun_gateway_loop_process(&gateway_meta, buffer);
        } else {
            MDF_LOGE("aliyun_platform_gateway_read error, ret: 0x%x", ret);
        }

#ifdef CONFIG_ALIYUN_PLATFORM_MDF

        if (g_refresh_device_list_sepr && (xSemaphoreTake(g_refresh_device_list_sepr, 0) == pdTRUE)) {
            ret = aliyun_gateway_refresh_subdevice_internal();

            if (ret == MDF_ERR_INVALID_STATE) {
                MDF_LOGD("need delay refresh");

                if (xTimerIsTimerActive(delay_refresh_timer)) {
                    xTimerReset(delay_refresh_timer, 0);
                } else {
                    xTimerStart(delay_refresh_timer, 0);
                }
            }
        }

#endif
    }

    MDF_FREE(aliyun_gateway_read_task_status);
    aliyun_mqtt_disconnet();
    aliyun_list_destroy();

    MDF_FREE(buffer);

#ifdef CONFIG_ALIYUN_PLATFORM_MDF

    if (g_refresh_device_list_sepr) {
        vSemaphoreDelete(g_refresh_device_list_sepr);
        g_refresh_device_list_sepr = NULL;
    }

#endif

    MDF_LOGW("aliyun_gateway_read_task is exit");
    vTaskDelete(NULL);
}

static void aliyun_gateway_write_task(void *arg)
{
    int *aliyun_gateway_write_task_status = (int *)arg;
    aliyun_buffer_t *mqtt_buf = NULL;

    MDF_LOGI("aliyun_gateway_write_task is running");

    while (*aliyun_gateway_write_task_status == TASK_RUNNING) {
        if (aliyun_platform_mqtt_read(&mqtt_buf, pdMS_TO_TICKS(1000)) == MDF_OK) {
            mdf_err_t err = aliyun_parse_mqtt_data((char *)mqtt_buf->topic, mqtt_buf->topic_len, (char *)mqtt_buf->payload, mqtt_buf->payload_len);

            if (err != MDF_OK) {
                MDF_LOGW("(%s(%d):%s(%d)", mqtt_buf->topic, mqtt_buf->topic_len, mqtt_buf->payload, mqtt_buf->payload_len);
            }

            MDF_FREE(mqtt_buf);
        }
    }

    MDF_FREE(aliyun_gateway_write_task_status);
    aliyun_platform_mqtt_deinit();
    MDF_LOGW("aliyun_gateway_write_task is exit");
    vTaskDelete(NULL);
}

mdf_err_t aliyun_gateway_init(void)
{
    aliyun_platform_mqtt_init();

    if (g_aliyun_read_task_status == NULL) {
        g_aliyun_read_task_status = MDF_MALLOC(sizeof(int));
        *g_aliyun_read_task_status = TASK_RUNNING;
        xTaskCreate(aliyun_gateway_read_task, "gateway_read", 5 * 1024,
                    g_aliyun_read_task_status, CONFIG_ALIYUN_TASK_DEFAULT_PRIOTY, NULL);
    }

    if (g_aliyun_write_task_status == NULL) {
        g_aliyun_write_task_status = MDF_MALLOC(sizeof(int));
        *g_aliyun_write_task_status = TASK_RUNNING;
        xTaskCreate(aliyun_gateway_write_task, "gateway_write", 4 * 1024,
                    g_aliyun_write_task_status, CONFIG_ALIYUN_TASK_DEFAULT_PRIOTY, NULL);
    }

    return MDF_OK;
}

mdf_err_t aliyun_gateway_deinit(void)
{
    if (g_aliyun_read_task_status != NULL) {
        *g_aliyun_read_task_status = TASK_STOP;
        g_aliyun_read_task_status = NULL;
    }

    if (g_aliyun_write_task_status != NULL) {
        *g_aliyun_write_task_status = TASK_STOP;
        g_aliyun_write_task_status = NULL;
    }

    return MDF_OK;
}

#ifdef CONFIG_ALIYUN_PLATFORM_MDF
mdf_err_t aliyun_gateway_refresh_subdevice(void)
{
    if (!g_refresh_device_list_sepr) {
        g_refresh_device_list_sepr = xSemaphoreCreateCounting(10, 0);
        assert(g_refresh_device_list_sepr);
    }

    xSemaphoreGive(g_refresh_device_list_sepr);
    return ESP_OK;
}
#endif
