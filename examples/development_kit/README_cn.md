[[EN]](./README.md)

# ESP32-MeshKit 指南

---

## 概述
ESP32-MeshKit 是基于 [ESP-WIFI-MESH](https://docs.espressif.com/projects/esp-idf/en/stable/api-guides/mesh.html) 的智能家居组网方案，包含以下硬件：

* [ESP32-MeshKit-Light](https://www.espressif.com/sites/default/files/documentation/esp32-meshkit-light_user_guide_cn.pdf)：板载 ESP32 芯片的智能灯，用于 ESP-WIFI-MESH 作为主干网络进行长供电的场景。

* [ESP32-MeshKit-Sense](https://github.com/espressif/esp-iot-solution/blob/master/documents/evaluation_boards/ESP32-MeshKit-Sense_guide_cn.md)：ESP-WIFI-MESH 在 Deep-sleep + Light-sleep 模式下的低功耗方案，可用于：

	* 监测 MeshKit 外设功耗 
	* 根据传感器数据控制 MeshKit 外设

* [ESP32-MeshKit-Button](button/README_cn.md)：ESP-WIFI-MESH 在超低功耗的场景下使用，平常处于断电状态，仅在唤醒时工作，并通过 [ESP-NOW](https://docs.espressif.com/projects/esp-idf/zh_CN/stable/api-reference/network/esp_now.html) 给 ESP-WIFI-MESH 设备发包。

对以上硬件进行配置和组网，您需要：

* 安装 ESP-Mesh App 的安卓或苹果手机（见 [ESP-Mesh App 章节](#esp-mesh-app)）
* 2.4 GHz Wi-Fi 网络，连接您的手机和 ESP-WIFI-MESH 设备

## 功能

1. 配网：[Mconfig](https://docs.espressif.com/projects/esp-mdf/zh_CN/latest/api-guides/mconfig.html) (MESH Network Configuration) 是 ESP-WIFI-MESH 配网的一种方案，首先使用 ESP-Mesh App 通过蓝牙给单个设备配网，然后再由已配网设备给未配网设备传递配网信息。

2. 通信：[Mlink](https://docs.espressif.com/projects/esp-mdf/en/latest/api-guides/mlink.html) (MESH LAN Communication) 是 ESP-WIFI-MESH 局域网控制的一种方案，根节点会建立 HTTP 服务器与 App 之间的通信，并将信息转发给 ESP-WIFI-MESH 网络内的其它设备。

3. 升级：[Mupgrade](https://docs.espressif.com/projects/esp-mdf/zh_CN/latest/api-guides/mupgrade.html) (MESH Upgrade) 是 ESP-WIFI-MESH OTA 升级的一种方案，旨在通过以下机制实现 ESP-WIFI-MESH 设备的高效升级。


## ESP-Mesh App 

* **安卓系统**：[源码](https://github.com/EspressifApp/EspMeshForAndroid)，[apk](https://www.espressif.com/zh-hans/support/download/apps?keys=&field_technology_tid%5B%5D=18)（安装包）

* **iOS 系统**：打开 App Store， 搜索 `ESP-Mesh`
* **微信小程序**：打开微信，搜索 `ESPMesh`

> 注： 所有版本中优先更新安卓版本

ESP-Mesh App 可帮助您调研和了解 ESP-WIFI-MESH，并进行二次开发。上述源码可用于开发您自己的应用。

## 硬件准备

* 手机打开蓝牙和 Wi-Fi，并连接目标路由器。

* 确保设备处于配网模式

    * 您需要至少一个 ESP32-MeshKit-Light 设备，因为仅该设备作为根节点（相当于网关）。通常，连续通断电三次可以使设备进入配网状态；
    
    * ESP32-MeshKit-Button 和 ESP32-MeshKit-Sense 不可作为根节点，所以只能加入已建立的 Mesh 网络。查看相关设备说明，了解设备进入配网模式操作。

## 配网

### 1. 基本步骤

* 打开 ESP-Mesh App，通过蓝牙扫描并提示周围处于配网模式的设备；

* 点击主界面中`添加设备`按钮，查看获取到的 ESP-WIFI-MESH 设备列表；

* 点击搜索框前的小箭头，对设备进行筛选；
	* `RSSI`：根据设备信号强度筛选设备 
	* `只显示收藏的`：只显示收藏的设备（点击设备图标，即可加入收藏）

* 选择需要添加的设备，点击`下一步`；

    <table>
        <tr>
            <td ><img src="docs/_static/zh_CN/enter_configuration_network.png" width="300"><p align=center>进入蓝牙配网</p></td>
            <td ><img src="docs/_static/zh_CN/get_device_list.png" width="300"><p align=center>获取设备列表</p></td>
        </tr>
    </table>

* 输入配置信息：
    * **Wi-Fi 名称**：手机所连 Wi-Fi 名称。注意仅支持 2.4 G；
    * **Mesh ID**：ESP-WIFI-MESH 网络唯一标识符，默认为路由器的 Mac 地址，相同的 `Mesh ID` 将组成一个网络；
    * **密码**：Wi-Fi 密码；
    * **More**：点此即可查看和修改有关 ESP-WIFI-MESH 网络内部的默认配置。更多配置详情，请参照 [ESP-WIFI-MESH 编程指南](https://docs.espressif.com/projects/esp-idf/zh_CN/stable/api-reference/network/esp_mesh.html)。
 
* 信息输入完成后，点击`下一步`；

    <table>
        <tr>
            <td ><img src="docs/_static/zh_CN/router_configuration.png" width="300"><p align=center>输入路由器信息</p></td>
            <td ><img src="docs/_static/zh_CN/more_configuration.png" width="300"><p align=center>输入 ESP-WIFI-MESH 配置信息</p></td>
        </tr>
    </table>

ESP-Mesh App 开始传输配置信息，并进行以下操作：

* App 首先会筛选蓝牙信号最强的设备，并与之建立连接，将配置信息和设备白名单列表传输给该设备；

* 设备收到配置信息后，尝试连接路由器校验配置信息是否正确；

* 信息校验正确后，设备通过蓝牙通知 App 配网成功，等待组网；

* 设备通过蓝牙成功组网后，给白名单设备配网，并进行组网。

    <table>
        <tr>
            <td ><img src="docs/_static/zh_CN/BLE_transmission.png" width="300"><p align=center>蓝牙传输</p></td>
            <td ><img src="docs/_static/zh_CN/waiting_for_networking.png" width="300"><p align=center>等待组网</p></td>
        </tr>
    </table>

### 2. 添加设备

如果 App 发现处于配网模式的 ESP-WIFI-MESH 设备，会自动弹出设备添加框，点击`加入网络`即完成配网。

<table>
    <tr>
        <td ><img src="docs/_static/zh_CN/discovery_device.png" width="300"><p align=center>加入网络</p></td>
        <td ><img src="docs/_static/zh_CN/select_device.png" width="300"><p align=center>选择加入</p></td>
    </tr>
</table>

### 3. “设备”界面

进入已添加设备列表：

* 短按设备，打开设置界面，自定义设备类型

    <table>
        <tr>
            <td ><img src="docs/_static/zh_CN/light_control.png" width="300"><p align=center>智能灯的控制界面</p></td>
            <td ><img src="docs/_static/zh_CN/button_control.png" width="300"><p align=center>自定义设备类型的控制界面</p></td>
        </tr>
   </table>

* 长按设备，可编辑配置
    * **自动化**：将设备之间进行关联，如将 ESP32-MeshKit-Button 与 ESP32-MeshKit-Light 关联，则可以通过 Button 直接控制灯的开关、颜色等。

		> 注意关联的主从关系。例如，将灯 B 关联到灯 A，当打开灯 A 时，灯 B 将随之打开，但是关闭灯 B 并不能关闭灯 A，除非将灯 A 关联到灯 B。

    * **发送命令**：用于设备的调试，添加自定义请求指令。
		
    <table>
        <tr>
            <td ><img src="docs/_static/zh_CN/editing_device.png" width="300"><p align=center>选择自动化</p></td>
            <td ><img src="docs/_static/zh_CN/more_configuration.png" width="300"><p align=center>自动化</p></td>
            <td ><img src="docs/_static/zh_CN/send_command.png" width="300"><p align=center>发送命令</p></td>
        </tr>
   </table>

### 4. “群组”界面

* **默认群组**：App 默认按照设备类型对设备进行分组，默认群组禁止解散
* **添加群组**：您可以添加自定群组，对设备进行分组控制

    <table>
        <tr>
            <td ><img src="docs/_static/zh_CN/add_group.png" width="300"><p align=center>添加群组</p></td>
            <td ><img src="docs/_static/zh_CN/group_name.png" width="300"><p align=center>编辑名称</p></td>
        </tr>
   </table>

### 5. “我的”界面

* **设置**：App 的版本信息，App 升级及常见问题解答；

* **拓扑结构**：ESP-WIFI-MESH 网络结构及组网信息。您可以通过长按节点，获取设备的组网信息。

<table>
    <tr>
        <td ><img src="docs/_static/zh_CN/version_information.png" width="300"><p align=center>App 版本信息</p></td>
        <td ><img src="docs/_static/zh_CN/topology.png" width="300"><p align=center>拓扑结构</p></td>
        <td ><img src="docs/_static/zh_CN/network_configuration.png" width="300"><p align=center>ESP-WIFI-MESH 网络配置信息</p></td>
    </tr>
</table>

### 6. 固件升级

长按已添加设备列表中的 ESP-WIFI-MESH 设备，在弹出的对话框中选择`固件升级`，选择以下任一方式升级固件：

* 固件拷贝升级：您可以将固件直接拷贝到手机的 `文件管理/手机存储/Espressif/Esp32/upgrade` 文件夹中。
* 远程链接升级：您可以将固件存放在云端（如 GitHub 上）或局域网内创建的 HTTP 服务器，在 App 端输入固件链接地址。

<table>
    <tr>
        <td ><img src="docs/_static/zh_CN/add_firmware.png" width="300"><p align=center>固件拷贝升级</p></td>
        <td ><img src="docs/_static/zh_CN/url_upgrade.png" width="300"><p align=center>远程链接升级</p></td>
        <td ><img src="docs/_static/zh_CN/upgrading.png" width="300"><p align=center>升级中</p></td>
    </tr>
</table>

## 驱动说明

ESP32-MeshKit 的硬件驱动全部使用了 [esp-iot-solution](https://github.com/espressif/esp-iot-solution) 中的相关驱动代码，可通过仓库链接进行代码更新。
