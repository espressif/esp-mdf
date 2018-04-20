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

#include "http_parser.h"

#include "mdf_common.h"
#include "mdf_http.h"

static const char *TAG = "mdf_http";

typedef struct http_header {
    const char *field;
    void *value;
    size_t value_size;
    int value_type;
    enum { NONE = 0, FIELD, VALUE } element;
} http_header_t;

typedef struct http_body {
    char *buf;
    size_t size;
} http_body_t;

typedef struct status_code {
    uint16_t status;
    const char *name;
} status_code_t;

status_code_t status_name[] = {
    {200, "OK"},
    {201, "Created"},
    {204, "No Content"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {500, "Internal Server Error"},
};

#define REQUEST_LINE_FORMAT "%s %s HTTP/1.1\r\n"    /**< method url http_version\r\n */
#define RESPONSE_STATUS_FORMAT "HTTP/1.1 %d %s\r\n" /**< http_version status_code status_str\r\n */
#define HEADER_LINE_CHAR_FORMAT "%s: %s\r\n"        /**< header_field: field_value\r\n */
#define HEADER_LINE_INT_FORMAT "%s: %d\r\n"

static int on_url(http_parser *parser, const char *buf, size_t len)
{
    if (parser->data) {
        memcpy(parser->data, buf, len);
    }

    parser->data = (char *)buf;

    http_parser_pause(parser, HPE_PAUSED);
    return 0;
}

char *__http_get_request_line(const char *http_buf,
                              enum mdf_http_method *method, char *url)
{
    if (!http_buf) {
        return NULL;
    }

    http_parser_settings settings_null = {
        .on_url = on_url,
    };

    http_parser parser = {
        .data = url,
    };

    http_parser_init(&parser, HTTP_REQUEST);
    http_parser_execute(&parser, &settings_null, http_buf, strlen(http_buf));

    if (method) {
        *method = parser.method;
    }

    return parser.data;
}

static int on_status(http_parser *parser, const char *buf, size_t len)
{
    http_parser_pause(parser, HPE_PAUSED);
    return 0;
}

esp_err_t mdf_http_get_response_status(const char *http_buf, uint16_t *status_code)
{
    MDF_PARAM_CHECK(http_buf);
    MDF_PARAM_CHECK(status_code);

    http_parser_settings settings_null = {
        .on_status = on_status,
    };

    http_parser parser;
    http_parser_init(&parser, HTTP_RESPONSE);
    http_parser_execute(&parser, &settings_null, http_buf, strlen(http_buf));
    *status_code = parser.status_code;

    return (!*status_code) ? ESP_FAIL : ESP_OK;
}


static int on_header_field(http_parser *parser, const char *buf, size_t len)
{
    http_header_t *header = (http_header_t *)parser->data;

    if (header->element != FIELD) {
        return 0;
    }

    if (!strncasecmp(header->field, buf, len)) {
        header->element = VALUE;
    }

    return 0;
}

static int on_header_value(http_parser *parser, const char *buf, size_t len)
{
    http_header_t *header = (http_header_t *)parser->data;

    if (header->element != VALUE) {
        return 0;
    }

    header->element = 0;

    if (header->value_type == 1 || header->value_type == 0) {
        if (strncasecmp(buf, "true", len) == 0) {
            *((uint8_t *)header->value) = 1;
        } else if (strncasecmp(buf, "false", len) == 0) {
            *((uint8_t *)header->value) = 0;
        } else {
            memcpy(header->value, buf, len);
        }

        goto EXIT;
    }

    if (header->value_type == 4) {
        *((char **)header->value) = (char *)buf;
        goto EXIT;
    }


    if (!(*buf == '-' || (*buf >= '0' && *buf <= '9'))) {
        MDF_LOGE("buf: %s, value_type: %d", buf, header->value_type);
        goto EXIT;
    }

    switch (header->value_type) {
        case 2:
            *((short *)header->value) = atoi(buf);
            break;

        case 3:
            *((int *)header->value) = atoi(buf);
            break;

        case 4:
            *((char **)header->value) = (char *)buf;
            break;

        default:
            MDF_LOGE("buf: %s, value_type: %d", buf, header->value_type);
            break;
    }

EXIT:
    http_parser_pause(parser, HPE_PAUSED);
    header->value_size = len;

    return 0;
}

esp_err_t __http_get_header(const char *http_buf, const char *field,
                            void *value, int value_type)
{
    MDF_PARAM_CHECK(http_buf);
    MDF_PARAM_CHECK(field);
    MDF_PARAM_CHECK(value);

    http_parser_settings settings_null = {
        .on_header_field = on_header_field,
        .on_header_value = on_header_value,
    };

    struct http_header headers = {
        .field      = field,
        .value      = value,
        .value_type = value_type,
        .element    = FIELD,
    };

    http_parser parser = {
        .data = &headers,
    };

    http_parser_init(&parser, HTTP_BOTH);
    http_parser_execute(&parser, &settings_null, http_buf, strlen(http_buf));

    return headers.value_size;
}

static int on_body(http_parser *parser, const char *buf, size_t len)
{
    http_body_t *body = (http_body_t *)parser->data;

    if (body->buf) {
        memcpy(body->buf, buf, len);
    }

    body->buf  = (char *)buf;
    body->size = len;
    http_parser_pause(parser, HPE_PAUSED);

    return 0;
}

char *__http_get_body(const char *http_buf, char *body, size_t *size)
{
    if (!http_buf) {
        return NULL;
    }

    http_body_t http_body = {
        .buf = body,
        .size = 0,
    };

    http_parser_settings settings_null = {
        .on_body = on_body,
    };

    http_parser parser = {
        .data = &http_body,
    };

    http_parser_init(&parser, HTTP_BOTH);
    http_parser_execute(&parser, &settings_null, http_buf, strlen(http_buf));

    if (size) {
        *size = http_body.size;
    }

    return http_body.size ? http_body.buf : NULL;
}

char *__http_get_url_path(const char *url, char *url_path)
{
    if (!url) {
        return NULL;
    }

    struct http_parser_url u;

    http_parser_url_init(&u);

    http_parser_parse_url(url, strlen(url), 0, &u);

    if (!(u.field_set & (1 << UF_PATH))) {
        return NULL;
    }

    const char *url_path_ptr = url + u.field_data[UF_PATH].off;

    if (url_path) {
        memcpy(url_path, url_path_ptr, u.field_data[UF_PATH].len);
        url_path[u.field_data[UF_PATH].len] = '\0';
    }

    return (char *)url_path_ptr;
}

char *__http_get_url_query(const char *url, char *url_query)
{
    if (!url) {
        return NULL;
    }

    struct http_parser_url u;

    http_parser_url_init(&u);

    http_parser_parse_url(url, strlen(url), 0, &u);

    if (!(u.field_set & (1 << UF_QUERY))) {
        return NULL;
    }

    const char *url_query_ptr = url + u.field_data[UF_QUERY].off;

    if (url_query_ptr == url) {
        return NULL;
    }

    if (url_query) {
        memcpy(url_query, url_query_ptr, u.field_data[UF_QUERY].len);
        url_query[u.field_data[UF_QUERY].len] = '\0';
    }

    return (char *)url_query_ptr;
}

esp_err_t mdf_http_set_request_line(char *http_buf, enum mdf_http_method method,
                                    const char *url)
{
    MDF_PARAM_CHECK(http_buf);
    MDF_PARAM_CHECK(url);

    sprintf(http_buf, REQUEST_LINE_FORMAT, http_method_str(method), url);

    return ESP_OK;
}

esp_err_t mdf_http_set_response_status(char *http_buf, uint16_t status)
{
    MDF_PARAM_CHECK(http_buf);

    for (int i = 0; i < sizeof(status_name) / sizeof(status_code_t); ++i) {
        if (status_name[i].status == status) {
            sprintf(http_buf, RESPONSE_STATUS_FORMAT, status, status_name[i].name);
            return ESP_OK;
        }
    }

    return ESP_FAIL;
}

esp_err_t __http_set_header(char *http_buf, const char *field, const void *value, int value_type)
{
    MDF_PARAM_CHECK(http_buf);
    MDF_PARAM_CHECK(field);
    MDF_PARAM_CHECK(value);

    http_buf += strlen(http_buf);

    if (value_type == 1) {
        sprintf(http_buf, HEADER_LINE_INT_FORMAT, field, (int)value);
    } else {
        sprintf(http_buf, HEADER_LINE_CHAR_FORMAT, field, (char *)value);
    }

    return ESP_OK;
}


size_t mdf_http_set_body(char *http_buf, const char *body, size_t size)
{
    MDF_PARAM_CHECK(http_buf);

    /**< calculate current str_len in http_buf */
    int head_size = strlen(http_buf);

    /**< plus len to http_buf for sprintf buffer */
    head_size += sprintf(http_buf + head_size, "Content-Length: %d\r\n\r\n", size);

    if (body && size) {
        memcpy(http_buf + head_size, body, size);
    }

    /**< return total len of http package */
    return head_size + size;
}
