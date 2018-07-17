[[中文]](../../zh_CN/application-notes/mdf_ota_guide_cn.md)

# ESP-Mesh Device Firmware Upgrade via App

## 1. Overview

`ESP-Mesh Device Firmware Upgrade via App`  is the device upgrade method by downloading firmware to the device's memory through mobile app.

To upgrade your ESP-Mesh devices, please make sure that the [ESP-Mesh app](https://www.espressif.com/en/support/download/apps?keys=&field_technology_tid%5B%5D=18) is installed on your mobile phone and the device networking and scanning have been completed in the app.

## 2. Steps of OTA Upgrade

1. Connect the phone to the computer and permit the computer to access the phone’s data.
2. Open the directory of the phone’s `Internal Storage` on the computer, and find the `Espressif` directory in the root directory of the phone's internal storage.
3. Create an `ESP32` directory in the `Espressif` directory and create an `upgrade` directory in the ESP32 directory.
4. Copy the bin files to the directory `/Espressif/ESP32/upgrade`.
5. Select the device for upgrading in the app, and tap `UPGRADE` on the app interface. You will see the list of bin files under the directory `/Espressif/ESP32/upgrade`.
6. Select a bin file to download on the device.

<table><tr>
<td ><img src="../../_static/mdf_app_screenshot_ota_1.jpg" width="300"></td>
<td ><img src="../../_static/mdf_app_screenshot_ota_2.jpg" width="300"></td>
</tr>
<tr>
<td align="center"> Figure 1: Choose the device to upgrade </td>
<td align="center"> Figure 2: Choose the bin file to download</td>
</tr>
</table>
