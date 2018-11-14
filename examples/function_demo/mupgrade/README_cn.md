[[EN]](./README.md)

# Mupgrade 示例

## 介绍

本示例将介绍如何快速使用 `Mupgrade` 进行 ESP-MESH 设备的升级。

## 工作流程

### 步骤 1：连接到路由器

将主机 PC 连接到 ESP-MESH 网络所在的路由器。

### 步骤 2：运行 HTTP 服务器

Python 有一个内置的 HTTP 服务器，可用于本示例升级服务器。
打开一个新的终端来运行 HTTP 服务器，然后运行这些命令来生成待升级的固件并启动服务器：

```shell
cd $MDF_PATH/esp-idf/examples/get-started/hello_world/
export IDF_PATH=$MDF_PATH/esp-idf/
make
cd build
python -m SimpleHTTPServer 8070
```
在服务器运行时，可以在 `http：// localhost：8070/` 浏览构建目录的内容。

>> 注意：
    1. 在某些系统上，命令可能是`python2 -m SimpleHTTPServer 8070`。
    2. 您可能已经注意到，当用于 OTA 更新时，`hello-world` 示例没有什么特别之处。这是因为由 ESP-MDF 构建的任何应用程序文件都可以用作 OTA 的应用程序映像。
    3. 如果您运行的任何防火墙软件将阻止对端口 8070 的传入访问，请将其配置为允许在运行示例时进行访问。

### 步骤 3：构建 OTA 示例

切换回 OTA 示例目录，然后键入 `make menuconfig` 以配置 OTA 示例。在 “Example Configuration” 子菜单下，填写以下详细信息：

* ESP-MESH 网络的配置信息
* 固件升级 URL。URL 将如下所示：

```
https://<host-ip-address>:<host-port>/<firmware-image-filename>

for e.g,
https://192.168.0.3:8070/hello-world.bin
```
保存更改，然后键入`make`来构建示例。

### 步骤 4：Flash OTA 示例

烧录 Flash 时，首先使用`erase_flash`擦除整个闪存（这将删除 ota_data 分区中的所有剩余数据），然后通过串口写入：

```shell
make erase_flash flash
```

### 步骤 5：运行 OTA 示例

示例启动后，将会输出 "Starting OTA example..."，然后：

1. 通过配置的 SSID 和密码连接路由器
2. 连接 HTTP 服务器，下载新的映像文件
3. 将映像文件写入 flash，并配置为下一次从该映像文件启动示例
4. 重启

## 故障排除

* 检查您的 PC 是否能够 ping 通 ESP32，同时检查 IP，路由器及 menuconfig 中的其他配置是否正确
* 检查是否有防火墙软件阻止 PC 上的连接操作
* 通过查看以下命令日志，检查是否能够看到配置文件 (default hello-world.bin)

 ```
 curl -v https：// <host-ip-address>：<host-port> / <firmware-image-filename>
 ```

* 如果您有其他 PC 或手机，尝试浏览不同主机所列文件

### errors “ota_begin error err = 0x104”

如果您遇到此项报错，请检查分区表中配置的（及实际的）flash 分区大小是否满足要求。默认的“双 OTA 分区”分区表仅提供 4 MB flash 分区。若 flash 分区大小不能满足 OTA 升级要求，您可以创建自定义分区表 CSV （查看 components/partition_table），并在 menuconfig 中进行配置。