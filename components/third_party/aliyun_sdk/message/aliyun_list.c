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

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "aliyun_list.h"
#include "aliyun_platform.h"
#include "mdf_mem.h"

#define ROUTING_TABLE_MARGIN (10)

static const char *TAG = "aliyun_list";

static SLIST_HEAD(sub_list_, sub_info_list) g_info_list;

static xSemaphoreHandle g_list_lock = NULL;

static void aliyun_list_mutex_lock(void)
{
    if (g_list_lock == NULL) {
        g_list_lock = xSemaphoreCreateMutex();
        configASSERT(g_list_lock);
        SLIST_INIT(&g_info_list);
    }

    while (xSemaphoreTake(g_list_lock, portMAX_DELAY) != pdPASS)
        ;
}

static void aliyun_list_mutex_unlock(void)
{
    xSemaphoreGive(g_list_lock);
}

mdf_err_t aliyun_list_destroy(void)
{
    aliyun_list_mutex_lock();

    sub_info_list_t *it, *tmp;
    SLIST_FOREACH_SAFE(it, &g_info_list, next, tmp) {
        SLIST_REMOVE(&g_info_list, it, sub_info_list, next);
        free(it);
    }
    aliyun_list_mutex_unlock();

    if (g_list_lock != NULL) {
        vQueueDelete(g_list_lock);
        g_list_lock = NULL;
    }

    return MDF_OK;
}

mdf_err_t aliyun_list_select_exist(const aliyun_device_meta_t *meta)
{
    MDF_PARAM_CHECK(meta);

    aliyun_list_mutex_lock();
    sub_info_list_t *it, *tmp;
    SLIST_FOREACH_SAFE(it, &g_info_list, next, tmp) {
        if (!memcmp(meta, &it->meta, sizeof(aliyun_device_meta_t))) {
            MDF_LOGI("List find product_key=%s, device_name=%s, mac=" MACSTR, it->meta.product_key, it->meta.device_name, MAC2STR(it->addrs));
            aliyun_list_mutex_unlock();
            return MDF_OK;
        }
    }
    aliyun_list_mutex_unlock();
    return MDF_FAIL;
}

mdf_err_t aliyun_list_get_by_meta(const aliyun_device_meta_t *meta, sub_info_list_t **sub_info)
{
    MDF_PARAM_CHECK(meta);
    MDF_PARAM_CHECK(sub_info);

    aliyun_list_mutex_lock();
    sub_info_list_t *it, *tmp;
    SLIST_FOREACH_SAFE(it, &g_info_list, next, tmp) {
        if (!memcmp(meta, &it->meta, sizeof(aliyun_device_meta_t))) {
            *sub_info = it;
            aliyun_list_mutex_unlock();
            return MDF_OK;
        }
    }
    aliyun_list_mutex_unlock();
    return MDF_FAIL;
}

mdf_err_t aliyun_list_insert_meta(const aliyun_device_meta_t *meta, const uint8_t addrs[], aliyun_device_type_t device_type)
{
    MDF_PARAM_CHECK(addrs);
    MDF_PARAM_CHECK(meta);

    mdf_err_t ret = MDF_OK;

    ret = aliyun_list_select_exist(meta);
    MDF_ERROR_CHECK(ret == MDF_OK, MDF_FAIL, "aliyun list select device exist, product_key: %s, device_name :%s", meta->product_key, meta->device_name);

    aliyun_list_mutex_lock();

    sub_info_list_t *item = (sub_info_list_t *)calloc(1, sizeof(sub_info_list_t));

    memcpy(item->addrs, addrs, ALIYUN_SUBDEVICE_ADDRS_MAXLEN);
    memcpy(&item->meta, meta, sizeof(aliyun_device_meta_t));
    item->no_subscribe = false;
    item->device_type = device_type;
    item->ota_type = ALIYUN_OTA_TYPE_NOT;
    item->status = ALIYUN_LIST_TOPO_ADD_START;

    sub_info_list_t *last = SLIST_FIRST(&g_info_list);

    if (last == NULL) {
        SLIST_INSERT_HEAD(&g_info_list, item, next);
    } else {
        sub_info_list_t *it;

        while ((it = SLIST_NEXT(last, next)) != NULL) {
            last = it;
        }

        SLIST_INSERT_AFTER(last, item, next);
    }

    aliyun_list_mutex_unlock();

    return MDF_OK;
}

mdf_err_t aliyun_list_delete_meta(const aliyun_device_meta_t *meta)
{
    MDF_PARAM_CHECK(meta);

    aliyun_list_mutex_lock();
    sub_info_list_t *it, *tmp;
    SLIST_FOREACH_SAFE(it, &g_info_list, next, tmp) {
        if (!memcmp(meta, &it->meta, sizeof(aliyun_device_meta_t))) {
            SLIST_REMOVE(&g_info_list, it, sub_info_list, next);
            free(it);
            aliyun_list_mutex_unlock();
            return MDF_OK;
        }
    }
    aliyun_list_mutex_unlock();
    return MDF_FAIL;
}

mdf_err_t aliyun_list_select_addrs(const aliyun_device_meta_t *meta, uint8_t addrs[])
{
    MDF_PARAM_CHECK(meta);
    MDF_PARAM_CHECK(addrs);

    aliyun_list_mutex_lock();
    sub_info_list_t *it, *tmp;
    SLIST_FOREACH_SAFE(it, &g_info_list, next, tmp) {
        if (!strcmp(it->meta.product_key, meta->product_key) && !strcmp(it->meta.device_name, meta->device_name)) {
            memcpy(addrs, it->addrs, ALIYUN_SUBDEVICE_ADDRS_MAXLEN);
            aliyun_list_mutex_unlock();
            return MDF_OK;
        }
    }
    aliyun_list_mutex_unlock();
    return MDF_FAIL;
}

mdf_err_t aliyun_list_get_by_addr(const uint8_t *addrs, sub_info_list_t **sub_info)
{
    MDF_PARAM_CHECK(addrs);
    MDF_PARAM_CHECK(sub_info);

    aliyun_list_mutex_lock();
    sub_info_list_t *it, *tmp;
    SLIST_FOREACH_SAFE(it, &g_info_list, next, tmp) {
        if (!memcmp(it->addrs, addrs, ALIYUN_SUBDEVICE_ADDRS_MAXLEN)) {
            *sub_info = it;
            aliyun_list_mutex_unlock();
            return MDF_OK;
        }
    }
    aliyun_list_mutex_unlock();
    return MDF_FAIL;
}

mdf_err_t aliyun_list_select_meta(const uint8_t *addrs, aliyun_device_meta_t *meta)
{
    MDF_PARAM_CHECK(addrs);
    MDF_PARAM_CHECK(meta);

    aliyun_list_mutex_lock();
    sub_info_list_t *it, *tmp;
    SLIST_FOREACH_SAFE(it, &g_info_list, next, tmp) {
        if (!memcmp(it->addrs, addrs, ALIYUN_SUBDEVICE_ADDRS_MAXLEN)) {
            memcpy(meta, &it->meta, sizeof(aliyun_device_meta_t));
            aliyun_list_mutex_unlock();
            return MDF_OK;
        }
    }
    aliyun_list_mutex_unlock();
    return MDF_FAIL;
}

mdf_err_t aliyun_list_select_status(const aliyun_device_meta_t *meta, uint32_t *status)
{
    MDF_PARAM_CHECK(meta);
    MDF_PARAM_CHECK(status);

    aliyun_list_mutex_lock();
    sub_info_list_t *it, *tmp;
    SLIST_FOREACH_SAFE(it, &g_info_list, next, tmp) {
        if (!strcmp(it->meta.product_key, meta->product_key) && !strcmp(it->meta.device_name, meta->device_name)) {
            *status = it->status;
            aliyun_list_mutex_unlock();
            return MDF_OK;
        }
    }
    aliyun_list_mutex_unlock();
    return MDF_FAIL;
}

static mdf_err_t aliyun_list_check_wait(uint32_t status)
{
    if (status == ALIYUN_LIST_TOPO_ADD_WAIT || status == ALIYUN_LIST_TOPO_DELETE_WAIT || status == ALIYUN_LIST_LOGIN_WAIT
            || status == ALIYUN_LIST_LOGOUT_WAIT || status == ALIYUN_LIST_DELETE_WAIT) {
        return MDF_OK;
    } else {
        return MDF_FAIL;
    }
}

mdf_err_t aliyun_list_select_refresh(uint8_t *routing_table, size_t table_size)
{
    MDF_PARAM_CHECK(routing_table);

    aliyun_list_mutex_lock();
    sub_info_list_t *it, *tmp;
    mdf_err_t ret = MDF_OK;
    SLIST_FOREACH_SAFE(it, &g_info_list, next, tmp) {
        bool exist_flag = false;

        for (int i = 0; i < table_size; i++) {
            if (!memcmp(routing_table + i * ALIYUN_SUBDEVICE_ADDRS_MAXLEN, it->addrs, ALIYUN_SUBDEVICE_ADDRS_MAXLEN)) {
                exist_flag = true;
                break;
            }
        }

        if (!exist_flag) { // doesn't exist in mesh network, means this device is leave, need logout for it
            if (it->status == ALIYUN_LIST_FINISH) {
                MDF_LOGD("aliyun_list_select_refresh product_key:%s, device_name:%s", it->meta.product_key, it->meta.device_name);
                it->status = ALIYUN_LIST_LOGOUT_START;
            } else if (ALIYUN_LIST_TOPO_ADD_START <= it->status && it->status < ALIYUN_LIST_FINISH) {
                ret = MDF_ERR_INVALID_STATE;
            }
        } else { //exist in mesh network
            if (it->status == ALIYUN_LIST_DELETE_WAIT) {
                it->status = ALIYUN_LIST_TOPO_ADD_START;
                it->no_subscribe = true;
            } else if (ALIYUN_LIST_DELETE_WAIT > it->status && it->status > ALIYUN_LIST_FINISH) {
                ret = MDF_ERR_INVALID_STATE;
            }
        }
    }
    aliyun_list_mutex_unlock();
    return ret;
}

mdf_err_t aliyun_list_get_by_timeout(sub_info_list_t **sub_list, size_t sub_list_sz, int *num)
{
    MDF_PARAM_CHECK(sub_list);

    sub_info_list_t *it, *tmp;
    mdf_err_t ret = MDF_OK;
    int i = 0;

    aliyun_list_mutex_lock();
    SLIST_FOREACH_SAFE(it, &g_info_list, next, tmp) {
        if (aliyun_list_check_wait(it->status) == MDF_OK && aliyun_platform_get_utc_ms() >= it->timeout) {
            sub_list[i++] = it;

            if (i >= sub_list_sz) {
                break;
            }
        }
    }
    aliyun_list_mutex_unlock();
    *num = i;

    if (i == 0) {
        ret = MDF_FAIL;
    }

    return ret;
}

mdf_err_t aliyun_list_get_by_unfinished(sub_info_list_t **sub_list, size_t sub_list_sz, int *num)
{
    MDF_PARAM_CHECK(sub_list);

    sub_info_list_t *it, *tmp;
    mdf_err_t ret = MDF_OK;
    aliyun_list_mutex_lock();

    int i = 0;
    SLIST_FOREACH_SAFE(it, &g_info_list, next, tmp) {
        if (aliyun_list_check_wait(it->status) != MDF_OK && it->status != ALIYUN_LIST_FINISH) {
            sub_list[i++] = it;

            if (i >= sub_list_sz) {
                break;
            }
        }
    }
    aliyun_list_mutex_unlock();
    *num = i;

    if (i == 0) {
        ret = MDF_FAIL;
    }

    return ret;
}

bool aliyun_list_is_device_in_routing_table(const uint8_t *addr)
{
    if (addr == NULL) {
        MDF_LOGE("parameter error(addr)");
        return false;
    }

    int table_size = aliyun_platform_get_routing_table_size() + ROUTING_TABLE_MARGIN;
    uint8_t *routing_table = MDF_MALLOC(table_size * ALIYUN_SUBDEVICE_ADDRS_MAXLEN);
    aliyun_platform_get_routing_table(routing_table, &table_size);

    for (int i = 0; i < table_size; i++) {
        if (memcmp(routing_table + i * ALIYUN_SUBDEVICE_ADDRS_MAXLEN, addr, ALIYUN_SUBDEVICE_ADDRS_MAXLEN) == 0) {
            MDF_FREE(routing_table);
            return true;
        }
    }

    MDF_FREE(routing_table);
    return false;
}

mdf_err_t aliyun_list_update_status(const aliyun_device_meta_t *meta, uint32_t status, uint32_t timeout)
{
    MDF_PARAM_CHECK(meta);

    aliyun_list_mutex_lock();
    sub_info_list_t *it, *tmp;
    SLIST_FOREACH_SAFE(it, &g_info_list, next, tmp) {
        if (!strcmp(it->meta.product_key, meta->product_key) && !strcmp(it->meta.device_name, meta->device_name)) {
            it->status = status;
            it->timeout = aliyun_platform_get_utc_ms() + timeout;
            aliyun_list_mutex_unlock();
            return MDF_OK;
        }
    }
    aliyun_list_mutex_unlock();
    return MDF_FAIL;
}

mdf_err_t aliyun_list_update_subdevice_reconnet(void)
{
    aliyun_list_mutex_lock();
    sub_info_list_t *it, *tmp;
    SLIST_FOREACH_SAFE(it, &g_info_list, next, tmp) {
        if (it->device_type == ALIYUN_DEVICE_TYPE_SUBDEVICE) {
            MDF_LOGW("product_key:%s, device_name:%s", it->meta.product_key, it->meta.device_name);
            it->status = ALIYUN_LIST_TOPO_ADD_START;
        }
    }
    aliyun_list_mutex_unlock();
    return MDF_OK;
}

mdf_err_t aliyun_list_update_msg_id(const aliyun_device_meta_t *meta, uint32_t msg_id)
{
    MDF_PARAM_CHECK(meta);

    aliyun_list_mutex_lock();
    sub_info_list_t *it, *tmp;
    SLIST_FOREACH_SAFE(it, &g_info_list, next, tmp) {
        if (!strcmp(it->meta.product_key, meta->product_key) && !strcmp(it->meta.device_name, meta->device_name)) {
            it->msg_id = msg_id;
            aliyun_list_mutex_unlock();
            return MDF_OK;
        }
    }
    aliyun_list_mutex_unlock();
    return MDF_FAIL;
}

mdf_err_t aliyun_list_update_msg_status(uint32_t msg_id, uint32_t status)
{
    aliyun_list_mutex_lock();
    sub_info_list_t *it, *tmp;
    SLIST_FOREACH_SAFE(it, &g_info_list, next, tmp) {
        if (it->msg_id == msg_id) {
            it->status = status;
            aliyun_list_mutex_unlock();
            return MDF_OK;
        }
    }
    aliyun_list_mutex_unlock();
    return MDF_FAIL;
}

mdf_err_t aliyun_list_update_ota_type(const char *product_key, const char *device_name, aliyun_ota_type_t type)
{
    MDF_PARAM_CHECK(product_key);
    MDF_PARAM_CHECK(device_name);

    aliyun_list_mutex_lock();
    sub_info_list_t *it, *tmp;
    SLIST_FOREACH_SAFE(it, &g_info_list, next, tmp) {
        if (!strcmp(it->meta.product_key, product_key) && !strcmp(it->meta.device_name, device_name)) {
            it->ota_type = type;
            aliyun_list_mutex_unlock();
            return MDF_OK;
        }
    }
    aliyun_list_mutex_unlock();
    return MDF_FAIL;
}

uint32_t aliyun_list_select_ota_start_addrs_num(void)
{
    uint32_t count = 0;

    aliyun_list_mutex_lock();
    sub_info_list_t *it, *tmp;
    SLIST_FOREACH_SAFE(it, &g_info_list, next, tmp) {
        if (it->ota_type == ALIYUN_OTA_TYPE_START) {
            count++;
        }
    }
    aliyun_list_mutex_unlock();
    return count;
}

static bool aliyun_list_check_addrs_repetition(uint8_t *addrs_list, uint32_t addrs_num, uint8_t *addrs)
{
    for (int i = 0; i < addrs_num; i++) {
        if (!memcmp(addrs_list + i * ALIYUN_SUBDEVICE_ADDRS_MAXLEN, addrs, ALIYUN_SUBDEVICE_ADDRS_MAXLEN)) {
            return true;
        }
    }

    return false;
}

uint32_t aliyun_list_select_ota_start_addrs_list(uint8_t *addrs_list, size_t size)
{
    MDF_PARAM_CHECK(addrs_list);

    uint32_t count = 0;
    aliyun_list_mutex_lock();
    sub_info_list_t *it, *tmp;
    SLIST_FOREACH_SAFE(it, &g_info_list, next, tmp) {
        if (it->ota_type == ALIYUN_OTA_TYPE_START && count * ALIYUN_SUBDEVICE_ADDRS_MAXLEN <= size) {
            if (!aliyun_list_check_addrs_repetition(addrs_list, count, it->addrs)) {
                memcpy(addrs_list + count * ALIYUN_SUBDEVICE_ADDRS_MAXLEN, it->addrs, ALIYUN_SUBDEVICE_ADDRS_MAXLEN);
                MDF_LOGD("ddrs: " MACSTR, MAC2STR(it->addrs));
                count++;
                it->ota_type = ALIYUN_OTA_TYPE_NOT;
            }
        }
    }
    aliyun_list_mutex_unlock();
    return count;
}
