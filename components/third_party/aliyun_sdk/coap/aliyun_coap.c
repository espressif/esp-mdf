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
#define COAP_DEFAULT_TIME 5
#define COAP_MAX_PDU_SIZE 1400

static const char *TAG  = "aliyun_coap";

static TaskHandle_t g_aliyun_coap_task_handle = NULL;

mdf_err_t aliyun_coap_send_app(const char *coap_url, const uint8_t *data, const size_t size)
{
    mdf_err_t ret;
    coap_context_t   *ctx = NULL;
    coap_session_t *session = NULL;
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

    session = coap_new_client_session(ctx, NULL, &src_addr, uri.scheme == COAP_URI_SCHEME_COAP_TCP ? COAP_PROTO_TCP : COAP_PROTO_UDP);

    if (!session) {
        ret = MDF_FAIL;
        goto clean_up;
    }

    request            = coap_new_pdu(session);

    request->type = COAP_MESSAGE_NON;
    request->tid   = coap_new_message_id(session);
    request->code = COAP_REQUEST_POST;
    coap_add_option(request, COAP_OPTION_URI_PATH, uri.path.length, uri.path.s);

    unsigned char buf[3];
    coap_add_option(request, COAP_OPTION_CONTENT_TYPE, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_JSON), buf);
    coap_add_data(request, size, (unsigned char *)data);

    coap_send(session, request);
    ret = MDF_OK;
clean_up:

    if (session) {
        coap_session_release(session);
    }

    if (ctx) {
        coap_free_context(ctx);
    }

    return ret;
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
    response->tid = coap_new_message_id(ctx->sessions);

    if (deviceinfo_async->tokenlen) {
        coap_add_token(response, deviceinfo_async->tokenlen, deviceinfo_async->token);
    }

    coap_add_option(response, COAP_OPTION_CONTENT_TYPE, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_TEXT_PLAIN), buf);
    coap_add_data(response, strlen(response_data), (unsigned char *)response_data);

    if (coap_send(ctx->sessions, response) == COAP_INVALID_TID) {

    }

    coap_delete_pdu(response);
    coap_async_state_t *tmp;
    coap_remove_async(ctx, ctx->sessions, deviceinfo_async->id, &tmp);
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
    response->tid = coap_new_message_id(ctx->sessions);

    if (connectap_async->tokenlen) {
        coap_add_token(response, connectap_async->tokenlen, connectap_async->token);
    }

    coap_add_option(response, COAP_OPTION_CONTENT_TYPE, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_TEXT_PLAIN), buf);
    coap_add_data(response, strlen(response_data), (unsigned char *)response_data);

    if (coap_send(ctx->sessions, response) == COAP_INVALID_TID) {

    }

    coap_delete_pdu(response);
    coap_async_state_t *tmp;
    coap_remove_async(ctx, ctx->sessions, connectap_async->id, &tmp);
    coap_free_async(connectap_async);
    connectap_async = NULL;
    free(response_data);
}

/*
 * The resource handler
 */
static void deviceinfo_async_handler(coap_context_t *ctx, struct coap_resource_t *resource, coap_session_t *session,
                                     coap_pdu_t *request, coap_binary_t *token, coap_string_t *query, coap_pdu_t *response)
{
    deviceinfo_async = coap_register_async(ctx, session, request, COAP_ASYNC_SEPARATE | COAP_ASYNC_CONFIRM, (void *)"no data");
}

static void connectap_async_handler(coap_context_t *ctx, struct coap_resource_t *resource, coap_session_t *session,
                                    coap_pdu_t *request, coap_binary_t *token, coap_string_t *query, coap_pdu_t *response)
{
    connectap_async = coap_register_async(ctx, session, request, COAP_ASYNC_SEPARATE | COAP_ASYNC_CONFIRM, (void *)"no data");
}

static void aliyun_coap_task(void *p)
{
    coap_context_t   *ctx = NULL;
    coap_address_t    serv_addr;

    int flags;
    coap_resource_t *deviceinfo_resource = NULL;
    coap_resource_t *connectap_resource = NULL;

    while (g_aliyun_coap_task_handle != NULL) {
        coap_endpoint_t *ep = NULL;

        coap_address_init(&serv_addr);
        serv_addr.addr.sin.sin_family      = AF_INET;
        serv_addr.addr.sin.sin_addr.s_addr = INADDR_ANY;
        serv_addr.addr.sin.sin_port        = htons(COAP_DEFAULT_PORT);

        ctx = coap_new_context(&serv_addr);

        if (!ctx) {
            MDF_LOGE("coap_new_context() failed");
            continue;
        }

        ep = coap_new_endpoint(ctx, &serv_addr, COAP_PROTO_UDP);

        if (!ep) {
            MDF_LOGE("udp: coap_new_endpoint() failed");
            goto clean_up;
        }

        /* Initialize the resource */
        deviceinfo_resource = coap_resource_init(coap_make_str_const(ALIYUN_URL_SYS_DEVICE_INFO_GET), 0);

        if (!deviceinfo_resource) {
            MDF_LOGE("coap_resource_init() failed");
            goto clean_up;
        }

        connectap_resource = coap_resource_init(coap_make_str_const(ALIYUN_URL_SYS_AWSS_DEVICE_CONNECTAP_INFO_GET), 0);

        if (!connectap_resource) {
            MDF_LOGE("coap_resource_init() failed");
            goto clean_up;
        }

        coap_register_handler(deviceinfo_resource, COAP_REQUEST_GET, deviceinfo_async_handler);
        coap_register_handler(connectap_resource, COAP_REQUEST_GET, connectap_async_handler);
        /* We possibly want to Observe the GETs */
        coap_resource_set_get_observable(deviceinfo_resource, 1);
        coap_resource_set_get_observable(connectap_resource, 1);
        coap_add_resource(ctx, deviceinfo_resource);
        coap_add_resource(ctx, connectap_resource);

        uint32_t wait_ms = COAP_DEFAULT_TIME * 1000;

        for (;;) {
            int result = coap_run_once(ctx, wait_ms);

            if (result < 0) {
                break;
            } else if (result && (uint32_t)result < wait_ms) {
                wait_ms -= result;
            }

            if (result) {
                wait_ms = COAP_DEFAULT_TIME * 1000;
            }

            // TODO 这里存疑
            if (deviceinfo_async) {
                deviceinfo_send_async_response(ctx, ctx->endpoint);
            }

            if (connectap_async) {
                connectap_send_async_response(ctx, ctx->endpoint);
            }
        }

clean_up:
        coap_free_context(ctx);
        coap_cleanup();
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
