[[English]](../../en/application-notes/add_new_device_en.md)

# 添加新类型的 ESP-MDF 设备

本文档目标是帮助开发者从零开始，基于现有的 ESP-MDF，添加一个全新类型的设备。

## 1. ESP-MDF 设备工作流程

在开始添加新设备之前，首先介绍 ESP-MDF 设备的工作流程：

<div align=center>
<img src="../../_static/esp_mdf_work_flow.png" width="600">
<p> ESP-MDF Device Workflow </p>
</div>

### 1.1. 设备初始化

ESP-MDF 设备的硬件初始化。如温度传感器设备对应的引脚初始化；智能灯设备对应引脚的 PWM 模块初始化等。不同类型的设备的驱动程序不一样，需要单独编写。以 `light_bulb` 驱动为例，需要进行如下初始化，

```c
// init GPIO of light_bulb
ESP_ERROR_CHECK(light_init(GPIO_NUM_4, GPIO_NUM_16, GPIO_NUM_5, GPIO_NUM_19, GPIO_NUM_23));
```

### 1.2. 添加设备属性

不同类型的设备具有不同的类型值（TID, Type ID）和属性（CID, characteristic ID）值。比如，灯和按键是不同的设备，且灯具有色彩，明暗和开关控制，而按键只有开关控制。
App 或服务器控制端在对设备进行具体操作之前，需要知道设备的 TID 和 CID，之后根据设备具有的 CID 获取设备状态，或者对设备状态进行设置。以下代码以 `light_bulb` 为例，设备特征添加了函数 `mdf device_add characteristics（...）`。

```c
ESP_ERROR_CHECK(mdf_device_add_characteristics(STATUS_CID, "on", PERMS_READ_WRITE_TRIGGER, 0, 1, 1));
ESP_ERROR_CHECK(mdf_device_add_characteristics(HUE_CID, "hue", PERMS_READ_WRITE_TRIGGER, 0, 360, 1));
ESP_ERROR_CHECK(mdf_device_add_characteristics(SATURATION_CID, "saturation", PERMS_READ_WRITE_TRIGGER, 0, 100, 1));
ESP_ERROR_CHECK(mdf_device_add_characteristics(VALUE_CID, "value", PERMS_READ_WRITE_TRIGGER, 0, 100, 1));
ESP_ERROR_CHECK(mdf_device_add_characteristics(COLOR_TEMPERATURE_CID, "color_temperature", PERMS_READ_WRITE_TRIGGER, 0, 100, 1));
ESP_ERROR_CHECK(mdf_device_add_characteristics(BRIGHTNESS_CID, "brightness", PERMS_READ_WRITE_TRIGGER, 0, 100, 1));
```

### 1.3. 注册状态更新接口

设备需要提供 `*_get_value` 和 `*_set_value` 两个 API，用于控制端获取和设置设备状态。这两个 API 通过注册方式添加到设备的 `mdf_device_request_task` 中。

```c
ESP_ERROR_CHECK(mdf_device_init_handle(light_bulb_event_loop_cb, light_bulb_get_value, light_bulb_set_value));
```

### 1.4. 软件初始化

该步骤是系统进入下一步 `配网` 或启动 `ESP-Mesh` 通信的基础，也是系统在运行过程中各模块保持状态同步的关键。主要包括：

* `mdf_event_loop_init(event_cb);` 应用层事件通知功能初始化
* `mdf_reboot_event_init();` 设备重启事件处理
* `mdf_wifi_init();` Wi-Fi 初始化
* `mdf_espnow_init();` ESP-NOW 初始化
* `mdf_console_init();` Console debug 初始化
* `mdf_espnow_debug_init();` ESP-NOW debug 初始化


### 1.5. 网络配置

在完成设备的硬件初始化和软件初始化之后，系统将从 NVS 中读取网络配置信息，如果读取成功（设备已经配置过网络信息），则跳过这一步，进入下一步 MESH 组网阶段。

设备网络配置采用 Blufi 蓝牙配网 和 ESP-NOW 链式配网相结合的方式，快速便捷。第一个设备采用蓝牙方式，其余设备均采用  ESP-NOW 链式配网方式。相关代码位于：`$MDF_PATH/components/mdf_network_config`。

```c
for (;;) {
    /**< receive network info from blufi */
    if (xQueueReceive(g_network_config_queue, network_config, 0)) {
        MDF_LOGD("blufi network configured success");
        ret = MDF_OK;
        break;
    }

    ......

    /**< receive network info from espnow channel */
    if (mdf_ap_info_get(&request_data->rssi, dest_addr) != MDF_OK) {
        MDF_LOGV("did not find the network equipment has been configured network");
        continue;
    }

    ......

    break;
}
```

### 1.6. ESP-Mesh 组网

现阶段 ESP-Mesh 采用手动组网和自组网相结合的方式，可以指定根节点，叶子节点和父节点，其余未特别指定的节点组网逻辑采用自组网方式。更多关于 ESP-Mesh 的组网过程，请参考 ESP-Mesh 文档 [Mesh 组网](https://espressif-docs.readthedocs-hosted.com/projects/esp-idf/zh_CN/latest/api-guides/mesh.html#mesh-networking)。

### 1.7. 创建设备处理任务

设备接收来自控制端发送的数据，之后解析数据获取操作指令，查询指令列表并执行，最后根据需要作出设备的回复。处理步骤包括如下几步。

* 接收控制端数据：

```c
device_data.request_size = mdf_wifi_mesh_recv(&src_addr, &data_type, device_data.request, WIFI_MESH_PACKET_MAX_SIZE, portMAX_DELAY);
```

* 解析数据获取指令字段：

```c
mesh_json_parse(device_data.request, "request", func_name);
```

* 查询指令列表，并执行对应的指令：

```c
/**< if we can find this HTTP PATH from our list, we will handle this request. */
for (int i = 0; g_device_handle_list[i].func; i++) {
    if (!strncmp(func_name, g_device_handle_list[i].func_name, strlen(g_device_handle_list[i].func_name))) {
        status_code = g_device_handle_list[i].func(&device_data);
        MDF_LOGV("status_code: %d, func: %s, type: %x", status_code, func_name, data_type.val);
        break;
    }
}
```

* 设备回复指令执行结果：

设备根据命令执行结果 `status_code` 决定是否回复 `命令执行状态` 给远程控制端。

```c
mdf_wifi_mesh_send(&dest_addr, &data_type, device_data.response, device_data.response_size);
```

### 1.8. 创建 HTTP 服务器，UDP 服务器，开启 mDNS 服务

当节点成为根节点后，会创建以下任务和服务：

* `HTTP Server Task`：用于根节点和外部设备通信，包括 `mdf_http_request_task` 和`mdf_http_response_task`；
* `UDP Server Task`：用于发现设备，接收 UDP 广播包和回复；
* `UDP Client Task`：用于广播设备状态；
* `mDNS Device Discovery Service`：用于 app 发现本地局域网内的根节点设备。

关于代码详情请查看 `$MDF_PASTH/functions/mdf_server/mdf_server.c`。

```c
/**< receive http request from app or remote server */
xTaskCreate(mdf_http_request_task, "mdf_http_server_request", 4096, NULL, 6, NULL);

/**< send http response to app or remote server */
xTaskCreate(mdf_http_response_task, "mdf_http_server_response", 4096, NULL, 6, &g_http_response_task_handle);

/**< send notice(change of device's status) to lacal area network */
xTaskCreate(mdf_notice_udp_client_task, "mdf_udp_client", 3072, NULL, MDF_TASK_DEFAULT_PRIOTY, NULL);

/**< receive UDP broadcast from app for device discovery */
xTaskCreate(mdf_notice_udp_server_task, "mdf_udp_server", 2048, NULL, MDF_TASK_DEFAULT_PRIOTY, NULL);

/**< start mDNS service for device discovery in local area net */
mdf_mdns_init(void);
```

## 2. 添加新 ESP-MDF 设备类型步骤

在 ESP-MDF 设备程序中和具体设备类型紧密相关的部分有：`设备驱动` 和 `设备状态更新接口`。因此，当添加一个新设备时，需要修改这两方面的代码。

### 2.1. 创建工程

拷贝 ESP-MDF 某个 example 到全新的目录下，或者自己搭建一个工程结构。详细过程请参考 [Get Started](../get-started/get_started_cn.md)。

### 2.2. 编写驱动

编写设备驱动，完成设备所有功能的接口封装。以灯为例，包括灯各个引脚对应的串口初始化、颜色设置/读取接口、开关设置/读取接口、闪烁设置接口等。

```c
mdf_err_t mdf_light_init(gpio_num_t red_gpio, gpio_num_t green_gpio, gpio_num_t blue_gpio,
                         gpio_num_t brightness_gpio, gpio_num_t color_temperature_gpio);
mdf_err_t mdf_light_deinit();

mdf_err_t mdf_light_set_rgb(uint8_t red, uint8_t green, uint8_t blue);
int mdf_light_get_value();

......

mdf_err_t mdf_light_set_switch(uint8_t status);
int mdf_light_get_switch();

mdf_err_t mdf_light_breath_set(int period_ms);
mdf_err_t mdf_light_blink_start(uint8_t red, uint8_t green, uint8_t blue, int freq);
mdf_err_t mdf_light_blink_stop();
mdf_err_t mdf_light_blink_set(uint8_t red, uint8_t green, uint8_t blue, int freq, int blink_ms);
```

### 2.3. 确定设备 TID 和 CID，编写相应接口

详细说明请参考章节 1.2 `添加设备属性` 和章节 1.3 `注册状态更新接口`。下面的代码以灯为例，确定灯的属性、状态设置/获取接口：

```c
enum light_status_cid {
    STATUS_CID            = 0,
    HUE_CID               = 1,
    SATURATION_CID        = 2,
    VALUE_CID             = 3,
    COLOR_TEMPERATURE_CID = 4,
    BRIGHTNESS_CID        = 5,
};

static esp_err_t light_bulb_set_value(uint8_t cid, int value);

static esp_err_t light_bulb_get_value(uint8_t cid, int *value);
```

其中 API `light_set_value` 和 `light_get_value` 用于 app 或服务器控制端更新设备的状态。

### 2.4. 组织 app_main 执行流程

组织 `app_main` 函数流程。主要包括：配置设备信息、设备初始化、配置 ESP-MDF 参数，及注册回调函数。以下代码以灯为例：

```c
void app_main()
{
    ESP_ERROR_CHECK(light_init(GPIO_NUM_4, GPIO_NUM_16, GPIO_NUM_5, GPIO_NUM_19, GPIO_NUM_23));

    ESP_ERROR_CHECK(mdf_device_init_config(1, "light", "0.0.1"));

    ESP_ERROR_CHECK(mdf_device_add_characteristics(STATUS_CID, "on", PERMS_READ_WRITE_TRIGGER, 0, 1, 1));
    ESP_ERROR_CHECK(mdf_device_add_characteristics(HUE_CID, "hue", PERMS_READ_WRITE_TRIGGER, 0, 360, 1));
    ESP_ERROR_CHECK(mdf_device_add_characteristics(SATURATION_CID, "saturation", PERMS_READ_WRITE_TRIGGER, 0, 100, 1));
    ESP_ERROR_CHECK(mdf_device_add_characteristics(VALUE_CID, "value", PERMS_READ_WRITE_TRIGGER, 0, 100, 1));
    ESP_ERROR_CHECK(mdf_device_add_characteristics(COLOR_TEMPERATURE_CID, "color_temperature", PERMS_READ_WRITE_TRIGGER, 0, 100, 1));
    ESP_ERROR_CHECK(mdf_device_add_characteristics(BRIGHTNESS_CID, "brightness", PERMS_READ_WRITE_TRIGGER, 0, 100, 1));

    ESP_ERROR_CHECK(mdf_device_init_handle(light_bulb_event_loop_cb, light_bulb_get_value, light_bulb_set_value));
    ESP_ERROR_CHECK(mdf_trigger_init());
}
```

## 3. 根节点与 App 通信流程

根节点是 Mesh 网络和外部通信的唯一通道。节点在成为根节点后会首先建立 HTTP Server，用于接收外部连接及数据通信。App 与根节点之间的通信流程主要分为以下三步：

1. App 获取根节点的 IP 和 Port
2. App 询问根节点获得网络设备列表
3. App 与 Mesh 网络中设备之间互相通信

<div align=center>
<img src="../../_static/mesh_comm_procedure.png" width="600">
<p> ESP-MDF 通信协议 </p>
</div>

## 4. 定制化模块

ESP-IDF 和 ESP-MDF 项目中模块采用 [component](https://esp-idf.readthedocs.io/en/latest/api-guides/build-system.html#component-makefiles) 组织方式，使得多个不同项目和示例提供可以复用代码。同时，用户可以根据各种定制化的功能需求，修改项目中的不同模块（位于目录 `$IDF_PATH/components`，`$MDF_PATH/components`）。具体方法是在本项目的目录下建立一个类似于 component 的目录，然后将 ESP-IDF 或者 ESP-MDF 中的某个模块拷贝到该目录下，之后通过修改该模块代码达到定制化功能的作用。详细过程请参考 ESP-IDF [Build System](https://esp-idf.readthedocs.io/en/latest/api-guides/build-system.html#build-system)。

## 5. 资源

* ESP32 官方论坛请点击 [ESP32 BBS](https://esp32.com/)。
* [ESP-MDF](https://github.com/espressif/esp-mdf) 是基于乐鑫物联网开发框架 [ESP-IDF](https://github.com/espressif/esp-idf)，详情请参考 [ESP-IDF 文档](https://esp-idf.readthedocs.io)。
