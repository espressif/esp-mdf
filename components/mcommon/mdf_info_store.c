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
#include "mdf_info_store.h"

static const char *TAG = "mdf_info_store";

esp_err_t mdf_info_init()
{
    static bool init_flag = false;

    if (!init_flag) {
        esp_err_t ret = nvs_flash_init();

        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            // NVS partition was truncated and needs to be erased
            // Retry nvs_flash_init
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }

        ESP_ERROR_CHECK(ret);

        init_flag = true;
    }

    return ESP_OK;
}

esp_err_t mdf_info_erase(const char *key)
{
    MDF_PARAM_CHECK(key);

    esp_err_t ret    = ESP_OK;
    nvs_handle handle = 0;

    /**< Initialize the default NVS partition */
    mdf_info_init();

    /**< Open non-volatile storage with a given namespace from the default NVS partition */
    ret = nvs_open(MDF_SPACE_NAME, NVS_READWRITE, &handle);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "Open non-volatile storage");

    /**
     * @brief If key is MDF_SPACE_NAME, erase all info in MDF_SPACE_NAME
     */
    if (!strcmp(key, MDF_SPACE_NAME)) {
        ret = nvs_erase_all(handle);
    } else {
        ret = nvs_erase_key(handle, key);
    }

    /**< Write any pending changes to non-volatile storage */
    nvs_commit(handle);

    /**< Close the storage handle and free any allocated resources */
    nvs_close(handle);

    MDF_ERROR_CHECK(ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND,
                    ret, "Erase key-value pair, key: %s", key);

    return ESP_OK;
}

esp_err_t mdf_info_save(const char *key, const void *value, size_t length)
{
    MDF_PARAM_CHECK(key);
    MDF_PARAM_CHECK(value);
    MDF_PARAM_CHECK(length > 0);

    esp_err_t ret     = ESP_OK;
    nvs_handle handle = 0;

    /**< Initialize the default NVS partition */
    mdf_info_init();

    /**< Open non-volatile storage with a given namespace from the default NVS partition */
    ret = nvs_open(MDF_SPACE_NAME, NVS_READWRITE, &handle);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "Open non-volatile storage");

    /**< set variable length binary value for given key */
    ret = nvs_set_blob(handle, key, value, length);

    /**< Write any pending changes to non-volatile storage */
    nvs_commit(handle);

    /**< Close the storage handle and free any allocated resources */
    nvs_close(handle);

    MDF_ERROR_CHECK(ret != ESP_OK, ret, "Set value for given key, key: %s", key);

    return ESP_OK;
}

esp_err_t __mdf_info_load(const char *key, void *value, size_t len, uint32_t type)
{
    MDF_PARAM_CHECK(key);
    MDF_PARAM_CHECK(value);
    MDF_PARAM_CHECK(len);

    esp_err_t ret     = ESP_OK;
    nvs_handle handle = 0;
    size_t *length = NULL;

    if (type == LENGTH_TYPE_NUMBER) {
        length  = (size_t *)&type;
        *length = len;
    } else if (type == LENGTH_TYPE_POINTER) {
        length = (size_t *)len;
    } else {
        MDF_LOGW("The type of parameter lenght is incorrect");
        return MDF_ERR_INVALID_ARG;
    }

    MDF_PARAM_CHECK(*length > 0);

    /**< Initialize the default NVS partition */
    mdf_info_init();

    /**< Open non-volatile storage with a given namespace from the default NVS partition */
    ret = nvs_open(MDF_SPACE_NAME, NVS_READWRITE, &handle);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "Open non-volatile storage");

    /**< get variable length binary value for given key */
    ret = nvs_get_blob(handle, key, value, length);

    /**< Close the storage handle and free any allocated resources */
    nvs_close(handle);

    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        MDF_LOGD("<ESP_ERR_NVS_NOT_FOUND> Get value for given key, key: %s", key);
        return ESP_ERR_NVS_NOT_FOUND;
    }

    MDF_ERROR_CHECK(ret != ESP_OK, ret, "Get value for given key, key: %s", key);

    return ESP_OK;
}
