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

#include "maliyun_linkkit_subdevice.h"
#include "maliyun_linkkit_list_priv.h"
#include "maliyun_linkkit_types_priv.h"

typedef struct {
    bool running;
    bool connected;
    bool initialized;
    int  device_id;
} gateway_info_t;

static const char *TAG = "maliyun_linkkit_gateway";
static gateway_info_t g_gateway = {false, false, false, MALIYUN_DEFAULT_DEVICE_ID};
static TaskHandle_t g_maliyun_linkkit_gateway_publish_task_handle  = NULL;
static TaskHandle_t g_maliyun_linkkit_gateway_subscribe_task_handle = NULL;
extern int awss_notify_dev_info(int type, int count);

static mdf_err_t maliyun_linkkit_gateway_write(uint8_t dest_addrs[], maliyun_subscribe_type_t type, void *buf, size_t buf_len)
{
    MDF_PARAM_CHECK(dest_addrs);
    MDF_PARAM_CHECK(buf);

    mdf_err_t ret                   = MDF_FAIL;
    mwifi_data_type_t data_type     = {.communicate = MWIFI_COMMUNICATE_MULTICAST};
    data_type.protocol              = MALIYUN_MESH_TYPE;
    data_type.custom                = type;

    MDF_LOGD("write, addr: " MACSTR ", size: %d", MAC2STR(dest_addrs), buf_len);

    ret = mwifi_root_write(dest_addrs, 1, &data_type, (uint8_t *)buf, buf_len, true);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "<%s> mwifi_root_write, dest_addrs: "MACSTR,
                    mdf_err_to_name(ret), MAC2STR(dest_addrs));
    return ret;
}

static mdf_err_t maliyun_linkkit_sub_login(uint8_t *dest_addrs, linkit_login_t *login_buf, size_t buf_len)
{
    mdf_err_t ret = MDF_OK;

    ret = maliyun_linkkit_list_get_id(dest_addrs, &login_buf->device_id);
    MDF_LOGI("dest_addrs: " MACSTR ", login_buf->device_id: %d, ret: %d", MAC2STR(dest_addrs), login_buf->device_id, ret);

    if (ret != MDF_OK) {
        MDF_LOGI("sub device login ------------ ");
        MDF_LOGD("product_key: %s", login_buf->meta_info.product_key);
        MDF_LOGD("product_secret:%s", login_buf->meta_info.product_secret);
        MDF_LOGD("device_name: %s", login_buf->meta_info.device_name);
        MDF_LOGD("device_secret: %s", login_buf->meta_info.device_secret);

        login_buf->device_id = IOT_Linkkit_Open(IOTX_LINKKIT_DEV_TYPE_SLAVE, &login_buf->meta_info);
        MDF_ERROR_GOTO(login_buf->device_id < 0, LOGIN_EXIT, "IOT_Linkkit_Open Failed !!");

        MDF_LOGI("sub device id: %d, mac: " MACSTR, login_buf->device_id, MAC2STR(dest_addrs));

        ret = IOT_Linkkit_Connect(login_buf->device_id);
        MDF_ERROR_GOTO(ret < 0, LOGIN_EXIT, "IOT_Linkkit_Connect Failed !!");

        ret = IOT_Linkkit_Report(login_buf->device_id, ITM_MSG_LOGIN, NULL, 0);
        MDF_ERROR_GOTO(ret != MDF_OK, LOGIN_EXIT, "IOT_Linkkit_Report Login Failed !!");

        ret = maliyun_linkkit_insert(login_buf->device_id, dest_addrs, login_buf->meta_info.product_key);
        MDF_ERROR_GOTO(ret != MDF_OK, LOGIN_EXIT, "maliyun_linkkit_insert error, ret: %d, device id: %d, mac: "
                       MACSTR, ret, login_buf->device_id, MAC2STR(dest_addrs));

        if (login_buf->sub_ota) {
            MDF_LOGI("sub device open sub_is_ota !!!");
            IOT_Linkkit_Report(login_buf->device_id, ITM_MSG_REPORT_SUBDEV_FIRMWARE_VERSION, (uint8_t *)login_buf->version, strlen(login_buf->version));
            IOT_Ioctl(IOTX_IOCTL_SET_OTA_DEV_ID, (void *)&login_buf->device_id);
        }
    } else {
        if (login_buf->device_id != g_gateway.device_id) {
            IOT_Linkkit_Report(login_buf->device_id, ITM_MSG_LOGIN, NULL, 0);
        }
    }

    if (login_buf->awss_notify) {
        MDF_LOGI("sub device open awss notify !!!");
#define AWSS_NOTIFY_SUCCESS 3
#define AWSS_NOTIFY_COUNT   1
        awss_notify_dev_info(AWSS_NOTIFY_SUCCESS, AWSS_NOTIFY_COUNT);
    }

    login_buf->status = MALIYUN_STATUS_SUCC;
    ret = maliyun_linkkit_gateway_write(dest_addrs, MALIYUN_LINKKIT_SUB_LOGIN, login_buf, sizeof(linkit_login_t));
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "maliyun_linkkit_node_login_reply Failed !!");
    return ESP_OK;

LOGIN_EXIT:

    if (login_buf->device_id > 0) {
        IOT_Linkkit_Close(login_buf->device_id);

        if (MDF_OK == maliyun_linkkit_list_get_id(dest_addrs, &login_buf->device_id)) {
            maliyun_linkkit_list_delete_id(login_buf->device_id);
        }

        login_buf->device_id = MALIYUN_DEFAULT_DEVICE_ID;
    }

    login_buf->status = MALIYUN_STATUS_FAIL;
    MDF_LOGE("LOGIN ERROR EXIT !!, device id : %d, mac: " MACSTR, login_buf->device_id, MAC2STR(dest_addrs));
    ret = maliyun_linkkit_gateway_write(dest_addrs, MALIYUN_LINKKIT_SUB_LOGIN, login_buf, sizeof(linkit_login_t));
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "maliyun_linkkit_node_login_reply Failed !!");
    return ESP_FAIL;
}

static mdf_err_t maliyun_linkkit_sub_logout(uint8_t *dest_addrs, linkit_logout_t *logout_buf, size_t buf_len)
{
    mdf_err_t ret  = MDF_FAIL;

    int  sub_device_id = MALIYUN_DEFAULT_DEVICE_ID;
    ret = maliyun_linkkit_list_get_id(dest_addrs, &sub_device_id);
    MDF_ERROR_CHECK(ret != MDF_OK || sub_device_id != logout_buf->device_id, MDF_FAIL, "maliyun linkkit logout device id error !!");

    MDF_ERROR_CHECK(logout_buf->device_id == g_gateway.device_id, ESP_FAIL,
                    "The device is a gateway and does not allow this mode to exit");

    MDF_ERROR_CHECK(logout_buf->device_id < 0, ESP_FAIL, "The device id is error !!");

    ret = IOT_Linkkit_Report(logout_buf->device_id, ITM_MSG_LOGOUT, NULL, 0);
    MDF_ERROR_GOTO(ret != MDF_OK, LOGOUT_EXIT, "IOT_Linkkit_Report Failed !!");

    ret = IOT_Linkkit_Close(logout_buf->device_id);
    MDF_ERROR_GOTO(ret != MDF_OK, LOGOUT_EXIT, "IOT_Linkkit_Close Failed !!");

    ret = maliyun_linkkit_list_delete_id(logout_buf->device_id);
    MDF_ERROR_GOTO(ret != MDF_OK, LOGOUT_EXIT, "maliyun_linkkit_list_delete_id Failed !!");

    logout_buf->status = MALIYUN_STATUS_SUCC;
    logout_buf->device_id = MALIYUN_DEFAULT_DEVICE_ID;
    ret = maliyun_linkkit_gateway_write(dest_addrs, MALIYUN_LINKKIT_SUB_LOGOUT, logout_buf, sizeof(linkit_logout_t));
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "maliyun_linkkit_logout_reply Failed !!");
    return ESP_OK;

LOGOUT_EXIT:
    logout_buf->status = MALIYUN_STATUS_FAIL;
    logout_buf->device_id = MALIYUN_DEFAULT_DEVICE_ID;
    MDF_LOGE("LOGOUT ERROR EXIT !!, device id :%d, mac:" MACSTR, logout_buf->device_id, MAC2STR(dest_addrs));
    ret = maliyun_linkkit_gateway_write(dest_addrs, MALIYUN_LINKKIT_SUB_LOGOUT, logout_buf, sizeof(linkit_logout_t));
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "maliyun_linkkit_logout_reply Failed !!");
    return ESP_FAIL;
}

static void maliyun_linkkit_reset_handler(iotx_devrst_evt_type_t evt, void *msg)
{
    MDF_LOGD("maliyun_linkkit_reset_handler");
}

static mdf_err_t maliyun_linkkit_publish(uint8_t dest_addrs[], maliyun_subscribe_type_t type, uint8_t *buf, size_t buf_len)
{
    MDF_PARAM_CHECK(dest_addrs);
    MDF_PARAM_CHECK(buf);

    mdf_err_t ret  = MDF_FAIL;

    if (!g_gateway.connected || !g_gateway.initialized) {
        return MDF_OK;
    }

    switch (type) {
        case MALIYUN_LINKKIT_PUB_LOGIN:
            maliyun_linkkit_sub_login(dest_addrs, (linkit_login_t *)buf, buf_len);
            break;

        case MALIYUN_LINKKIT_PUB_LOGOUT:
            maliyun_linkkit_sub_logout(dest_addrs, (linkit_logout_t *)buf, buf_len);
            break;

        case MALIYUN_LINKKIT_PUB_RESET:{
             linkit_reset_t *reset_buf = (linkit_reset_t *)buf;
             IOT_DevReset_Report((iotx_dev_meta_info_t *)&reset_buf->meta_info, maliyun_linkkit_reset_handler,NULL);
            }
            break;

        case MALIYUN_LINKKIT_PUB_REPORT: {
            maliyun_linkkit_report_t *report_buf = (maliyun_linkkit_report_t *)buf;

            int sub_device_id = MALIYUN_DEFAULT_DEVICE_ID;
            ret = maliyun_linkkit_list_get_id(dest_addrs, &sub_device_id);
            MDF_ERROR_CHECK(ret != MDF_OK || sub_device_id != report_buf->device_id, MDF_FAIL, "maliyun linkkit report device id error !!");

            if (sub_device_id == g_gateway.device_id && report_buf->message_type == MALIYUN_LINKKIT_MSG_REPORT_SUBDEV_FIRMWARE_VERSION) {
                MDF_LOGI("The firmware version push mode can only be used by sub-devices.");
            } else {
                IOT_Linkkit_Report(report_buf->device_id, report_buf->message_type, report_buf->payload, report_buf->payload_len);
            }
        }
        break;

        case MALIYUN_LINKKIT_PUB_QUERY: {
            maliyun_linkkit_query_t *query_buf = (maliyun_linkkit_query_t *)buf;

            int  sub_device_id = MALIYUN_DEFAULT_DEVICE_ID;
            ret = maliyun_linkkit_list_get_id(dest_addrs, &sub_device_id);
            MDF_ERROR_CHECK(ret != MDF_OK || sub_device_id != query_buf->device_id, MDF_FAIL, "maliyun linkkit query device id error !!");

            IOT_Linkkit_Query(query_buf->device_id, query_buf->message_type, query_buf->payload, query_buf->payload_len);
        }
        break;

        case MALIYUN_LINKKIT_PUB_TRIGGEREVENT: {
            maliyun_linkkit_trigger_event_t *trigger_event_buf = (maliyun_linkkit_trigger_event_t *)buf;

            int  sub_device_id = MALIYUN_DEFAULT_DEVICE_ID;
            ret = maliyun_linkkit_list_get_id(dest_addrs, &sub_device_id);
            MDF_ERROR_CHECK(ret != MDF_OK || sub_device_id != trigger_event_buf->device_id, MDF_FAIL,
                            "maliyun linkkit trigger event device id error !!");

            IOT_Linkkit_TriggerEvent(trigger_event_buf->device_id, trigger_event_buf->eventid_payload,
                                     trigger_event_buf->eventid_len, trigger_event_buf->eventid_payload + MALIYUN_SEPARATOR
                                     + trigger_event_buf->eventid_len, trigger_event_buf->payload_len);
        }
        break;

        default:
            MDF_LOGW("maliyun_linkkit_publish type is not find !!");
            break;

    }

    return ret;
}

static void maliyun_linkkit_gateway_publish_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    uint8_t *data = NULL;
    size_t size   = MALIYUN_PAYLOAD_LEN;
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};
    mwifi_data_type_t data_type      = {0};

    MDF_LOGI("aliyun_linkkit_gateway_send_task is running");

    while (g_gateway.running) {

        if (!mwifi_is_connected()) {
            vTaskDelay(MALIYUN_YIELD_MS / portTICK_PERIOD_MS);
            continue;
        }

        ret = mwifi_root_read(src_addr, &data_type, &data, &size, portMAX_DELAY);
        MDF_ERROR_GOTO(ret != MDF_OK, FREE_MEM, "<%s> mwifi_root_read", mdf_err_to_name(ret));

        if (data_type.upgrade) {
            ret = mupgrade_root_handle(src_addr, data, size);
            MDF_ERROR_GOTO(ret != MDF_OK, FREE_MEM, "<%s> mupgrade_handle", mdf_err_to_name(ret));
            continue;
        }

        MDF_LOGD("Root receive, addr: " MACSTR ", size: %d",
                 MAC2STR(src_addr), size);

        switch (data_type.protocol) {
            case MALIYUN_MESH_TYPE: {
                maliyun_linkkit_publish(src_addr, (maliyun_subscribe_type_t)data_type.custom, data, size);
            }
            break;

            default: {
                MDF_LOGW("Does not support the protocol: %d", data_type.protocol);
            }
            break;
        }

FREE_MEM:
        MDF_FREE(data);

    }

    MDF_LOGW("maliyun_linkkit_gateway_publish_task is exit");

    g_maliyun_linkkit_gateway_publish_task_handle = NULL;

    vTaskDelete(NULL);
}

static mdf_err_t maliyun_connect_succ_event_handler(void)
{
    MDF_LOGI("Cloud Connect Success");
    g_gateway.connected =  true;
    mwifi_post_root_status(true);
    uint8_t dest_addrs[MWIFI_ADDR_LEN] = MWIFI_ADDR_ANY;
    return maliyun_linkkit_gateway_write(dest_addrs, MALIYUN_LINKKIT_SUB_GATEWAY_STATUS,
                                         MALIYUN_LINKKIT_GATEWAY_CONNECT_SUCC, strlen(MALIYUN_LINKKIT_GATEWAY_CONNECT_SUCC));
}

static mdf_err_t maliyun_connect_fail_event_handler(void)
{
    MDF_LOGW("Cloud Connect fail");
    g_gateway.connected =  false;
    mwifi_post_root_status(false);
    uint8_t dest_addrs[MWIFI_ADDR_LEN] = MWIFI_ADDR_ANY;
    return maliyun_linkkit_gateway_write(dest_addrs, MALIYUN_LINKKIT_SUB_GATEWAY_STATUS,
                                         MALIYUN_LINKKIT_GATEWAY_CONNECT_FAIL, strlen(MALIYUN_LINKKIT_GATEWAY_CONNECT_FAIL));
}

// Wifi disconnection, IP loss, short external network.
static mdf_err_t maliyun_disconnect_event_handler(void)
{
    MDF_LOGW("Cloud Disconnected");
    g_gateway.connected =  false;
    mwifi_post_root_status(false);
    uint8_t dest_addrs[MWIFI_ADDR_LEN] = MWIFI_ADDR_ANY;
    return maliyun_linkkit_gateway_write(dest_addrs, MALIYUN_LINKKIT_SUB_GATEWAY_STATUS,
                                         MALIYUN_LINKKIT_GATEWAY_DISCONNECTED, strlen(MALIYUN_LINKKIT_GATEWAY_DISCONNECTED));
}

static int maliyun_down_raw_data_arrived_event_handler(int device_id, const unsigned char *payload,
        int payload_len)
{
    MDF_LOGI("maliyun_down_raw_data_arrived_event_handler");
    esp_err_t ret;
    uint8_t dest_addrs[MWIFI_ADDR_LEN];

    ret = maliyun_linkkit_list_get_mac(device_id, dest_addrs);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "maliyun_linkkit_list_get_mac fail !");

    ret = maliyun_linkkit_gateway_write(dest_addrs, MALIYUN_LINKKIT_SUB_RAWDATA_ARRIVED, (uint8_t *)payload, payload_len);
    return ret;
}

static int maliyun_property_set_event_handler(int device_id, const char *request, int request_len)
{
    MDF_LOGD("maliyun_property_set_event_handler");

    esp_err_t ret;
    uint8_t   dest_addrs[MWIFI_ADDR_LEN];

    ret = maliyun_linkkit_list_get_mac(device_id, dest_addrs);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "maliyun_linkkit_list_get_mac fail !");

    ret = maliyun_linkkit_gateway_write(dest_addrs, MALIYUN_LINKKIT_SUB_PROPERTY_SET, (char *)request, request_len);
    return ret;
}

static int maliyun_report_reply_event_handler(int device_id, int msgid, int code, const char *reply,
        int reply_len)
{
    MDF_LOGD("maliyun linkkit report reply, device_id:%d, msgid:%d, code:%d", device_id, msgid, code);

    if (reply_len > 0) {
        MDF_LOGD("reply:%s,reply_len:%d", reply, reply_len);
    }

    mdf_err_t ret;
    uint8_t   dest_addrs[MWIFI_ADDR_LEN];

    ret = maliyun_linkkit_list_get_mac(device_id, dest_addrs);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "maliyun_linkkit_list_get_mac fail !");

    maliyun_linkkit_report_reply_t *report_reply_buf = MDF_MALLOC(sizeof(maliyun_linkkit_report_reply_t) + reply_len);

    report_reply_buf->message_id = msgid;
    report_reply_buf->code = code;
    report_reply_buf->reply_len = reply_len;

    if (reply_len > 0) {
        strncpy(report_reply_buf->reply, reply, reply_len);
    }

    ret = maliyun_linkkit_gateway_write(dest_addrs, MALIYUN_LINKKIT_SUB_REPORT_REPLY,
                                        report_reply_buf, sizeof(maliyun_linkkit_report_reply_t) + reply_len);

    MDF_FREE(report_reply_buf);
    return ret;
}

static int maliyun_trigger_event_reply_event_handler(int device_id, int msgid, int code, const char *eventid,
        int eventid_len, const char *message, int message_len)
{
    MDF_LOGI("maliyun_trigger_event_reply_event_handler");

    mdf_err_t ret;
    uint8_t   dest_addrs[MWIFI_ADDR_LEN];

    ret = maliyun_linkkit_list_get_mac(device_id, dest_addrs);
    MDF_ERROR_CHECK(ret != MDF_OK, MDF_FAIL, "maliyun_linkkit_list_get_mac fail !");


    maliyun_linkkit_trigger_event_reply_t *trigger_event_buf = MDF_MALLOC(sizeof(maliyun_linkkit_trigger_event_reply_t)
            + eventid_len + MALIYUN_SEPARATOR + message_len);

    trigger_event_buf->message_id = msgid;
    trigger_event_buf->code = code;
    trigger_event_buf->eventid_len = eventid_len;
    trigger_event_buf->message_len = message_len;
    strncpy(trigger_event_buf->eventid_message, eventid, eventid_len);
    *(trigger_event_buf->eventid_message + eventid_len) = 0;
    strncpy(trigger_event_buf->eventid_message
            + MALIYUN_SEPARATOR + eventid_len, message, message_len);

    ret = maliyun_linkkit_gateway_write(dest_addrs, MALIYUN_LINKKIT_SUB_TRIGGER_EVENT_REPLY,
                                        trigger_event_buf, sizeof(maliyun_linkkit_trigger_event_reply_t) + eventid_len + MALIYUN_SEPARATOR + message_len);
    MDF_FREE(trigger_event_buf);
    return MDF_OK;
}

static int maliyun_timestamp_reply_event_handler(const char *timestamp)
{
    MDF_LOGI("maliyun_timestamp_reply_event_handler");

    uint8_t dest_addrs[MWIFI_ADDR_LEN] = MWIFI_ADDR_ANY;
    return maliyun_linkkit_gateway_write(dest_addrs, MALIYUN_LINKKIT_SUB_TIMESTAMP_REPLY, (char *)timestamp, strlen(timestamp));
}

static int maliyun_topolist_reply_event_handler(int device_id, int id, int code, const char *topo_list, int topo_list_len)
{
    MDF_LOGE("device_id:%d", device_id);
    MDF_LOGE("id:%d", id);
    MDF_LOGD("code:%d", code);
    MDF_LOGD("topo_list:%s", topo_list);
    MDF_LOGD("topo_list_len:%d", topo_list_len);
    return ESP_OK;
}

static int maliyun_permit_join_event_handler(const char *product_key, int timeout)
{
    MDF_LOGE("Product Key: %s, Time: %d", product_key, timeout);
    return ESP_OK;
}

static int maliyun_initialized_event_handler(int device_id)
{
    MDF_LOGI("maliyun_initialized id:%d", device_id);

    if (g_gateway.device_id == device_id) {
        g_gateway.initialized = true;
    }

    return ESP_OK;
}


static int maliyun_fota_event_handler(int type, const char *version)
{
    MDF_LOGD("maliyun_fota_event_handler");
    uint8_t *buffer = MDF_MALLOC(MALIYUN_FOTA_BUF_LEN);
    int     buffer_len = MALIYUN_FOTA_BUF_LEN;

    if (type == 0) {
        MDF_LOGI("New Firmware Version: %s", version);
        IOT_Linkkit_Query(g_gateway.device_id, ITM_MSG_QUERY_FOTA_DATA, buffer, buffer_len);
    }

    MDF_FREE(buffer);
    return ESP_OK;
}

int maliyun_service_request_event_handler(int test1, const char *test2, int test3, const char *test4, int test15,
        char **test6, int *test7)
{
    MDF_LOGE("maliyun_service_request_event_handler");
    return ESP_OK;
}

int maliyun_property_get_event_handler(int test1, const char *test2, int test3, char **test4, int *test5)
{
    MDF_LOGE("maliyun_property_get_event_handler");
    return ESP_OK;
}

static void maliyun_linkkit_gateway_subscribe_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    MDF_LOGI("aliyun_linkkit_gateway_subscribe_task is running");

    while (!mwifi_is_connected()) {
        vTaskDelay(MALIYUN_YIELD_MS / portTICK_RATE_MS);
    }

    mwifi_post_root_status(false);

    // Maliyun Info Init
    maliyun_linkkit_list_create();

    // Choose Login Server
    int domain_type = IOTX_CLOUD_REGION_SHANGHAI;
    IOT_Ioctl(IOTX_IOCTL_SET_DOMAIN, (void *)&domain_type);

    /* Register Callback */
    IOT_RegisterCallback(ITE_CONNECT_SUCC, maliyun_connect_succ_event_handler);
    IOT_RegisterCallback(ITE_CONNECT_FAIL, maliyun_connect_fail_event_handler);
    IOT_RegisterCallback(ITE_DISCONNECTED, maliyun_disconnect_event_handler);
    IOT_RegisterCallback(ITE_RAWDATA_ARRIVED, maliyun_down_raw_data_arrived_event_handler);
    IOT_RegisterCallback(ITE_SERVICE_REQUEST, maliyun_service_request_event_handler);
    IOT_RegisterCallback(ITE_PROPERTY_SET, maliyun_property_set_event_handler);
    IOT_RegisterCallback(ITE_PROPERTY_GET, maliyun_property_get_event_handler);
    IOT_RegisterCallback(ITE_REPORT_REPLY, maliyun_report_reply_event_handler);
    IOT_RegisterCallback(ITE_TRIGGER_EVENT_REPLY, maliyun_trigger_event_reply_event_handler);
    IOT_RegisterCallback(ITE_TIMESTAMP_REPLY, maliyun_timestamp_reply_event_handler);
    IOT_RegisterCallback(ITE_TOPOLIST_REPLY, maliyun_topolist_reply_event_handler);
    IOT_RegisterCallback(ITE_PERMIT_JOIN, maliyun_permit_join_event_handler);
    IOT_RegisterCallback(ITE_INITIALIZE_COMPLETED, maliyun_initialized_event_handler);
    IOT_RegisterCallback(ITE_FOTA, maliyun_fota_event_handler);

    iotx_linkkit_dev_meta_info_t  gateway_meta_info = {0};
    HAL_GetProductKey(gateway_meta_info.product_key);
    HAL_GetProductSecret(gateway_meta_info.product_secret);
    HAL_GetDeviceName(gateway_meta_info.device_name);
    HAL_GetDeviceSecret(gateway_meta_info.device_secret);

    g_gateway.device_id = IOT_Linkkit_Open(IOTX_LINKKIT_DEV_TYPE_MASTER, &gateway_meta_info);

    ret = IOT_Linkkit_Connect(g_gateway.device_id);
    MDF_LOGI("IOT_Linkkit_Connect return status :%d", ret);

    uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
    esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);

    ret = maliyun_linkkit_insert(g_gateway.device_id, sta_mac, gateway_meta_info.product_key);
    MDF_LOGI("maliyun_linkkit_insert device id :%d, ret :%d, mac:" MACSTR, g_gateway.device_id, ret, MAC2STR(sta_mac));

    while (g_gateway.running) {
        IOT_Linkkit_Yield(MALIYUN_YIELD_MS);
    }

    MDF_LOGW("aliyun_linkkit_gateway_yield_task is exit");

    IOT_Linkkit_Close(g_gateway.device_id);

    IOT_SetLogLevel(IOT_LOG_NONE);

    maliyun_linkkit_list_destroy();

    g_gateway.connected = false;
    g_gateway.initialized = false;
    g_gateway.device_id = MALIYUN_DEFAULT_DEVICE_ID;
    g_maliyun_linkkit_gateway_subscribe_task_handle = NULL;

    vTaskDelete(NULL);
}

mdf_err_t maliyun_linkkit_gateway_init(void)
{
    g_gateway.running = true;

    if (g_maliyun_linkkit_gateway_subscribe_task_handle == NULL) {
        MDF_LOGD("xTaskCreate aliyun_linkkit_gateway_subscribe_task !!");

        if (pdTRUE != xTaskCreate(maliyun_linkkit_gateway_subscribe_task, "gateway_sub_task", 8 * 1024, NULL,
                                  CONFIG_MDF_TASK_DEFAULT_PRIOTY, &g_maliyun_linkkit_gateway_subscribe_task_handle)) {
            return MDF_FAIL;
        }
    }

    if (g_maliyun_linkkit_gateway_publish_task_handle == NULL) {
        MDF_LOGD("xTaskCreate maliyun_linkkit_gateway_publish_task !!");

        if (pdTRUE != xTaskCreate(maliyun_linkkit_gateway_publish_task, "gateway_pub_task", 4 * 1024, NULL,
                                  CONFIG_MDF_TASK_DEFAULT_PRIOTY, &g_maliyun_linkkit_gateway_publish_task_handle)) {
            return MDF_FAIL;
        }
    }

    return MDF_OK;
}

static mdf_err_t _maliyun_linkkit_gateway_deinit_cmd()
{
    const char *gateway_deinit = "{\"cmd\":\"gateway_deinit\"}";
    mwifi_data_type_t data_type     = {.communicate = MWIFI_COMMUNICATE_MULTICAST};
    return  mwifi_write(NULL, &data_type, gateway_deinit, strlen(gateway_deinit), true);
}

mdf_err_t maliyun_linkkit_gateway_deinit(void)
{
    if (g_gateway.running) {
        g_gateway.running = false;
        return _maliyun_linkkit_gateway_deinit_cmd();
    }

    return MDF_OK;
}

mdf_err_t maliyun_linkkit_gateway_refresh_subdevice(void)
{
    int  sub_device_id = MALIYUN_DEFAULT_DEVICE_ID;
    size_t table_size    = esp_mesh_get_routing_table_size();
    uint8_t *routing_table = MDF_MALLOC(table_size * sizeof(mesh_addr_t));
    ESP_ERROR_CHECK(esp_mesh_get_routing_table((mesh_addr_t *)routing_table,
                    table_size * sizeof(mesh_addr_t), (int *)&table_size));

    while (MDF_OK == maliyun_linkkit_list_get_offline_id(routing_table, table_size, &sub_device_id)) {
        uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
        maliyun_linkkit_list_get_mac(sub_device_id, sta_mac);
        MDF_LOGI("Close device id :%d, mac:" MACSTR, sub_device_id, MAC2STR(sta_mac));

        IOT_Linkkit_Report(sub_device_id, ITM_MSG_LOGOUT, NULL, 0);
        IOT_Linkkit_Close(sub_device_id);
        maliyun_linkkit_list_delete_id(sub_device_id);
    }

    MDF_FREE(routing_table);
    return MDF_OK;
}
