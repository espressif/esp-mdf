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

#ifndef __MUPGRADE_H__
#define __MUPGRADE_H__

#include "esp_ota_ops.h"
#include "esp_partition.h"

#include "mdf_common.h"
#include "mwifi.h"

#ifdef __cplusplus
extern "C" {
#endif /**< _cplusplus */

/**
 * @brief mupgrade error code definition
 */
#define MDF_ERR_MUPGRADE_FIRMWARE_NOT_INIT   (MDF_ERR_MUPGRADE_BASE + 1) /**< Uninitialized firmware configuration */
#define MDF_ERR_MUPGRADE_FIRMWARE_PARTITION  (MDF_ERR_MUPGRADE_BASE + 2) /**< Partition table error */
#define MDF_ERR_MUPGRADE_FIRMWARE_INVALID    (MDF_ERR_MUPGRADE_BASE + 3) /**< Non-project generated firmware */
#define MDF_ERR_MUPGRADE_FIRMWARE_INCOMPLETE (MDF_ERR_MUPGRADE_BASE + 4) /**< The firmware received by the device is incomplete */
#define MDF_ERR_MUPGRADE_FIRMWARE_DOWNLOAD   (MDF_ERR_MUPGRADE_BASE + 5) /**< Firmware write flash error */
#define MDF_ERR_MUPGRADE_FIRMWARE_FINISH     (MDF_ERR_MUPGRADE_BASE + 6) /**< The firmware has been written to completion */
#define MDF_ERR_MUPGRADE_DEVICE_NO_EXIST     (MDF_ERR_MUPGRADE_BASE + 7) /**< The device that needs to be upgraded does not exist */
#define MDF_ERR_MUPGRADE_SEND_PACKET_LOSS    (MDF_ERR_MUPGRADE_BASE + 8) /**< Request device upgrade status failed */
#define MDF_ERR_MUPGRADE_NOT_INIT            (MDF_ERR_MUPGRADE_BASE + 9) /**< Upgrade configuration is not initialized */

/**
 * @brief enumerated list OF MDF event id
 */
#define MDF_EVENT_MUPGRADE_STARTED           (MDF_EVENT_MUPGRADE_BASE + 1) /**< The device starts to upgrade */
#define MDF_EVENT_MUPGRADE_STATUS            (MDF_EVENT_MUPGRADE_BASE + 2) /**< Proactively report progress */
#define MDF_EVENT_MUPGRADE_FINISH            (MDF_EVENT_MUPGRADE_BASE + 3) /**< The upgrade is complete and the new firmware will run after the reboot */
#define MDF_EVENT_MUPGRADE_STOPED            (MDF_EVENT_MUPGRADE_BASE + 4) /**< The device stop to upgrade */

/**
 * @brief Firmware subcontract upgrade
 */
#define MUPGRADE_PACKET_MAX_SIZE             (1024)  /**< Maximum length of a single packet transmitted */
#define MUPGRADE_PACKET_MAX_NUM              (4096) /**< The maximum number of packets */

/**
 * @brief Bit operations to get and modify a bit in an array
 */
#define MUPGRADE_GET_BITS(data, bits)        ( ((data)[(bits) >> 0x3]) & ( 1 << ((bits) & 0x7)) )
#define MUPGRADE_SET_BITS(data, bits)        do { ((data)[(bits) >> 0x3]) |= ( 1 << ((bits) & 0x7)); } while(0);

/**
 * @brief Type of packet
 */
#define MUPGRADE_TYPE_DATA                   (0x1)
#define MUPGRADE_TYPE_STATUS                 (0x2)

/**
 * @brief Firmware packet
 */
typedef struct {
    uint8_t type;   /**< Type of packet, MUPGRADE_TYPE_DATA */
    uint16_t seq;   /**< Sequence */
    uint16_t size;  /**< Size */
    uint8_t data[MUPGRADE_PACKET_MAX_SIZE]; /**< Firmware */
}  __attribute__((packed)) mupgrade_packet_t;

/**
 * @brief Status packet
 */
typedef struct {
    uint8_t type;              /**< Type of packet, MUPGRADE_TYPE_STATUS */
    char name[32];             /**< Unique identifier of the firmware */
    mdf_err_t error_code;      /**< Upgrade status */
    size_t total_size;         /**< Total length of firmware */
    size_t written_size;       /**< The length of the flash has been written */
    uint8_t progress_array[0]; /**< Identify if each packet of data has been written */
} __attribute__((packed)) mupgrade_status_t;

/**
 * @brief Mupgrade config
 */
typedef struct {
    QueueHandle_t queue;       /**< Queue handle */
    esp_ota_handle_t handle;   /**< OTA handle */
    const esp_partition_t *partition; /**< Pointer to partition structure obtained using
                                           esp_partition_find_first or esp_partition_get. */
    uint32_t start_time;       /**< Start time of the upgrade */
    mupgrade_status_t status;  /**< Upgrade status */
} mupgrade_config_t;

/**
 * @brief List of devices' status during the upgrade process
 */
typedef struct {
    size_t unfinished_num;    /**< The number of devices to be upgraded */
    uint8_t *unfinished_addr; /**< MAC address of devices to be upgraded */

    size_t successed_num;     /**< The number of devices that succeeded to upgrade */
    uint8_t *successed_addr;  /**< MAC address of devices that succeeded to upgrade */

    size_t requested_num;     /**< The number of devices to be upgraded */
    uint8_t *requested_addr;  /**< This address is used to buffer the result of the request during the upgrade process */
} mupgrade_result_t;

/**
 * @brief  Initialize the upgrade status and erase the upgrade partition
 *
 * @attention Only called at the root
 *
 * @param  name Unique identifier of the firmware
 * @param  size Total length of firmware,
 *              If image size is not yet known, pass OTA_SIZE_UNKNOWN and call
 *              `mupgrade_firmware_download_finished()` after the firmware download is complete.
 *
 * @return
 *    - MDF_OK
 *    - MDF_ERR_INVALID_ARG
 *    - MDF_ERR_MUPGRADE_FIRMWARE_PARTITION
 */
mdf_err_t mupgrade_firmware_init(const char *name, size_t size);

/**
 * @brief  Write firmware to flash
 *
 * @attention Only called at the root
 *
 * @param  data Pointer to the firmware
 * @param  size The length of the data
 *
 * @return
 *    - MDF_OK
 *    - MDF_ERR_MUPGRADE_FIRMWARE_NOT_INIT
 *    - MDF_ERR_MUPGRADE_FIRMWARE_FINISH
 */
mdf_err_t mupgrade_firmware_download(const void *data, size_t size);

/**
 * @brief Finish OTA update and validate newly written app image.
 *
 * @attention Only called `mupgrade_firmware_init()` firmware size unknown at root node
 *
 * @param  image_size of new OTA app image
 *
 * @return
 *    - MDF_OK
 *    - MDF_ERR_MUPGRADE_FIRMWARE_NOT_INIT
 */
mdf_err_t mupgrade_firmware_download_finished(size_t image_size);

/**
 * @brief  Check if the firmware is generated by this project
 *
 * @param  partition The partition where the firmware to be checked is located
 *
 * @return
 *    - MDF_OK
 *    - MDF_ERR_MUPGRADE_FIRMWARE_INVALID
 */
mdf_err_t mupgrade_firmware_check(const esp_partition_t *partition);

/**
 * @brief  Root sends firmware to other nodes
 *
 * @attention Only called at the root
 *
 * @param  dest_addrs     Destination nodes of mac
 * @param  dest_addrs_num Number of destination nodes
 * @param  result         Must call mupgrade_result_free to free memory
 *
 * @return
 *    - MDF_OK
 *    - MDF_ERR_MUPGRADE_FIRMWARE_NOT_INIT
 *    - MDF_ERR_MUPGRADE_DEVICE_NO_EXIST
 */
mdf_err_t mupgrade_firmware_send(const uint8_t *dest_addrs, size_t dest_addrs_num,
                                 mupgrade_result_t *result);

/**
 * @brief Stop Root to send firmware to other nodes
 *
 * @return
 *    - MDF_OK
 *   - MDF_ERR_NOT_SUPPORTED
 */
mdf_err_t mupgrade_firmware_stop();

/**
 * @brief  Free memory in the results list
 *
 * @param  result Pointer to device upgrade status
 *
 * @return
 *    - MDF_OK
 *    - MDF_ERR_INVALID_ARG
 */
mdf_err_t mupgrade_result_free(mupgrade_result_t *result);

/**
 * @brief  Handling upgrade data sent by ROOT
 *
 * @attention Only called at the non-root
 *
 * @param  addr root address
 * @param  data Upgrade instructions or firmware
 * @param  size The length of the data
 *
 * @return
 *    - MDF_OK
 *    - MDF_ERR_MUPGRADE_FIRMWARE_DOWNLOAD
 *    - MDF_ERR_MUPGRADE_NOT_INIT
 */
mdf_err_t mupgrade_handle(const uint8_t *addr, const void *data, size_t size);

/**
 * @brief  Handling the status of non-root responses
 *
 * @attention Only called at the root
 *
 * @param  addr Non-root address
 * @param  data State of non-root response
 * @param  size The length of the data
 *
 * @return
 *    - MDF_OK
 *    - MDF_ERR_TIMEOUT
 */
mdf_err_t mupgrade_root_handle(const uint8_t *addr, const void *data, size_t size);

/**
 * @brief Get the status of the upgrade
 *
 * @param status The status of the upgrade
 *
 * @return
*    - MDF_OK
 *   - MDF_ERR_INVALID_ARG
 *   - MDF_ERR_NOT_SUPPORTED
 */
mdf_err_t mupgrade_get_status(mupgrade_status_t *status);

/**
 * @brief Upgrade error back to previous version
 *
 * @return
 *    - MDF_OK
 *    - MDF_FAIL
 */
mdf_err_t mupgrade_version_fallback();

#ifdef __cplusplus
}
#endif /**< _cplusplus */
#endif /**< __MUPGRADE_H__ */
