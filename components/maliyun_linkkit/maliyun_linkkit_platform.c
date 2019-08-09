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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "esp_log.h"
#include "esp_ota_ops.h"

#include "mupgrade.h"
#include "mwifi.h"

#include "ota_api.h"
#include "iot_export_linkkit.h"

#include "maliyun_linkkit_list_priv.h"

static const char *TAG   = "maliyun_linkkit_platform";

extern int dm_ota_get_ota_handle(void **handle);

/**
 * @brief initialize a firmware upgrade.
 *
 * @param None
 * @return None.
 * @see None.
 * @note None.
 */

void HAL_Firmware_Persistence_Start(void)
{
    mdf_err_t ret                               = MDF_OK;
    uint32_t ota_size                           = 0;
    void *ota_handle                            = NULL;
    char ota_version[IOTX_FIRMWARE_VERSION_LEN] = {0};

    dm_ota_get_ota_handle(&ota_handle);

    if (ota_handle == NULL) {
        MDF_LOGI("dm_ota_get_ota_handle failed");
    }

    IOT_OTA_Ioctl(ota_handle, IOT_OTAG_VERSION, ota_version, IOTX_FIRMWARE_VERSION_LEN);
    MDF_LOGI("IOT_OTAG_VERSION: %s", ota_version);

    IOT_OTA_Ioctl(ota_handle, IOT_OTAG_FILE_SIZE, &ota_size, 4);
    MDF_LOGI("IOT_OTAG_FILE_SIZE: %d", (uint32_t)ota_size);

    ret = mupgrade_firmware_init(ota_version, ota_size);
    MDF_LOGI("<%s> mupgrade_firmware_init", mdf_err_to_name(ret));
}

/**
 * @brief save firmware upgrade data to flash.
 *
 * @param[in] buffer: @n A pointer to a buffer to save data.
 * @param[in] length: @n The length, in bytes, of the data pointed to by the buffer parameter.
 * @return 0, Save success; -1, Save failure.
 * @see None.
 * @note None.
 */

int HAL_Firmware_Persistence_Write(char *buffer, uint32_t length)
{
    mdf_err_t ret = MDF_OK;
    ret = mupgrade_firmware_download((const void *)buffer, length);

    if (ret != MDF_OK) {
        MDF_LOGE("HAL_Firmware_Persistence_Write failed! ret=0x%2x", ret);
        return MDF_FAIL;
    }

    return length;
}


/**
 * @brief indicate firmware upgrade data complete, and trigger data integrity checking,
     and then reboot the system.
 *
 * @param None.
 * @return 0: Success; -1: Failure.
 * @see None.
 * @note None.
 */

int HAL_Firmware_Persistence_Stop(void)
{
    mdf_err_t ret                                   = MDF_OK;
    void    *ota_handle                             = NULL;
    char    ota_version[IOTX_FIRMWARE_VERSION_LEN]  = {0};
    char    ota_product_key[IOTX_PRODUCT_KEY_LEN]   = {0};
    int     ota_product_list_num                    = 0;

    dm_ota_get_ota_handle(&ota_handle);

    IOT_OTA_Ioctl(ota_handle, IOT_OTAG_VERSION, ota_version, IOTX_FIRMWARE_VERSION_LEN);

    char *ota_product_key_end = strchr(ota_version, '-');
    MDF_ERROR_CHECK(ota_product_key_end == NULL, MDF_FAIL, "Check ota version is error.");

    strncpy(ota_product_key, ota_version, ota_product_key_end - ota_version);
    MDF_LOGI("ota_product_key:%s", ota_product_key);

    ota_product_list_num = maliyun_linkkit_list_get_product_num(ota_product_key);

    if (ota_product_list_num > 0) {
        MDF_LOGI("ota_product_list_num:%d", ota_product_list_num);

        uint8_t *ota_product_list = malloc(ota_product_list_num * ETH_ALEN);
        maliyun_linkkit_list_get_mac_array(ota_product_key, ota_product_list);
        MDF_LOGI("mliyun_mac_of_same_product");

        ret = mupgrade_firmware_send(ota_product_list, ota_product_list_num, NULL);
        MDF_LOGI("<%s> mupgrade_firmware_send", mdf_err_to_name(ret));

        mwifi_data_type_t data_type = {.communicate = MWIFI_COMMUNICATE_MULTICAST};
        const char *restart_str = "{\"cmd\":\"restart\"}";
        ret = mwifi_root_write(ota_product_list, ota_product_list_num, &data_type, restart_str, strlen(restart_str), true);
        MDF_LOGI("mwifi_root_write cmd restart");
        free(ota_product_list);
    }

    return ret;
}


/* auth type */
enum AWSS_AUTH_TYPE {
    AWSS_AUTH_TYPE_OPEN,
    AWSS_AUTH_TYPE_SHARED,
    AWSS_AUTH_TYPE_WPAPSK,
    AWSS_AUTH_TYPE_WPA8021X,
    AWSS_AUTH_TYPE_WPA2PSK,
    AWSS_AUTH_TYPE_WPA28021X,
    AWSS_AUTH_TYPE_WPAPSKWPA2PSK,
    AWSS_AUTH_TYPE_MAX = AWSS_AUTH_TYPE_WPAPSKWPA2PSK,
    AWSS_AUTH_TYPE_INVALID = 0xff,
};

/* encryt type */
enum AWSS_ENC_TYPE {
    AWSS_ENC_TYPE_NONE,
    AWSS_ENC_TYPE_WEP,
    AWSS_ENC_TYPE_TKIP,
    AWSS_ENC_TYPE_AES,
    AWSS_ENC_TYPE_TKIPAES,
    AWSS_ENC_TYPE_MAX = AWSS_ENC_TYPE_TKIPAES,
    AWSS_ENC_TYPE_INVALID = 0xff,
};

int HAL_Awss_Connect_Ap(
    uint32_t connection_timeout_ms,
    char ssid[HAL_MAX_SSID_LEN],
    char passwd[HAL_MAX_PASSWD_LEN],
    enum AWSS_AUTH_TYPE auth,
    enum AWSS_ENC_TYPE encry,
    uint8_t bssid[ETH_ALEN],
    uint8_t channel)
{
    mwifi_init_config_t init_config     = MWIFI_INIT_CONFIG_DEFAULT();
    mwifi_config_t ap_config            = {0};

    MDF_ERROR_ASSERT(mwifi_init(&init_config));

    if (ssid != NULL) {
        memcpy(ap_config.router_ssid, ssid, strlen(ssid));
    }

    if (passwd != NULL) {
        memcpy(ap_config.router_password, passwd, strlen(passwd));
    }

    if (bssid != NULL) {
        memcpy(ap_config.router_bssid, bssid, ETH_ALEN);
    }

    MDF_LOGI("ap ssid: %s, password: %s", ssid, passwd);
    MDF_LOGI("ap bssid: " MACSTR, MAC2STR(bssid));


#ifdef CONFIG_MALIYUN_NETWORK_ID
    uint32_t crc_data = crc32_le(0, (uint8_t *)CONFIG_MALIYUN_NETWORK_ID, strlen(CONFIG_MALIYUN_NETWORK_ID));

    for (uint8_t i = ETH_ALEN; i > 0; i--) {
        ap_config.mesh_id[i - 1] = (bssid[i - 1] + (crc_data >> ((i - 1) * 6)) % (2 << 6)) % (2 << 8);
    }

#endif

#ifdef CONFIG_MALIYUN_NETWORK_KEY
    memcpy(ap_config.mesh_password, CONFIG_MALIYUN_NETWORK_KEY, 64);
#endif

    MDF_ERROR_ASSERT(mwifi_set_config(&ap_config));
    MDF_ERROR_ASSERT(mwifi_start());

    for (int i = 0; i <= connection_timeout_ms; i += 100) {
        if (mwifi_is_connected()) {
            if (!esp_mesh_is_root()) {
                break;
            } else {
                tcpip_adapter_ip_info_t local_ip;

                esp_err_t ret = tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &local_ip);

                if ((ESP_OK == ret) && (local_ip.ip.addr != INADDR_ANY)) {
                    break;
                }
            }
        }

        if (i >= connection_timeout_ms) {
            MDF_LOGW("connection_timeout_ms");
            return MDF_FAIL;
        }

        vTaskDelay(100 / portTICK_RATE_MS);
    }

    return ESP_OK;
}

int HAL_Sys_Net_Is_Ready()
{
    if (esp_mesh_is_root()) {
        tcpip_adapter_ip_info_t local_ip;

        esp_err_t ret = tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &local_ip);

        if ((ESP_OK == ret) && (local_ip.ip.addr != INADDR_ANY)) {
            return true;
        }

        return false;
    } else {
        return mwifi_is_connected();
    }
}