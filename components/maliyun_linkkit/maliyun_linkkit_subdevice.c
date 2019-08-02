/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2019 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
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
#include "mupgrade.h"
#include "mlink.h"

#include "mqtt_wrapper.h"
#include "wifi_provision_api.h"

#include "maliyun_linkkit_subdevice.h"
#include "maliyun_linkkit_types_priv.h"

static const char *TAG = "maliyun_linkkit_subdevice";

typedef struct subdevice_info {
    int running;
    int awss_notify;
    int is_ota;
    int gateway_status;
    int login_status;
    int  message_id;
    int  device_id;
} subdevice_info_t;

typedef struct {
    maliyun_subscribe_type_t type;
    void *func;
} maliyun_linkkit_subscribe_t;

static subdevice_info_t g_subdevice = { false, false, true, false, false, 0, MALIYUN_DEFAULT_DEVICE_ID};
static TaskHandle_t g_aliyun_linkkit_subscribe_task_handle = NULL;
static SemaphoreHandle_t g_message_notify = NULL;

static mdf_err_t maliyun_linkkit_write(maliyun_publish_type_t type, void *buf, size_t buf_len)
{
    MDF_PARAM_CHECK(buf);

    mdf_err_t ret                   = MDF_FAIL;
    mwifi_data_type_t data_type     = {
        .communicate = MWIFI_COMMUNICATE_MULTICAST,
        .protocol              = MALIYUN_MESH_TYPE,
        .custom                = type,
    };

    ret = mwifi_write(NULL, &data_type, buf, buf_len, true);
    return ret;
}

mdf_err_t maliyun_linkkit_login(uint32_t timeout_ms)
{
    mdf_err_t ret = MDF_FAIL;

    linkit_login_t *login_buf = MDF_MALLOC(sizeof(linkit_login_t));

    login_buf->message_id = g_subdevice.message_id;
    login_buf->awss_notify = g_subdevice.awss_notify;
    login_buf->sub_ota = g_subdevice.is_ota;
    login_buf->device_id = MALIYUN_DEFAULT_DEVICE_ID;
    login_buf->status = MALIYUN_STATUS_FAIL;

    HAL_GetProductKey(login_buf->meta_info.product_key);
    HAL_GetProductSecret(login_buf->meta_info.product_secret);
    HAL_GetDeviceName(login_buf->meta_info.device_name);
    HAL_GetDeviceSecret(login_buf->meta_info.device_secret);

    if (login_buf->sub_ota) {
        HAL_GetFirmwareVersion(login_buf->version);
    }

    MDF_LOGI("product_key:%s", login_buf->meta_info.product_key);
    MDF_LOGI("product_secret:%s", login_buf->meta_info.product_secret);
    MDF_LOGI("device_name:%s", login_buf->meta_info.device_name);
    MDF_LOGI("device_secret:%s", login_buf->meta_info.device_secret);
    MDF_LOGI("awss_notify:%d", login_buf->awss_notify);
    MDF_LOGI("FirmwareVersion:%s", login_buf->version);

    ret = maliyun_linkkit_write(MALIYUN_LINKKIT_PUB_LOGIN, login_buf, sizeof(linkit_login_t));

    if (login_buf != NULL) {
        MDF_FREE(login_buf);
    }

    MDF_ERROR_CHECK(ret != ESP_OK, ret, "maliyun_linkkit_write error");

    if (g_message_notify != NULL) {
        ret = xSemaphoreTake(g_message_notify, timeout_ms / portTICK_RATE_MS);

        if (ret == pdTRUE) {
            return  g_subdevice.login_status ? ESP_OK : ESP_FAIL;
        } else {
            return ESP_ERR_TIMEOUT;
        }
    } else {
        g_message_notify = xSemaphoreCreateBinary();
        MDF_LOGI("g_message_notify is NULL");
        return ESP_FAIL;
    }
}

mdf_err_t maliyun_linkkit_logout(uint32_t timeout_ms)
{
    mdf_err_t ret                   = MDF_FAIL;

    linkit_logout_t *logout_buf = MDF_MALLOC(sizeof(linkit_logout_t));

    logout_buf->message_id = g_subdevice.message_id;
    logout_buf->device_id = g_subdevice.device_id;
    logout_buf->status = MALIYUN_STATUS_FAIL;
    HAL_GetProductKey(logout_buf->product_key);
    HAL_GetDeviceName(logout_buf->device_name);
    MDF_LOGI("product_key:%s", logout_buf->product_key);
    MDF_LOGI("device_name:%s", logout_buf->device_name);

    ret = maliyun_linkkit_write(MALIYUN_LINKKIT_PUB_LOGOUT, logout_buf, sizeof(linkit_logout_t));

    if (logout_buf != NULL) {
        MDF_FREE(logout_buf);
    }

    MDF_ERROR_CHECK(ret != ESP_OK, ret, "maliyun_linkkit_write error");

    if (g_message_notify != NULL) {

        ret = xSemaphoreTake(g_message_notify, timeout_ms / portTICK_RATE_MS);
        if (ret == pdTRUE) {
            return  !g_subdevice.login_status ? ESP_OK : ESP_FAIL;
        } else {
            return ESP_ERR_TIMEOUT;
        }

    } else {
        g_message_notify = xSemaphoreCreateBinary();
        return ESP_FAIL;
    }
}

mdf_err_t maliyun_linkkit_reset(uint32_t timeout_ms)
{
    mdf_err_t ret                   = MDF_FAIL;

    linkit_reset_t *reset_buf = MDF_MALLOC(sizeof(linkit_reset_t));

    reset_buf->message_id = g_subdevice.message_id;
    reset_buf->device_id = g_subdevice.device_id;
    reset_buf->status = MALIYUN_STATUS_FAIL;
    HAL_GetProductKey(reset_buf->meta_info.product_key);
    HAL_GetDeviceName(reset_buf->meta_info.device_name);

    MDF_LOGI("product_key:%s", reset_buf->meta_info.product_key);
    MDF_LOGI("device_name:%s", reset_buf->meta_info.device_name);

    ret = maliyun_linkkit_write(MALIYUN_LINKKIT_PUB_RESET, reset_buf, sizeof(linkit_reset_t));

    if (reset_buf != NULL) {
        MDF_FREE(reset_buf);
    }

    MDF_ERROR_CHECK(ret != ESP_OK, ret, "maliyun_linkkit_write error");

    if (g_message_notify != NULL) {

        ret = xSemaphoreTake(g_message_notify, timeout_ms / portTICK_RATE_MS);
        if (ret == pdTRUE) {
            return  !g_subdevice.login_status ? ESP_OK : ESP_FAIL;
        } else {
            return ESP_ERR_TIMEOUT;
        }

    } else {
        g_message_notify = xSemaphoreCreateBinary();
        return ESP_FAIL;
    }
}

mdf_err_t maliyun_linkkit_report(maliyun_linkkit_msg_type_t message_type, uint8_t *payload,
                                 size_t payload_len)
{

    mdf_err_t    ret                = MDF_FAIL;

    maliyun_linkkit_report_t *report_buf = MDF_MALLOC(sizeof(maliyun_linkkit_report_t) + payload_len);

    report_buf->message_id   = g_subdevice.message_id;
    report_buf->device_id    = g_subdevice.device_id;
    report_buf->message_type = message_type;
    report_buf->payload_len  = payload_len;

    if (payload_len > 0) {
        MDF_PARAM_CHECK(payload);
        memcpy(report_buf->payload, payload, payload_len);
    }

    ret = maliyun_linkkit_write(MALIYUN_LINKKIT_PUB_REPORT, (uint8_t *)report_buf, sizeof(maliyun_linkkit_report_t) + payload_len);

    MDF_FREE(report_buf);
    return ret;
}

mdf_err_t maliyun_linkkit_query(maliyun_linkkit_msg_type_t message_type, uint8_t *payload,
                                size_t payload_len)
{
    mdf_err_t    ret                   = MDF_FAIL;

    maliyun_linkkit_query_t *query_buf = MDF_MALLOC(sizeof(maliyun_linkkit_query_t) + payload_len);

    query_buf->message_id    = g_subdevice.message_id;
    query_buf->device_id    = g_subdevice.device_id;
    query_buf->message_type = message_type;
    query_buf->payload_len  = payload_len;

    if (payload_len > 0) {
        MDF_PARAM_CHECK(payload);
        memcpy(query_buf->payload, payload, payload_len);
    }

    ret = maliyun_linkkit_write(MALIYUN_LINKKIT_PUB_QUERY, (uint8_t *)query_buf, sizeof(maliyun_linkkit_query_t) + payload_len);

    MDF_FREE(query_buf);
    return ret;
}


mdf_err_t maliyun_linkkit_trigger_event(char *eventid, size_t eventid_len,
                                        char *payload, size_t payload_len)
{
    MDF_PARAM_CHECK(eventid);

    mdf_err_t    ret                   = MDF_FAIL;

    maliyun_linkkit_trigger_event_t *trigger_event_buf = MDF_MALLOC(sizeof(maliyun_linkkit_trigger_event_t) + eventid_len + 1 + payload_len);

    trigger_event_buf->message_id = g_subdevice.message_id;
    trigger_event_buf->device_id = g_subdevice.device_id;
    trigger_event_buf->eventid_len = eventid_len;
    trigger_event_buf->payload_len = payload_len;
    strncpy(trigger_event_buf->eventid_payload, eventid, eventid_len);
    *(trigger_event_buf->eventid_payload + eventid_len) = 0;

    if (payload_len > 0) {
        strncpy(trigger_event_buf->eventid_payload + eventid_len + 1, payload, payload_len);
    }

    ret = maliyun_linkkit_write(MALIYUN_LINKKIT_PUB_TRIGGEREVENT, (uint8_t *)trigger_event_buf,
                                sizeof(maliyun_linkkit_trigger_event_t) + eventid_len + payload_len);

    MDF_FREE(trigger_event_buf);
    return ret;
}


static maliyun_linkkit_subscribe_t g_linkkit_subscribe_list [MALIYUN_LINKKIT_SUB_TYPE_MAX] = {

    {MALIYUN_LINKKIT_SUB_AWSS,               	    NULL},
    {MALIYUN_LINKKIT_SUB_LOGIN,              	    NULL},
    {MALIYUN_LINKKIT_SUB_LOGOUT,              	    NULL},
    {MALIYUN_LINKKIT_SUB_RESET,                     NULL},
    {MALIYUN_LINKKIT_SUB_GATEWAY_STATUS,            NULL},
    {MALIYUN_LINKKIT_SUB_RAWDATA_ARRIVED,           NULL},
    {MALIYUN_LINKKIT_SUB_PROPERTY_SET,              NULL},
    {MALIYUN_LINKKIT_SUB_REPORT_REPLY,              NULL},
    {MALIYUN_LINKKIT_SUB_TRIGGER_EVENT_REPLY,       NULL},
    {MALIYUN_LINKKIT_SUB_TIMESTAMP_REPLY,           NULL},
};

mdf_err_t maliyun_linkkit_subscribe_set(const maliyun_subscribe_type_t type, const void *func)
{
    MDF_PARAM_CHECK(func);

    for (int i = 0; i < MALIYUN_LINKKIT_SUB_TYPE_MAX; i++) {
        if (g_linkkit_subscribe_list[i].type == type) {
            g_linkkit_subscribe_list[i].func = (void *)func;
            return ESP_OK;
        }
    }

    MDF_LOGW("aliyun linkkit subscribe list is full !!");
    return ESP_FAIL;
}

static mdf_err_t maliyun_linkkit_subscribe(const maliyun_subscribe_type_t type, const void *data, size_t data_len)
{
    MDF_PARAM_CHECK(data);

    switch (type) {
        case MALIYUN_LINKKIT_SUB_AWSS:
            MDF_LOGD("MALIYUN_LINKKIT_SUB_AWSS");

            if (g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_AWSS].type == MALIYUN_LINKKIT_SUB_AWSS
                    && g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_AWSS].func != NULL) {
                ((maliyun_linkkit_awss_cb_t)(g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_AWSS].func))(*(int *)data);
            }

            break;

        case MALIYUN_LINKKIT_SUB_LOGIN: {
            MDF_LOGI("MALIYUN_LINKKIT_SUB_LOGIN");
            linkit_login_t *login_reply_buf = (linkit_login_t *)data;

            if (login_reply_buf->status == MALIYUN_STATUS_SUCC) {
                g_subdevice.login_status = true;
                g_subdevice.device_id = login_reply_buf->device_id;
                g_subdevice.awss_notify = false;
            } else {
                g_subdevice.login_status = false;
            }

            if (g_message_notify != NULL) {
                xSemaphoreGive(g_message_notify);
            }

            if (g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_LOGIN].type == MALIYUN_LINKKIT_SUB_LOGIN
                    && g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_LOGIN].func != NULL) {
                ((maliyun_linkkit_login_cb_t)(g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_LOGIN].func))
                ((int)login_reply_buf->status, (const char *)login_reply_buf->meta_info.product_key, (const char *)login_reply_buf->meta_info.device_name);
            }
        }
        break;

        case MALIYUN_LINKKIT_SUB_LOGOUT: {
            MDF_LOGI("MALIYUN_LINKKIT_SUB_LOGOUT");
            linkit_logout_t *logout_reply_buf = (linkit_logout_t *)data;

            if (logout_reply_buf->status == MALIYUN_STATUS_SUCC) {
                g_subdevice.device_id = MALIYUN_DEFAULT_DEVICE_ID;
                g_subdevice.login_status = false;
            }

            if (g_message_notify != NULL) {
                xSemaphoreGive(g_message_notify);
            }

            if (g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_LOGOUT].type == MALIYUN_LINKKIT_SUB_LOGOUT
                    && g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_LOGOUT].func != NULL) {
                ((maliyun_linkkit_logout_cb_t)(g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_LOGOUT].func))
                ((int)logout_reply_buf->status, (const char *)logout_reply_buf->product_key, (const char *)logout_reply_buf->device_name);
            }
        }
        break;

        case MALIYUN_LINKKIT_SUB_RESET: {
            MDF_LOGI("MALIYUN_LINKKIT_SUB_RESET");
            linkit_reset_t *reset_reply_buf = (linkit_reset_t *)data;

            if (reset_reply_buf->status == MALIYUN_STATUS_SUCC) {
                g_subdevice.device_id = MALIYUN_DEFAULT_DEVICE_ID;
                g_subdevice.login_status = false;
            }

            if (g_message_notify != NULL) {
                xSemaphoreGive(g_message_notify);
            }

            if (g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_RESET].type == MALIYUN_LINKKIT_SUB_RESET
                    && g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_RESET].func != NULL) {
                ((maliyun_linkkit_reset_cb_t)(g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_RESET].func))
                ((int)reset_reply_buf->status, (const char *)reset_reply_buf->meta_info.product_key, (const char *)reset_reply_buf->meta_info.device_name);
            }
        }
        break;

        case MALIYUN_LINKKIT_SUB_GATEWAY_STATUS:
            MDF_LOGD("MALIYUN_LINKKIT_SUB_GATEWAY_STATUS");

            if (g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_GATEWAY_STATUS].type == MALIYUN_LINKKIT_SUB_GATEWAY_STATUS
                    && g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_GATEWAY_STATUS].func != NULL) {
                ((maliyun_linkkit_gateway_status_cb_t)(g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_GATEWAY_STATUS].func))
                ((char *)data, (size_t)data_len);
            }

            break;

        case MALIYUN_LINKKIT_SUB_RAWDATA_ARRIVED:
            MDF_LOGI("MALIYUN_LINKKIT_SUB_RAWDATA_ARRIVED");

            if (g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_RAWDATA_ARRIVED].type == MALIYUN_LINKKIT_SUB_RAWDATA_ARRIVED
                    && g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_RAWDATA_ARRIVED].func != NULL) {
                ((maliyun_linkkit_rawdata_arrived_cb_t)(g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_RAWDATA_ARRIVED].func))
                ((const unsigned char *)data, (size_t)data_len);
            }

            break;

        case MALIYUN_LINKKIT_SUB_PROPERTY_SET:
            MDF_LOGD("MALIYUN_LINKKIT_SUB_PROPERTY_SET");

            if (g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_PROPERTY_SET].type == MALIYUN_LINKKIT_SUB_PROPERTY_SET
                    && g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_PROPERTY_SET].func != NULL) {
                ((maliyun_linkkit_propery_set_cb_t)(g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_PROPERTY_SET].func))
                ((const char *)data, (size_t)data_len);
            }

            break;

        case MALIYUN_LINKKIT_SUB_REPORT_REPLY:
            MDF_LOGD("MALIYUN_LINKKIT_SUB_REPORT_REPLY");

            if (g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_REPORT_REPLY].type == MALIYUN_LINKKIT_SUB_REPORT_REPLY
                    && g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_REPORT_REPLY].func != NULL) {
                maliyun_linkkit_report_reply_t *report_reply_buf = (maliyun_linkkit_report_reply_t *)data;
                ((maliyun_linkkit_report_reply_cb_t)(g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_REPORT_REPLY].func))
                ((int)report_reply_buf->message_id, (int)report_reply_buf->code,
                 (const char *)report_reply_buf->reply, (size_t)report_reply_buf->reply_len);
            }

            break;

        case MALIYUN_LINKKIT_SUB_TRIGGER_EVENT_REPLY:
            MDF_LOGI("MALIYUN_LINKKIT_SUB_TRIGGER_EVENT_REPLY");

            if (g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_TRIGGER_EVENT_REPLY].type == MALIYUN_LINKKIT_SUB_TRIGGER_EVENT_REPLY
                    && g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_TRIGGER_EVENT_REPLY].func != NULL) {
                maliyun_linkkit_trigger_event_reply_t *trigger_event_reply_buf = (maliyun_linkkit_trigger_event_reply_t *)data;
                ((maliyun_linkkit_trigger_event_reply_cb_t)(g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_TRIGGER_EVENT_REPLY].func))
                ((int)trigger_event_reply_buf->message_id,
                 (int)trigger_event_reply_buf->code,
                 (const char *)trigger_event_reply_buf->eventid_message,
                 (size_t)trigger_event_reply_buf->eventid_len,
                 (const char *)trigger_event_reply_buf->eventid_message + MALIYUN_SEPARATOR +
                 trigger_event_reply_buf->eventid_len,
                 (size_t)trigger_event_reply_buf->message_len);
            }

            break;

        case MALIYUN_LINKKIT_SUB_TIMESTAMP_REPLY:
            MDF_LOGI("MALIYUN_LINKKIT_SUB_TRIGGER_EVENT_REPLY");

            if (g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_TIMESTAMP_REPLY].type == MALIYUN_LINKKIT_SUB_TIMESTAMP_REPLY
                    && g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_TIMESTAMP_REPLY].func != NULL) {
                ((maliyun_linkkit_timestamp_reply_cb_t)(g_linkkit_subscribe_list[MALIYUN_LINKKIT_SUB_TIMESTAMP_REPLY].func))
                ((const char *)data);
            }

            break;

        default:
            MDF_LOGW("aliyun_linkkit_check_cb_t type is null");
            break;
    }

    return MDF_OK;
}

static void maliyun_linkkit_event_monitor(int event)
{
    MDF_LOGD("maliyun_linkkit_event_monitor:%d", event);
    maliyun_linkkit_subscribe(MALIYUN_LINKKIT_SUB_AWSS, (void *) &event, sizeof(event));
}

static void maliyun_linkkit_subscribe_task(void *arg)
{
    mdf_err_t ret    = MDF_OK;
    uint8_t   *data  = NULL;
    size_t    size   = MALIYUN_PAYLOAD_LEN;
    mwifi_data_type_t data_type      = {0x0};
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};

    while (g_subdevice.running) {
        if (!mwifi_is_connected()) {
            vTaskDelay(MALIYUN_YIELD_MS / portTICK_PERIOD_MS);
            continue;
        }

        size = MALIYUN_PAYLOAD_LEN;
        ret = mwifi_read(src_addr, &data_type, &data, &size, portMAX_DELAY);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> Receive a packet targeted to self over the mesh network",
                           mdf_err_to_name(ret));

        if (data_type.upgrade) {
            ret = mupgrade_handle(src_addr, data, size);
            MDF_ERROR_GOTO(ret != MDF_OK, FREE_MEM, "<%s> mupgrade_handle", mdf_err_to_name(ret));
            goto FREE_MEM;
        }

        MDF_LOGD("Node receive, addr: " MACSTR ", size: %d", MAC2STR(src_addr), size);

        switch (data_type.protocol) {
            case MALIYUN_MESH_TYPE: {
                maliyun_linkkit_subscribe(data_type.custom, data, size);

                break;
            }

            case MLINK_PROTO_HTTPD: {
                MDF_LOGI("MLINK_PROTO_HTTPD:%s", (char *)data);
                cJSON *pJson = NULL;
                cJSON *pnext  = NULL;

                pJson = cJSON_Parse((char *)data);
                MDF_ERROR_CONTINUE(!pJson, "cJSON_Parse");
                pnext = cJSON_GetObjectItem(pJson, "cmd");
                MDF_ERROR_GOTO(pnext == NULL, JSON_EXIT, "cJSON Parse IsOTA Error");

                if (!strcmp(pnext->valuestring, "restart")) {
                    MDF_LOGW("The device will restart after 3 seconds");
                    vTaskDelay(3000 / portTICK_RATE_MS);
                    esp_restart();
                }

JSON_EXIT:
                /**< TODO FREE()*/
                cJSON_Delete(pJson);
                break;
            }
        }

FREE_MEM:
        MDF_FREE(data);
    }

    MDF_FREE(data);
    g_aliyun_linkkit_subscribe_task_handle = NULL;
    vTaskDelete(NULL);
}

mdf_err_t maliyun_linkkit_task_init(void)
{
    mdf_err_t ret = MDF_OK;
    g_subdevice.running = true;

    if (g_message_notify == NULL) {
        g_message_notify = xSemaphoreCreateBinary();
    }

    iotx_event_regist_cb(maliyun_linkkit_event_monitor);

    if (g_aliyun_linkkit_subscribe_task_handle == NULL) {
        MDF_LOGD("xTaskCreate aliyun_linkkit_task !!");
        ret = xTaskCreate(maliyun_linkkit_subscribe_task, "linkkit_sub_task", 4 * 1024, NULL,
                          CONFIG_MDF_TASK_DEFAULT_PRIOTY, &g_aliyun_linkkit_subscribe_task_handle);
    }

    return ret;
}

mdf_err_t maliyun_linkkit_task_deinit(void)
{
    g_subdevice.running = false;
    return MDF_OK;
}

bool maliyun_linkkit_get_login_status(void)
{
    return g_subdevice.login_status;
}

mdf_err_t maliyun_linkkit_set_gateway_status(bool status)
{
    g_subdevice.gateway_status = status;
    return MDF_OK;
}

bool maliyun_linkkit_get_gateway_status(void)
{
    g_subdevice.gateway_status = mwifi_get_root_status();

    if (g_subdevice.gateway_status == false) {
        g_subdevice.login_status = false;
    }

    return g_subdevice.gateway_status;
}

mdf_err_t maliyun_linkkit_start_awss(void)
{
    g_subdevice.awss_notify = true;
    MDF_LOGI("maliyun start awss network config:%d", g_subdevice.awss_notify);
    awss_config_press();
    awss_start();
    return MDF_OK;
}