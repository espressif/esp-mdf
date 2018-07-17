[[English]](../../en/application-notes/mdf_ota_guide_en.md)

# ESP-Mesh 设备固件升级

## 1. 概述

ESP-Mesh 设备固件升级是指通过移动端 app 将固件文件发送给 ESP-Mesh 设备的一种升级方式。
在具体操作前，请确认手机端已安装 [ESP-Mesh app](https://www.espressif.com/zh-hans/support/download/apps?keys=&field_technology_tid%5B%5D=18)，且在 app 中已完成 `配网` 和 `设备扫描` 操作。

## 2. 设备升级操作流程

1. 连接手机至电脑，并允许电脑对手机进行 `数据访问`；
2. 在电脑端 `文件浏览器` 左侧 `我的电脑` 下打开手机 `内部存储`，并在手机内部存储的根目录中找到 `Espressif` 目录；
3. 在 `Espressif` 目录下建立 `Esp32` 目录，并在 Esp32 目录下建立 `upgrade` 目录；
4. 拷贝固件文件至目录 `/Espressif/Esp32/upgrade` 下；
5. 回到 app 界面，长按准备升级的设备，选择 `升级` 功能，此时可以看到刚刚放在目录 `/Espressif/Esp32/upgrade` 下固件文件列表；
6. 选择固件文件对设备设备升级。

<table><tr>
<td ><img src="../../_static/mdf_app_screenshot_ota_1.jpg" width="300"></td>
<td ><img src="../../_static/mdf_app_screenshot_ota_2.jpg" width="300"></td>
</tr>
<tr>
<td align="center"> Figure 1: 选择升级设备 </td>
<td align="center"> Figure 2: 选择固件文件 </td>
</tr>
</table>
