/* Console example — various system commands

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_console.h"
#include "esp_system.h"

#include "driver/rtc_io.h"
#include "argtable3/argtable3.h"
#include "soc/rtc_cntl_reg.h"

#include "mdf_wifi_mesh.h"

static void register_free();
static void register_restart();
static void register_tasklist();
static void register_routetable();

void register_system()
{
    register_free();
    register_restart();

    register_tasklist();
    register_routetable();
}

/** 'free' command prints available heap memory */
static int free_mem(int argc, char **argv)
{
    ets_printf("free heap: %d\n", esp_get_free_heap_size());
    return 0;
}
static void register_free()
{
    const esp_console_cmd_t cmd = {
        .command = "free",
        .help = "Get the total size of heap memory available",
        .hint = NULL,
        .func = &free_mem,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/** 'restart' command restarts the program */
static int restart(int argc, char **argv)
{
    ets_printf("Restarting\n");
    esp_restart();
}
static void register_restart()
{
    const esp_console_cmd_t cmd = {
        .command = "restart",
        .help = "Restart the program",
        .hint = NULL,
        .func = &restart,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/** 'tasklist' command list all tasks in system */
static int tasklist(int argc, char **argv)
{
    ets_printf("list all tasks in system\n");

    char *task_info = malloc(2048);
    assert(task_info);

    vTaskList(task_info);
    ets_printf("\n%s\n", task_info);
    free(task_info);

    return 0;
}
static void register_tasklist()
{
    const esp_console_cmd_t cmd = {
        .command = "tasklist",
        .help = "list all tasks in system",
        .hint = NULL,
        .func = &tasklist,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/** 'routetable' command get route table of mdf network */
static int routetable(int argc, char **argv)
{
    ets_printf("get route table of mdf network\n");

    int ret = 0;
    ret = esp_mesh_get_type();
    ets_printf("mdf node type: %s%s%s%s\n",
               ret == MESH_ROOT ? "MESH_ROOT" : "",
               ret == MESH_NODE ? "MESH_NODE" : "",
               ret == MESH_LEAF ? "MESH_LEAF" : "",
               ret == MESH_IDLE ? "MESH_IDLE" : "");

    ret = esp_mesh_get_total_node_num();
    ets_printf("mdf total node num: %d\n", ret);

    ret = esp_mesh_get_routing_table_size();

    if (ret == 0) {
        ets_printf("mesh has not been started or connected\n");
        return 1;
    }

    ets_printf("mdf router table size: %d\n", ret);

    int mdf_node_num       = 0;
    int router_table_size  = 0;
    char *router_table_buf = NULL;

    router_table_size = esp_mesh_get_routing_table_size() * 6;
    router_table_buf  = calloc(1, router_table_size);
    assert(router_table_buf);

    char *ptr_temp = router_table_buf;
    ESP_ERROR_CHECK(esp_mesh_get_routing_table((mesh_addr_t *)ptr_temp,
                    2048, &mdf_node_num));

    for (int i = 0; i < mdf_node_num; i++, ptr_temp += 6) {
        ets_printf(MACSTR"%s", MAC2STR(ptr_temp), i % 8 ? ", ":"\n");
    }

    free(router_table_buf);
    return 0;
}

static void register_routetable()
{
    const esp_console_cmd_t cmd = {
        .command = "routetable",
        .help = "list all nodes over mdf network",
        .hint = NULL,
        .func = &routetable,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
