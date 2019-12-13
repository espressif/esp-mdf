[[EN]](./README.md)

# Mconfig 示例

## 介绍

本示例将介绍如何基于 `Mconfig` 模块 APIs 配置 ESP-WIFI-MESH 网络。在 ESP-WIFI-MESH 网络中，需要配置的网络信息包含路由信息、ESP-WIFI-MESH 网络配置信息、设备白名单。配网过程中使用 RSA 算法（非对称加密）进行密钥协商，使用 128-AES 算法（对称加密）进行数据加密，使用 CRC 算法（循环冗余校验）进行校验和验证。了解更多有关信息，请参考 [Mconfig](https://docs.espressif.com/projects/esp-mdf/zh_CN/latest/api-guides/mconfig.html)。

## 配网流程

完整配网流程可参考：[整体流程](https://docs.espressif.com/projects/esp-mdf/zh_CN/latest/api-guides/mconfig.html#id2)。

1. 手机连接到路由器上，并确保路由器是 2.4 GHz
2. 打开微信，搜索 `ESPMesh` 或扫描如下二维码：（也可使用 ESP-WIFI-MESH APP）

	<div align=center>
	<img src="ESPMesh_program.png" width="800">
	<p>配网小程序</p>
	</div>

3. 输入配置参数：

    <table>
        <tr>
            <td ><img src="choose_configuration.png" width="300"><p>选择配网方式</p></td>
            <td ><img src="get_device_list.png" width="300"><p>获取设备列表</p></td>
            <td ><img src="enter_configuration.png" width="300"><p>输入配置信息</p></td>
            <td ><img src="transfer_configuration.png" width="300"><p>传输配置信息</p></td>
        </tr>
    </table>

## 注意事项

使用 `mconfig` 配网模块需要启用蓝牙和硬件 MPI（bignum）加速, 请添加如下配置

```
#
# Bluetooth
#
CONFIG_BT_ENABLED=y
CONFIG_GATTC_ENABLE=
CONFIG_BLE_SMP_ENABLE=
CONFIG_BT_ACL_CONNECTIONS=2
CONFIG_BLE_SCAN_DUPLICATE=
CONFIG_BT_BLE_DYNAMIC_ENV_MEMORY=y

#
# mbedTLS
#
CONFIG_MBEDTLS_HARDWARE_MPI=y
```