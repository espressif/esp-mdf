// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "esp_log.h"
#include "rom/gpio.h"
#include "rom/spi_flash.h"
#include "bootloader_config.h"
#include "bootloader_init.h"
#include "bootloader_utility.h"
#include "bootloader_common.h"
#include "sdkconfig.h"
#include "esp_image_format.h"

static const char* TAG = "boot";

#ifdef CONFIG_MDF_BOOTLOADER_CUSTOMIZATION
#include "bootloader_flash.h"

static uint32_t g_secure_flag = 0;
static uint32_t g_bootloader_cnt = 0;
uint32_t g_flag_prt_addr  = 0;

// -----------------------------------------------------------
// | g_bootloader_cnt A | g_bootloader_cnt B | g_secure_flag |
// -----------------------------------------------------------
static void bootlooder_cnt_save(void)
{
    bootloader_flash_read(g_flag_prt_addr + SPI_SEC_SIZE * 2, &g_secure_flag, sizeof(g_secure_flag), false);

    if (g_secure_flag == 0) {
        bootloader_flash_erase_sector(g_flag_prt_addr / SPI_SEC_SIZE + 1);
        bootloader_flash_write(g_flag_prt_addr + SPI_SEC_SIZE * 1, &g_bootloader_cnt, sizeof(g_bootloader_cnt), false);

        g_secure_flag = 1;
        bootloader_flash_erase_sector(g_flag_prt_addr / SPI_SEC_SIZE + 2);
        bootloader_flash_write(g_flag_prt_addr + SPI_SEC_SIZE * 2, &g_secure_flag, sizeof(g_secure_flag), false);
    } else {
        bootloader_flash_erase_sector(g_flag_prt_addr / SPI_SEC_SIZE);
        bootloader_flash_write(g_flag_prt_addr, &g_bootloader_cnt, sizeof(g_bootloader_cnt), false);

        g_secure_flag = 0;
        bootloader_flash_erase_sector(g_flag_prt_addr / SPI_SEC_SIZE + 2);
        bootloader_flash_write(g_flag_prt_addr + SPI_SEC_SIZE * 2, &g_secure_flag, sizeof(g_secure_flag), false);
    }

    return true;
}

static void bootlooder_cnt_load(void)
{
    bootloader_flash_read(g_flag_prt_addr + SPI_SEC_SIZE * 2, &g_secure_flag, sizeof(g_secure_flag), false);

    if (g_secure_flag == 0) {
        bootloader_flash_read(g_flag_prt_addr, &g_bootloader_cnt, sizeof(g_bootloader_cnt), false);
    } else {
        bootloader_flash_read(g_flag_prt_addr + SPI_SEC_SIZE * 1, &g_bootloader_cnt, sizeof(g_bootloader_cnt), false);
    }
}

// esp-mdf use the flag to decide whether return to factory partition,
// must be after load_partition_table(), because g_flag_prt_addr was assigned in it.
static void update_bootloader_cnt(void)
{
    bootlooder_cnt_load();

    // if customer want to disable factory partition, set g_bootloader_cnt 0
    if (g_bootloader_cnt) {
        if (g_bootloader_cnt == UINT32_MAX) {
            // first time power-on or been erased
            g_bootloader_cnt = 1;
        } else {
            g_bootloader_cnt++;
        }

        ESP_LOGI(TAG, "g_bootloader_cnt: %d", g_bootloader_cnt);
        bootlooder_cnt_save();
    }
}

#endif /*!< CONFIG_MDF_BOOTLOADER_CUSTOMIZATION */

static esp_err_t select_image (esp_image_metadata_t *image_data);
static int selected_boot_partition(const bootloader_state_t *bs);
/*
 * We arrive here after the ROM bootloader finished loading this second stage bootloader from flash.
 * The hardware is mostly uninitialized, flash cache is down and the app CPU is in reset.
 * We do have a stack, so we can do the initialization in C.
 */
void call_start_cpu0()
{
    // 1. Hardware initialization
    if(bootloader_init() != ESP_OK){
        return;
    }

    // 2. Select image to boot
    esp_image_metadata_t image_data;
    if(select_image(&image_data) != ESP_OK){
        return;
    }

    // 3. Loading the selected image
    bootloader_utility_load_image(&image_data);
}

// Selects image to boot
static esp_err_t select_image (esp_image_metadata_t *image_data)
{
    // 1. Load partition table
    bootloader_state_t bs = { 0 };
    if (!bootloader_utility_load_partition_table(&bs)) {
        ESP_LOGE(TAG, "load partition table error!");
        return ESP_FAIL;
    }

#ifdef CONFIG_MDF_BOOTLOADER_CUSTOMIZATION
    update_bootloader_cnt();
#endif

    // 2. Select boot partition
    int boot_index = selected_boot_partition(&bs);
    if(boot_index == INVALID_INDEX) {
        return ESP_FAIL; // Unrecoverable failure (not due to corrupt ota data or bad partition contents)
    }

    // 3. Load the app image for booting
    if (!bootloader_utility_load_boot_image(&bs, boot_index, image_data)) {
        return ESP_FAIL;
    }

#ifdef CONFIG_MDF_BOOTLOADER_CUSTOMIZATION
    bootlooder_cnt_load();

    if (g_bootloader_cnt) {
        if (g_bootloader_cnt >= CONFIG_MDF_BOOTLOADER_COUNT_MIN
                && g_bootloader_cnt <= CONFIG_MDF_BOOTLOADER_COUNT_MAX) {
            ESP_LOGI(TAG, "g_bootloader_cnt: %d, enter factory app", g_bootloader_cnt);

            memset(&image_data, 0, sizeof(esp_image_metadata_t));

            if (!bootloader_utility_load_boot_image(&bs, FACTORY_INDEX, image_data)) {
                return ESP_FAIL;
            }
        }

        g_bootloader_cnt = UINT32_MAX;
        bootlooder_cnt_save();
    }

#endif /*!< CONFIG_MDF_BOOTLOADER_CUSTOMIZATION */

    return ESP_OK;
}

/*
 * Selects a boot partition.
 * The conditions for switching to another firmware are checked.
 */
static int selected_boot_partition(const bootloader_state_t *bs)
{
    int boot_index = bootloader_utility_get_selected_boot_partition(bs);
    if (boot_index == INVALID_INDEX) {
        return boot_index; // Unrecoverable failure (not due to corrupt ota data or bad partition contents)
    } else {
        // Factory firmware.
#ifdef CONFIG_BOOTLOADER_FACTORY_RESET
        if (bootloader_common_check_long_hold_gpio(CONFIG_BOOTLOADER_NUM_PIN_FACTORY_RESET, CONFIG_BOOTLOADER_HOLD_TIME_GPIO) == 1) {
            ESP_LOGI(TAG, "Detect a condition of the factory reset");
            bool ota_data_erase = false;
#ifdef CONFIG_BOOTLOADER_OTA_DATA_ERASE
            ota_data_erase = true;
#endif
            const char *list_erase = CONFIG_BOOTLOADER_DATA_FACTORY_RESET;
            ESP_LOGI(TAG, "Data partitions to erase: %s", list_erase);
            if (bootloader_common_erase_part_type_data(list_erase, ota_data_erase) == false) {
                ESP_LOGE(TAG, "Not all partitions were erased");
            }
            return bootloader_utility_get_selected_boot_partition(bs);
        }
#endif
       // TEST firmware.
#ifdef CONFIG_BOOTLOADER_APP_TEST
        if (bootloader_common_check_long_hold_gpio(CONFIG_BOOTLOADER_NUM_PIN_APP_TEST, CONFIG_BOOTLOADER_HOLD_TIME_GPIO) == 1) {
            ESP_LOGI(TAG, "Detect a boot condition of the test firmware");
            if (bs->test.offset != 0) {
                boot_index = TEST_APP_INDEX;
                return boot_index;
            } else {
                ESP_LOGE(TAG, "Test firmware is not found in partition table");
                return INVALID_INDEX;
            }
        }
#endif
        // Customer implementation.
        // if (gpio_pin_1 == true && ...){
        //     boot_index = required_boot_partition;
        // } ...
    }
    return boot_index;
}
