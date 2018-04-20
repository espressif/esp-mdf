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

#include "esp_system.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"

#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "esp_vfs_fat.h"

#include "mdf_console_debug.h"

#ifdef CONFIG_MDF_CONSOLE_DEBUG

static char *prompt = "";

static void initialize_console()
{
    /* Disable buffering on stdin*/
    setvbuf(stdin, NULL, _IONBF, 0);

    /* ATTENTION: NOT DISABLE STDOUT BUFFERING */
    // setvbuf(stdout, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK(uart_driver_install(CONFIG_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0));

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_CONSOLE_UART_NUM);

    /* Initialize the console */
    esp_console_config_t console_config = {
        .max_cmdline_args = 8,
        .max_cmdline_length = 256,
#if CONFIG_LOG_COLORS
        .hint_color = atoi(LOG_COLOR_CYAN)
#endif
    };
    ESP_ERROR_CHECK(esp_console_init(&console_config));

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback *) &esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(100);
}

void mdf_console_task(void *arg)
{
    int ret = 0;

    for (;;) {
        char *line = linenoise(prompt);

        if (line == NULL) {
            continue;
        }

        linenoiseHistoryAdd(line);

        esp_err_t err = esp_console_run(line, &ret);

        if (err == ESP_ERR_NOT_FOUND) {
            printf("Unrecognized command\n");
        } else if (err == ESP_ERR_INVALID_ARG) {
            // command was empty
        } else if (err == ESP_OK && ret != ESP_OK) {
            printf("Command returned non-zero error code: 0x%x\n", ret);
        } else if (err != ESP_OK) {
            printf("Internal error: 0x%x\n", err);
        }

        linenoiseFree(line);
    }

    vTaskDelete(NULL);
}
#endif /**< CONFIG_MDF_CONSOLE_DEBUG */

extern void register_system(void);
void mdf_console_init(void)
{
#ifdef CONFIG_MDF_CONSOLE_DEBUG
    initialize_console();

    /* Register commands */
    esp_console_register_help_command();
    register_system();

    /* Figure out if the terminal supports escape sequences */
    if (linenoiseProbe()) {
        printf("\n"
               "Your terminal application does not support escape sequences.\n"
               "Line editing and history features are disabled.\n"
               "On Windows, try using Putty instead.\n");
        linenoiseSetDumbMode(1);
#if CONFIG_LOG_COLORS
        /* Since the terminal doesn't support escape sequences,
         * don't use color codes in the prompt.
         */
        prompt = "mdf> ";
#endif //CONFIG_LOG_COLORS
    } else {
        prompt = LOG_COLOR_I "mdf> " LOG_RESET_COLOR;
    }

    xTaskCreate(mdf_console_task, "mdf_console_task", 1024 * 2, NULL,
                MDF_TASK_DEFAULT_PRIOTY - 3, NULL);

    printf("========== mdf_console_init ==========\n");
#endif /**< CONFIG_MDF_CONSOLE_DEBUG */
}
