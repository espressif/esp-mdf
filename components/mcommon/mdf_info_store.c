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

#include "mdf_common.h"
#include "mdf_info_store.h"

static const char *TAG = "mdf_info_store";

esp_err_t mdf_info_init()
{
    static bool init_flag = false;

    if (!init_flag) {
        esp_err_t ret = nvs_flash_init();

        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
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

    mdf_info_init();

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

    nvs_commit(handle);
    nvs_close(handle);

    MDF_ERROR_CHECK(ret != ESP_OK, ret, "Erase key-value pair, key: %s", key);

    return ESP_OK;
}

esp_err_t mdf_info_save(const char *key, const void *value, size_t length)
{
    MDF_PARAM_CHECK(key);
    MDF_PARAM_CHECK(value);
    MDF_PARAM_CHECK(length > 0);

    esp_err_t ret     = ESP_OK;
    nvs_handle handle = 0;

    mdf_info_init();

    ret = nvs_open(MDF_SPACE_NAME, NVS_READWRITE, &handle);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "Open non-volatile storage");

    ret = nvs_set_blob(handle, key, value, length);
    nvs_commit(handle);
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
        length  = &type;
        *length = len;
    } else if (type == LENGTH_TYPE_POINTER) {
        length = (size_t *)len;
    } else {
        MDF_LOGW("The type of parameter lenght is incorrect");
        return MDF_ERR_INVALID_ARG;
    }

    MDF_PARAM_CHECK(*length > 0);

    mdf_info_init();

    ret = nvs_open(MDF_SPACE_NAME, NVS_READWRITE, &handle);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "Open non-volatile storage");

    ret = nvs_get_blob(handle, key, value, length);
    nvs_close(handle);

    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        MDF_LOGD("<ESP_ERR_NVS_NOT_FOUND> Get value for given key, key: %s", key);
        return ESP_ERR_NVS_NOT_FOUND;
    }

    MDF_ERROR_CHECK(ret != ESP_OK, ret, "Get value for given key, key: %s", key);

    return ESP_OK;
}
