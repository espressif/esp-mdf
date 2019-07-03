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

#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "esp_vfs_fat.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"

#include "mdf_common.h"

static bool g_running_flag = false;
static const char *TAG = "mdebug_console";
static const char *prompt = "";

/**
 * @brief Console command history can be stored to and loaded from a file.
 *        The easiest way to do this is to use FATFS filesystem on top of
 *        wear_levelling library.
 */
#if CONFIG_MDEBUG_STORE_HISTORY

#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"

static mdf_err_t initialize_filesystem()
{
    esp_err_t ret = MDF_OK;
    static wl_handle_t wl_handle;
    const esp_vfs_fat_mount_config_t mount_config = {
        .max_files = 4,
        .format_if_mount_failed = true
    };

    /**< Initialize FAT filesystem in SPI flash and register it in VFS */
    ret = esp_vfs_fat_spiflash_mount(MOUNT_PATH, CONFIG_MDEBUG_PARTITION_LABEL,
                                     &mount_config, &wl_handle);
    MDF_ERROR_CHECK(ret != ESP_OK, ret, "Failed to mount FATFS");

    return MDF_OK;
}

#endif /**< CONFIG_MDEBUG_STORE_HISTORY */

static void initialize_console()
{
    /**< Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /**< Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    /**< Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

    /**< Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode. */
    const uart_config_t uart_config = {
        .baud_rate    = CONFIG_CONSOLE_UART_BAUDRATE,
        .data_bits    = UART_DATA_8_BITS,
        .parity       = UART_PARITY_DISABLE,
        .stop_bits    = UART_STOP_BITS_1,
        .use_ref_tick = true
    };
    ESP_ERROR_CHECK(uart_param_config(CONFIG_CONSOLE_UART_NUM, &uart_config));

    /**< Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK(uart_driver_install(CONFIG_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0));

    /**< Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_CONSOLE_UART_NUM);

    /**< Initialize the console */
    esp_console_config_t console_config = {
        .max_cmdline_args   = 8,
        .max_cmdline_length = 256,
#if CONFIG_LOG_COLORS
        .hint_color = atoi(LOG_COLOR_CYAN)
#endif
    };
    ESP_ERROR_CHECK(esp_console_init(&console_config));

    /**< Configure linenoise line completion library */
    /**< Enable multiline editing. If not set, long commands will scroll within
     * single line. */
    linenoiseSetMultiLine(1);

    /**< Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);

    /**< Set command history size */
    linenoiseHistorySetMaxLen(100);

#if CONFIG_MDEBUG_STORE_HISTORY
    /**< Load command history from filesystem */
    linenoiseHistoryLoad(HISTORY_PATH);
#endif /**< CONFIG_MDEBUG_STORE_HISTORY */
}

static void console_handle_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    mdf_err_t err = MDF_OK;
    char *line = NULL;

    while (g_running_flag) {
        /** Get a line using linenoise.
         * The line is returned when ENTER is pressed. */
        line = linenoise(prompt);

        /**< Ignore empty lines */
        if (line == NULL) {
            continue;
        }

        /**< Add the command to the history */
        linenoiseHistoryAdd(line);

#if CONFIG_MDEBUG_STORE_HISTORY
        /**< Save command history to filesystem */
        linenoiseHistorySave(HISTORY_PATH);
#endif

        /**< Try to run the command */
        err = esp_console_run(line, &ret);

        if (err == ESP_ERR_NOT_FOUND) {
            MDF_LOGW("Unrecognized command");
        } else if (err == ESP_ERR_INVALID_ARG) {
            MDF_LOGW("Invalid argument");
            // command was empty
        } else if (err == ESP_OK && ret != ESP_OK) {
            MDF_LOGW("<%s> Command returned non-zero error", esp_err_to_name(ret));
        } else if (err != ESP_OK) {
            MDF_LOGW("<%s> Internal error", esp_err_to_name(err));
        }

        /**< linenoise allocates line buffer on the heap, so need to free it */
        linenoiseFree(line);
    }

    MDF_LOGI("console handle exit");
    vTaskDelete(NULL);
}

mdf_err_t mdebug_console_init()
{
#if CONFIG_MDEBUG_STORE_HISTORY
    initialize_filesystem();
#endif /**< CONFIG_MDEBUG_STORE_HISTORY */

    initialize_console();
    g_running_flag = true;

    /**< Register help commands */
    esp_console_register_help_command();

    /**< Prompt to be printed before each line.
     * This can be customized, made dynamic, etc. */
#if CONFIG_LOG_COLORS
    prompt = LOG_COLOR_I "mdf> " LOG_RESET_COLOR;
#else
    /**< Since the terminal doesn't support escape sequences,
     * don't use color codes in the prompt. */
    prompt = "mdf> ";
#endif /**<  CONFIG_LOG_COLORS */

    printf("\nThis is an example of ESP-MDF console component.\n"
           "Type 'help' to get the list of commands.\n"
           "Use UP/DOWN arrows to navigate through command history.\n"
           "Press TAB when typing command name to auto-complete.\n\n");

    /**< Figure out if the terminal supports escape sequences */
    if (linenoiseProbe()) { /**< zero indicates success */
        MDF_LOGW("Your terminal application does not support escape sequences.\n"
                 "Line editing and history features are disabled.\n"
                 "On Windows, try using Putty instead.\n");
        linenoiseSetDumbMode(1);
    }

    xTaskCreate(console_handle_task, "console_handle", 1024 * 4, NULL, 1, NULL);

    return MDF_OK;
}

mdf_err_t mdebug_console_deinit()
{
    g_running_flag = false;
    esp_console_deinit();

    return MDF_OK;
}
