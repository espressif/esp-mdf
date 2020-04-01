[[EN]](./README.md)

# ESP32-MeshKit-Light

## 概述

ESP32-MeshKit-Light 是基于 [ESP-WIFI-MESH](https://docs.espressif.com/projects/esp-idf/en/stable/api-guides/mesh.html) 应⽤的智能电灯（板载 ESP32 芯片），包含配网、升级、本地控制和设备联动等功能，它将帮助您更好地了解 ESP-WIFI-MESH 的相关特性，并对 ESP32-MeshKit-Light 的程序进⾏⼆次开发。在运行本示例之前，请先详细阅读 [ESP32-MeshKit 指南](../README_cn.md)。

> 注：本示例不限于 ESP32-MeshKit-Light，您可以直接使用 ESP32 模块外接 LED 灯使用。

## 硬件说明

[ESP32-MeshKit-Light](https://www.espressif.com/sites/default/files/documentation/esp32-meshkit-light_user_guide_cn.pdf) 支持 5 路 PWM IO 口，支持色温（CW）灯和彩色（RGB）灯调节，色温灯的输出功率为 9 W，彩色灯的输出功率为 3.5 W。

1. 内部模组视图和引脚排序

<div align=center>
<img src="../docs/_static/light_module.png"  width="400">
<p> 内部模组视图 </p>
</div>

2. 引脚定义

|模块脚号 | 符号 | 状态 | 描述 | 
|:---:|:---:|:---:|:---|
|  1, 7 | GND | P| 接地脚|
| 2| CHIP_PU| I| 芯片使能（高电平有效），模块内部有上拉，为外部使能备用|
| 3 | GIPO32| I/O|RTC 32K_XP（32.768 kHz 晶振输入），功能扩展备用脚 | 
| 4 | GPIO33| I/O|RTC 32K_XN（32.768 kHz 晶振输入），功能扩展备用脚 |
| 5|GIPO0| I/O|IC 内部上拉，功能扩展备用脚 |
| 6| VDD3.3V|P | 模块供电脚，3V3|
| 8 | GPIO4|O | PWM_R 输出控制脚|
| 9 | GPIO16|O|PWM_G 输出控制脚，备用 UART 接口 (URXD)(注意：在包含 PSRAM 的模组上不能使用 GPIO16、17 引脚)|
| 10 | GPIO5|O|PWM_B 输出控制脚，备用 UART 接口 (UTXD)|
| 11 | GPIO23|O |PWM_BR 输出控制脚 |
| 12 | GPIO19|O |PWM_CT 输出控制脚 |
| 13 | GPIO22| I/O|PWM 共用脚，功能扩展备用脚 |
| 14 | GPIO15| I|IC 内部上拉，功能扩展备用脚 |
| 15 | GPIO2| O|IC 内部下拉，功能扩展备用脚 |
| 16 | UORXD| I/O|UART 调试和软件烧录接口接收脚 |
| 17 | UOTXD| I/O|UART 调试和软件烧录接口发送脚|
| 19 | ANT| I/O| 外置天线输出脚 |
| 18, 20| GND | P| RF 接地|


## 指示灯状态

连续通断电三次（关 > 开 > 关 > 开 > 关 > 开），可以使 ESP32-MeshKit-Light 进入配网模式。

| 指示灯颜色 | 状态 |
| ----------- | ------ |
| 黄色（呼吸）  | 进入配网模式，等待组网 |
| 橙色（呼吸） | 连接路由器，校验从 ESP-WIFI-MESH App 接收到的配网信息 |
| 绿色（呼吸） | 路由器信息校验成功，并发送给其它白名单设备 |
| 白色（常亮）| 组网成功 |
| 淡蓝色（呼吸三秒） | 开始升级 |
| 蓝色（呼吸） | 升级成功等待重启 |
| 红色（常亮） | 异常重启 |

## 工程介绍

### 简介

ESP-WIFI-MESH 是一种基于 Wi-Fi 协议构建的网络协议。ESP-WIFI-MESH 允许在大的物理区域（室内和室外）上分布的多个设备（以下称为节点）在单个 WLAN（无线局域网）下互连。ESP-WIFI-MESH 具有自组织和自我修复功能，意味着网络可以自主构建和维护。

Light 工程包含以下功能：

 - [构建 ESP-WIFI-MESH 网络](https://docs.espressif.com/projects/esp-idf/en/stable/api-guides/mesh.html#building-a-network)：ESP-WIFI-MESH 网络构建过程涉及根节点选择，然后逐层形成下游连接，直到所有节点都加入网络。
 - [网络配置](https://docs.espressif.com/projects/esp-mdf/zh_CN/latest/api-guides/mconfig.html#)：目的是将配置信息便捷、高效地传递给 ESP-WIFI-MESH 设备。
 - [固件升级](https://docs.espressif.com/projects/esp-mdf/zh_CN/latest/api-guides/mupgrade.html)：目的是通过断点续传、数据压缩、版本回退和固件检查等机制实现 ESP-WIFI-MESH 设备高效的升级。
 - [局域网通信](https://docs.espressif.com/projects/esp-mdf/zh_CN/latest/api-guides/mlink.html)：通过APP控制 ESP-WIFI-MESH 网络设备，包括：设备发现，控制，升级等。

### 代码分析

为了更好地理解 light 工程的实现，本节将对工程 light 中代码的详细分析。

#### 工程结构

```
examples/development_kit/light/
├── CMakeLists.txt /* Cmake compiling parameters for the demo */
├── components /* Contains the components used by the demo */
│   └── light_driver /* light driver component */
│   └── light_handle /* light status handle component */
│   └── mesh_utils /* mesh utils component */
├── main /* Stores the main `.c` and `.h` application code files for this demo */
│   ├── Kconfig.projbuild /* Demo configuration file */
│   └── light.c /* main application codes, more info below */
├── Makefile /* Make compiling parameters for the demo */
├── partitions.csv /* Partition table file */
├── README_cn.md /* Quick start guide */
├── README.md /* Quick start guide */
├── sdkconfig /* Current parameters of `make menuconfig` */
├── sdkconfig.defaults /* Default parameters of `make menuconfig` */
└── sdkconfig.old /* Previously saved parameters of `make menuconfig` */
```

 - `light.c`：包含以下主要的应用代码，这些代码是实现 ESP-WIFI-MESH 所必需的。
    - 初始化 Wi-Fi 协议栈
    - 初始化 ESP-WIFI-MESH 协议栈
    - 初始化 ESP-NOW
    - 初始化 LED 驱动
    - 局域网通信配置
    - 设备关联任务初始化

#### 源码分析

ESP32 系统初始化完成后，将调用 `app_main`。下面的代码块展示了 `app_main` 函数的主要实现。

```
void app_main()
{
    ……
    ……
    ……

    /**
     * @brief Continuous power off and restart more than three times to reset the device
     */
    if (restart_count_get() >= LIGHT_RESTART_COUNT_RESET) {
        MDF_LOGW("Erase information saved in flash");
        mdf_info_erase(MDF_SPACE_NAME);
    }

    /**
     * @brief   1.Initialize event loop, receive event
     *          2.Initialize wifi with station mode
     *          3.Initialize espnow(ESP-NOW is a kind of connectionless WiFi communication protocol)
     */
    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));
    MDF_ERROR_ASSERT(wifi_init());
    MDF_ERROR_ASSERT(mespnow_init());

    /**
     * @brief Light driver initialization
     */
    MDF_ERROR_ASSERT(light_driver_init(&driver_config));

    /**
     * @brief   1.Get Mwifi initialization configuration information and Mwifi AP configuration information from nvs flash.
     *          2.If there is no network configuration information in the nvs flash,
     *              obtain the network configuration information through the blufi or mconfig chain.
     *          3.Indicate the status of the device by means of a light
     */
    if (mdf_info_load("init_config", &init_config, sizeof(mwifi_init_config_t)) == MDF_OK
            && mdf_info_load("ap_config", &ap_config, sizeof(mwifi_config_t)) == MDF_OK) {
        if (restart_is_exception()) {
            light_driver_set_rgb(255, 0, 0); /**< red */
        } else {
            light_driver_set_switch(true);
        }
    } else {
        light_driver_breath_start(255, 255, 0); /**< yellow blink */
        MDF_ERROR_ASSERT(get_network_config(&init_config, &ap_config, LIGHT_TID, LIGHT_NAME));
        MDF_LOGI("mconfig, ssid: %s, password: %s, mesh_id: " MACSTR,
                 ap_config.router_ssid, ap_config.router_password,
                 MAC2STR(ap_config.mesh_id));
    }

    /**
     * @brief Configure MLink (LAN communication module)
     */
    MDF_ERROR_ASSERT(esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac));
    snprintf(name, sizeof(name), "light_%02x%02x", sta_mac[4], sta_mac[5]);
    MDF_ERROR_ASSERT(mlink_add_device(LIGHT_TID, name, CONFIG_LIGHT_VERSION));
    MDF_ERROR_ASSERT(mlink_add_characteristic(LIGHT_CID_STATUS, "on", CHARACTERISTIC_FORMAT_INT, CHARACTERISTIC_PERMS_RWT, 0, 3, 1));
    ……
    MDF_ERROR_ASSERT(mlink_add_characteristic_handle(mlink_get_value, mlink_set_value));

    /**
     * @brief Initialize trigger handler
     *          while characteristic of device reaching conditions， will trigger the corresponding action.
     */
    MDF_ERROR_ASSERT(mlink_trigger_init());

    /**
     * @brief Initialize espnow_to_mwifi_task for forward esp-now data to the wifi mesh network.
     * esp-now data from button or other device.
     */
    xTaskCreate(espnow_to_mwifi_task, "espnow_to_mwifi", 1024 * 3,  NULL, 1, NULL);

    /**
     * @brief Add a request handler, handling request for devices on the LAN.
     */
    MDF_ERROR_ASSERT(mlink_set_handle("show_layer", light_show_layer));

    /**
     * @brief Initialize and start esp-mesh network according to network configuration information.
     */
    MDF_ERROR_ASSERT(mwifi_init(&init_config));
    MDF_ERROR_ASSERT(mwifi_set_config(&ap_config));
    MDF_ERROR_ASSERT(mwifi_start());

    /**
     * @brief Handling data between wifi mesh devices.
     */
    xTaskCreate(node_handle_task, "node_handle", 8 * 1024,
                NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);

    /**
     * @brief Periodically print system information.
     */
    TimerHandle_t timer = xTimerCreate("show_system_info", 10000 / portTICK_RATE_MS,
                                       true, NULL, show_system_info_timercb);
    xTimerStart(timer, 0);
}
```

包含以下代码：

 - `mdf_event_loop_init(event_loop_cb)`：事件处理回调函数初始化，所有的事件都将发送到这个函数中
 - `wifi_init()`：初始化 Wi-Fi 协议栈
 - `mespnow_init()`：初始化 ESP-NOW
 - `light_driver_init(&driver_config)`：初始化 led 驱动
 - `get_network_config(&init_config, &ap_config)`：获取网络配置信息
 - `mlink_add_device(LIGHT_TID, name, CONFIG_LIGHT_VERSION)`：添加设备到局域网控制模块
 - `mlink_trigger_init()`：初始化设备关联模块
 - `mwifi_init(&init_config)`：初始化 ESP-WIFI-MESH 协议栈
 - `mwifi_set_config(&ap_config)`：设置 ESP-WIFI-MESH 参数
 - `mwifi_start()`：使能 ESP-WIFI-MESH
 - `xTimerCreate("show_system_info", 10000 / portTICK_RATE_MS, true, NULL, show_system_info_timercb)`：新建 Timer 用于周期打印系统信息

##### 1. 初始化 Wi-Fi 协议栈

这一节将介绍用于初始化 Wi-Fi 协议栈的代码。

```
static mdf_err_t wifi_init()
{
    mdf_err_t ret          = nvs_flash_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        MDF_ERROR_ASSERT(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    MDF_ERROR_ASSERT(ret);

    tcpip_adapter_init();
    MDF_ERROR_ASSERT(esp_event_loop_init(NULL, NULL));
    MDF_ERROR_ASSERT(esp_wifi_init(&cfg));
    MDF_ERROR_ASSERT(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    MDF_ERROR_ASSERT(esp_wifi_set_mode(WIFI_MODE_STA));
    MDF_ERROR_ASSERT(esp_wifi_set_ps(WIFI_PS_NONE));
    MDF_ERROR_ASSERT(esp_mesh_set_6m_rate(false));
    MDF_ERROR_ASSERT(esp_wifi_start());

    return MDF_OK;
}
```

包含以下代码：

 - `nvs_flash_init()`：在使能 Wi-Fi 之前必须初始化 nvs flash
 - `tcpip_adapter_init()`：初始化 TCP/IP 协议栈
 - `esp_wifi_init(&cfg)`：初始化 Wi-Fi 
 - `esp_wifi_start()` 根据当前配置启动 Wi-Fi

##### 2. 获取网络配置信息

获取网络配置信息的方式有三种：

 - 从 nvs flash 中加载
 - 通过[蓝牙网络配置协议(blufi)](https://docs.espressif.com/projects/esp-mdf/en/latest/api-guides/mconfig.html#mconfig-blufi)获取
 - 通过[链式配网](https://docs.espressif.com/projects/esp-mdf/en/latest/api-guides/mconfig.html#mconfig-chain)获取

主要介绍后面两种方式：

 1. 通过蓝牙网络配置协议(blufi)获取

包含以下代码：

 - `mconfig_blufi_security_init`：blufi 加密初始化
 - `esp_bt_controller_init` 和 `esp_bt_controller_enable`：初始化、使能蓝牙 Controller
 - `esp_bluedroid_init` 和 `esp_bluedroid_enable`：初始化、使能蓝牙 Host
 - `esp_ble_gap_register_callback`：注册 GAP 回调函数
 - `esp_blufi_register_callbacks`：注册 blufi 回调函数
 - `esp_blufi_profile_init`：blufi 配置文件初始化

 2. 通过链式配网获取

链式配网分为两种角色，两种设备间通过 ESP-NOW 通信：

 - 主设备：向请求网络配置信息的从设备配置发送网络配置信息
 - 从设备：向主设备请求网络配置信息的设备

**链式配网从设备：**

包含以下代码：

 - `esp_wifi_set_promiscuous_rx_cb(wifi_sniffer_cb)`：注册 sniffer 回调函数，监听环境中的 IEEE802.11 Wi-Fi 数据包
 - `scan_mesh_device`：扫描环境中的主设备
 - `mespnow_write(MESPNOW_TRANS_PIPE_MCONFIG, dest_addr, espnow_data, espnow_size, portMAX_DELAY)`：向主设备请求网络配置信息
 - `mespnow_read(MESPNOW_TRANS_PIPE_MCONFIG, dest_addr, espnow_data, &espnow_size, 1000 / portTICK_RATE_MS)`：从主设备获取加密后的网络配置信息
 - `mespnow_read(MESPNOW_TRANS_PIPE_MCONFIG, src_addr, whitelist_compress_data, (size_t *)&whitelist_compress_size, 10000 / portTICK_RATE_MS)`：从主设备接收压缩、加密后白名单列表
 - `mconfig_queue_write(&chain_data->mconfig_data, 0)`：发送网络配置信息到队列，同时标志该设备网络配置完成

**链式配网主设备：**

包含以下代码：

 - `esp_wifi_set_vendor_ie(true, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_1, &ie_data)`：设置 Beacon 包中的 IEEE802.11 供应商特定信息元素用以标识该设备是链式配网主设备
 - `mespnow_read(MESPNOW_TRANS_PIPE_MCONFIG, src_addr, espnow_data, &espnow_size, MCONFIG_CHAIN_EXIT_DELAY / portTICK_RATE_MS)`：接收从设备发来的网络配置请求
 - `mconfig_device_verify(mconfig_data->whitelist_data, mconfig_data->whitelist_size, src_addr, pubkey_pem)`：检测该从设备是否在白名单中，不在白名单中的设备无法加入该 ESP-WIFI-MESH 网络
 - `mespnow_write(MESPNOW_TRANS_PIPE_MCONFIG, src_addr, espnow_data, (MCONFIG_RSA_CIPHERTEXT_SIZE - MCONFIG_RSA_PLAINTEXT_MAX_SIZE) + sizeof(mconfig_chain_data_t), portMAX_DELAY)`：向从设备发送加密后的网络配置信息
 - `mespnow_write(MESPNOW_TRANS_PIPE_MCONFIG, src_addr, whitelist_compress_data, whitelist_compress_size, portMAX_DELAY);`：向从设备发送压缩、加密后的白名单列表

##### 3. 局域网控制模块

包含以下代码：

 - `mlink_add_device(LIGHT_TID, name, CONFIG_LIGHT_VERSION)`：添加设备
 - `mlink_add_characteristic(LIGHT_CID_STATUS, "on", CHARACTERISTIC_FORMAT_INT, CHARACTERISTIC_PERMS_RWT, 0, 3, 1)`：添加设备特征
 - `mlink_add_characteristic_handle(mlink_get_value, mlink_set_value)`：添加设备特征处理函数

##### 4. 设备关联模块

包含以下代码：

 - `mlink_trigger_init()`：设备关联模块初始化
 - `xTaskCreate(trigger_handle_task, "trigger_handle", 1024 * 3,  NULL, 1, NULL)`：新建设备关联模块处理任务
 - `mlink_trigger_handle(MLINK_COMMUNICATE_MESH)`：根据当前设备的特征是否满足设置的触发条件进行相应操作，触发条件由 APP 或调用 `mlink_trigger_add()` 进行配置

##### 5. 初始化 ESP-WIFI-MESH 协议栈

包含以下代码：

 - `mwifi_init(&init_config)`：初始化 ESP-WIFI-MESH
 - `mwifi_set_config(&ap_config)`：设置 ESP-WIFI-MESH 配置信息
 - `mwifi_start()`：启动 ESP-WIFI-MESH

##### 6. 事件处理

 - `mdf_event_loop_init(event_loop_cb)`：注册事件回调函数，之后所有的事件将发送到 `event_loop_cb` 函数
 - 不同的事件反映了设备当前所处的不同状态，例如：mesh 组网后，作为根节点的设备得到 IP 地址将触发 `MDF_EVENT_MWIFI_ROOT_GOT_IP` 事件

```
static mdf_err_t event_loop_cb(mdf_event_loop_t event, void *ctx)
{
    MDF_LOGI("event_loop_cb, event: 0x%x", event);
    mdf_err_t ret = MDF_OK;

    switch (event) {
        case MDF_EVENT_MWIFI_PARENT_DISCONNECTED:
            MDF_LOGI("Parent is disconnected on station interface");

            if (!esp_mesh_is_root()) {
                break;
            }

            ret = mlink_notice_deinit();
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_notice_deinit", mdf_err_to_name(ret));

            ret = mlink_httpd_stop();
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_httpd_stop", mdf_err_to_name(ret));
            break;

        case MDF_EVENT_MWIFI_ROOT_GOT_IP: {
            MDF_LOGI("Root obtains the IP address");

            ret = mlink_notice_init();
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_notice_init", mdf_err_to_name(ret));

            uint8_t sta_mac[MWIFI_ADDR_LEN] = {0x0};
            MDF_ERROR_ASSERT(esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac));

            ret = mlink_notice_write("http", strlen("http"), sta_mac);
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_httpd_write", mdf_err_to_name(ret));

            ret = mlink_httpd_start();
            MDF_ERROR_BREAK(ret != MDF_OK, "<%s> mlink_httpd_start", mdf_err_to_name(ret));

            if (!g_root_write_task_handle) {
                xTaskCreate(root_write_task, "root_write", 4 * 1024,
                            NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, &g_root_write_task_handle);
            }

            if (!g_root_read_task_handle) {
                xTaskCreate(root_read_task, "root_read", 8 * 1024,
                            NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, &g_root_read_task_handle);
            }

            break;
        }

        case MDF_EVENT_MLINK_SYSTEM_REBOOT:
            MDF_LOGW("Restart PRO and APP CPUs");
            esp_restart();
            break;

        case MDF_EVENT_MLINK_SET_STATUS:
            if (!g_event_group_trigger) {
                g_event_group_trigger = xEventGroupCreate();
            }

            xEventGroupSetBits(g_event_group_trigger, EVENT_GROUP_TRIGGER_HANDLE);
            break;

        case MDF_EVENT_MESPNOW_RECV:
            if ((int)ctx == MESPNOW_TRANS_PIPE_CONTROL) {
                xEventGroupSetBits(g_event_group_trigger, EVENT_GROUP_TRIGGER_RECV);
            }

            break;

        default:
            break;
    }

    return MDF_OK;
}
```

##### 7. 节点任务

 - `xTaskCreate(request_handle_task, "request_handle", 8 * 1024, NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);`：新建节点数据处理任务
 - `xTaskCreate(root_write_task, "root_write", 4 * 1024, NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, &g_root_write_task_handle);`：新建根节点向外部 IP 网络转发 ESP-WIFI-MESH 数据包，根据数据类型向不同的目的地址发送数据包
 - `xTaskCreate(root_read_task, "root_read", 8 * 1024, NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, &g_root_read_task_handle)`：新建根节点向 ESP-WIFI-MESH 网络转发外部 IP 网络数据包

 1. 根节点转发外部 IP 网络数据到 ESP-WIFI-MESH

 - 首先需要判断该节点是否是根节点
 - 从外部 IP 网络中读取数据
 - 向地址列表中的设备发送数据

```
static void root_read_task(void *arg)
{
    mdf_err_t ret               = MDF_OK;
    mlink_httpd_t *httpd_data   = NULL;
    mwifi_data_type_t data_type = {
        .compression = true,
        .communicate = MWIFI_COMMUNICATE_MULTICAST,
    };

    MDF_LOGI("root_read_task is running");

    while (mwifi_is_connected() && esp_mesh_get_layer() == MESH_ROOT) {
        if (httpd_data) {
            MDF_FREE(httpd_data->addrs_list);
            MDF_FREE(httpd_data->data);
            MDF_FREE(httpd_data);
        }

        ret = mlink_httpd_read(&httpd_data, portMAX_DELAY);
        MDF_ERROR_CONTINUE(ret != MDF_OK || !httpd_data, "<%s> mwifi_root_read", mdf_err_to_name(ret));
        MDF_LOGD("Root receive, addrs_num: %d, addrs_list: " MACSTR ", size: %d, data: %.*s",
                 httpd_data->addrs_num, MAC2STR(httpd_data->addrs_list),
                 httpd_data->size, httpd_data->size, httpd_data->data);

        memcpy(&data_type.custom, &httpd_data->type, sizeof(mlink_httpd_type_t));
        ret = mwifi_root_write(httpd_data->addrs_list, httpd_data->addrs_num,
                               &data_type, httpd_data->data, httpd_data->size, true);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_root_write", mdf_err_to_name(ret));
    }

    MDF_LOGW("root_read_task is exit");

    if (httpd_data) {
        MDF_FREE(httpd_data->addrs_list);
        MDF_FREE(httpd_data->data);
        MDF_FREE(httpd_data);
    }

    g_root_read_task_handle = NULL;
    vTaskDelete(NULL);
}
```

 2. 节点处理 ESP-WIFI-MESH 数据

 - 首先需要判断该节点是否已经是 ESP-WIFI-MESH 网络中的设备
 - 读取目的地址为自己的数据包
 - 判断该数据包类型是否为固件升级数据包，若是，则进行固件升级相关处理
 - 若是局域网控制类型，则进行局域网控制相关处理
 - 若是来自非根设备的数据，则向根节点转发该数据包

```
void request_handle_task(void *arg)
{
    mdf_err_t ret = MDF_OK;
    uint8_t *data = NULL;
    size_t size   = MWIFI_PAYLOAD_LEN;
    mwifi_data_type_t data_type      = {0x0};
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};

    for (;;) {
        if (!mwifi_is_connected()) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            continue;
        }

        size = MWIFI_PAYLOAD_LEN;
        MDF_FREE(data);
        ret = mwifi_read(src_addr, &data_type, &data, &size, portMAX_DELAY);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> Receive a packet targeted to self over the mesh network",
                           mdf_err_to_name(ret));

        if (data_type.upgrade) {
            ret = mupgrade_handle(src_addr, data, size);
            MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mupgrade_handle", mdf_err_to_name(ret));

            continue;
        }

        MDF_LOGI("Node receive, addr: " MACSTR ", size: %d, data: %.*s", MAC2STR(src_addr), size, size, data);

        mlink_httpd_type_t *httpd_type = (mlink_httpd_type_t *)&data_type.custom;

        ret = mlink_handle(src_addr, httpd_type, data, size);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mlink_handle", mdf_err_to_name(ret));

        if (httpd_type->from == MLINK_HTTPD_FROM_DEVICE) {
            data_type.protocol = MLINK_PROTO_NOTICE;
            ret = mwifi_write(NULL, &data_type, "status", strlen("status"), true);
            MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mlink_handle", mdf_err_to_name(ret));
        }
    }

    MDF_FREE(data);
    vTaskDelete(NULL);
}
```
