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

#include "esp_partition.h"
#include "esp_ota_ops.h"

#include "light_driver.h"

#include "mdf_device_handle.h"

static const char *TAG = "factory";

#define WIFI_CONN_TIMEOUT         (20 * 1000)
#define FACTORY_UPGRADE_TIMEOUT   (120 * 1000)
#define FACTORY_UPGRADE_PKT_MAX   (1460)

typedef struct {
    char ssid[32];
    char password[64];
    char server_ip[16];
    uint16_t server_port;
    char file_name[32];
    int timeout_ms;
    uint32_t light_io_num[5];
} server_conn_t;

static SemaphoreHandle_t xSem_wifi_conn         = NULL;
static TimerHandle_t g_upgrade_timer            = NULL;
static const esp_partition_t *upgrade_partition = NULL;
static esp_ota_handle_t upgrade_handle          = 0;
static int upgrade_start_time                   = 0;

const static server_conn_t factory_default_conn = {
    .ssid     = "esp32-mdf",
    .password = "espressif",
    .server_ip       = "192.168.1.254",
    .server_port     = 2323,
    .file_name       = "/mdf_demo.bin",
    .light_io_num    = {25, 26, 27, 19, 23}
};

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            MDF_LOGI("SYSTEM_EVENT_STA_START");
            ESP_ERROR_CHECK(esp_wifi_connect());
            break;

        case SYSTEM_EVENT_STA_GOT_IP:
            MDF_LOGI("SYSTEM_EVENT_STA_GOT_IP");
            xSemaphoreGive(xSem_wifi_conn);
            break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
            MDF_LOGI("SYSTEM_EVENT_STA_DISCONNECTED, free_heap: %d", esp_get_free_heap_size());
            MDF_ERROR_CHECK(esp_wifi_connect(), ESP_FAIL, "esp_wifi_connect fail");
            break;

        default:
            break;
    }

    return ESP_OK;
}

static esp_err_t factory_connect(char ssid[32], char password[64])
{
    wifi_config_t wifi_config = { 0 };
    xSem_wifi_conn            = xSemaphoreCreateBinary();
    wifi_init_config_t cfg    = WIFI_INIT_CONFIG_DEFAULT();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    memcpy(wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    memcpy(wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    MDF_LOGI("setting WiFi configuration SSID: %s, PWD: %s",
             wifi_config.sta.ssid, wifi_config.sta.password);

    /**< timeout for wifi connect and got ip, default 20s */
    if (xSemaphoreTake(xSem_wifi_conn, WIFI_CONN_TIMEOUT / portTICK_RATE_MS) != pdTRUE) {
        MDF_LOGE("xSemaphoreTake fail");
        return ESP_FAIL;
    }

    return ESP_OK;
}

static void ota_timeout_cb(void *timer)
{
    xTimerStop(g_upgrade_timer, portMAX_DELAY);
    xTimerDelete(g_upgrade_timer, 0);
    g_upgrade_timer = NULL;

    light_blink_set(255, 0, 0, 4, 3000);/**< red blink */
    esp_restart();
}

void app_main()
{
    MDF_LOGI("******** ENTER MESH FACTORY PARTITION ********");

    esp_err_t ret               = ESP_OK;
    int sockfd                  = MDF_SOCKET_INVALID_FD;
    char *buffer                = mdf_calloc(1, FACTORY_UPGRADE_PKT_MAX);
    ssize_t buffer_size         = 0;
    char *http_body             = NULL;
    size_t http_body_size       = 0;
    ssize_t upgrade_writed_size = 0;
    int upgrade_bin_total_size  = 0;
    nvs_handle handle           = 0;
    size_t factory_conn_len     = sizeof(server_conn_t);
    server_conn_t factory_conn  = { 0 };


    /**< init mvs and get "factory_conn" */
    ret = nvs_flash_init();
    MDF_ERROR_GOTO(ret != ESP_OK, INIT_ERR_EXIT, "nvs_flash_init, ret: %d", ret);

    ret = nvs_open("FLAG_SPACE", NVS_READWRITE, &handle);
    MDF_ERROR_GOTO(ret != ESP_OK, INIT_ERR_EXIT, "nvs_open ret:%x", ret);

    ret = nvs_get_blob(handle, "factory_conn", &factory_conn, &factory_conn_len);
    nvs_close(handle);
    MDF_ERROR_GOTO(ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND,
                   INIT_ERR_EXIT, "nvs_get_blob, ret: %d", ret);

    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        MDF_LOGW("******** FACTORY_CONN NOT FOUND, USE DEFAULT ********");
        memcpy(&factory_conn, &factory_default_conn, factory_conn_len);
    }

    /**< init mdf light and set default green color */
    ret = light_init(factory_conn.light_io_num[0], factory_conn.light_io_num[1],
                     factory_conn.light_io_num[2], factory_conn.light_io_num[3],
                     factory_conn.light_io_num[4]);
    MDF_ERROR_GOTO(ret < 0, INIT_ERR_EXIT, "light_init, ret: %d", ret);

    light_yellow();/**< set light yellow */


    MDF_LOGI("*****************************************************");
    MDF_LOGI(" ssid: %s, password: %s", factory_conn.ssid, factory_conn.password);
    MDF_LOGI(" server_ip: %s, server_port: %d", factory_conn.server_ip, factory_conn.server_port);
    MDF_LOGI(" file_name: %s, timeout_ms: %d", factory_conn.file_name, factory_conn.timeout_ms);
    MDF_LOGI(" light_io_num: [%d][%d][%d][%d][%d]", factory_conn.light_io_num[0],
             factory_conn.light_io_num[1], factory_conn.light_io_num[2],
             factory_conn.light_io_num[3], factory_conn.light_io_num[4]);
    MDF_LOGI("*****************************************************");

    /**< connect to factory upgrade server */
    ret = factory_connect(factory_conn.ssid, factory_conn.password);
    MDF_ERROR_GOTO(ret < 0, WIFI_ERR_EXIT, "factory_connect, ret: %d", ret);

    sockfd = mdf_socket_tcp_client_create(factory_conn.server_ip, factory_conn.server_port);
    MDF_ERROR_GOTO(sockfd < 0, HTTP_ERR_EXIT, "mdf_socket_tcp_client_create, sockfd: %d", sockfd);

    ret = mdf_http_set_request_line(buffer, MDF_HTTP_GET, factory_conn.file_name);
    MDF_ERROR_GOTO(ret < 0, HTTP_ERR_EXIT, "mdf_http_set_request_line, ret: %d", ret);

    http_body_size = mdf_http_set_body(buffer, NULL, 0);
    MDF_ERROR_GOTO(http_body_size <= 0, HTTP_ERR_EXIT, "mdf_http_set_body, ret: %d", http_body_size);

    ret = send(sockfd, buffer, strlen(buffer), 0);
    MDF_ERROR_GOTO(ret < 0, HTTP_ERR_EXIT, "send GET request to server failed: %d", ret);

    /* Now read server response */
    buffer_size = recv(sockfd, buffer, FACTORY_UPGRADE_PKT_MAX, 0);
    MDF_ERROR_GOTO(buffer_size <= 0, HTTP_ERR_EXIT, "recv failed: %d", errno);

    /**< start ota progress */
    upgrade_partition = esp_ota_get_next_update_partition(NULL);
    ret = esp_ota_begin(upgrade_partition, OTA_SIZE_UNKNOWN, &upgrade_handle);
    MDF_ERROR_GOTO(ret != ESP_OK, OTA_ERR_EXIT, "esp_ota_begin failed, ret: %x", ret);

    g_upgrade_timer = xTimerCreate("factory_ota_timer", FACTORY_UPGRADE_TIMEOUT / portTICK_RATE_MS,
                                   false, NULL, ota_timeout_cb);
    xTimerStart(g_upgrade_timer, 0);
    light_blink_start(255, 255, 0, 4);/**< yellow blink */

    upgrade_start_time = xTaskGetTickCount();
    MDF_LOGI("esp_ota_begin succeeded");

    http_body = mdf_http_get_body(buffer, &http_body_size);

    if (http_body && http_body_size > 0) {
        ret = esp_ota_write(upgrade_handle, buffer, http_body_size);
        MDF_ERROR_GOTO(ret != ESP_OK, OTA_ERR_EXIT, "esp_ota_write failed, ret: %x", ret);

        upgrade_writed_size += http_body_size;
        MDF_LOGV("write size: %d, upgrade_writed_size: %d", http_body_size, upgrade_writed_size);
    }

    ret = mdf_http_get_header(buffer, "Content-Length", &upgrade_bin_total_size);
    MDF_ERROR_GOTO(ret < 0, OTA_ERR_EXIT, "mdf_http_get_header, ret: %d", ret);

    for (int write_size = 0; write_size < upgrade_bin_total_size; write_size += buffer_size) {
        buffer_size = recv(sockfd, buffer, FACTORY_UPGRADE_PKT_MAX, 0);
        MDF_ERROR_GOTO(buffer_size <= 0, OTA_ERR_EXIT, "Send GET request to server failed: %d", errno);

        ret = esp_ota_write(upgrade_handle, buffer, buffer_size);
        MDF_ERROR_GOTO(ret != ESP_OK, OTA_ERR_EXIT, "esp_ota_write failed, ret: %x", ret);

        upgrade_writed_size += buffer_size;
        MDF_LOGV("write size: %d, upgrade_writed_size: %d", buffer_size, upgrade_writed_size);

        MDF_LOGI("write size: %d, internal_ota progress: %d %%",
                 buffer_size, write_size * 100 / upgrade_bin_total_size);
    }

    MDF_LOGI("total write binary data length: %d, spend time: %ds", upgrade_writed_size,
             (xTaskGetTickCount() - upgrade_start_time) * portTICK_RATE_MS / 1000);

    ret = esp_ota_end(upgrade_handle);
    MDF_ERROR_GOTO(ret != ESP_OK, OTA_ERR_EXIT, "esp_ota_end failed, ret: %x", ret);

    ret = esp_ota_set_boot_partition(upgrade_partition);
    MDF_ERROR_GOTO(ret != ESP_OK, OTA_ERR_EXIT, "esp_ota_set_boot_partition failed! ret: %x", ret);

    light_blink_set(0, 255, 0, 4, 3000);    /**< green blink */

    MDF_LOGI("ota set boot partition, wait for reboot");
    goto EXIT;

INIT_ERR_EXIT:
    light_blink_set(0, 0, 255, 4, 3000);    /**< blue blink */
    goto EXIT;

WIFI_ERR_EXIT:
    light_blink_set(255, 0, 0, 4, 3000);    /**< red blink */
    goto EXIT;

HTTP_ERR_EXIT:
    light_blink_set(20, 0, 0, 4, 3000);     /**< light red blink */
    goto EXIT;

OTA_ERR_EXIT:
    light_blink_set(255, 128, 0, 3, 3000);  /**< orange blink */
    goto EXIT;

EXIT:
    mdf_free(buffer);
    esp_restart();
    return;
}
