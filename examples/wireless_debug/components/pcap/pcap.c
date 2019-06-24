/* pcap encoder.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include "esp_types.h"
#include "esp_err.h"
#include "esp_log.h"

#include "mdf_common.h"

#include "pcap.h"

static const char *TAG = "pcap";

/**
 * @brief Pcap File Header Type Definition
 */
typedef struct {
    uint32_t magic;     /*!< Magic Number */
    uint16_t major;     /*!< Major Version */
    uint16_t minor;     /*!< Minor Version */
    uint32_t zone;      /*!< Time Zone Offset */
    uint32_t sigfigs;   /*!< Timestamp Accuracy */
    uint32_t snaplen;   /*!< Max Length to Capture */
    uint32_t link_type; /*!< Link Layer Type */
} pcap_file_header_t;

/**
 * @brief Pcap Packet Header Type Definition
 */
typedef struct {
    uint32_t seconds;        /*!< Number of seconds since January 1st, 1970, 00:00:00 GMT */
    uint32_t microseconds;   /*!< Number of microseconds when the packet was captured(offset from seconds) */
    uint32_t capture_length; /*!< Number of bytes of captured data, not longer than packet_length */
    uint32_t packet_length;  /*!< Actual length of current packet */
} pcap_packet_header_t;

static FILE *file = NULL;

esp_err_t pcap_capture_packet(void *payload, uint32_t length, uint32_t seconds, uint32_t microseconds)
{
    MDF_ERROR_CHECK(!file, ESP_FAIL, "file empty");

    size_t real_write = 0;
    pcap_packet_header_t header = {
        .seconds = seconds,
        .microseconds = microseconds,
        .capture_length = length,
        .packet_length = length
    };
    real_write = fwrite(&header, sizeof(header), 1, file);
    MDF_ERROR_CHECK(real_write != 1, ESP_FAIL, "Write packet header error");
    real_write = fwrite(payload, sizeof(uint8_t), length, file);
    MDF_ERROR_CHECK(real_write != length, ESP_FAIL, "Write packet payload error");
    /* Flush content in the buffer into device */
    fflush(file);
    return ESP_OK;
}

esp_err_t pcap_close(void)
{
    MDF_ERROR_CHECK(!file, ESP_OK, "file not exist, or already close");

    if (fclose(file)) {
        ESP_LOGE(TAG, "Close pcap file failed");
        file = NULL;
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Close Pcap file OK");
    file = NULL;
    return ESP_OK;
}

esp_err_t pcap_new(pcap_config_t *config)
{
    file = config->fp;
    /* Write Pcap File header */
    pcap_file_header_t header = {
        .magic = PCAP_MAGIC_BIG_ENDIAN,
        .major = PCAP_VERSION_MAJOR,
        .minor = PCAP_VERSION_MINOR,
        .zone = PCAP_TIME_ZONE_GMT,
        .sigfigs = 0,
        .snaplen = 0x40000,
        .link_type = config->link_type
    };
    size_t real_write = fwrite(&header, sizeof(header), 1, file);

    MDF_ERROR_GOTO(real_write != 1, err_write, "Write Pcap file header error");

    /* Flush content in the buffer into device */
    fflush(file);
    return ESP_OK;

    /* Error Handling */
err_write:
    fclose(file);
    return ESP_FAIL;
}
