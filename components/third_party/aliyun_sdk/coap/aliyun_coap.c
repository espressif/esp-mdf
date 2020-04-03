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


#include "coap.h"

#include "aliyun_url.h"
#include "aliyun_pack.h"
#include "aliyun_coap.h"
#include "aliyun_platform.h"

#define AWSS_NOTIFY_HOST     "255.255.255.255"
#define COAP_DEFAULT_TIME_SEC 5
#define COAP_DEFAULT_TIME_USEC 0

static const char *TAG  = "aliyun_coap";

static TaskHandle_t g_aliyun_coap_task_handle = NULL;

mdf_err_t aliyun_coap_send_app(char *coap_url, uint8_t *data, size_t size)
{
    mdf_err_t ret;
    coap_context_t   *ctx = NULL;
    coap_address_t    dst_addr, src_addr;
    coap_pdu_t       *request = NULL;
    coap_uri_t uri;

    ret = coap_split_uri((const uint8_t *)coap_url, strlen(coap_url), &uri);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "coap_split_uri error");

    coap_address_init(&src_addr);
    src_addr.addr.sin.sin_family      = AF_INET;
    src_addr.addr.sin.sin_port        = htons(0);
    src_addr.addr.sin.sin_addr.s_addr = INADDR_ANY;
    ctx = coap_new_context(&src_addr);

    MDF_ERROR_CHECK(ctx == NULL, MDF_FAIL, "coap_new_context is NULL");
    coap_address_init(&dst_addr);
    dst_addr.addr.sin.sin_family      = AF_INET;
    dst_addr.addr.sin.sin_port        = htons(COAP_DEFAULT_PORT);
    dst_addr.addr.sin.sin_addr.s_addr = inet_addr(AWSS_NOTIFY_HOST);

    request            = coap_new_pdu();

    request->hdr->type = COAP_MESSAGE_NON;
    request->hdr->id   = coap_new_message_id(ctx);
    request->hdr->code = COAP_REQUEST_POST;
    coap_add_option(request, COAP_OPTION_URI_PATH, uri.path.length, uri.path.s);

    unsigned char buf[3];
    coap_add_option(request, COAP_OPTION_CONTENT_TYPE, coap_encode_var_bytes(buf, COAP_MEDIATYPE_APPLICATION_JSON), buf);
    coap_add_data(request, size, (unsigned char *)data);

    coap_send_confirmed(ctx, ctx->endpoint, &dst_addr, request);

    coap_free_context(ctx);
    return MDF_OK;
}

static coap_async_state_t *deviceinfo_async = NULL;
static coap_async_state_t *connectap_async = NULL;

static void deviceinfo_send_async_response(coap_context_t *ctx, const coap_endpoint_t *local_if)
{
    coap_pdu_t *response;
    unsigned char buf[3];
    uint32_t msg_id = aliyun_platform_get_msg_id();
    char *response_data  = aliyun_pack_coap_device_info_get(msg_id);

    response = coap_pdu_init(deviceinfo_async->flags & COAP_MESSAGE_CON, COAP_RESPONSE_CODE(205), 0, COAP_MAX_PDU_SIZE);
    response->hdr->id = coap_new_message_id(ctx);

    if (deviceinfo_async->tokenlen) {
        coap_add_token(response, deviceinfo_async->tokenlen, deviceinfo_async->token);
    }

    coap_add_option(response, COAP_OPTION_CONTENT_TYPE, coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);
    coap_add_data(response, strlen(response_data), (unsigned char *)response_data);

    if (coap_send(ctx, local_if, &deviceinfo_async->peer, response) == COAP_INVALID_TID) {

    }

    coap_delete_pdu(response);
    coap_async_state_t *tmp;
    coap_remove_async(ctx, deviceinfo_async->id, &tmp);
    coap_free_async(deviceinfo_async);
    deviceinfo_async = NULL;
    free(response_data);
}


static void connectap_send_async_response(coap_context_t *ctx, const coap_endpoint_t *local_if)
{
    coap_pdu_t *response;
    unsigned char buf[3];
    uint32_t msg_id = aliyun_platform_get_msg_id();
    char *response_data  = aliyun_pack_coap_connectapp_info_get(msg_id);

    response = coap_pdu_init(connectap_async->flags & COAP_MESSAGE_CON, COAP_RESPONSE_CODE(205), 0, COAP_MAX_PDU_SIZE);
    response->hdr->id = coap_new_message_id(ctx);

    if (connectap_async->tokenlen) {
        coap_add_token(response, connectap_async->tokenlen, connectap_async->token);
    }

    coap_add_option(response, COAP_OPTION_CONTENT_TYPE, coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);
    coap_add_data(response, strlen(response_data), (unsigned char *)response_data);

    if (coap_send(ctx, local_if, &connectap_async->peer, response) == COAP_INVALID_TID) {

    }

    coap_delete_pdu(response);
    coap_async_state_t *tmp;
    coap_remove_async(ctx, connectap_async->id, &tmp);
    coap_free_async(connectap_async);
    connectap_async = NULL;
    free(response_data);
}

/*
 * The resource handler
 */
static void deviceinfo_async_handler(coap_context_t *ctx, struct coap_resource_t *resource,
                                     const coap_endpoint_t *local_interface, coap_address_t *peer,
                                     coap_pdu_t *request, str *token, coap_pdu_t *response)
{
    deviceinfo_async = coap_register_async(ctx, peer, request, COAP_ASYNC_SEPARATE | COAP_ASYNC_CONFIRM, (void *)"no data");
}

static void connectap_async_handler(coap_context_t *ctx, struct coap_resource_t *resource,
                                    const coap_endpoint_t *local_interface, coap_address_t *peer,
                                    coap_pdu_t *request, str *token, coap_pdu_t *response)
{
    connectap_async = coap_register_async(ctx, peer, request, COAP_ASYNC_SEPARATE | COAP_ASYNC_CONFIRM, (void *)"no data");
}

static void aliyun_coap_task(void *p)
{
    coap_context_t   *ctx = NULL;
    coap_address_t    serv_addr;
    fd_set            readfds;
    struct timeval    tv;
    int flags;
    coap_resource_t *deviceinfo_resource = NULL;
    coap_resource_t *connectap_resource = NULL;

    while (g_aliyun_coap_task_handle != NULL) {

        coap_address_init(&serv_addr);
        serv_addr.addr.sin.sin_family      = AF_INET;
        serv_addr.addr.sin.sin_addr.s_addr = INADDR_ANY;
        serv_addr.addr.sin.sin_port        = htons(COAP_DEFAULT_PORT);
        ctx                                = coap_new_context(&serv_addr);

        if (ctx != NULL) {
            flags = fcntl(ctx->sockfd, F_GETFL, 0);
            fcntl(ctx->sockfd, F_SETFL, flags | O_NONBLOCK);

            tv.tv_usec = COAP_DEFAULT_TIME_USEC;
            tv.tv_sec = COAP_DEFAULT_TIME_SEC;
            /* Initialize the resource */
            deviceinfo_resource = coap_resource_init((unsigned char *)ALIYUN_URL_SYS_DEVICE_INFO_GET, strlen(ALIYUN_URL_SYS_DEVICE_INFO_GET), 0);
            connectap_resource = coap_resource_init((unsigned char *)ALIYUN_URL_SYS_AWSS_DEVICE_CONNECTAP_INFO_GET, strlen(ALIYUN_URL_SYS_AWSS_DEVICE_CONNECTAP_INFO_GET), 0);

            if (deviceinfo_resource) {
                coap_register_handler(deviceinfo_resource, COAP_REQUEST_GET, deviceinfo_async_handler);
                coap_register_handler(connectap_resource, COAP_REQUEST_GET, connectap_async_handler);
                coap_add_resource(ctx, deviceinfo_resource);
                coap_add_resource(ctx, connectap_resource);

                for (;;) {
                    FD_ZERO(&readfds);
                    FD_CLR(ctx->sockfd, &readfds);
                    FD_SET(ctx->sockfd, &readfds);

                    int result = select(ctx->sockfd + 1, &readfds, 0, 0, &tv);

                    if (result > 0) {
                        if (FD_ISSET(ctx->sockfd, &readfds)) {
                            coap_read(ctx);
                        }
                    } else if (result < 0) {
                        break;
                    } else {
                        ESP_LOGD(TAG, "select timeout");
                    }

                    if (deviceinfo_async) {
                        deviceinfo_send_async_response(ctx, ctx->endpoint);
                    }

                    if (connectap_async) {
                        connectap_send_async_response(ctx, ctx->endpoint);
                    }
                }
            }

            coap_free_context(ctx);
        }
    }

    vTaskDelete(NULL);
}

mdf_err_t aliyun_coap_service_init()
{
    if (g_aliyun_coap_task_handle == NULL) {
        xTaskCreate(aliyun_coap_task, "aliyun_coap_task", 4096, NULL, CONFIG_ALIYUN_TASK_DEFAULT_PRIOTY, &g_aliyun_coap_task_handle);
    }

    return MDF_OK;
}

void aliyun_coap_service_deinit(void)
{
    if (g_aliyun_coap_task_handle != NULL) {
        g_aliyun_coap_task_handle = NULL;
    }
}
