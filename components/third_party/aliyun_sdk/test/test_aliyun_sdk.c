#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aliyun_gateway.h"
#include "aliyun_kv.h"
#include "aliyun_subdevice.h"

#include "cJSON.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "mwifi.h"
#include "sys/time.h"
#include "time.h"
#include "unity.h"

#define TOTAL_NUMBER_TEST_CASE 10

#define POWER_SWITCH_1 (0)
#define POWER_SWITCH_2 (2)
#define POWER_SWITCH_3 (4)

static const char *TAG = "test_aliyun";
static const char company[] = "ESPRESSIF";
static const char identifier_freeheap[] = "FreeHeap";
static const char identifier_periodic[] = "Periodic";
static const char identifier_testcase[] = "TestCase";

static int g_flag = 0;
static SemaphoreHandle_t g_reply_smr;
static struct ReplyMsg {
    int code;
    char data[2048];
} g_reply_msg;

mdf_err_t environment_init()
{
    g_reply_smr = xSemaphoreCreateBinary();
    assert(g_reply_smr != NULL);

    mdf_err_t err = aliyun_gateway_init();

    if (err != MDF_OK) {
        return err;
    }

    err = aliyun_subdevice_init();

    if (err != MDF_OK) {
        return err;
    }

    gpio_config_t gpio_cfg = { 0 };
    gpio_cfg.intr_type = GPIO_INTR_DISABLE;
    gpio_cfg.mode = GPIO_MODE_INPUT_OUTPUT;
    gpio_cfg.pin_bit_mask = (((1ULL) << POWER_SWITCH_1) | ((1ULL) << POWER_SWITCH_2) | ((1ULL) << POWER_SWITCH_3));
    gpio_cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    return gpio_config(&gpio_cfg);
}

void setUp(void)
{
    if (g_flag == 0) {
        ESP_ERROR_CHECK(environment_init());
        printf("Prepare for aliyun test\n");
    }

    g_flag++;
}

void tearDown(void)
{
    if (g_flag == TOTAL_NUMBER_TEST_CASE) {
        ESP_ERROR_CHECK(aliyun_gateway_deinit());
        ESP_ERROR_CHECK(aliyun_subdevice_deinit());
        printf("Finish for aliyun test\n");
    }
}

static mdf_err_t device_post_testcase_event_cb(uint32_t msg_id, int code, const char *data)
{
    if (code == 200) {
        MDF_LOGI("Device post event successful");
    } else {
        MDF_LOGW("Device post event failed, ERROR_CODE=%d", code);
    }

    return MDF_OK;
}

static void device_post_testcase_event(uint32_t testcase_id)
{
    cJSON *value = cJSON_CreateObject();
    TEST_ASSERT(value != NULL);
    TEST_ASSERT(cJSON_AddNumberToObject(value, "TestCaseId", testcase_id) != NULL);
    char *value_str = cJSON_PrintUnformatted(value);
    TEST_ASSERT(value_str != NULL);
    mdf_err_t ret = aliyun_subdevice_post_event(identifier_testcase, value_str);
    TEST_ASSERT(ret == MDF_OK);
    free(value_str);
    cJSON_Delete(value);
}

TEST_CASE("login", "[aliyun_sdk") /* 测试登录状态获取 */
{
    aliyun_device_meta_t device_meta = { 0 };
    TEST_ASSERT_EQUAL(MDF_OK, aliyun_get_meta(&device_meta));

    int max_retry = 10;

    while (max_retry > 0 && aliyun_subdevice_add_to_gateway(&device_meta, 1000) != MDF_OK) {
        vTaskDelay(pdMS_TO_TICKS(1000 * 1));
        max_retry--;
    }

    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, max_retry, "Add subdevice failed");

    max_retry = 10;

    while (max_retry > 0 && aliyun_subdevice_get_login_status() != MDF_OK) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        max_retry--;
    }

    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, max_retry, "Login in failed");
}

static mdf_err_t device_request_ntp_cb(int64_t utc_ms)
{
    MDF_LOGI("Request ntp success, utc=%lld", utc_ms);
    memset(g_reply_msg.data, 0, sizeof(g_reply_msg.data));
    memcpy(g_reply_msg.data, &utc_ms, sizeof(int64_t));
    assert(xSemaphoreGive(g_reply_smr) == pdTRUE);
    return MDF_OK;
}
TEST_CASE("Request ntp", "[aliyun_sdk]") /* 测试 NTP 校时 */
{
    TEST_ASSERT(aliyun_subdevice_set_callback(ALIYUN_MQTT_REQUEST_NTP_RESPONSE, device_request_ntp_cb) == MDF_OK);

    TEST_ASSERT_EQUAL_MESSAGE(MDF_OK, aliyun_subdevice_request_ntp(), "Device request ntp failed");
    TEST_ASSERT_MESSAGE(xSemaphoreTake(g_reply_smr, pdMS_TO_TICKS(5 * 1000)) == pdTRUE,
                        "Wait cloud reply request ntp timeout");
    int64_t utc_ms = *(int64_t *)(g_reply_msg.data);
    time_t utc_s = time(NULL);
    TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(2, abs(utc_s - (utc_ms / 1000)), "Celebrate time failed");
}

static mdf_err_t device_delete_info_cb(uint32_t msg_id, int code, const char *data)
{
    if (code == 200) {
        MDF_LOGI("Device delete information successful");
    } else {
        MDF_LOGW("Device delete information(tag) failed, ERROR_CODE=%d", code);
    }

    memset(g_reply_msg.data, 0, sizeof(g_reply_msg.data));
    g_reply_msg.code = code;
    strncpy(g_reply_msg.data, data, sizeof(g_reply_msg.data));
    assert(xSemaphoreGive(g_reply_smr) == pdTRUE);

    return MDF_OK;
}
TEST_CASE("delete info(tag)", "[aliyun_sdk]") /* 测试删除设备标签 */
{
    TEST_ASSERT(aliyun_subdevice_set_callback(ALIYUN_MQTT_DELETE_DEVICEINFO_REPLY, device_delete_info_cb) == MDF_OK);

    cJSON *array = cJSON_CreateArray();
    TEST_ASSERT(array != NULL);
    cJSON *item = cJSON_CreateObject();
    TEST_ASSERT(item != NULL);
    cJSON_AddItemToArray(array, item);
    TEST_ASSERT(cJSON_AddStringToObject(item, "attrKey", "Company") != NULL);
    char *array_str = cJSON_PrintUnformatted(array);
    TEST_ASSERT(array_str != NULL);
    TEST_ASSERT_EQUAL_MESSAGE(MDF_OK, aliyun_subdevice_delete_deviceinfo(array_str),
                              "Device delete device info(tag) failed");
    free(array_str);
    cJSON_Delete(array);

    TEST_ASSERT_MESSAGE(xSemaphoreTake(g_reply_smr, pdMS_TO_TICKS(5 * 1000)) == pdTRUE,
                        "Wait cloud reply delete deviceinfo(tag) timeout");
    TEST_ASSERT_EQUAL_MESSAGE(200, g_reply_msg.code, "Delete device info(tag) failed");
}

static mdf_err_t device_update_info_cb(uint32_t msg_id, int code, const char *data)
{
    if (code == 200) {
        MDF_LOGI("Device update information(tag) successful");
    } else {
        MDF_LOGW("Device update information(tag) failed, ERROR_CODE=%d", code);
    }

    memset(g_reply_msg.data, 0, sizeof(g_reply_msg.data));
    g_reply_msg.code = code;
    strncpy(g_reply_msg.data, data, sizeof(g_reply_msg.data));
    assert(xSemaphoreGive(g_reply_smr) == pdTRUE);

    return MDF_OK;
}
TEST_CASE("update info(tag)", "[aliyun_sdk]") /* 测试更新设备标签（更新一个同样的标签和值将失败） */
{
    TEST_ASSERT(aliyun_subdevice_set_callback(ALIYUN_MQTT_UPDATE_DEVICEINFO_REPLY, device_update_info_cb) == MDF_OK);

    cJSON *array = cJSON_CreateArray();
    TEST_ASSERT(array != NULL);
    cJSON *item = cJSON_CreateObject();
    TEST_ASSERT(item != NULL);
    cJSON_AddItemToArray(array, item);
    TEST_ASSERT(cJSON_AddStringToObject(item, "attrKey", "Company") != NULL);
    TEST_ASSERT(cJSON_AddStringToObject(item, "attrValue", company) != NULL);
    char *array_str = cJSON_PrintUnformatted(array);
    TEST_ASSERT(array_str != NULL);
    TEST_ASSERT_EQUAL_MESSAGE(MDF_OK, aliyun_subdevice_update_deviceinfo(array_str),
                              "Device update device info(tag) failed");
    free(array_str);
    cJSON_Delete(array);

    TEST_ASSERT_MESSAGE(xSemaphoreTake(g_reply_smr, pdMS_TO_TICKS(5 * 1000)) == pdTRUE,
                        "Wait cloud reply update deviceinfo(tag) timeout");
    TEST_ASSERT_EQUAL_MESSAGE(200, g_reply_msg.code, "Update device info(tag) failed");
}

static mdf_err_t device_get_config_cb(uint32_t msg_id, int code, const char *data)
{
    if (code == 200) {
        MDF_LOGI("Device update information(tag) successful");
        printf("%s\n", data);
    } else {
        MDF_LOGW("Device update information(tag) failed, ERROR_CODE=%d", code);
    }

    memset(g_reply_msg.data, 0, sizeof(g_reply_msg.data));
    g_reply_msg.code = code;
    strncpy(g_reply_msg.data, data, sizeof(g_reply_msg.data));
    assert(xSemaphoreGive(g_reply_smr) == pdTRUE);

    return MDF_OK;
}
TEST_CASE("Get configuration", "[aliyun_sdk]") /* 测试获取远程配置信息（配置文件 URL） */
{
    TEST_ASSERT(aliyun_subdevice_set_callback(ALIYUN_MQTT_GET_CONFIG_REPLY, device_get_config_cb) == MDF_OK);

    TEST_ASSERT_EQUAL_MESSAGE(MDF_OK, aliyun_subdevice_get_config(),
                              "Device get configuration failed");
    TEST_ASSERT_MESSAGE(xSemaphoreTake(g_reply_smr, pdMS_TO_TICKS(5 * 1000)) == pdTRUE,
                        "Get configuration timeout");
    TEST_ASSERT_EQUAL_MESSAGE(200, g_reply_msg.code, "Get configuration failed");
}

static mdf_err_t device_post_property_cb(uint32_t msg_id, int code, const char *data)
{
    if (code == 200) {
        MDF_LOGI("Device post property successful");
    } else {
        MDF_LOGW("Device post property failed, ERROR_CODE=%d", code);
    }

    memset(g_reply_msg.data, 0, sizeof(g_reply_msg.data));
    g_reply_msg.code = code;
    strncpy(g_reply_msg.data, data, sizeof(g_reply_msg.data));
    assert(xSemaphoreGive(g_reply_smr) == pdTRUE);
    return MDF_OK;
}
TEST_CASE("post property", "[aliyun_sdk]") /* 测试物模型之发布属性值 */
{
    ESP_ERROR_CHECK(aliyun_subdevice_set_callback(ALIYUN_MQTT_POST_PROPERTY_REPLY, device_post_property_cb));

    cJSON *properties = cJSON_CreateObject();
    TEST_ASSERT(properties != NULL);
    int level = gpio_get_level((gpio_num_t)POWER_SWITCH_1);
    TEST_ASSERT(cJSON_AddNumberToObject(properties, "PowerSwitch_1", level) != NULL);
    level = gpio_get_level((gpio_num_t)POWER_SWITCH_2);
    TEST_ASSERT(cJSON_AddNumberToObject(properties, "PowerSwitch_2", level) != NULL);
    level = gpio_get_level((gpio_num_t)POWER_SWITCH_3);
    TEST_ASSERT(cJSON_AddNumberToObject(properties, "PowerSwitch_3", level) != NULL);
    char *json_string = cJSON_PrintUnformatted(properties);
    TEST_ASSERT(json_string != NULL);
    TEST_ASSERT_EQUAL_MESSAGE(MDF_OK, aliyun_subdevice_post_property(json_string), "Post properties to cloud failed");
    free(json_string);
    cJSON_Delete(properties);

    TEST_ASSERT_MESSAGE(xSemaphoreTake(g_reply_smr, pdMS_TO_TICKS(5 * 1000)) == pdTRUE,
                        "Wait cloud reply post properties timeout");
    TEST_ASSERT_EQUAL_MESSAGE(200, g_reply_msg.code, "Cloud reply post properties is not 200(OK)");
}

static mdf_err_t device_post_event_cb(uint32_t msg_id, int code, const char *data)
{
    if (code == 200) {
        MDF_LOGI("Device post event successful");
    } else {
        MDF_LOGW("Device post event failed, ERROR_CODE=%d", code);
    }

    memset(g_reply_msg.data, 0, sizeof(g_reply_msg.data));
    g_reply_msg.code = code;
    strncpy(g_reply_msg.data, data, sizeof(g_reply_msg.data));
    assert(xSemaphoreGive(g_reply_smr) == pdTRUE);

    return MDF_OK;
}
TEST_CASE("post event", "[aliyun_sdk]") /* 测试物模型之发布事件 */
{
    TEST_ASSERT(aliyun_subdevice_set_callback(ALIYUN_MQTT_POST_EVENT_REPLY, device_post_event_cb) == MDF_OK);

    cJSON *value = cJSON_CreateObject();
    TEST_ASSERT(value != NULL);
    TEST_ASSERT(cJSON_AddNumberToObject(value, "count", 1) != NULL);
    char *value_str = cJSON_PrintUnformatted(value);
    TEST_ASSERT(value_str != NULL);
    mdf_err_t ret = aliyun_subdevice_post_event(identifier_periodic, value_str);
    TEST_ASSERT_EQUAL_MESSAGE(MDF_OK, ret, "Post event to cloud failed");
    free(value_str);
    cJSON_Delete(value);

    TEST_ASSERT_MESSAGE(xSemaphoreTake(g_reply_smr, pdMS_TO_TICKS(5 * 1000)) == pdTRUE,
                        "Wait cloud reply post event timeout");
    TEST_ASSERT_EQUAL_MESSAGE(200, g_reply_msg.code, "Cloud reply post event is not 200(OK)");
}

static mdf_err_t device_property_set_cb(uint32_t msg_id, const char *params, size_t request_len)
{
    MDF_LOGI("aliyun_propery_set_cb: %.*s(%zu)", (int)request_len, params, request_len);

    cJSON *property = cJSON_Parse(params);
    MDF_ERROR_CHECK(property == NULL, MDF_FAIL, "json parse failed");
    cJSON *item = cJSON_GetObjectItem(property, "PowerSwitch_1");

    if (item != NULL) {
        gpio_set_level((gpio_num_t)POWER_SWITCH_1, (item->valueint == 0) ? 0 : 1);
    }

    item = cJSON_GetObjectItem(property, "PowerSwitch_2");

    if (item != NULL) {
        gpio_set_level((gpio_num_t)POWER_SWITCH_2, (item->valueint == 0) ? 0 : 1);
    }

    item = cJSON_GetObjectItem(property, "PowerSwitch_3");

    if (item != NULL) {
        gpio_set_level((gpio_num_t)POWER_SWITCH_3, (item->valueint == 0) ? 0 : 1);
    }

    aliyun_subdevice_reply_property_set(msg_id, 200, NULL);
    cJSON_Delete(property);
    assert(xSemaphoreGive(g_reply_smr) == pdTRUE);
    return MDF_OK;
}
TEST_CASE("properties set", "[aliyun_sdk]") /* 测试物模型之云端设置设备属性 */
{
    TEST_ASSERT(aliyun_subdevice_set_callback(ALIYUN_MQTT_PROPERTY_SET, device_property_set_cb) == MDF_OK);
    TEST_ASSERT(aliyun_subdevice_set_callback(ALIYUN_MQTT_POST_EVENT_REPLY, device_post_testcase_event_cb) == MDF_OK);

    device_post_testcase_event(0);
    TEST_ASSERT_MESSAGE(xSemaphoreTake(g_reply_smr, pdMS_TO_TICKS(5 * 1000)) == pdTRUE,
                        "Wait server send properties set message timeout");
}

static mdf_err_t device_service_invoke_cb(uint32_t msg_id, const char *identifier, const char *params, size_t params_len)
{
    MDF_LOGI("aliyun service invoke cb, identifier=%s, param=%.*s(%zu)", identifier, (int)params_len, params, params_len);
    cJSON *service = cJSON_Parse(params);
    assert(service != NULL);
    cJSON *reply = NULL;

    if (strcmp(identifier, identifier_freeheap) == 0) {
        reply = cJSON_CreateObject();
        assert(reply != NULL);
        cJSON *obj = cJSON_AddNumberToObject(reply, "FreeHeap", esp_get_free_heap_size());
        assert(obj != NULL);
    }

    if (reply != NULL) {
        char *data = cJSON_PrintUnformatted(reply);
        assert(data != NULL);
        aliyun_subdevice_reply_service_invoke(msg_id, 200, identifier, data);
        free(data);
    }

    cJSON_Delete(service);
    cJSON_Delete(reply);
    assert(xSemaphoreGive(g_reply_smr));
    return MDF_OK;
}
TEST_CASE("Service invoke", "[aliyun_sdk]") /* 测试物模型之云端调用设备服务 */
{
    TEST_ASSERT(aliyun_subdevice_set_callback(ALIYUN_MQTT_SERVICE_INVOKE, device_service_invoke_cb) == MDF_OK);
    TEST_ASSERT(aliyun_subdevice_set_callback(ALIYUN_MQTT_POST_EVENT_REPLY, device_post_testcase_event_cb) == MDF_OK);
    device_post_testcase_event(1);
    TEST_ASSERT_MESSAGE(xSemaphoreTake(g_reply_smr, pdMS_TO_TICKS(5 * 1000)) == pdTRUE,
                        "Wait server send service invoke message timeout");
}

static mdf_err_t device_get_dsltemplate_cb(uint32_t msg_id, int code, const char *data)
{
    if (code == 200) {
        MDF_LOGI("Device get dsltemplate successful");
    } else {
        MDF_LOGW("Device get dsltemplate failed, ERROR_CODE=%d", code);
    }

    memset(g_reply_msg.data, 0, sizeof(g_reply_msg.data));
    g_reply_msg.code = code;
    strncpy(g_reply_msg.data, data, sizeof(g_reply_msg.data));
    assert(xSemaphoreGive(g_reply_smr) == pdTRUE);

    return MDF_OK;
}
TEST_CASE("Get TSL", "[aliyun_sdk]") /* 测试物模型之获取 TSL 模板 */
{
    TEST_ASSERT(aliyun_subdevice_set_callback(ALIYUN_MQTT_SUBSCRIBE_GET_DSLTEMPLATE_REPLY, device_get_dsltemplate_cb) == MDF_OK);

    TEST_ASSERT_EQUAL_MESSAGE(MDF_OK, aliyun_subdevice_get_dslttemplate(),
                              "Device get TSL template failed");
    TEST_ASSERT_MESSAGE(xSemaphoreTake(g_reply_smr, pdMS_TO_TICKS(5 * 1000)) == pdTRUE,
                        "Get TSL template timeout");
    TEST_ASSERT_EQUAL_MESSAGE(200, g_reply_msg.code, "Cloud reply TSL template is not 200(OK)");
}
