[[中文]](../../zh_CN/get-started/get_started_cn.md)

# ESP-MDF Get Started

This document is intended to help users set up the software environment for the development of mesh applications using ESP32-based hardware produced by Espressif. Through a simple example, the document shows how to use ESP-MDF (Espressif Mesh Development Framework).

## Hardware Preparation

At least two [ESP32-DevKitC V4](https://esp-idf.readthedocs.io/en/latest/hw-reference/modules-and-boards.html#esp32-devkitc-v4) are needed to start development with the use of ESP-MDF.

Introduction to ESP32-DevKitC V4:

| Interface / Module | Description |
| ------------------ | :---------- |
| ESP32-WROOM-32 | [ESP32-WROOM-32 module](http://esp-idf.readthedocs.io/en/latest/hw-reference/modules-and-boards.html#esp-modules-and-boards-ESP32-WROOM-32) |
| EN           | reset button; pressing this button will reset the system|
| Boot         | download button; press this button and then the EN button, the system will enter download mode, and program the flash via the serial port |
| USB          | USB interface that provides power supply for the board and acts as the interface for connecting the PC and the ESP32-WROOM-32|
| IO           | lead out most of the pins of ESP32-WROOM-32 |

<div align=center>
<img src="../../_static/esp32_devkitc_v4_front.jpg" width="300">
<p> ESP32-DevKitC V4 </p>
</div>

## Get ESP-MDF

To get ESP-MDF, open terminal, navigate to the directory to put the ESP-MDF, and clone it using `git clone` command:

```sh
mkdir ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-mdf.git
```

ESP-MDF will be downloaded into `~/esp/esp-mdf`.

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

Use command `echo` to check whether the `MDF_PATH` has been correctly set:

```sh
echo $MDF_PATH
```

If it has been correctly set, it will output the path of ESP-MDF project directory, such as: `~/esp/esp-mdf`.

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

This will build the application including ESP-IDF/ESP-MDF components, upload binaries to your ESP32 board and start the monitor.

If there is no error in the process abovementioned, the RGBCW light will flash yellow, indicating that the device has entered the networking mode.

## Control ESP-MDF Device

The configuration of networking parameters for mesh lights, Bluetooth networking configuration, device control, and firmware upgrade can be done on the mobile phone app. To do so, users will need to:

1. Install the Mesh Android APK.
2. Configure the network on the app:
    * Add device
    * Enter configuration parameters
    * Network with the device
4. Get device information: return to the home page to check the list of all networked devices.
5. Mesh Light Control: color Control, upgrade, reboot, etc.
6. Networking mode: reboot the device for three consecutive times.

## Update ESP-MDF

After some time of using ESP-MDF, you may want to update it to take advantage of new features or bug fixes. The simplest way to do so is by deleting existing `esp-mdf` folder and cloning it again, which is same as when doing initial installation described in sections `Get ESP-MDF`.

Another solution is to update only what has changed. This method is useful if you have a slow connection to the GitHub. To do the update run the following commands:

```sh
cd ~/esp/esp-mdf
git pull
git submodule update --init --recursive
```

The `git pull` command fetches and merges changes from ESP-MDF repository on GitHub. Then `git submodule update --init --recursive` updates existing submodules or gets new ones. Since the submodules on GitHub are represented as links to other repositories, this command is needed to get them onto your PC.
