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

#include <ctype.h>

/**< SD card module head files */
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "sys/dirent.h"
#include "wpa2/utils/base64.h"

#include "mdf_common.h"
#include "sdcard.h"

static const char *TAG = "sdcard";

#define SDCARD_BASE_PATH "/sdcard"
#define SDCARD_FILE_NAME_MAX_LEN 64

static bool g_is_mount_flag = false;

mdf_err_t sdcard_init()
{
    ESP_LOGI(TAG, "Initializing SD card");
    ESP_LOGI(TAG, "Using SDMMC peripheral");
    mdf_err_t ret      = MDF_OK;
    sdmmc_card_t *card = NULL;;
    sdmmc_host_t host  = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    gpio_set_pull_mode(GPIO_NUM_15, GPIO_PULLUP_ONLY);   /**< CMD, needed in 4- and 1- line modes */
    gpio_set_pull_mode(GPIO_NUM_2, GPIO_PULLUP_ONLY);    /**< D0, needed in 4- and 1- line modes */
    gpio_set_pull_mode(GPIO_NUM_4, GPIO_PULLUP_ONLY);    /**< D1, needed in 4-line mode only */
    gpio_set_pull_mode(GPIO_NUM_12, GPIO_PULLUP_ONLY);   /**< D2, needed in 4-line mode only */
    gpio_set_pull_mode(GPIO_NUM_13, GPIO_PULLUP_ONLY);   /**< D3, needed in 4- and 1- line modes */

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .max_files              = 4,
        .format_if_mount_failed = true,
        // .allocation_unit_size = 16 * 1024
    };

    MDF_LOGW("free heap: %d", esp_get_free_heap_size());

    // initialize SD card and mount FAT filesystem.
    ret = esp_vfs_fat_sdmmc_mount(SDCARD_BASE_PATH, &host,
                                  &slot_config, &mount_config, &card);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "Failed to initialize the card"
                    "Make sure SD card lines have pull-up resistors in place.");

    /** print card info if mount successfully */
    sdmmc_card_print_info(stdout, card);
    g_is_mount_flag = true;

    return MDF_OK;
}

esp_err_t sdcard_remove_file(const char *file_name)
{
    struct stat st = { 0 };
    char file_path[SDCARD_FILE_NAME_MAX_LEN] = {0};
    sprintf(file_path, SDCARD_BASE_PATH "/%s", file_name);

    if (stat(file_path, &st) == MDF_OK) {
        unlink(file_path);
    } else if (!strcmp(file_name, "*") || !strncmp(file_name, "*.", 2)) {
        struct dirent *entry = NULL;

        DIR *dirptr = opendir(SDCARD_BASE_PATH);
        MDF_ERROR_CHECK(dirptr == NULL, MDF_FAIL, "opendir failed");

        while ((entry = readdir(dirptr)) != NULL) {
            if (strcmp(file_name, "*")) {
                const char *file_type = file_name + 2;

                if (!strstr(entry->d_name, file_type)) {
                    continue;
                }
            }

            sprintf(file_path, SDCARD_BASE_PATH "/%s", entry->d_name);

            if (stat(file_path, &st) == 0) {
                MDF_LOGI("remove file: %14s", entry->d_name);
                unlink(file_path);
            }
        }

        closedir(dirptr);
    } else {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t sdcard_rename_file(const char *file_name_old, const char *file_name_new)
{
    char file_path_old[SDCARD_FILE_NAME_MAX_LEN] = {0};
    char file_path_new[SDCARD_FILE_NAME_MAX_LEN] = {0};
    sprintf(file_path_old, SDCARD_BASE_PATH "/%s", file_name_old);
    sprintf(file_path_new, SDCARD_BASE_PATH "/%s", file_name_new);

    mdf_err_t ret = rename(file_path_old, file_path_new);
    MDF_ERROR_CHECK(ret != MDF_OK, ret, "Rename failed, old: %s, new: %s",
                    file_name_old, file_name_new);

    return ESP_OK;
}


static ssize_t file_size(char *file_name)
{
    char file_path[SDCARD_FILE_NAME_MAX_LEN] = {0};
    sprintf(file_path, SDCARD_BASE_PATH "/%s", file_name);

    FILE *fp = fopen(file_path, "r");
    MDF_ERROR_CHECK(!fp, -1, "file open failed");

    fseek(fp, 0L, SEEK_END);
    ssize_t size = ftell(fp);

    fclose(fp);
    return size;
}

esp_err_t sdcard_list_file(const char *file_name)
{
    struct dirent *entry = NULL;
    struct stat f_stat = { 0 };
    char file_path[SDCARD_FILE_NAME_MAX_LEN] = {0};

    DIR *dirptr = opendir(SDCARD_BASE_PATH);
    MDF_ERROR_CHECK(dirptr == NULL, MDF_FAIL, "opendir failed");

    sprintf(file_path, SDCARD_BASE_PATH "/%s", file_name);

    if (stat(file_path, &f_stat) == MDF_OK) {
        unlink(file_path);
        return MDF_OK;
    }

    if (strcmp(file_name, "*") && strncmp(file_name, "*.", 2)) {
        MDF_LOGW("file_name: %s", file_name);
        return MDF_FAIL;
    }

    for (int count = 0; (entry = readdir(dirptr)) != NULL; ++count) {
        if (strcmp(file_name, "*")) {
            const char *file_type = file_name + 1;

            if (!strstr(entry->d_name, file_type)) {
                continue;
            }
        }

        sprintf(file_path, SDCARD_BASE_PATH "/%s", entry->d_name);
        stat(file_path, &f_stat);

        time_t mtime = f_stat.st_mtime;
        struct tm mtm;
        localtime_r(&mtime, &mtm);

        char timebuf[32] = {0};
        strftime(timebuf, 32, "%b %d %H:%M:%S", &mtm);

        MDF_LOGI("<%02d>  %s  %7ld  %s", count, timebuf, f_stat.st_size, entry->d_name);
    }

    closedir(dirptr);
    return ESP_OK;
}

esp_err_t sdcard_print_file(const char *file_name, file_format_t type, ssize_t size)
{
    MDF_PARAM_CHECK(file_name);
    MDF_PARAM_CHECK(type != FILE_TYPE_NONE);
    MDF_PARAM_CHECK(size >= 0);

    /**< the size of 'BUF_MAX_LEN * 8bit' must be divisible by 6bit */
#define BUF_MAX_LEN    1024
#define BYTES_PER_LINE 16

    char file_path[SDCARD_FILE_NAME_MAX_LEN] = { 0 };
    sprintf(file_path, SDCARD_BASE_PATH "/%s", file_name);
    uint8_t *buffer = MDF_MALLOC(BUF_MAX_LEN);

    FILE *fp = fopen(file_path, "r");

    if (fp == NULL) {
        MDF_LOGE("%s no exist", file_name);
        fclose(fp);

        MDF_FREE(buffer);
        return ESP_FAIL;
    }

    size_t read_size = 0;
    size_t buff_size = 0;

    do {
        buff_size = MIN(size, BUF_MAX_LEN);
        read_size = fread(buffer, 1, buff_size, fp);
        size -= read_size;

        switch (type) {
            case FILE_TYPE_HEX: {
                char temp_buffer[BYTES_PER_LINE + 3]; //for not-byte-accessible memory
                char hex_buffer[3 * BYTES_PER_LINE + 1];
                char *ptr_line;
                int bytes_cur_line;
                int buff_len = read_size;

                do {
                    if (buff_len > BYTES_PER_LINE) {
                        bytes_cur_line = BYTES_PER_LINE;
                    } else {
                        bytes_cur_line = buff_len;
                    }

                    if (!esp_ptr_byte_accessible(buffer)) {
                        //use memcpy to get around alignment issue
                        memcpy(temp_buffer, buffer, (bytes_cur_line + 3) / 4 * 4);
                        ptr_line = temp_buffer;
                    } else {
                        ptr_line = (char *)buffer;
                    }

                    for (int i = 0; i < bytes_cur_line; i++) {
                        sprintf(hex_buffer + 3 * i, "%02x ", ptr_line[i]);
                    }

                    printf("%s\n", hex_buffer);
                    buffer += bytes_cur_line;
                    buff_len -= bytes_cur_line;
                } while (buff_len);

                break;
            }

            case FILE_TYPE_BIN: {
                char temp_buffer[BYTES_PER_LINE + 3]; //for not-byte-accessible memory
                char *ptr_line;
                //format: field[length]
                // ADDR[10]+"   "+DATA_HEX[8*3]+" "+DATA_HEX[8*3]+"  |"+DATA_CHAR[8]+"|"
                char hd_buffer[10 + 3 + BYTES_PER_LINE * 3 + 3 + BYTES_PER_LINE + 1 + 1];
                char *ptr_hd;
                int bytes_cur_line;
                int buff_len = read_size;

                do {
                    if (buff_len > BYTES_PER_LINE) {
                        bytes_cur_line = BYTES_PER_LINE;
                    } else {
                        bytes_cur_line = buff_len;
                    }

                    if (!esp_ptr_byte_accessible(buffer)) {
                        //use memcpy to get around alignment issue
                        memcpy(temp_buffer, buffer, (bytes_cur_line + 3) / 4 * 4);
                        ptr_line = temp_buffer;
                    } else {
                        ptr_line = (char *)buffer;
                    }

                    ptr_hd = hd_buffer;

                    ptr_hd += sprintf(ptr_hd, "%p ", buffer);

                    for (int i = 0; i < BYTES_PER_LINE; i++) {
                        if ((i & 7) == 0) {
                            ptr_hd += sprintf(ptr_hd, " ");
                        }

                        if (i < bytes_cur_line) {
                            ptr_hd += sprintf(ptr_hd, " %02x", ptr_line[i]);
                        } else {
                            ptr_hd += sprintf(ptr_hd, "   ");
                        }
                    }

                    ptr_hd += sprintf(ptr_hd, "  |");

                    for (int i = 0; i < bytes_cur_line; i++) {
                        if (isprint((int)ptr_line[i])) {
                            ptr_hd += sprintf(ptr_hd, "%c", ptr_line[i]);
                        } else {
                            ptr_hd += sprintf(ptr_hd, ".");
                        }
                    }

                    ptr_hd += sprintf(ptr_hd, "|");

                    printf("%s\n", hd_buffer);
                    buffer += bytes_cur_line;
                    buff_len -= bytes_cur_line;
                } while (buff_len);

                break;
            }

            case FILE_TYPE_STRING:
                printf("%.*s", read_size, buffer);
                break;

            case FILE_TYPE_BASE64: {
                uint8_t *b64_buf = base64_encode(buffer, read_size, NULL);
                printf("%s", b64_buf);
                MDF_FREE(b64_buf);
                break;
            }

            default:
                break;
        }
    } while (read_size > 0 && size > 0);

    return ESP_OK;
}

esp_err_t sdcard_write_file(const char *file_name, uint32_t offset, const void *data, size_t size)
{
    mdf_err_t ret = MDF_FAIL;
    char file_path[SDCARD_FILE_NAME_MAX_LEN] = { 0 };
    size_t real_write = 0;

    snprintf(file_path, SDCARD_FILE_NAME_MAX_LEN, SDCARD_BASE_PATH "/%s", file_name);

    FILE *fp = fopen(file_path, "a+");
    MDF_ERROR_GOTO(fp == NULL, EXIT, "Create file %s failed", file_name);

    if (offset == UINT32_MAX) {
        ret = fseek(fp, 0, SEEK_END);
        MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "seek packet error");
    } else {
        ret = fseek(fp, offset, SEEK_SET);
        MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "seek packet error");
    }

    real_write = fwrite(data, 1, size, fp);
    MDF_ERROR_GOTO(real_write != size, EXIT, "Write packet error");

    ret = MDF_OK;

EXIT:

    if (fp) {
        fclose(fp);
    }

    return ret;
}

esp_err_t sdcard_read_file(const char *file_name, uint32_t offset, uint8_t *data, size_t *size)
{
    mdf_err_t ret = MDF_FAIL;
    char file_path[SDCARD_FILE_NAME_MAX_LEN] = { 0 };

    snprintf(file_path, SDCARD_FILE_NAME_MAX_LEN, SDCARD_BASE_PATH "/%s", file_name);

    FILE *fp = fopen(file_path, "r");
    MDF_ERROR_GOTO(fp == NULL, EXIT, "Create file %s failed", file_name);

    ret = fseek(fp, offset, SEEK_SET);
    MDF_ERROR_GOTO(ret != MDF_OK, EXIT, "seek packet error");

    *size = fread(data, 1, *size, fp);
    MDF_ERROR_GOTO(*size <= 0, EXIT, "Read packet error");

    ret = MDF_OK;

EXIT:

    if (fp) {
        fclose(fp);
    }

    return ret;
}

bool sdcard_is_mount()
{
    return g_is_mount_flag;
}