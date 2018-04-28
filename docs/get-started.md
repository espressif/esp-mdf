## ESP-MDF Get Started

This document is intended to help users set up the software environment for the development of mesh applications using hardware based on the ESP32 by Espressif. It illustrates how to use ESP-MDF (Espressif Mesh Development Framework) with a simple example.

## About ESP-MDF

The ESP-MDF is available as a set of components to extend the functionality already delivered by the [ESP-IDF](https://github.com/espressif/esp-idf) (Espressif IoT Development Framework).

## Get ESP-MDF

To get ESP-MDF, open terminal, navigate to the directory to put the ESP-MDF, and clone it using `git clone` command:

```sh
mkdir ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-mdf.git
```

ESP-MDF will be downloaded into `~/esp/esp-mdf`.

Below is a description of ESP-MDF project directory.

```
esp-mdf
├── build                                    // files generated through compiling
├── components
|   ├── base_components                      // base components in project
|   |   ├── bootloader                       // replace bootloader component in ESP-IDF(refer to README.md in this directory)
|   |   ├── mdf_event_loop                   // event loop component in application
|   |   ├── mdf_http_parse                   // http package parsing component
|   |   ├── mdf_info_store                   // nvs data store component
|   |   ├── mdf_json                         // json data processing component
|   |   ├── mdf_misc                         // miscellaneous component
|   |   ├── mdf_openssl                      // openssl encapsulation component
|   |   ├── mdf_ota                          // OTA encapsulation component
|   |   ├── mdf_reboot_event                 // reboot event management component
|   |   └── mdf_socket                       // socket encapsulation component
|   ├── device_handle                        // device handle component
|   |   ├── include
|   |   ├── component.mk
|   |   └── mdf_device_handle.c
|   ├── functions
|   |   ├── mdf_debug                        // debug function include espnow and console
|   |   ├── mdf_low_power                    // low power function
|   |   ├── mdf_network_config               // network configuration function
|   |   ├── mdf_server                       // server function
|   |   ├── mdf_sniffer                      // sniffer function
|   |   ├── mdf_trigger                      // device inter-connection function
|   |   └── mdf_upgrade                      // device upgrade function
|   └── protocol_stack
|       ├── mdf_espnow                       // esp-now protocol encapsulation component
|       └── mdf_wifi_mesh                    // esp-mesh protocol encapsulation component
├── docs
├── example
|   ├── factory                              // factory upgrade example
|   ├── light_bulb                           // light_bulb example
|   └── README.md
├── esp-idf                                  // project SDK is submodule of ESP-MDF
├── tools
|   ├── check_log.sh                         // log analysis script
|   ├── format.sh                            // code format script
|   ├── coredump_recv_server.py              // python script running in server to receive coredump data
|   ├── idf_monitor.py
|   ├── multi_downloads.sh                   // compiling and downloading script for multiple devices
|   ├── multi_open_serials.sh                // opening serial ports of multiple devices
|   └── setup_toolchain.sh                   // set up the compiling environment
├── LICENSE
├── project.mk                               // main Project Makefile
└── sdkconfig.defaults                       // project default configuration files
```

## Set up ESP Toolchain

To use ESP-MDF, you need set up the ESP-IDF first. Configure your PC according to [ESP32 Documentation](http://esp-idf.readthedocs.io/en/latest/?badge=latest). [Windows](http://esp-idf.readthedocs.io/en/latest/get-started/windows-setup.html), [Linux](http://esp-idf.readthedocs.io/en/latest/get-started/linux-setup.html) and [Mac OS](http://esp-idf.readthedocs.io/en/latest/get-started/macos-setup.html) operating systems are supported.

We suggest users develop ESP-MDF in Ubuntu. Run the script `setup_toolchain.sh` in Ubuntu to set up the development environment, including the installation of all SDK, the configuration of cross-compilation tool chains, and the configuration of system variables.

```sh
./tools/setup_toolchain.sh
```

You have a choice to compile and upload code to the ESP32 by command line with [make](http://esp-idf.readthedocs.io/en/latest/get-started/make-project.html) or using [Eclipse IDE](http://esp-idf.readthedocs.io/en/latest/get-started/eclipse-setup.html).

> We are using `~/esp` directory to install the toolchain, ESP-MDF and sample applications. You can use a different directory, but need to adjust respective commands.

If this is your first exposure to the ESP32 and [ESP-IDF](https://github.com/espressif/esp-idf), then it is recommended to get familiar with [hello_world](https://github.com/espressif/esp-idf/tree/master/examples/get-started/hello_world) and [blink](https://github.com/espressif/esp-idf/tree/master/examples/get-started/blink) examples first. Once you can build, upload and run these two examples, then you are ready to proceed to the next section.

## Set up Path to ESP-MDF

The toolchain accesses ESP-MDF via MDF_PATH environment variable. This variable should be set up on your PC, otherwise the projects will not build. The process to set it up is analogous to setting up the `IDF_PATH` variable, please refer to instructions in ESP-IDF documentation under [Add IDF_PATH to User Profile](https://esp-idf.readthedocs.io/en/latest/get-started/add-idf_path-to-profile.html).

Use command `echo` to check whether the `MDF_PATH` has been correctly setted:

```sh
echo $MDF_PATH
```

If it has been correctly setted, it will output the path of esp-mdf project directory, such as: `~/esp/esp-mdf`.

## Start a Project

After initial preparation you are ready to build the first mesh application for the ESP32. To demonstrate how to build an application, we will use `light_bulb` project from examples directory in the MDF.

Copy `examples/light_bulb` to `~/esp` directory:

```sh
cd ~/esp
cp -r $MDF_PATH/examples/light_bulb .
```

In the `light_bulb` example, we lead out six pins of ESP32 DevKitC V4 and connect them to an RGBCW light respectively to show the controlling effect of the lights (color and status):

| Pin         | GND  | IO4  | IO16  | IO5  | IO19              | IO23       |
| :---------- | :--- | :--- | :---  | :--- | :---------------- | :--------- |
| Description | GND  | Red  | Green | Blue | Color Temperature | Brightness |

## Connect and Configure

Connect the ESP32 board to the PC, check under what serial port the board is visible and verify if serial communication works as described in [ESP-IDF Documentation](https://esp-idf.readthedocs.io/en/latest/get-started/index.html#connect).

At the terminal window, go to the directory of `light-bulb` application and configure it with `menuconfig` by selecting the serial port and upload speed:

```sh
cd ~/esp/light_bulb
make menuconfig
```

Save the configuration.

## Build, Flash and Monitor

Now you can build, upload and check the application. Run:

```sh
make erase_flash flash monitor -j5
```

This will build the application including ESP-IDF / ESP-MDF components, upload binaries to your ESP32 board and start the monitor.

```
rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:2
load:0x3fff0028,len:4
load:0x3fff002c,len:4660
load:0x40078000,len:0
load:0x40078000,len:14536
entry 0x40078ed4
I (634) cpu_start: Pro cpu up.
I (634) cpu_start: Single core mode
I (634) heap_init: Initializing. RAM available for dynamic allocation:
I (638) heap_init: At 3FFAFF10 len 000000F0 (0 KiB): DRAM
I (644) heap_init: At 3FFD18B8 len 0000E748 (57 KiB): DRAM
I (650) heap_init: At 3FFE0440 len 00003BC0 (14 KiB): D/IRAM
I (656) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (663) heap_init: At 40093BF0 len 0000C410 (49 KiB): IRAM
I (669) cpu_start: Pro cpu start user code
I (16) cpu_start: Starting scheduler on PRO CPU.
I (50) wifi: wifi firmware version: 2fc15c0
I (51) wifi: config NVS flash: enabled
I (55) wifi: config nano formating: disabled
I (59) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (68) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (92) wifi: Init dynamic tx buffer num: 32
I (92) wifi: Init data frame dynamic rx buffer num: 32
I (92) wifi: Init management frame dynamic rx buffer num: 32
I (95) wifi: wifi driver task: 3ffd9f74, prio:23, stack:4096
I (101) wifi: Init static rx buffer num: 10
I (104) wifi: Init dynamic rx buffer num: 32
W (109) phy_init: failed to load RF calibration data (0x1102), falling back to full calibration
I (405) phy: phy_version: 383.0, 79a622c, Jan 30 2018, 15:38:06, 0, 2
I (414) wifi: mode : sta (30:ae:a4:80:52:64) + softAP (30:ae:a4:80:52:65)
I (425) mdf_device_handle: [mdf_device_init_handle, 804]:******************* SYSTEM INFO *******************
I (436) mdf_device_handle: [mdf_device_init_handle, 805]:idf version      : v3.1-dev-726-gbae9709
I (445) mdf_device_handle: [mdf_device_init_handle, 808]:device version   : light_805264-0.0.1-1
I (455) mdf_device_handle: [mdf_device_init_handle, 809]:compile time     : Apr 19 2018 12:10:00
I (464) mdf_device_handle: [mdf_device_init_handle, 810]:free heap        : 123308 B
I (472) mdf_device_handle: [mdf_device_init_handle, 811]:CPU cores        : 2
I (480) mdf_device_handle: [mdf_device_init_handle, 814]:function         : WiFi/BT/BLE
I (489) mdf_device_handle: [mdf_device_init_handle, 815]:silicon revision : 0
I (497) mdf_device_handle: [mdf_device_init_handle, 817]:flash            : 4 MB external
I (506) mdf_device_handle: [mdf_device_init_handle, 818]:***************************************************
I (516) ESPNOW: espnow [version: 1.0] init
I (527) light_bulb: [light_bulb_event_loop_cb, 128]:***********************************
I (536) light_bulb: [light_bulb_event_loop_cb, 129]:*    ENTER CONFIG NETWORK MODE    *
I (545) light_bulb: [light_bulb_event_loop_cb, 130]:***********************************
I (553) BTDM_INIT: BT controller compile version [ae44fa3]

I (560) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (839) mdf_blufi_network_config: [mdf_blufi_init, 525]:bt addr: 30:ae:a4:80:52:66
I (840) mdf_blufi_network_config: [mdf_blufi_init, 526]:blufi version: 0102
I (847) mdf_blufi_network_config: [mdf_blufi_event_callback, 376]:blufi init finish
I (853) mdf_blufi_network_config: [mdf_blufi_set_name, 287]:mdf blufi name: MESH_1_805266
I (1480) mdf_network_config: [mdf_network_handle, 270]:generate RSA public and private keys
```

If there is no error in the abovementioned process, the RGBCW light will flash yellow, indicating that the device has entered the networking mode.

## Control ESP-MDF Device

The configuration of networking parameters for mesh lights, Bluetooth networking configuration, device control, and firmware upgrade can be done on the mobile phone app. To do so, users will need to:

1. Install the Mesh Android APK.
2. Configure the network on the app:
    * Add device
    * Enter configuration parameters
    * Network with the device
4. Get device information: return to the home page to check the list of all networked devices.
5. Mesh light control: color control, upgrade, reboot, etc.
6. Networking mode: reboot the device for three consecutive times.

## Update ESP-MDF

After some time of using ESP-MDF, you may want to update it to take advantage of new features or bug fixes. The simplest way to do so is by deleting existing `esp-mdf` folder and cloning it again, which is same as when doing initial installation described in sections `Get ESP-MDF`.

Another solution is to update only what has changed. This method is useful if you have a slow connection to the GitHub. To do the update run the following commands:

```sh
cd ~/esp/esp-mdf
git pull
git submodule update --init --recursive
```

The `git pull` command is fetching and merging changes from ESP-MDF repository on GitHub. Then `git submodule update --init --recursive` is updating existing submodules or getting a fresh copy of new ones. On GitHub the submodules are represented as links to other repositories and require this additional command to get them onto your PC.

## Related Documents

* For details of the ESP32 development board, please refer to [ESP32 DevKitC V4](https://esp-idf.readthedocs.io/en/latest/hw-reference/modules-and-boards.html#esp32-devkitc-v4).

* [ESP-IDF Programming Guide](https://esp-idf.readthedocs.io/en/latest/) is the documentation for Espressif IoT Development Framework.
