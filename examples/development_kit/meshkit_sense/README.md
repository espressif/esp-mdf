[[中文]](./README_cn.md)

# ESP32-MeshKit-Sense Example

This example shows the following:
 
 * How ESP32-MeshKit-Sense can control the associated [ESP32-MeshKit-Light](https://www.espressif.com/sites/default/files/documentation/esp32-meshkit-light_user_guide_en.pdf) according to the readings from onboard sensors (detected sensor status).
 
 * Demonstrates the low-power sensor solution powered by the ESP32 chip that allows ULP (Ultra Low Power) co-processor to read sensor data in Deep-sleep mode.

> Currently, ESP32-MeshKit-Sense can only be used to turn on/off ESP32-MeshKit-Light according to the ambient light sensor data. More features will be added in the future, such as controlling the associated devices according to the temperature & humidity sensor readings.

## 1. Overview

[ESP32-MeshKit-Sense](https://github.com/espressif/esp-iot-solution/blob/master/documents/evaluation_boards/ESP32-MeshKit-Sense_guide_en.md) is a development board that integrates a temperature & humidity sensor (HTS221), an ambient light sensor (BH1750), and various LED indicators.

This sensor device consumes extremely low power which makes it suitable for setup environments where the usage of cables can be inconvenient and cumbersome, like building corners, industrial fields, etc. In these places, batteries are just what is needed as a long-term power supply.

ESP32-MeshKit-Sense belongs to [ESP32-MeshKit](https://docs.espressif.com/projects/esp-mdf/en/latest/hw-reference/esp32-meshkit.html) - a set of hardware devices specifically designed with the goal to demonstrate the features of the [ESP-MESH](https://docs.espressif.com/projects/esp-idf/en/stable/api-guides/mesh.html) network protocol.

In a few words, this protocol is an extension of the Wi-Fi protocol which is designed for the ESP32 chip. ESP-MESH takes the best of both Wi-Fi and Bluetooth technologies, combines their advantages, and provides innovative solutions that make you re-think how wireless networks can be built using the same networking equipment.

[ESP-MDF](https://github.com/espressif/esp-mdf/blob/master/README.md), or Espressif Mesh Development Framework, is a development framework for the ESP-MESH protocol based on the ESP32 chip. It is ESP-MDF that offers a low-power solution for ESP32-MeshKit-Sense.

For more details about Espressif's lower-power solution, please refer to [ESP32 Low-Power Management Overview](https://github.com/espressif/esp-iot-solution/blob/master/documents/low_power_solution/esp32_lowpower_solution_en.md).

## 2. Preparation

### 2.1. Hardware

* [ESP32-MeshKit-Sense](https://github.com/espressif/esp-iot-solution/blob/master/documents/evaluation_boards/ESP32-MeshKit-Sense_guide_en.md) development board
* Power supply: Lithium battery or via Mini USB
* One or more [ESP32-MeshKit-Light](https://www.espressif.com/sites/default/files/documentation/esp32-meshkit-light_user_guide_en.pdf) lightbulbs (screw them into your sockets)
* [ESP-Prog](https://github.com/espressif/esp-iot-solution/blob/master/documents/evaluation_boards/ESP-Prog_guide_en.md) debugging tool (optional)
* E-ink screen (optional)

<div align=center>
<img src="docs/_static/MeshKit-Sense-ALL.jpg" width="600">
</div>

ESP32-MeshKit-Sense features all the necessary hardware components:

* **Microcontroller**: ESP32 module
* **Power supply interface**: 
	* Mini USB
	* 2-pin connector for an external battery
* **Debugging interface**: For connecting ESP-Prog
* **Display component**:
	* Display interface 
	* LED indicators
* **Sensor**: 
	* Temperature & humidity sensor (HTS221) 
	* Ambient light sensor (BH1750)

<div align=center>
<img src="docs/_static/ESP32-MeshKit-Sense.png" width="600">
</div>

Jumper caps can be used at 5 places on ESP32-MeshKit-Sense. Please find the table below detailing their functions and placement:  

| Location   | Action | Function | Placement |
|------------|--------|----------|-----------|
| EXT5V-CH5V | Connects the 5 V USB power supply to the input pin of the charge management chip. | When these pins are shorted, the battery charging is enabled. | Place a jumper cap during battery charging  |
| CHVBA-VBA  | Connects the output pin of the charge management chip to the battery's positive terminal. | When these pins are shorted, the battery charging is enabled. | Place a jumper cap during battery charging |
| DCVCC-SUFVCC | Connects the power supply to the input of the DC-DC converter. | Used for measuring the power consumption of the whole device. | Place a jumper cap when the device is powered up via battery or USB. |
| 3.3V-PER_3.3V | Connects the 3.3 V power supply to the power supply for all peripherals. | Used for measuring the power consumption of peripherals. | Place a jumper cap when peripherals are used. |
| 3.3V-3.3V_ESP32 | Connects the 3.3 V power supply to the power supply for the module. | Used for measuring the power consumption of the module (as well as Reset Protect chip) | Place a jumper cap when modules are used. |

ESP32-MeshKit-Sense is equipped with 6 LED indicators, whose functions and indicated statuses are listed in the table below:

| LED Indicator | Function | Status |
| --------------|----------|--------|
| Charge indicator (two color) | Indicates charging status | Red: charging<br> Green: fully charged  |
| BAT indicator (two color)    | Indicates battery level   | Red: battery low<br> Green: battery sufficient|
| WiFi indicator   | Indicates WiFi connection status | Slow flashing: network configuring<br> Rapid flashing: networking<br> Solid: successfully networked  |
| Network indicator | Indicates network connection status | Not applicable to this example |
| Sensor indicator  | Indicates sensor on/off status | On: operating<br> Off: power off | 
| Screen indicator  | Indicates screen on/off status | On: operating<br> Off: power off |

### 2.2. Software

Now you need to set up the development environment, then the ULP Assembly Environment, and after that configure the example.

#### Step 1. Development Environment Setup

Please proceed to the document [Getting Started with ESP-MDF](https://docs.espressif.com/projects/esp-mdf/en/latest/get-started/index.html) and complete the following steps only:

1. Get ESP-MDF
2. Set up Path to ESP-MDF

#### Step 2. ULP Assembly Environment Setup

Please refer to [ULP introduction on Espressif Github](https://github.com/espressif/esp-iot-solution/blob/master/documents/low_power_solution/esp32_ulp_co-processor_and_assembly_environment_setup_en.md) for the detailed introduction to ULP co-processor and assembly environment setup.

#### Step 3. Example Configuration

Copy `development_kit/meshkit_sense` to the `~/esp` directory and connect your board.

After that, go to `~/esp/development_kit/meshkit_sense` and configure the example by running the command `make menuconfig`(Make) or `idf.py menuconfig`(CMake).

<div align=center>
<img src="docs/_static/meshkit_sense_configurations.png" width="800">
<p> ESP32 Configurations </p>
</div>

In the menu, navigate to `Example Configuration` where you can configure the following parameters:

* **Wakeup types from Deep-sleep mode**

	* **ULP wakeup (`Use ulp as a wakeup stub`)**

		Wake up CPU according to ULP's data readings from sensors.
	
		> You also need to configure:
		>
		>* **ULP data reading interval (`Time interval of ulp read data from sensor`)**: Time interval for ULP to read data from sensors in Deep-sleep mode
		>
		>* **Minimum light intensity threshold (`Low threshold of luminance to trigger`)**: CPU wakes up when the ambient light intensity is lower than the minimum threshold.
		>* **Maximum light intensity threshold (`Upper threshold of luminance to trigger`)**: CPU wakes up when the ambient light intensity is higher than the maximum threshold.
    
	* **GPIO wakeup (`Use extern interrupt 1 as a wakeup stub`)**
		
		Wake up CPU with the WakeUp button (GPIO34). Upon wakeup, the device enters Mesh mode.
	
	* **Timer wakeup (`Use timer as a wakeup stub`)**
	
		Wake up CPU at a specified time interval (disabled by default).
		
		> You also need to configure:
		>
    	> * **Wakeup interval (`Timer interval of deepsleep`)**: Time interval at which CPU wakes up.

* **Enable E-ink screen**: If enabled, E-ink screen will display the latest set of data read by ULP after wakeup.

### 2.3. Run the Example

Build and flash the example onto your device and start the log monitoring program by running the following command:

Make:
`make erase_flash flash monitor -j5`

CMake:
`idf.py erase_flash flash monitor`

When the above process is successfully completed, the Sensor indicator on ESP32-MeshKit-Sense turns solid red, indicating that the device is powered on and functions normally. Meanwhile, the WiFi indicator starts slowly flashing green, indicating that the device is in **Network Configuration mode**. 

At this point, use ESP-MESH App to configure the Mesh network. The app is available for [Android](https://www.espressif.com/en/support/download/apps) and [iOS](https://itunes.apple.com/cn/app/esp-mesh/id1420425921?mt=8).

Please refer to the [demo video](https://player.youku.com/embed/XMzc1ODE4MDYwOA==) for details.

## 3. Operation Modes

In this example, ESP32-MeshKit-Sense operates in three modes: 

* Network Configuration mode
* Mesh mode
* Deep-sleep mode

By default, once initially powered on, the device goes into Network Configuration mode. When successfully networked, the device goes into Mesh mode, and then quickly switches to Deep-sleep mode.

### 3.1. Network Configuration Mode

ESP32-MeshKit-Sense enters this mode when initially powered on or when the configuration information is reset.

### 3.2. Mesh Mode

As soon as ESP32-MeshKit-Sense joins an ESP-MESH network, the device goes into Mesh mode and functions as a Mesh device, with all the features provided by ESP-MDF, such as LAN control, and upgrade.

### 3.3. Deep-sleep Mode

After a specified period of inactivity, ESP32-MeshKit-Sense enters Deep-sleep Mode in which it consumes extremely low power because the CPU is powered off. Meanwhile, ULP regularly reads sensor data and compares its sensor readings against the pre-set parameters. When the readings meet pre-set conditions, the device enters Mesh mode making CPU wake up to implement the associated operations and then re-enters Deep-sleep mode.      

### 3.4. Mode Switch

You can switch modes using the WakeUp button (GPIO34).

Below are the available actions for the WakeUp button: 

* **In Deep-sleep mode, press the WakeUp button**: wakes up CPU and brings the device into Mesh mode.

* After that, you can

    * **Press the WakeUp button again**: Puts the device back into Deep-sleep mode.
    
    * **Long press the WakeUp button for 3 seconds**: Reboots the device, resets all the configuration information, and brings the device into Network Configuration mode.

## 4. Workflow

<div align=center>
<img src="docs/_static/MeshKit-Sense-Workflow.png" width="600">
<p> ESP32-MeshKit-Sense Workflow </p>
</div>

The left part of the figure above demonstrates the general workflow, while the right part describes the workflow when the device is in Deep-sleep mode.

### 4.1. Network Configuration Process

1. When the device is initially powered on, or the configuration information is reset, its Wi-Fi indicator slowly flashes, signaling that the device is in **Network Configuration mode**.

2. Configure a network with ESP32-Mesh App ([iOS](https://itunes.apple.com/cn/app/esp-mesh/id1420425921?mt=8) and [Android](https://github.com/EspressifApp/Esp32MeshForAndroid/raw/master/release/mesh.apk)).
3. When the Wi-Fi indicator starts flashing rapidly, the device enters **Networking status**.
4. When networking is finished and the device goes into Mesh mode, the Wi-Fi indicator stops flashing, meaning that the device is **successfully networked**.
5. After idling for a specified time, the device goes into Deep-sleep mode.
6. In Deep-sleep mode, ULP regularly wakes up to read the data from sensors, which is indicated by the on/off status of the Sensor indicator:
	* On (flash): ULP is reading data from sensors
	* Off: ULP is in deep sleep
7. If the specified conditions are met:
   * ULP wakes up CPU
   * Device connects to the ESP-MESH network and implements the associated operations 
   * Device re-enters Deep-sleep mode

### 4.2. Control the Associated Devices

Please follow the steps below:

1. Use ESP32-Mesh App to build an ESP-MESH network with ESP32-MeshKit-Sense and ESP32-MeshKit-Light devices.
> **Note:** ESP32-MeshKit-Sense can only be added to an existing ESP-MESH network. For more information, please refer to [ESP32-MeshKit Guide](../README.md)

2. Associate ESP32-MeshKit-Sense with ESP32-MeshKit-Light devices in ESP32-Mesh App

    * When the ambient light intensity value is lower than the minimum threshold, ESP32-MeshKit-Sense turns on ESP32-MeshKit-Light.
    
    * When the ambient light intensity value is higher than the maximum threshold, ESP32-MeshKit-Sense turns off ESP32-MeshKit-Light.
    
3. Press the WakeUp button on ESP32-MeshKit-Sense, only the Sensor indicator will be flashing, meaning that ESP32-MeshKit-Sense enters Deep-sleep mode while ULP keeps running and reading sensor data at a default interval of 500 ms which can be modified.

    * When the ambient light sensor is covered, ESP32-MeshKit-Sense wakes up, turns on ESP32-MeshKit-Light, and re-enters Deep-sleep mode.
    
    * When the ambient light sensor is exposed to bright light, ESP32-MeshKit-Sense wakes up, turns off ESP32-MeshKit-Light, and re-enters Deep-sleep mode.

## 5. ULP-related Operation

### 5.1. Set Wakeup Trigger Values

You can set the threshold values of a respective parameter that will trigger the device to wake up. It includes max value, min value, and variation.

Please find the following functions in the file `meshkit_sense/sense_driver/ulp_operation/ulp_sensor.c`:

* `set_ulp_hts221_humidity_threshold(...)`: Set the values for the humidity sensor.

* `set_ulp_hts221_temperature_threshold(...)`: Set the values for the temperature sensor.

* `set_ulp_bh1750_luminance_threshold(...)`: Set the values for the ambient light sensor.

> **Note:** Variation is the maximum data difference between the initial sensor data when ESP32-MeshKit-Sense enters Deep-sleep mode and ULP's sensor readings after ESP32-MeshKit-Sense enters Deep-sleep mode. If the variation is exceeded, ESP32-MeshKit-Sense will be triggered to wake up. For example, when the variation is set to 100, and the initial sensor data is 400, ESP32-MeshKit-Sense will be woken up if ULP's sensor reading is lower than 300 or higher than 500. 

### 5.2. Get the Latest ULP Readings

After the device wakes up, you can use the below functions to get the newest ULP readings from different sensors in Deep-sleep mode:

* `get_ulp_bh1750_luminance(...)`: Get the latest luminance value

* `get_ulp_hts221_temperature(...)`: Get the latest temperature value

* `get_ulp_hts221_humidity(...)`: Get the latest humidity value

## 6. Power Consumption

ESP32-MeshKit-Sense is specifically designed to offer a low-power sensor solution, since ESP32 allows ULP (Ultra Low Power) co-processor to read sensor data in Deep-sleep mode.

The table below lists the current consumption and running duration of ESP32-MeshKit-Sense in different scenarios. Note that the current is measured with its specific DCVCC-SUFVCC power-supply feature.

| Scenario | Current | Duration |
|----------|---------|----------|
| ULP is in Deep-sleep mode and inactive | 21 uA | Custom value (1000 ms by default) |
| ULP reads sensor data in Deep-sleep mode | 3.4 mA | 150 ms |
| Start-up process after waking up | 40 mA | 1000 ms |
| Normal operation | 100 mA | < 3000 ms |

While in Deep-sleep mode and inactive, the device consumes extremely low power. The longer the ULP's sleep duration, the lower the average power consumption. However, the time interval to detect the environmental changes will be equally longer, therefore slowing down responsiveness. You can set a duration that best fits your project-specific requirements.

## 7. Related Resources

* [Deep Sleep Wake Stubs](https://docs.espressif.com/projects/esp-idf/en/stable/api-guides/deep-sleep-stub.html)
* [ESP32-MeshKit-Sense Hardware Design Guidelines](https://github.com/espressif/esp-iot-solution/blob/master/documents/evaluation_boards/ESP32-MeshKit-Sense_guide_en.md)
* [ESP32-MeshKit-Light User Guide](https://www.espressif.com/sites/default/files/documentation/esp32-meshkit-light_user_guide_en.pdf)
* [Introduction to the ESP-Prog Board](https://github.com/espressif/esp-iot-solution/blob/master/documents/evaluation_boards/ESP-Prog_guide_en.md)
* [Introduction to the ESP32 ULP Co-processor and Assembly Environment Setup](https://github.com/espressif/esp-iot-solution/blob/master/documents/low_power_solution/esp32_ulp_co-processor_and_assembly_environment_setup_en.md)
* [ESP32 Low-Power Management Overview](https://github.com/espressif/esp-iot-solution/blob/master/documents/low_power_solution/esp32_lowpower_solution_en.md)