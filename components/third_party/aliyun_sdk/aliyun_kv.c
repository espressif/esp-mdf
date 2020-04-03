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

#include "aliyun_kv.h"
#include "nvs_flash.h"

#define ALIYUN_KV "ALIYUN-KEY"

static const char *TAG = "aliyun_kv";

static struct {
    bool valid;
    aliyun_device_meta_t meta;
} g_meta_cache;

static mdf_err_t aliyun_kv_init(void)
{
    static bool init_flag = false;

    if (!init_flag) {
        mdf_err_t ret = nvs_flash_init();

        if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }

        ESP_ERROR_CHECK(ret);
        init_flag = true;
    }

    return MDF_OK;
}

mdf_err_t aliyun_set_product_key(char *product_key)
{
    MDF_PARAM_CHECK(product_key);

    mdf_err_t ret = MDF_OK;
    nvs_handle handle;

    ret = nvs_open(ALIYUN_KV, NVS_READWRITE, &handle);

    if (ret != MDF_OK) {
        MDF_LOGE("aliyun_set_product_key nvs_open error, ret:%d", ret);
        return MDF_FAIL;
    } else {
        ret = nvs_set_str(handle, "ProductKey", product_key);

        if (ret != MDF_OK) {
            MDF_LOGW("HAL_SetProductSecret nvs_set_str error, ret:%d", ret);
        }

        nvs_close(handle);
    }

    MDF_LOGV("aliyun_set_product_key :%s", product_key);
    return ret;
}

mdf_err_t aliyun_set_device_name(char *device_name)
{
    MDF_PARAM_CHECK(device_name);

    mdf_err_t ret = MDF_OK;
    nvs_handle handle;

    ret = nvs_open(ALIYUN_KV, NVS_READWRITE, &handle);

    if (ret != MDF_OK) {
        MDF_LOGE("aliyun_set_device_name nvs_open error, ret:%d", ret);
        return MDF_FAIL;
    } else {
        ret = nvs_set_str(handle, "DeviceName", device_name);

        if (ret != MDF_OK) {
            MDF_LOGW("HAL_SetProductSecret nvs_set_str error, ret:%d", ret);
        }

        nvs_close(handle);
    }

    MDF_LOGV("aliyun_set_device_name :%s", device_name);
    return ret;
}

mdf_err_t aliyun_set_device_secret(char *device_secret)
{
    MDF_PARAM_CHECK(device_secret);

    mdf_err_t ret = MDF_OK;
    nvs_handle handle;

    ret = nvs_open(ALIYUN_KV, NVS_READWRITE, &handle);

    if (ret != MDF_OK) {
        MDF_LOGE("aliyun_set_device_secret nvs_open error, ret:%d", ret);
        return MDF_FAIL;
    } else {
        ret = nvs_set_str(handle, "DeviceSecret", device_secret);

        if (ret != MDF_OK) {
            MDF_LOGW("HAL_SetProductSecret nvs_set_str error, ret:%d", ret);
        }

        nvs_close(handle);
    }

    MDF_LOGV("aliyun_set_device_secret :%s", device_secret);
    return ret;
}

mdf_err_t aliyun_set_product_secret(char *product_secret)
{
    MDF_PARAM_CHECK(product_secret);

    mdf_err_t ret = MDF_OK;
    nvs_handle handle;

    ret = nvs_open(ALIYUN_KV, NVS_READWRITE, &handle);

    if (ret != MDF_OK) {
        MDF_LOGE("aliyun_set_product_secret nvs_open error, ret:%d", ret);
        return MDF_FAIL;
    } else {
        ret = nvs_set_str(handle, "ProductSecret", product_secret);

        if (ret != MDF_OK) {
            MDF_LOGW("HAL_SetProductSecret nvs_set_str error, ret:%d", ret);
        }

        nvs_close(handle);
    }

    MDF_LOGV("aliyun_set_product_secret :%s", product_secret);
    return ret;
}

mdf_err_t aliyun_get_product_key(char *product_key, size_t size)
{
    MDF_PARAM_CHECK(product_key);

    if (g_meta_cache.valid == true) {
        strncpy(product_key, g_meta_cache.meta.product_key, size);
        return MDF_OK;
    }

    mdf_err_t ret = MDF_OK;
    nvs_handle handle;
    size_t read_len = size;

    ret = nvs_open(ALIYUN_KV, NVS_READWRITE, &handle);

    if (ret != MDF_OK) {
        MDF_LOGE("aliyun_get_product_key nvs_open error, ret:%d", ret);
    } else {
        ret = nvs_get_str(handle, "ProductKey", product_key, (size_t *)&read_len);

        if (ret == ESP_ERR_NVS_NOT_FOUND) {
            MDF_LOGW("aliyun_get_product_key nvs_get_str not found");
#ifdef CONFIG_ALIYUN_META_PRODUCT_KEY
            MDF_LOGW("write menuconfig config :%s to aliyun_kv", CONFIG_ALIYUN_META_PRODUCT_KEY);
            ret = aliyun_set_product_key(CONFIG_ALIYUN_META_PRODUCT_KEY);
            strncpy(product_key, CONFIG_ALIYUN_META_PRODUCT_KEY, size);
#endif
        } else if (ret != MDF_OK) {
            MDF_LOGW("aliyun_get_product_key nvs_get_str error, size: %u, ret:%d", size, ret);
        }

        nvs_close(handle);
    }

    MDF_LOGV("aliyun_get_product_key :%s", product_key);
    return ret;
}

mdf_err_t aliyun_get_device_name(char *device_name, size_t size)
{
    MDF_PARAM_CHECK(device_name);

    if (g_meta_cache.valid == true) {
        strncpy(device_name, g_meta_cache.meta.device_name, size);
        return MDF_OK;
    }

    mdf_err_t ret = MDF_OK;
    nvs_handle handle;
    size_t read_len = size;

    ret = nvs_open(ALIYUN_KV, NVS_READWRITE, &handle);

    if (ret != MDF_OK) {
        MDF_LOGE("aliyun_get_device_name nvs_open error, ret:%d", ret);
    } else {
        ret = nvs_get_str(handle, "DeviceName", device_name, (size_t *)&read_len);

        if (ret == ESP_ERR_NVS_NOT_FOUND) {
            MDF_LOGW("aliyun_get_device_name nvs_get_str not found");
#ifdef CONFIG_ALIYUN_META_DEVICE_NAME
            MDF_LOGW("write menuconfig config :%s to aliyun_kv", CONFIG_ALIYUN_META_DEVICE_NAME);
            ret = aliyun_set_device_name(CONFIG_ALIYUN_META_DEVICE_NAME);
            strncpy(device_name, CONFIG_ALIYUN_META_DEVICE_NAME, size);
#endif
        } else if (ret != MDF_OK) {
            MDF_LOGW("aliyun_get_device_name nvs_get_str error, ret:%d", ret);
        }

        nvs_close(handle);
    }

    MDF_LOGV("aliyun_get_device_name :%s", device_name);
    return ret;
}

mdf_err_t aliyun_get_device_secret(char *device_secret, size_t size)
{
    MDF_PARAM_CHECK(device_secret);

    if (g_meta_cache.valid == true) {
        strncpy(device_secret, g_meta_cache.meta.device_secret, size);
        return MDF_OK;
    }

    mdf_err_t ret = MDF_OK;
    nvs_handle handle;
    size_t read_len = size;

    ret = nvs_open(ALIYUN_KV, NVS_READWRITE, &handle);

    if (ret != MDF_OK) {
        MDF_LOGE("aliyun_get_device_secret nvs_open error, ret:%d", ret);
    } else {
        ret = nvs_get_str(handle, "DeviceSecret", device_secret, (size_t *)&read_len);

        if (ret == ESP_ERR_NVS_NOT_FOUND) {
            MDF_LOGW("aliyun_get_device_secret nvs_get_str not found");
#ifdef CONFIG_ALIYUN_META_DEVICE_SECRET
            MDF_LOGW("write menuconfig config :%s to aliyun_kv", CONFIG_ALIYUN_META_DEVICE_SECRET);
            ret = aliyun_set_device_secret(CONFIG_ALIYUN_META_DEVICE_SECRET);
            strncpy(device_secret, CONFIG_ALIYUN_META_DEVICE_SECRET, size);
#endif
        } else if (ret != MDF_OK) {
            MDF_LOGW("aliyun_get_device_secret nvs_get_str error, ret:%d", ret);
        }

        nvs_close(handle);
    }

    MDF_LOGV("aliyun_get_device_secret :%s", device_secret);
    return ret;
}

mdf_err_t aliyun_get_product_secret(char *product_secret, size_t size)
{
    MDF_PARAM_CHECK(product_secret);

    mdf_err_t ret = MDF_OK;
    nvs_handle handle;
    size_t read_len = size;

    ret = nvs_open(ALIYUN_KV, NVS_READWRITE, &handle);

    if (ret != MDF_OK) {
        MDF_LOGE("aliyun_get_product_secret nvs_open error, ret:%d", ret);
    } else {
        ret = nvs_get_str(handle, "ProductSecret", product_secret, (size_t *)&read_len);

        if (ret == ESP_ERR_NVS_NOT_FOUND) {
            MDF_LOGW("aliyun_get_product_secret nvs_get_str not found");
#ifdef CONFIG_ALIYUN_META_PRODUCT_SECRET
            MDF_LOGW("write menuconfig config :%s to aliyun_kv", CONFIG_ALIYUN_META_PRODUCT_SECRET);
            ret = aliyun_set_product_secret(CONFIG_ALIYUN_META_PRODUCT_SECRET);
            strncpy(product_secret, CONFIG_ALIYUN_META_PRODUCT_SECRET, size);
#endif
        } else if (ret != MDF_OK) {
            MDF_LOGW("aliyun_get_product_secret nvs_get_str error, ret:%d", ret);
        }

        nvs_close(handle);
    }

    MDF_LOGV("aliyun_get_product_secret :%s", product_secret);
    return ret;
}

mdf_err_t aliyun_get_meta(aliyun_device_meta_t *meta)
{
    MDF_PARAM_CHECK(meta);

    mdf_err_t ret = MDF_OK;

    ret = aliyun_kv_init();
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_kv_init error");

    ret = aliyun_get_product_key(meta->product_key, ALIYUN_PRODUCT_KEY_MAXLEN - 1);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_get_product_key error");
    ret = aliyun_get_device_name(meta->device_name, ALIYUN_DEVICE_NAME_MAXLEN - 1);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_get_device_name error");
    ret = aliyun_get_device_secret(meta->device_secret, ALIYUN_DEVICE_SECRET_MAXLEN - 1);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "aliyun_get_device_secret error");

    memcpy(&g_meta_cache.meta, meta, sizeof(aliyun_device_meta_t));
    g_meta_cache.valid = true;
    return ret;
}
