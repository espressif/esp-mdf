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
#include "mdf_json.h"
#include "mdf_info_store.h"
#include "mdf_espnow_debug.h"
#include "mdf_device_handle.h"
#include "mdf_low_power.h"

#define RESTART_DELAY_TIME           2000
#define CHARACTERISTICS_MAX_NUM      30
#define EVENT_MAX_NUM                20
#define DEVICE_EVENT_TIMEOUT         portMAX_DELAY

typedef struct trigger_compare {
    union {
        struct {
            uint32_t equal: 1;
            uint32_t unequal: 1;
            uint32_t greater_than: 1;
            uint32_t less_than: 1;
            uint32_t variation: 1;
            uint32_t rising: 1;
            uint32_t falling: 1;
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

enum trigger_type {
    TRIGGER_LINKAGE,
    TRIGGER_SYNC,
    TRIGGER_DELAY,
};

typedef struct trigger_device {
    char name[16];
    uint8_t trigger_cid;
    uint8_t trigger_type;
    int trigger_params[3];
    trigger_compare_t trigger_compare;
    char *execute_content;
    struct trigger_device *next;
    char raw_data[0];
} trigger_device_t;

typedef struct {
    ssize_t event_size;
    char event_name[16];
} trigger_store_manager_t;

static trigger_device_t *g_trigger_device   = NULL;
static SemaphoreHandle_t g_trigger_isr_xSem = NULL;
static const char *TAG                      = "mdf_trigger";

extern device_operate_t mdf_device_get_value;

static esp_err_t mdf_trigger_get_event(device_data_t *device_data)
{
    esp_err_t ret           = ESP_OK;
    device_data->request[0] = '\0';

    for (trigger_device_t *trigger_device = g_trigger_device->next; trigger_device; trigger_device = trigger_device->next) {
        ret = mdf_json_pack(device_data->request, "[]", trigger_device->raw_data);
        MDF_ERROR_BREAK(ret < 0, "mdf_json_pack, ret: %d", ret);
    }

    device_data->response_size = mdf_json_pack(device_data->response, "events", device_data->request);
    MDF_ERROR_CHECK(device_data->response_size < 0, ESP_FAIL, "mdf_json_pack, ret: %d", ret);

    return ESP_OK;
}

static esp_err_t mdf_trigger_set_event(device_data_t *device_data)
{
    esp_err_t ret    = ESP_OK;
    char *events[32] = {0};
    int events_num   = mdf_json_parse(device_data->request, "events", events);

    for (int i = 0; i < events_num; ++i) {
        trigger_device_t *trigger_device       = NULL;
        char event_name[32]                    = {0};
        char *trigger_content                  = NULL;
        trigger_device_t *trigger_device_prior = g_trigger_device;

        ret = mdf_json_parse(events[i], "name", event_name);
        MDF_ERROR_BREAK(ret < 0, "mdf_json_parse, ret: %d", ret);

        for (trigger_device = trigger_device_prior->next; trigger_device;
                trigger_device_prior = trigger_device, trigger_device = trigger_device->next) {
            if (!strcasecmp(trigger_device->name, event_name)) {
                MDF_LOGD("remove event: %s", trigger_device->name);
                ret = mdf_info_erase(trigger_device->name);
                MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_erase, ret: %d", ret);

                trigger_device_prior->next = trigger_device->next;
                mdf_free(trigger_device);
                trigger_device = trigger_device_prior;
                break;
            }
        }

        trigger_device = mdf_calloc(1, sizeof(trigger_device_t) + strlen(events[i]) + 1);
        strncpy(trigger_device->name, event_name, sizeof(trigger_device->name));
        strcpy(trigger_device->raw_data, events[i]);
        mdf_free(events[i]);
        cJSON_Minify(trigger_device->raw_data);

        ret = mdf_json_parse(trigger_device->raw_data, "trigger_cid", &trigger_device->trigger_cid);
        MDF_ERROR_CONTINUE(ret < 0, "mdf_json_parse, ret: %d", ret);

        do {
            char trigger_compare_json[64] = {0};
            trigger_compare_t *trigger_compare = &trigger_device->trigger_compare;

            ret = mdf_json_parse(trigger_device->raw_data, "trigger_compare", trigger_compare_json);
            MDF_ERROR_BREAK(ret < 0, "mdf_json_parse, ret: %d", ret);

            trigger_compare->flag.equal        = (mdf_json_parse(trigger_compare_json, "==",
                                                  &trigger_compare->equal) == ESP_OK) ? true : false;
            trigger_compare->flag.unequal      = (mdf_json_parse(trigger_compare_json, "!=",
                                                  &trigger_compare->unequal) == ESP_OK) ? true : false;
            trigger_compare->flag.greater_than = (mdf_json_parse(trigger_compare_json, ">",
                                                  &trigger_compare->greater_than) == ESP_OK) ? true : false;
            trigger_compare->flag.less_than    = (mdf_json_parse(trigger_compare_json, "<",
                                                  &trigger_compare->less_than) == ESP_OK) ? true : false;
            trigger_compare->flag.variation    = (mdf_json_parse(trigger_compare_json, "~",
                                                  &trigger_compare->variation) == ESP_OK) ? true : false;
            trigger_compare->flag.rising       = (mdf_json_parse(trigger_compare_json, "/",
                                                  &trigger_compare->rising) == ESP_OK) ? true : false;
            trigger_compare->flag.falling      = (mdf_json_parse(trigger_compare_json, "\\",
                                                  &trigger_compare->falling) == ESP_OK) ? true : false;
        } while (0);

        ret = mdf_json_parse(trigger_device->raw_data, "trigger_content", &trigger_content);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_parse, trigger_content, ret: %d", ret);

        char request_temp[16] = {0};

        if (mdf_json_parse(trigger_content, "request", request_temp) < 0) {
            MDF_LOGW("mdf_json_parse, ret: %d", ret);
            mdf_free(trigger_device);
            return ESP_FAIL;
        }

        if (!strcasecmp(request_temp, "sync")) {
            trigger_device->trigger_type = TRIGGER_SYNC;

            if (mdf_json_parse(trigger_content, "execute_cid", trigger_device->trigger_params) < 0) {
                MDF_LOGW("mdf_json_parse, ret: %d", ret);
                mdf_free(trigger_device);
                return ESP_FAIL;
            }
        } else if (!strcasecmp(request_temp, "linkage")) {
            trigger_device->trigger_type = TRIGGER_LINKAGE;

            if (mdf_json_parse(trigger_device->raw_data, "execute_content", &trigger_device->execute_content) < 0) {
                MDF_LOGW("mdf_json_parse, ret: %d", ret);
                mdf_free(trigger_device);
                return ESP_FAIL;
            }
        } else if (!strcasecmp(request_temp, "delay")) {
            trigger_device->trigger_type = TRIGGER_DELAY;

            if (mdf_json_parse(trigger_device->raw_data, "time", trigger_device->trigger_params) < 0) {
                MDF_LOGW("mdf_json_parse, ret: %d", ret);
                mdf_free(trigger_device);
                return ESP_FAIL;
            }

            if (mdf_json_parse(trigger_device->raw_data, "frequency", trigger_device->trigger_params + 1) < 0) {
                MDF_LOGW("mdf_json_parse, ret: %d", ret);
                mdf_free(trigger_device);
                return ESP_FAIL;
            }

            trigger_device->trigger_params[2] = xTaskGetTickCount() + trigger_device->trigger_params[0] * 1000 * portTICK_RATE_MS;

            if (mdf_json_parse(trigger_device->raw_data, "execute_content", &trigger_device->execute_content) < 0) {
                MDF_LOGW("mdf_json_parse, ret: %d", ret);
                mdf_free(trigger_device);
                return ESP_FAIL;
            }
        }

        trigger_device->next   = g_trigger_device->next;
        g_trigger_device->next = trigger_device;

        ret = mdf_device_get_value(trigger_device->trigger_cid, &trigger_device->trigger_compare.value);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_device_get_value, ret: %d", ret);

        MDF_LOGD("events_name: %s", trigger_device->name);
        MDF_LOGD("trigger_type: %d", trigger_device->trigger_type);
        MDF_LOGD("value: %d", trigger_device->trigger_compare.value);
    }

    trigger_store_manager_t *store_manager = mdf_calloc(1, EVENT_MAX_NUM * sizeof(trigger_store_manager_t));
    trigger_device_t *trigger_device     = g_trigger_device->next;

    for (int i = 0; trigger_device && i < EVENT_MAX_NUM; trigger_device = trigger_device->next, i++) {
        strncpy(store_manager[i].event_name, trigger_device->name, sizeof(store_manager[i].event_name));
        store_manager[i].event_size = sizeof(trigger_device_t) + strlen(trigger_device->raw_data) + 1;

        ret = mdf_info_save(store_manager[i].event_name, trigger_device, store_manager[i].event_size);
        MDF_ERROR_BREAK(ret < 0, "mdf_info_save, ret: %d", ret);
    }

    ret = mdf_info_save("store_manager", store_manager, EVENT_MAX_NUM * sizeof(trigger_store_manager_t));
    mdf_free(store_manager);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_save, ret: %d", ret);

    return ESP_OK;
}

static esp_err_t mdf_trigger_remove_event(device_data_t *device_data)
{
    esp_err_t ret               = ESP_OK;
    char *events[EVENT_MAX_NUM] = {0};
    char event_name[32]         = {0};
    int events_num              = mdf_json_parse(device_data->request, "events", events);

    for (int i = 0; i < events_num; ++i) {
        ret = mdf_json_parse(events[i], "name", event_name);
        mdf_free(events[i]);
        MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_json_parse, ret: %d", ret);

        trigger_device_t *trigger_device_prior = g_trigger_device;

        for (trigger_device_t *trigger_device = trigger_device_prior->next; trigger_device;
                trigger_device_prior = trigger_device, trigger_device = trigger_device->next) {
            if (!strncasecmp(trigger_device->name, event_name, sizeof(trigger_device->name))) {
                MDF_LOGD("remove event: %s", trigger_device->name);
                ret = mdf_info_erase(trigger_device->name);
                MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_erase, ret: %d", ret);

                trigger_device_prior->next = trigger_device->next;
                mdf_free(trigger_device);
                trigger_device = trigger_device_prior;
                break;
            }
        }
    }

    trigger_store_manager_t *store_manager = mdf_calloc(1, EVENT_MAX_NUM * sizeof(trigger_store_manager_t));
    trigger_device_t *trigger_device         = g_trigger_device->next;

    for (int i = 0; trigger_device && i < EVENT_MAX_NUM; trigger_device = trigger_device->next, i++) {
        strncpy(store_manager[i].event_name, trigger_device->name, sizeof(store_manager[i].event_name));
        store_manager[i].event_size = sizeof(trigger_device_t) + strlen(trigger_device->raw_data) + 1;
    }

    ret = mdf_info_save("store_manager", store_manager, EVENT_MAX_NUM * sizeof(trigger_store_manager_t));
    mdf_free(store_manager);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_info_save, ret: %d", ret);

    return ESP_OK;
}

static esp_err_t mdf_trigger_handle(trigger_device_t *event)
{
    esp_err_t ret                 = ESP_OK;
    int value                     = -1;
    char **execute_mac_json       = NULL;
    wifi_mesh_addr_t execute_addr = {0};
    int execute_addr_num          = 0;
    wifi_mesh_data_type_t type    = {
        .proto = MDF_PROTO_JSON,
    };
    trigger_compare_t *trigger_compare = &event->trigger_compare;

    ret = mdf_device_get_value(event->trigger_cid, &value);
    MDF_ERROR_CHECK(ret < 0, ESP_FAIL, "mdf_device_get_value, ret: %d", ret);

    bool equal        = (!trigger_compare->flag.equal
                         || (trigger_compare->flag.equal
                             && trigger_compare->equal == value)) ? true : false;
    bool unequal      = (!trigger_compare->flag.unequal
                         || (trigger_compare->flag.unequal
                             && trigger_compare->unequal != value)) ? true : false;
    bool greater_than = (!trigger_compare->flag.greater_than
                         || (trigger_compare->flag.greater_than
                             && trigger_compare->greater_than < value)) ? true : false;
    bool less_than    = (!trigger_compare->flag.less_than
                         || (trigger_compare->flag.less_than
                             && trigger_compare->less_than > value)) ? true : false;
    bool variation    = (!trigger_compare->flag.variation
                         || (trigger_compare->flag.variation
                             && trigger_compare->variation <= abs(value - trigger_compare->value))) ? true : false;
    bool rising       = (!trigger_compare->flag.rising
                         || (trigger_compare->flag.rising
                             && (value - trigger_compare->value >= variation
                                 && value >= trigger_compare->rising))) ? true : false;
    bool falling      = (!trigger_compare->flag.falling
                         || (trigger_compare->flag.falling
                             && (trigger_compare->value - value >= variation
                                 && value <= trigger_compare->falling))) ? true : false;

    MDF_LOGV("variation: %d, value: %d, trigger: %d, rising: %d",
             variation, value, trigger_compare->value, trigger_compare->rising);

    char execute_content_tmp[128] = {0};

    if (event->trigger_type == TRIGGER_SYNC) {
        if (trigger_compare->value == value) {
            return ESP_OK;
        }

        event->execute_content = execute_content_tmp;
        sprintf(event->execute_content, "{\"request\":\"set_status\",\"characteristics\":[{\"cid\":%d,\"value\":%d}]}",
                event->trigger_params[0], value);
    } else if (event->trigger_type  == TRIGGER_DELAY) {
        if (xTaskGetTickCount() < event->trigger_params[2] || event->trigger_params[1] <= 0) {
            return ESP_OK;
        }

        event->trigger_params[1]--;
        event->trigger_params[2] = xTaskGetTickCount() + event->trigger_params[0] * 1000 * portTICK_RATE_MS;
    }

    trigger_compare->value = value;

    if (!equal || !unequal || !greater_than || !less_than || !variation || !rising || !falling) {
        return ESP_OK;
    }

    execute_mac_json = mdf_malloc(100 * sizeof(char *));
    execute_addr_num = mdf_json_parse(event->raw_data, "execute_mac", execute_mac_json);
    MDF_ERROR_GOTO(execute_addr_num <= 0, EXIT, "mdf_json_parse, ret: %d", ret);

    for (int i = 0; i < execute_addr_num; ++i) {
        str2mac(execute_mac_json[i], execute_addr.mac);

        if (mdf_get_running_mode() & TRANS_ESPNOW) {
            ret = mdf_low_power_send(&execute_addr, event->execute_content, strlen(event->execute_content));
            MDF_ERROR_GOTO(ret < 0, EXIT, "espnow_control_send, ret: %d", ret);
        } else {
            ret = mdf_wifi_mesh_send(&execute_addr, &type, event->execute_content, strlen(event->execute_content));
            MDF_ERROR_GOTO(ret < 0, EXIT, "mdf_wifi_mesh_send, ret: %d", ret);
        }

        MDF_LOGD("send execute_addr: "MACSTR", data: %s", MAC2STR(execute_addr.mac), event->execute_content);
    }

EXIT:
    mdf_free(execute_mac_json);
    return ret;
}

static void trigger_handle_task(void *arg)
{
    esp_err_t ret           = ESP_OK;
    mdf_running_mode_t mode = 0;

    mdf_info_load("device_mode", &mode, sizeof(mdf_running_mode_t));

    for (;;) {
        xSemaphoreTake(g_trigger_isr_xSem, portMAX_DELAY);

        for (trigger_device_t *event = g_trigger_device->next; event; event = event->next) {
            ret = mdf_trigger_handle(event);
            MDF_ERROR_CONTINUE(ret < 0, "mdf_trigger_handle, ret: %d", ret);
        }
    }

    vTaskDelete(NULL);
}

esp_err_t mdf_trigger_isr()
{
    if(!g_trigger_isr_xSem){
        g_trigger_isr_xSem = xSemaphoreCreateBinary();
    }

    xSemaphoreGive(g_trigger_isr_xSem);

    return ESP_OK;
}

esp_err_t mdf_trigger_init()
{
    esp_err_t ret          = 0;
    g_trigger_device       = mdf_malloc(sizeof(trigger_device_t));
    g_trigger_device->next = NULL;

    if(!g_trigger_isr_xSem){
        g_trigger_isr_xSem = xSemaphoreCreateBinary();
    }

    MDF_ERROR_CHECK(!mdf_device_get_value, ESP_FAIL, "get_status is not registered, can not be triggered using the event");

    trigger_store_manager_t *store_manager = mdf_calloc(1, EVENT_MAX_NUM * sizeof(trigger_store_manager_t));
    ret = mdf_info_load("store_manager", store_manager, EVENT_MAX_NUM * sizeof(trigger_store_manager_t));

    if (ret > 0) {
        for (int i = 0; i < EVENT_MAX_NUM && store_manager[i].event_size; ++i) {
            MDF_LOGD("trigger_device name: %s", store_manager[i].event_name);

            trigger_device_t *trigger_device = mdf_calloc(1, store_manager[i].event_size);
            ret = mdf_info_load(store_manager[i].event_name, trigger_device, store_manager[i].event_size);
            if(ret < 0){
                MDF_LOGW("mdf_info_load, ret: %d", ret);
                mdf_free(trigger_device);
                continue;
            }

            mdf_json_parse(trigger_device->raw_data, "execute_content", &trigger_device->execute_content);
            mdf_device_get_value(trigger_device->trigger_cid, &trigger_device->trigger_compare.value);

            trigger_device->next   = g_trigger_device->next;
            g_trigger_device->next = trigger_device;
        }
    }

    mdf_free(store_manager);

    ESP_ERROR_CHECK(mdf_device_add_handle("set_event", mdf_trigger_set_event));
    ESP_ERROR_CHECK(mdf_device_add_handle("get_event", mdf_trigger_get_event));
    ESP_ERROR_CHECK(mdf_device_add_handle("remove_event", mdf_trigger_remove_event));

    xTaskCreate(trigger_handle_task, "trigger_handle_task", 1024 * 4,
                NULL, MDF_TASK_DEFAULT_PRIOTY - 3, NULL);

    return ESP_OK;
}
