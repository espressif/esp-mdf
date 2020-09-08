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
#include "mupgrade.h"
#include "mdf_event_loop.h"
#include "mwifi.h"

#include "aliyun_list.h"
#include "aliyun_mdf.h"
#include "mdf_mem.h"

static const char *TAG = "aliyun_mdf";

bool aliyun_mdf_get_is_gateway(void)
{
    return esp_mesh_is_root();
}

mdf_err_t aliyun_mdf_set_cloud_connect_status(bool status)
{
    return mwifi_post_root_status(status);
}

bool aliyun_mdf_get_cloud_connect_status(void)
{
    return mwifi_get_root_status();
}

mdf_err_t aliyun_mdf_gateway_write(const uint8_t *dest_addrs, uint32_t dest_addrs_num, aliyun_msg_type_t type, const void *data, size_t size)
{
    MDF_PARAM_CHECK(data);

    aliyun_mesh_t mesh;
    mesh.mesh.type = type;
    mesh.mesh.version = ALIYUN_MESH_PROTOCOL_VERSION;
    mdf_err_t ret = MDF_FAIL;
    mwifi_data_type_t data_type = {
        .communicate = MWIFI_COMMUNICATE_MULTICAST,
        .protocol = ALIYUN_MDF_PROTOCL,
        .custom = mesh.value,
    };

    ret = mwifi_root_write(dest_addrs, dest_addrs_num, &data_type, data, size, true);
    return ret;
}

mdf_err_t aliyun_mdf_gateway_read(uint8_t *src_addr, aliyun_msg_type_t *type,
                                  uint8_t *data, size_t *size, uint32_t wait_ticks)
{
    mdf_err_t ret = MDF_OK;
    mwifi_data_type_t data_type = { 0 };

    ret = mwifi_root_read(src_addr, &data_type, data, size, wait_ticks);

    if (ret == MDF_OK) {
        *type = data_type.custom;

        if (data_type.upgrade) { // This mesh package contains upgrade data.
            ret = mupgrade_root_handle(src_addr, data, *size);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "mupgrade_handle Error");
            return MDF_ERR_TIMEOUT;
        }

        if (data_type.protocol == ALIYUN_MDF_PROTOCL) {
            return MDF_OK;
        } else {
            return MDF_ERR_CUSTOM_BASE;
        }
    } else {
        return ret;
    }
}

mdf_err_t aliyun_mdf_subdevice_write(aliyun_msg_type_t type, const void *data, size_t size)
{
    MDF_PARAM_CHECK(data);

    mdf_err_t ret = MDF_FAIL;
    mwifi_data_type_t data_type = {
        .communicate = MWIFI_COMMUNICATE_MULTICAST,
        .protocol = ALIYUN_MDF_PROTOCL,
        .custom = type,
    };

    ret = mwifi_write(NULL, &data_type, data, size, true);
    return ret;
}

mdf_err_t aliyun_mdf_subdevice_read(aliyun_msg_type_t *type,
                                    void **data, size_t *size, uint32_t wait_ticks)
{
    mdf_err_t ret = MDF_OK;
    mwifi_data_type_t data_type = { 0 };
    uint8_t src_addr[6] = { 0 };

    ret = mwifi_read(src_addr, &data_type, (uint8_t **)data, size, wait_ticks);

    if (ret == MDF_OK) {
        *type = data_type.custom;

        if (data_type.upgrade) { // This mesh package contains upgrade data.
            ret = mupgrade_handle(src_addr, *data, *size);
            MDF_FREE(*data);
            MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "mupgrade_handle Error");
            return MDF_ERR_TIMEOUT;

        } else if (data_type.protocol == ALIYUN_MDF_PROTOCL) {
            return MDF_OK;
        } else {
            return MDF_ERR_CUSTOM_BASE;
        }
    } else {
        return ret;
    }
}

int aliyun_mdf_get_routing_table_size(void)
{
    return esp_mesh_get_routing_table_size();
}

mdf_err_t aliyun_mdf_get_routing_table(uint8_t *routing_table, int *table_size)
{
    return esp_mesh_get_routing_table((mesh_addr_t *)routing_table,
                                      *table_size * sizeof(mesh_addr_t), table_size);
}

mdf_err_t aliyun_mdf_ota_start(char *ota_version, size_t ota_size)
{
    return mupgrade_firmware_init(ota_version, ota_size);
}

mdf_err_t aliyun_mdf_ota_write(uint8_t *buffer, size_t length)
{
    return mupgrade_firmware_download(buffer, length);
}

const char *g_aliyun_notify_reset_msg = "{\"cmd\":\"ota\",\"delay\":0}";

mdf_err_t aliyun_mdf_ota_stop(void)
{
    mdf_err_t ret = MDF_OK;
    mupgrade_result_t upgrade_result = { 0 };

    uint32_t addrs_num = aliyun_list_select_ota_start_addrs_num();
    MDF_ERROR_CHECK(addrs_num == 0, MDF_FAIL, "aliyun_list_select_ota_start_addrs_num Error");

    MDF_LOGD("addrs_num: %d", addrs_num);

    uint8_t *addrs_list = MDF_CALLOC(1, ALIYUN_SUBDEVICE_ADDRS_MAXLEN * addrs_num);

    addrs_num = aliyun_list_select_ota_start_addrs_list(addrs_list, ALIYUN_SUBDEVICE_ADDRS_MAXLEN * addrs_num);
    MDF_ERROR_CHECK(addrs_num == 0, MDF_FAIL, "aliyun_list_select_ota_start_addrs_list Error");
    MDF_LOGD("addrs_num: %d", addrs_num);

    for (uint32_t i = 0; i < addrs_num; i++) {
        MDF_LOGW("count: %d, ddrs: " MACSTR, i, MAC2STR(addrs_list + i * ALIYUN_SUBDEVICE_ADDRS_MAXLEN));
    }

    ret = mupgrade_firmware_send(addrs_list, addrs_num, &upgrade_result);
    MDF_LOGI("mupgrade_firmware_send Error, ret：%d, upgrade_result.successed_num: %d", ret, upgrade_result.successed_num);

    ret = aliyun_mdf_gateway_write(addrs_list, addrs_num, ALIYUN_GATEWAY_NOTIFY_SUBDEVICE_RESET,
                                   (uint8_t *)g_aliyun_notify_reset_msg, strlen(g_aliyun_notify_reset_msg));
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_list_select_ota_start_addrs_list Error");
    ret = mupgrade_result_free(&upgrade_result);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "mupgrade_result_free Error");

    return MDF_OK;
}

mdf_err_t aliyun_mdf_event_loop_send(mdf_event_loop_t event_id, void *ctx)
{
    return mdf_event_loop_send(event_id, ctx);
}
