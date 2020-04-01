[[中文]](./README_cn.md)

# ESP32-MeshKit-Light

## Overview

[ESP32-MeshKit-Light](https://www.espressif.com/sites/default/files/documentation/esp32-meshkit-light_user_guide_en.pdf) is a smart lighting solution based on [ESP-WIFI-MESH](https://docs.espressif.com/projects/esp-idf/en/stable/api-guides/mesh.html). The ESP-MeshKit solution features network configuration, upgrade, local control, device association, etc.

ESP32-MeshKit-Light consists of light bulbs with integrated ESP32 chips. The kit will help you better understand ESP-WIFI-MESH features and how to further develop ESP-Meshkit-Light. Before reading this document, please refer to [ESP32-MeshKit Guide](../README.md).

> **Note**: This demo is not limited to ESP32-MeshKit-Light. It can also be used for an ESP32 module connected to an external LED.


## Hardware

The board integrated into ESP32-MeshKit-Light supports 5 types of PWM IO interfaces. The Light's color temperature (CW) and hue (RGB) can be adjusted with the output power of 9 W and 3.5 W respectively.

### 1. Inside View and Pin Layout

<div align=center>
<img src="../docs/_static/light_module.png"  width="400">
<p> Inside View </p>
</div>

### 2. Pin Definition

|No. | Name | Type | Description |
| :----: | :----: | :----: |:---- |
|1, 7| GND| P |   Ground   |
|2 | CHIP_PU| I | Chip enabling (High: On); module internal pull-up; alternative for external enabling |
|3 | GPIO32 | I/O | RTC 32K_XP (32.768 kHz crystal oscillator input); alternative for function expansion|
|4 | GPIO33 | I/O | RTC 32K_XN (32.768 kHz crystal oscillator output); alternative for function expansion|
|5 | GPIO0 | I/O| IC internal pull-up; alternative for function expansion |
|6 | VDD3.3V | P | Power supply, 3V3   |
|8 | GPIO4 | O | PWM_R output control |
|9 | GPIO16 | O | PWM_G output control; alternate UART interface (URXD); GPIO16 and GPIO17 can not be used in modules that are integrated with PSRAM |
|10 | GPIO5 | O | PWM_B output control; alternate UART interface (UTXD) |
|11 | GPIO23 | O | PWM_BR output control |
|12 | GPIO19 | O | PWM_CT output control |
|13 | GPIO22 | I/O | Shared by PWM; alternative for function expansion |
|14 | GPIO15 | I | IC internal pull-up; alternative for function expansion |
|15 | GIPO2 | O | IC internal pull-down; alternative for function expansion |
|16 | UORXD | I/O | UART interface for debugging and receive end in software downloading |
|17 | UOTXD | I/O | UART interface for debugging and transmit end in software downloading |
|19 | ANT | I/O | External antenna output |
|18, 20 | GND | P | RF ground |


## Statuses Represented by Light Colors

To bring ESP32-MeshKit-Light into network configuration mode, turn it off and on for three consecutive times.

| Light Color | Status |
| ----------- | ------ |
| Yellow (breathing) | Waiting to be configured, in network configuration mode. |
| Orange (breathing) | Connecting to the router to verify the network configuration information received from ESP-Mesh App. |
| Green (breathing) | Router information verified successfully and is being sent to other whitelisted devices |
| White (solid) | Networked successfully |
| Light blue (breathing for 3 seconds) | Starting to upgrade |
| Blue (breathing) | Upgraded successfully and waiting to restart|
| Red (solid) | Abnormal reboot |


## Project Overview

### Introduction

ESP-WIFI-MESH is a networking protocol built atop the Wi-Fi protocol. ESP-WIFI-MESH allows numerous devices (henceforth referred to as nodes) spread over a large physical area (both indoors and outdoors) to be interconnected under a single WLAN (Wireless Local-Area Network). ESP-WIFI-MESH is self-organizing and self-healing meaning the network can be built and maintained autonomously.

Light project realizes the following features:

 - [Building an ESP-WIFI-MESH Network](https://docs.espressif.com/projects/esp-idf/en/stable/api-guides/mesh.html#building-a-network): involves selecting a root node, then forming downstream connections layer by layer until all nodes have joined the network. 
 - [Mesh Network Configuration](https://docs.espressif.com/projects/esp-mdf/en/latest/api-guides/mconfig.html): sends network configuration information to ESP-WIFI-MESH devices in a convenient and efficient manner.
 - [Mesh Upgrade](https://docs.espressif.com/projects/esp-mdf/en/latest/api-guides/mupgrade.html): implements efficient upgrading of ESP-WIFI-MESH devices via automatic retransmission of failed fragments, data compression, reverting to an earlier version and firmware check.
 - [Communicating via LAN](https://docs.espressif.com/projects/esp-mdf/zh_CN/latest/api-guides/mlink.html): controls ESP-WIFI-MESH network devices through App, including: device discovery, control, upgrade, etc. Prerequisite: the mobile phone and mesh network are on the same LAN.

### Code Analysis

For your better understanding of the implementation of the Light project, this section provides a detailed analysis of the code used in this project.

#### Project Structure

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

 - `light.c` contains the following main application code, which is necessary to implement ESP-WIFI-MESH.
    - Code to initialize Wi-Fi stack
    - Code to initialize ESP-WIFI-MESH stack
    - Code to initialize ESP-NOW
    - Code to initialize LED driver
    - Code to configure LAN communication
    - Code to initialize trigger handler

#### Source Code Analysis

Once ESP32 system is initialized, `app_main` will be called. The following code block shows the main implementation of `app_main` function.

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

Including the following code:

 - `mdf_event_loop_init(event_loop_cb)`: initializes event handler callback function, and all events will be sent to this function.
 - `wifi_init()`: initializes Wi-Fi stack.
 - `mespnow_init()`: initializes ESP-NOW.
 - `light_driver_init(&driver_config)`: initializes LED driver.
 - `get_network_config(&init_config, &ap_config)`: gets network configuration information.
 - `mlink_add_device(LIGHT_TID, name, CONFIG_LIGHT_VERSION)`: adds a device to LAN communication module.
 - `mlink_trigger_init()`: initializes trigger handler module.
 - `mwifi_init(&init_config)`: initializes ESP-WIFI-MESH stack.
 - `mwifi_set_config(&ap_config)`: sets parameters for ESP-WIFI-MESH.
 - `mwifi_start()`: enables ESP-WIFI-MESH.
 - `xTimerCreate("show_system_info", 10000 / portTICK_RATE_MS, true, NULL, show_system_info_timercb)`: creates a Timer to print system information periodically.

##### 1. Initialize Wi-Fi Stack

This section introduces the code used to initialize Wi-Fi stack in details.

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

Including the following code:

 - `nvs_flash_init()`: initializes nvs flash before enabling Wi-Fi.
 - `tcpip_adapter_init()`: initializes TCP/IP stack.
 - `esp_wifi_init(&cfg)`: initializes Wi-Fi.
 - `esp_wifi_start()`: starts Wi-Fi according to current configuration information.

##### 2. Get Network Configuration Information

You can get such information in three ways:

 - Load from nvs flash
 - Get from [Mconfig-BluFi](https://docs.espressif.com/projects/esp-mdf/en/latest/api-guides/mconfig.html#mconfig-blufi)
 - Get from [Mconfig-Chain](https://docs.espressif.com/projects/esp-mdf/en/latest/api-guides/mconfig.html#mconfig-chain)

In this section we are going to introduce the last two methods:

1. Get from [Mconfig-BluFi](https://docs.espressif.com/projects/esp-mdf/en/latest/api-guides/mconfig.html#mconfig-blufi).

Including the following code:

 - `mconfig_blufi_security_init`: initializes BluFi encryption.
 - `esp_bt_controller_init` and `esp_bt_controller_enable`: initialize and enable Bluetooth Controller.
 - `esp_bluedroid_init` and `esp_bluedroid_enable`: initialize and enable Bluetooth Host.
 - `esp_ble_gap_register_callback`: registers GAP callback function.
 - `esp_blufi_register_callbacks`: registers BluFi callback function.
 - `esp_blufi_profile_init`: initializes BluFi profile.

2. Get from [Mconfig-Chain](https://docs.espressif.com/projects/esp-mdf/en/latest/api-guides/mconfig.html#mconfig-chain)

Mconfig-Chain splits devices into two types communicating with each other via ESP-NOW:

 - Master: a device that initiates a connection and sends network configuration information to a slave device. 
 - Slave: a device that accepts a connection request from a master and sends the request for network configuration information to a master.

**Slave:** 

Including the following code:

 - `esp_wifi_set_promiscuous_rx_cb(wifi_sniffer_cb)`: registers sniffer callback function, and listens for IEEE802.11 Wi-Fi packets nearby.
 - `scan_mesh_device`: scans surrounding masters. 
 - `mespnow_write(MESPNOW_TRANS_PIPE_MCONFIG, dest_addr, espnow_data, espnow_size, portMAX_DELAY)`: sends the request for network configuration information to a master.
 - `mespnow_read(MESPNOW_TRANS_PIPE_MCONFIG, dest_addr, espnow_data, &espnow_size, 1000 / portTICK_RATE_MS)`: receives encrypted network configuration information from the master.
 - `mespnow_read(MESPNOW_TRANS_PIPE_MCONFIG, src_addr, whitelist_compress_data, (size_t *)&whitelist_compress_size, 10000 / portTICK_RATE_MS)`: receives compressed and encrypted whitelist from the master.
 - `mconfig_queue_write(&chain_data->mconfig_data, 0)`: sends network configuration information to the queue and marks the completion of the network configuration of the device.

**Master:**

Including the following code:

 - `esp_wifi_set_vendor_ie(true, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_1, &ie_data)`: sets IEEE802.11 vendor information element into beacon frames to identify this device as the master in mesh network configuration chain.
 - `mespnow_read(MESPNOW_TRANS_PIPE_MCONFIG, src_addr, espnow_data, &espnow_size, MCONFIG_CHAIN_EXIT_DELAY / portTICK_RATE_MS)`: receives network configuration request from a slave.
 - `mconfig_device_verify(mconfig_data->whitelist_data, mconfig_data->whitelist_size, src_addr, pubkey_pem)`: checks whether this slave is whitelisted, if not, the device cannot connect to this ESP-WIFI-MESH network.
 - `mespnow_write(MESPNOW_TRANS_PIPE_MCONFIG, src_addr, espnow_data, (MCONFIG_RSA_CIPHERTEXT_SIZE - MCONFIG_RSA_PLAINTEXT_MAX_SIZE) + sizeof(mconfig_chain_data_t), portMAX_DELAY)`: sends encrypted network configuration information to the slave device.
 - `mespnow_write(MESPNOW_TRANS_PIPE_MCONFIG, src_addr, whitelist_compress_data, whitelist_compress_size, portMAX_DELAY);`: sends compressed and encrypted whitelist to the slave device.

##### 3. LAN Communication Module

Including the following code:

 - `mlink_add_device(LIGHT_TID, name, CONFIG_LIGHT_VERSION)`: adds a device.
 - `mlink_add_characteristic(LIGHT_CID_STATUS, "on", CHARACTERISTIC_FORMAT_INT, CHARACTERISTIC_PERMS_RWT, 0, 3, 1)`: adds device characteristic information.
 - `mlink_add_characteristic_handle(mlink_get_value, mlink_set_value)`: adds characteristic handler function for a device.

##### 4. Trigger Handler

Including the following code:

 - `mlink_trigger_init()`: initializes trigger handler.
 - `xTaskCreate(trigger_handle_task, "trigger_handle", 1024 * 3,  NULL, 1, NULL)`: creates trigger handler task.
 - `mlink_trigger_handle(MLINK_COMMUNICATE_MESH)`: conducts corresponding operation according to the trigger which is configured by App or by calling `mlink_trigger_add()`.

##### 5. Initialize ESP-WIFI-MESH Protocol

Including the following code:

 - `mwifi_init(&init_config)`: initializes ESP-WIFI-MESH.
 - `mwifi_set_config(&ap_config)`: sets ESP-WIFI-MESH configuration information.
 - `mwifi_start()`: launches ESP-WIFI-MESH.

##### 6. Event Handler

 - `mdf_event_loop_init(event_loop_cb)`: registers event callback function, and send all the events to this function.
 - An event indicates the current status of the device, for instance, the  device as a root node gets IP address will trigger `MDF_EVENT_MWIFI_ROOT_GOT_IP` event.

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

##### 7. Node Task

 - `xTaskCreate(request_handle_task, "request_handle", 8 * 1024, NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);`: creates node data processing task.
 - `xTaskCreate(root_write_task, "root_write", 4 * 1024, NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, &g_root_write_task_handle);`: newly created root node transmits ESP-WIFI-MESH data packets to external IP network, and to various target addresses according to the data types.
 - `xTaskCreate(root_read_task, "root_read", 8 * 1024, NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, &g_root_read_task_handle)`: newly created root node transmits data packets from external IP network to ESP-WIFI-MESH.

1. Root node transmits data packets from external IP network to ESP-WIFI-MESH: 

 - first identify whether this node is the root;
 - read data from external IP network;
 - send data to the devices listed in address list.

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

2. Node processes ESP-WIFI-MESH data:

 - first identify whether this node has been connected to ESP-WIFI-MESH network;
 - read the target address into its own data packet;
 - check whether this data packet is a firmware upgrade packet, if so, the device will conduct firmware upgrade;
 - check whether this packet belongs to LAN communication module, if so, the node will conduct corresponding operation;
 - check whether this packet is from non-root node devices, if so, the node transmits this packet to root node.

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
