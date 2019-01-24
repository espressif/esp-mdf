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

#include "mlink.h"
#include "mwifi.h"
#include "mlink_trigger.h"

#define MLINK_TRIGGER_LIST_MAX_NUM 32
#define MLINK_TRIGGER_STORE_KEY    "MLINK_TRIGGER"

/**
 * @brief Used to compare whether the trigger condition is met
 */
typedef struct trigger_compare {
    union {
        struct {
            uint32_t equal        : 1;
            uint32_t unequal      : 1;
            uint32_t greater_than : 1;
            uint32_t less_than    : 1;
            uint32_t variation    : 1;
            uint32_t rising       : 1;
            uint32_t falling      : 1;
        };
        uint32_t val;
    } flag;

    int value;
    int equal;
    int unequal;
    int greater_than;
    int less_than;
    int variation;
    int rising;
    int falling;
} trigger_compare_t;

/**
 * @brief Type of trigger function
 */
enum trigger_type {
    TRIGGER_LINKAGE,
    TRIGGER_SYNC,
    TRIGGER_DELAY,
};

/**
 * @brief List of trigger functions
 */
typedef struct mlink_trigger {
    char name[16];
    uint8_t trigger_cid;
    uint8_t trigger_type;
    int trigger_params[4];
    trigger_compare_t trigger_compare;
    uint16_t raw_data_size;
    uint16_t addrs_num;
    uint8_t *addrs_list;
    char *execute_content;
    struct mlink_trigger *next;
} mlink_trigger_t;

/**
 * @brief Store trigger function
 */
typedef struct {
    uint16_t size;
    char name[16];
} mlink_trigger_store_t;

static const char *TAG                 = "mlink_trigger";
static mlink_trigger_t *g_trigger_list = NULL;
extern mlink_characteristic_func_t mlink_device_get_value;

static mlink_trigger_t *mlink_trigger_parse(const char *raw_data)
{
    mdf_err_t ret                      = MDF_OK;
    char request_str[16]               = {0};
    char *trigger_content_str          = NULL;
    char *trigger_compare_str          = NULL;
    char **addrs_list_str              = NULL;
    mlink_trigger_t *trigger_item      = MDF_CALLOC(1, sizeof(mlink_trigger_t));
    trigger_compare_t *trigger_compare = &trigger_item->trigger_compare;

    memset(trigger_item, 0, sizeof(trigger_compare_t));

    trigger_item->raw_data_size = strlen(raw_data) + 1;

    ret = mlink_json_parse(raw_data, "name", trigger_item->name);
    MDF_ERROR_GOTO(ret < 0, EXIT, "Parse the json formatted string");

    ret = mlink_json_parse(raw_data, "trigger_cid", &trigger_item->trigger_cid);
    MDF_ERROR_GOTO(ret < 0, EXIT, "Parse the json formatted string");

    MDF_LOGD("name: %s, cid: %d", trigger_item->name, trigger_item->trigger_cid);

    ret = mlink_json_parse(raw_data, "execute_mac", &trigger_item->addrs_num);
    MDF_ERROR_GOTO(ret < 0, EXIT, "Parse the json formatted string");

    addrs_list_str = MDF_CALLOC(trigger_item->addrs_num, sizeof(char *));
    trigger_item->addrs_list = MDF_CALLOC(trigger_item->addrs_num, 6);
    ret = mlink_json_parse(raw_data, "execute_mac", addrs_list_str);
    MDF_ERROR_GOTO(ret < 0, EXIT, "Parse the json formatted string");

    for (int i = 0; i < trigger_item->addrs_num; ++i) {
        mlink_mac_str2hex(addrs_list_str[i], trigger_item->addrs_list + 6 * i);
        MDF_LOGV("str: %s, addrs_list: " MACSTR, addrs_list_str[i], MAC2STR(trigger_item->addrs_list + 6 * i));
        MDF_FREE(addrs_list_str[i]);
    }

    MDF_FREE(addrs_list_str);

    ret = mlink_json_parse(raw_data, "trigger_compare", &trigger_compare_str);
    MDF_ERROR_GOTO(ret < 0, EXIT, "Parse the json formatted string");

    trigger_compare->flag.equal        = (mlink_json_parse(trigger_compare_str, "==", &trigger_compare->equal) == ESP_OK) ? true : false;
    trigger_compare->flag.unequal      = (mlink_json_parse(trigger_compare_str, "!=", &trigger_compare->unequal) == ESP_OK) ? true : false;
    trigger_compare->flag.greater_than = (mlink_json_parse(trigger_compare_str, ">",  &trigger_compare->greater_than) == ESP_OK) ? true : false;
    trigger_compare->flag.less_than    = (mlink_json_parse(trigger_compare_str, "<",  &trigger_compare->less_than) == ESP_OK) ? true : false;
    trigger_compare->flag.variation    = (mlink_json_parse(trigger_compare_str, "~",  &trigger_compare->variation) == ESP_OK) ? true : false;
    trigger_compare->flag.rising       = (mlink_json_parse(trigger_compare_str, "/",  &trigger_compare->rising) == ESP_OK) ? true : false;
    trigger_compare->flag.falling      = (mlink_json_parse(trigger_compare_str, "\\", &trigger_compare->falling) == ESP_OK) ? true : false;

    trigger_compare->value = -1;

    MDF_FREE(trigger_compare_str);

    ret = mlink_json_parse(raw_data, "trigger_content", &trigger_content_str);
    MDF_ERROR_GOTO(ret < 0, EXIT, "Parse the json formatted string");

    ret = mlink_json_parse(trigger_content_str, "request", request_str);
    MDF_ERROR_GOTO(ret < 0, EXIT, "Parse the json formatted string");

    if (!strcasecmp(request_str, "sync")) {
        trigger_item->trigger_type = TRIGGER_SYNC;
        ret = mlink_json_parse(trigger_content_str, "execute_cid", trigger_item->trigger_params);
        MDF_ERROR_GOTO(ret < 0, EXIT, "Parse the json formatted string");
    } else if (!strcasecmp(request_str, "linkage")) {
        trigger_item->trigger_type = TRIGGER_LINKAGE;
        ret = mlink_json_parse(raw_data, "execute_content", &trigger_item->execute_content);
        MDF_ERROR_GOTO(ret < 0, EXIT, "Parse the json formatted string");
    } else {
        ret = ESP_FAIL;
        MDF_LOGW("no support request: %s", request_str);
        goto EXIT;
    }

EXIT:

    if (ret != MDF_OK) {
        MDF_FREE(trigger_item->addrs_list);
        MDF_FREE(trigger_item->execute_content);
        MDF_FREE(trigger_item);
    }

    MDF_FREE(trigger_content_str);
    MDF_FREE(trigger_compare_str);
    MDF_FREE(addrs_list_str);

    return (ret == MDF_OK) ? trigger_item : NULL;
}

mdf_err_t mlink_trigger_store_update()
{
    mdf_err_t ret   = MDF_OK;
    int trigger_num = 0;
    mlink_trigger_store_t *trigger_store = MDF_CALLOC(MLINK_TRIGGER_LIST_MAX_NUM, sizeof(mlink_trigger_store_t));

    for (mlink_trigger_t *trigger_idex = g_trigger_list->next; trigger_idex; trigger_idex = trigger_idex->next, trigger_num++) {
        strncpy(trigger_store[trigger_num].name, trigger_idex->name, sizeof(trigger_store[trigger_num].name));
        trigger_store[trigger_num].size = trigger_idex->raw_data_size;
    }

    if (trigger_num > 0) {
        ret = mdf_info_save(MLINK_TRIGGER_STORE_KEY, trigger_store, trigger_num * sizeof(mlink_trigger_store_t));
    } else {
        ret = mdf_info_erase(MLINK_TRIGGER_STORE_KEY);
    }

    MDF_FREE(trigger_store);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_save, ret: %d", ret);

    return MDF_OK;
}

static mdf_err_t mlink_handle_get_trigger(mlink_handle_data_t *handle_data)
{
    mdf_err_t ret      = ESP_OK;
    char *trigger_json = NULL;
    char *raw_data     = NULL;

    for (mlink_trigger_t *trigger_idex = g_trigger_list->next; trigger_idex; trigger_idex = trigger_idex->next) {
        raw_data = MDF_MALLOC(trigger_idex->raw_data_size);
        ret = mdf_info_load(trigger_idex->name, raw_data, trigger_idex->raw_data_size);
        MDF_ERROR_GOTO(ret != MDF_OK, EXIT, " Load the information");

        mlink_json_pack(&trigger_json, "[]", raw_data);
        MDF_FREE(raw_data);
    }

    if (!trigger_json) {
        return MDF_OK;
    }

    handle_data->resp_size = mlink_json_pack(&handle_data->resp_data, "trigger", trigger_json);

EXIT:
    MDF_FREE(raw_data);
    MDF_FREE(trigger_json);
    return ret;
}

mdf_err_t mlink_trigger_add(const char *trigger_raw_data)
{
    MDF_PARAM_CHECK(trigger_raw_data);

    mdf_err_t ret = MDF_OK;
    mlink_trigger_t *trigger_list = g_trigger_list;
    mlink_trigger_t *trigger_item = NULL;

    trigger_item = mlink_trigger_parse(trigger_raw_data);
    MDF_ERROR_CHECK(!trigger_item, MDF_FAIL, "mlink_trigger_parse");

    for (mlink_trigger_t *trigger_idex = trigger_list->next; trigger_idex;
            trigger_list = trigger_idex, trigger_idex = trigger_idex->next) {
        if (!strcasecmp(trigger_idex->name, trigger_item->name)) {
            MDF_LOGD("remove event: %s", trigger_idex->name);
            ret = mdf_info_erase(trigger_idex->name);
            MDF_ERROR_CONTINUE(ret != MDF_OK, "mdf_info_erase, ret: %d", ret);

            trigger_list->next = trigger_idex->next;
            MDF_FREE(trigger_idex->addrs_list);
            MDF_FREE(trigger_idex->execute_content);
            MDF_FREE(trigger_idex);
            trigger_idex = trigger_list;
            break;
        }
    }

    trigger_item->next   = g_trigger_list->next;
    g_trigger_list->next = trigger_item;

    ret = mdf_info_save(trigger_item->name, trigger_raw_data, strlen(trigger_raw_data) + 1);
    MDF_ERROR_CHECK(ret < 0, ret, "Save the information");

    ret = mlink_trigger_store_update();
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "mlink_trigger_store_update");

    return MDF_OK;
}

static mdf_err_t mlink_handle_set_trigger(mlink_handle_data_t *handle_data)
{
    mdf_err_t ret   = MDF_OK;
    int trigger_num = 0;
    char *trigger_raw_data[MLINK_TRIGGER_LIST_MAX_NUM] = {NULL};

    ret = mlink_json_parse(handle_data->req_data, "events", &trigger_num);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "Parse the json formatted string");

    ret = mlink_json_parse(handle_data->req_data, "events", trigger_raw_data);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "Parse the json formatted string");

    for (int i = 0; i < trigger_num; ++i) {
        ret = mlink_trigger_add(trigger_raw_data[i]);
        MDF_ERROR_CHECK(ret != MDF_OK, ret, "mlink_trigger_add");

        MDF_LOGD("mlink_handle_set_trigger success");
    }

    mdf_event_loop_send(MDF_EVENT_MLINK_SET_TRIGGER, NULL);

    return MDF_OK;
}

static mdf_err_t mlink_handle_remove_trigger(mlink_handle_data_t *handle_data)
{
    mdf_err_t ret         = ESP_OK;
    char trigger_name[16] = {0};
    int trigger_num       = 0;
    char *trigger_list[MLINK_TRIGGER_LIST_MAX_NUM] = {0};

    ret = mlink_json_parse(handle_data->req_data, "events", &trigger_num);
    MDF_ERROR_CHECK(ret < 0, ret, "Parse the json formatted string");

    ret = mlink_json_parse(handle_data->req_data, "events", trigger_list);
    MDF_ERROR_CHECK(ret < 0, ret, "Parse the json formatted string");

    for (int i = 0; i < trigger_num; ++i) {
        ret = mlink_json_parse(trigger_list[i], "name", trigger_name);
        MDF_FREE(trigger_list[i]);
        MDF_ERROR_CONTINUE(ret < 0, "Parse the json formatted string");

        mlink_trigger_t *trigger_idex_prior = g_trigger_list;

        for (mlink_trigger_t *trigger_idex = trigger_idex_prior->next; trigger_idex;
                trigger_idex_prior = trigger_idex, trigger_idex = trigger_idex->next) {
            if (!strcasecmp(trigger_idex->name, trigger_name)) {
                MDF_LOGD("remove event: %s", trigger_idex->name);
                ret = mdf_info_erase(trigger_idex->name);
                MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_erase, ret: %d", ret);

                trigger_idex_prior->next = trigger_idex->next;
                MDF_FREE(trigger_idex->addrs_list);
                MDF_FREE(trigger_idex->execute_content);
                MDF_FREE(trigger_idex);
                trigger_idex = trigger_idex_prior;
                break;
            }
        }
    }

    ret = mlink_trigger_store_update();
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "mlink_trigger_store_update");

    return ESP_OK;
}

mdf_err_t mlink_trigger_handle(mlink_communicate_t communicate)
{
    mdf_err_t ret = ESP_OK;
    int value     = -1;

    for (mlink_trigger_t *trigger_idex = g_trigger_list->next; trigger_idex; trigger_idex = trigger_idex->next) {
        trigger_compare_t *cmp = &trigger_idex->trigger_compare;
        ret = mlink_device_get_value(trigger_idex->trigger_cid, &value);
        MDF_ERROR_CONTINUE(ret < 0, "<%s> mlink_device_get_value, cid: %d", mdf_err_to_name(ret), trigger_idex->trigger_cid);

        bool equal        = (!cmp->flag.equal        || (cmp->flag.equal        && cmp->equal == value)) ? true       : false;
        bool unequal      = (!cmp->flag.unequal      || (cmp->flag.unequal      && cmp->unequal != value)) ? true     : false;
        bool greater_than = (!cmp->flag.greater_than || (cmp->flag.greater_than && cmp->greater_than < value)) ? true : false;
        bool less_than    = (!cmp->flag.less_than    || (cmp->flag.less_than    && cmp->less_than > value)) ? true    : false;
        bool variation    = (!cmp->flag.variation    || (cmp->flag.variation    && cmp->variation <= abs(value - cmp->value))) ? true : false;
        bool rising       = (!cmp->flag.rising       || (cmp->flag.rising       && (value - cmp->value >= variation))) ? true : false;
        bool falling      = (!cmp->flag.falling      || (cmp->flag.falling      && (cmp->value - value >= variation))) ? true : false;

        MDF_LOGD("name: %s, cid: %d, value: %d, trigger_type: %d, variation: %d, trigger: %d, rising: %d",
                 trigger_idex->name, trigger_idex->trigger_cid, value,
                 trigger_idex->trigger_type, variation, cmp->value, cmp->rising);

        if (trigger_idex->trigger_type == TRIGGER_SYNC) {
            if (cmp->value == value) {
                continue;
            }

            asprintf(&trigger_idex->execute_content, "{\"request\":\"set_status\",\"characteristics\":[{\"cid\":%d,\"value\":%d}]}",
                     trigger_idex->trigger_params[0], value);
        }

        cmp->value = value;

        if (!equal || !unequal || !greater_than || !less_than || !variation || !rising || !falling) {
            continue;
        }

        if (communicate == MLINK_COMMUNICATE_MESH) {
            mwifi_data_type_t data_type  = {
                .protocol = MLINK_PROTO_HTTPD,
            };
            mlink_httpd_type_t httpd_type = {
                .format = MLINK_HTTPD_FORMAT_JSON,
                .from   = MLINK_HTTPD_FROM_DEVICE,
                .resp   = false,
            };

            memcpy(&data_type.custom, &httpd_type, sizeof(mlink_httpd_type_t));

            for (int i = 0; i < trigger_idex->addrs_num; ++i) {
                ret = mwifi_write(trigger_idex->addrs_list + 6 * i, &data_type,
                                  trigger_idex->execute_content, strlen(trigger_idex->execute_content), true);
            }
        } else if (communicate == MLINK_COMMUNICATE_ESPNOW) {
            ret = mlink_espnow_write(trigger_idex->addrs_list, trigger_idex->addrs_num, trigger_idex->execute_content,
                                     strlen(trigger_idex->execute_content), portMAX_DELAY);
        }

        MDF_LOGD("addrs_num: %d, addrs_list: " MACSTR ", execute_content: %s",
                 trigger_idex->addrs_num, MAC2STR(trigger_idex->addrs_list),
                 trigger_idex->execute_content);

        if (trigger_idex->trigger_type == TRIGGER_SYNC) {
            MDF_FREE(trigger_idex->execute_content);
        }

        MDF_ERROR_CHECK(ret != ESP_OK, ret, "<%s> mlink_espnow_write", mdf_err_to_name(ret));
    }

    return MDF_OK;
}

mdf_err_t mlink_trigger_init()
{
    if (g_trigger_list) {
        return MDF_OK;
    }

    mdf_err_t ret                 = MDF_OK;
    g_trigger_list                = MDF_CALLOC(1, sizeof(mlink_trigger_t));
    char *trigger_raw_data        = NULL;
    mlink_trigger_t *trigger_item = NULL;
    mlink_trigger_store_t *trigger_store = MDF_CALLOC(MLINK_TRIGGER_LIST_MAX_NUM, sizeof(mlink_trigger_store_t));

    if (mdf_info_load(MLINK_TRIGGER_STORE_KEY, trigger_store,
                      MLINK_TRIGGER_LIST_MAX_NUM * sizeof(mlink_trigger_store_t)) == MDF_OK) {
        for (int i = 0; i < MLINK_TRIGGER_LIST_MAX_NUM && trigger_store[i].size; ++i) {
            MDF_FREE(trigger_raw_data);
            trigger_raw_data = MDF_CALLOC(1, trigger_store[i].size);
            ret = mdf_info_load(trigger_store[i].name, trigger_raw_data, trigger_store[i].size);
            MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> Load the information", mdf_err_to_name(ret));

            trigger_item = mlink_trigger_parse(trigger_raw_data);
            MDF_ERROR_CONTINUE(!trigger_item, "<%s> mlink_trigger_parse", mdf_err_to_name(ret));

            trigger_item->next   = g_trigger_list->next;
            g_trigger_list->next = trigger_item;
        }
    }

    MDF_FREE(trigger_store);
    MDF_FREE(trigger_raw_data);

    mlink_set_handle("set_event", mlink_handle_set_trigger);
    mlink_set_handle("get_event", mlink_handle_get_trigger);
    mlink_set_handle("remove_event", mlink_handle_remove_trigger);

    return ESP_OK;
}

bool mlink_trigger_is_exist()
{
    MDF_ERROR_CHECK(!g_trigger_list, false, "Mlink_trigger is not initialized");

    return (g_trigger_list->next == NULL) ? false : true;
}
