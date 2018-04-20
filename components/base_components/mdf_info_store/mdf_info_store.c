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

#include "nvs.h"
#include "nvs_flash.h"

#include "mdf_common.h"
#include "mdf_info_store.h"

static const char *TAG = "mdf_info_store";

esp_err_t mdf_info_init()
{
    static bool init_flag = false;

    if (!init_flag) {
        esp_err_t ret = nvs_flash_init();

        if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
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
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "nvs_open ret:%x", ret);

    /**< if key is MDF_SPACE_NAME, erase all info in MDF_SPACE_NAME */
    if (!strcmp(key, MDF_SPACE_NAME)) {
        ret = nvs_erase_all(handle);
    } else {
        ret = nvs_erase_key(handle, key);
    }

    nvs_commit(handle);

    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "nvs_erase_key key:%s, ret:%x", key, ret);

    return ESP_OK;
}

ssize_t mdf_info_save(const char *key,  const void *value, size_t length)
{
    MDF_PARAM_CHECK(key);
    MDF_PARAM_CHECK(value);
    MDF_PARAM_CHECK(length > 0);

    esp_err_t ret     = ESP_OK;
    nvs_handle handle = 0;

    mdf_info_init();

    ret = nvs_open(MDF_SPACE_NAME, NVS_READWRITE, &handle);
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "nvs_open ret:%x", ret);

    /**< Reduce the number of flash write */
    char *tmp = (char *)mdf_malloc(length);
    ret = nvs_get_blob(handle, key, tmp, &length);

    /**< if the value is exist and not changed */
    if ((ret == ESP_OK) && !memcmp(tmp, value, length)) {
        mdf_free(tmp);
        nvs_close(handle);
        return length;
    }

    mdf_free(tmp);

    ret = nvs_set_blob(handle, key, value, length);
    nvs_commit(handle);
    nvs_close(handle);
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "nvs_set_blob key:%s, ret:%x", key, ret);

    return length;
}

ssize_t mdf_info_load(const char *key, void *value, size_t length)
{
    MDF_PARAM_CHECK(key);
    MDF_PARAM_CHECK(value);
    MDF_PARAM_CHECK(length > 0);

    esp_err_t ret     = ESP_OK;
    nvs_handle handle = 0;

    mdf_info_init();

    ret = nvs_open(MDF_SPACE_NAME, NVS_READWRITE, &handle);
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "nvs_open ret:%x", ret);

    ret = nvs_get_blob(handle, key, value, &length);
    nvs_close(handle);

    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        MDF_LOGV("not found, ret: %d, key: %s", ret, key);
        return ESP_FAIL;
    }

    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "nvs_get_blob key:%s, ret:%x", key, ret);

    return length;
}

bool mdf_info_is_exist(const char *key)
{
    MDF_PARAM_CHECK(key);

    esp_err_t ret     = ESP_OK;
    nvs_handle handle = 0;
    uint8_t value     = 0;
    size_t length     = 1;

    mdf_info_init();

    ret = nvs_open(MDF_SPACE_NAME, NVS_READWRITE, &handle);
    MDF_ERROR_CHECK(ret != ESP_OK, ESP_FAIL, "nvs_open ret:%x", ret);

    ret = nvs_get_blob(handle, key, &value, &length);
    nvs_close(handle);

    return ret != ESP_OK ? false : true;
}
