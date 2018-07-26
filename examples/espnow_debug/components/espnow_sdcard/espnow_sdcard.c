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

/**< C Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**< RTOS Includes */
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

/**< SD card module head files */
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "sys/dirent.h"
#include "wpa2/utils/base64.h"

#include "mdf_common.h"
#include "espnow_sdcard.h"

static const char *TAG = "espnow_sdcard";
static SemaphoreHandle_t g_sdcard_write_mutex = NULL;

#define MAC2STR_H(a) (a)[0], (a)[1], (a)[2]
#define MACSTR_H "%02x%02x%02x"

esp_err_t espnow_sd_log_write(const uint8_t *mac_addr, const char *data)
{
    xSemaphoreTake(g_sdcard_write_mutex, portMAX_DELAY);

    char mac_str[30] = { 0 };
    sprintf(mac_str, "/sdard/"MACSTR_H".log", MAC2STR_H(&mac_addr[3]));

    FILE *f_log = fopen(mac_str, "a");

    if (!f_log) {
        MDF_LOGE("fopen fail");
        xSemaphoreGive(g_sdcard_write_mutex);
        return ESP_FAIL;
    }

    fprintf(f_log, "%s\n", data);
    fclose(f_log);

    xSemaphoreGive(g_sdcard_write_mutex);
    return ESP_OK;
}

esp_err_t espnow_sd_coredump_write(const uint8_t *mac_addr,
                                   uint8_t *data, size_t len, uint8_t oprt)
{
    xSemaphoreTake(g_sdcard_write_mutex, portMAX_DELAY);

    FILE *f_dump          = NULL;
    struct stat st        = { 0 };
    char dump_str[24]     = { 0 };
    char dump_str_tmp[24] = { 0 };

    sprintf(dump_str_tmp, "/sdard/"MACSTR_H".tmp", MAC2STR_H(&mac_addr[3]));

    switch (oprt) {
        /**< write to coredump temp file */
        case ESPNOW_SDCARD_WRITE_TEM:
            f_dump = fopen(dump_str_tmp, "a");
            MDF_ERROR_GOTO(!f_dump, EXIT, "fopen fail");

            size_t f_size = fwrite(data, len, 1, f_dump);

            if (f_size <= 0) {
                MDF_LOGW("fwrite fail, f_size: %d", f_size);
                fclose(f_dump);
                goto EXIT;
            }

            fclose(f_dump);
            break;

        /**< remove old coredump file, and rename temp file to write to formal */
        case ESPNOW_SDCARD_SAVE:
            if (stat(dump_str_tmp, &st) == 0) {
                sprintf(dump_str, "/sdard/"MACSTR_H".dmp", MAC2STR_H(&mac_addr[3]));

                if (stat(dump_str, &st) == 0) {
                    MDF_LOGI("delete the old coreudmp file: %s", dump_str);
                    unlink(dump_str);
                }

                if (rename(dump_str_tmp, dump_str) != 0) {
                    MDF_LOGE("Rename failed, [%s]--->[%s]", dump_str_tmp, dump_str);
                }
            } else {
                MDF_LOGW("coredump file not exists, file: %s", dump_str_tmp);
            }

            break;

        /**< delete temp coredump file (if it exists) once receive new coredump */
        case ESPNOW_SDCARD_DEL_TEM:
            if (stat(dump_str_tmp, &st) == 0) {
                unlink(dump_str_tmp);
            } else {
                MDF_LOGI("coredump file not exists, file: %s", dump_str_tmp);
            }

            break;

        default:
            MDF_LOGE("espnow_sd_coredump_write unsupported oprt: %d", oprt);
            break;
    }

    xSemaphoreGive(g_sdcard_write_mutex);
    return ESP_OK;

EXIT:
    xSemaphoreGive(g_sdcard_write_mutex);
    return ESP_FAIL;
}

esp_err_t espnow_sd_ls(const char *type)
{
    DIR *dirptr = NULL;

    if ((dirptr = opendir("/sdard")) == NULL) {
        MDF_LOGE("opendir failed");
        return ESP_FAIL;
    }

    int file_cnt = 0;
    struct dirent *entry = NULL;

    while ((entry = readdir(dirptr)) != NULL) {
        if (!strcmp("ALL", type) || strstr(entry->d_name, type)) {
            MDF_LOGI("[%d]name: %16s, type: %d", file_cnt++, entry->d_name, entry->d_type);
        }
    }

    closedir(dirptr);

    return ESP_OK;
}

esp_err_t espnow_sd_rm(const char *file)
{
    struct stat st = { 0 };
    char file_path[32] = { 0 };
    sprintf(file_path, "/sdard/%s", file);
    char *f_type = !strcmp("log", file) ? "LOG" :
                   !strcmp("dmp", file) ? "DMP" : NULL;

    if (stat(file_path, &st) == 0) {
        unlink(file_path);
    } else if (f_type) {
        DIR *dirptr = NULL;
        struct dirent *entry = NULL;

        if ((dirptr = opendir("/sdard")) == NULL) {
            MDF_LOGE("opendir failed\n");
            return ESP_FAIL;
        }

        while ((entry = readdir(dirptr)) != NULL) {
            if (!strstr(entry->d_name, f_type)) {
                continue;
            }

            sprintf(file_path, "/sdard/%s", entry->d_name);

            if (stat(file_path, &st) == 0) {
                MDF_LOGI("remove file: %14s", entry->d_name);
                unlink(file_path);
            }
        }

        closedir(dirptr);
    } else {
        MDF_LOGE("file no exist");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t espnow_sd_read(const char *file)
{
    /**< the size of 'BUF_LEN * 8bit' must be divisible by 6bit */
#define BUF_LEN       1023
#define DATA_ITEM_LEN 1

    struct stat st = { 0 };
    size_t read_len = 0;
    char file_path[32] = { 0 };
    sprintf(file_path, "/sdard/%s", file);

    if (stat(file_path, &st) != 0) {
        MDF_LOGW("file not exists");
        return ESP_FAIL;
    } else {
        unsigned char *src_buf = mdf_malloc(BUF_LEN);
        unsigned char *b64_buf = NULL;
        FILE *fp;

        if ((fp = fopen(file_path, "r")) == NULL) {
            MDF_LOGE("fopen failed: %s", file_path);
            fclose(fp);
            mdf_free(src_buf);
            return ESP_FAIL;
        }

        printf("================= CORE DUMP START =================\n");

        while ((read_len = fread(src_buf, DATA_ITEM_LEN, BUF_LEN, fp)) > 0) {
            b64_buf = base64_encode(src_buf, read_len, NULL);
            printf("%s", b64_buf);
            mdf_free(b64_buf);
        }

        printf("================= CORE DUMP END ===================\n");
        fclose(fp);
        mdf_free(src_buf);
    }

    return ESP_OK;
}

esp_err_t espnow_sd_init(void)
{
    gpio_set_pull_mode(GPIO_NUM_15, GPIO_PULLUP_ONLY);   /**< CMD, needed in 4- and 1- line modes */
    gpio_set_pull_mode(GPIO_NUM_2, GPIO_PULLUP_ONLY);    /**< D0, needed in 4- and 1-line modes */
    gpio_set_pull_mode(GPIO_NUM_4, GPIO_PULLUP_ONLY);    /**< D1, needed in 4-line mode only */
    gpio_set_pull_mode(GPIO_NUM_12, GPIO_PULLUP_ONLY);   /**< D2, needed in 4-line mode only */
    gpio_set_pull_mode(GPIO_NUM_13, GPIO_PULLUP_ONLY);   /**< D3, needed in 4- and 1-line modes */

    sdmmc_host_t host               = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files              = 20
    };
    sdmmc_card_t *card;

    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        MDF_LOGE("==============================================================");
        MDF_LOGE(" sdmmc mount fail, please check whether or not insert SD card ");
        MDF_LOGE("==============================================================");
        return ESP_FAIL;
    }

    /**< create sdcard write mutex */
    g_sdcard_write_mutex = xSemaphoreCreateMutex();

    MDF_LOGI("espnow_sdcard_init success");

    return ESP_OK;
}
