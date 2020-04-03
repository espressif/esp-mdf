
<!-- @import "[TOC]" {cmd="toc" depthFrom=1 depthTo=6 orderedList=false} -->

<!-- code_chunk_output -->

- [Aliyun Linkkit 应用指南](#aliyun-linkkit-应用指南)
  - [概述](#概述)
  - [快速开始](#快速开始)
    - [阿里云飞燕设备部署](#阿里云飞燕设备部署)
    - [创建一个入墙开关类产品](#创建一个入墙开关类产品)
    - [配置 example 以连接阿里云](#配置-example-以连接阿里云)
      - [get-started example](#get-started-example)

<!-- /code_chunk_output -->
# Aliyun Linkkit 应用指南

## 概述

Aliyun Linkkit 是基于 [ESP-MESH](https://docs.espressif.com/projects/esp-idf/en/stable/api-guides/mesh.html) 的智能家居组网方案，可配套 "mesh App" 使用，以帮助用户更快捷的将 ESP-MESH 应用于产品开发。

- ESP-MESH 作为主干网络用于长供电的场景中，设备可作为根节点（相当于网关）、中间节点和叶子节点,所有设备通过根节点登录到阿里服务器，进而完成间接通讯。以下是 ESP-MESH 所支持的功能：
  - 支持 MESH 组网,并完成登录,用户操作无感知
  - 支持远程控制设备状态.
  - 支持厂商 OTA 更新固件
  - 支持天猫精灵控制
  - 后期增加支持公版 App 配网控制.

## 快速开始

### 阿里云飞燕设备部署

1. 登录阿里云飞燕平台：[点击这里进入平台](https://living.aliyun.com/#/)

    > 登录平台后, 点击网页上方 [文档中心](https://living.aliyun.com/doc#index.html) 查阅平台相关文档。
    > <td ><img src="docs/_static/ClickDocuments.png" width="600"></td><br>
    > 阅读[快速入门](https://help.aliyun.com/document_detail/142147.html)，了解如何创建阿里云生活物联平台的项目，并按照其中的步骤创建一个测试项目。在添加设备步骤中选择 ESP32-WROOM-32DC，并且可以跳过“开发设备”这一步骤，如果不涉及 APP 控制，也可以跳过“配置 APP” 。

### 创建一个入墙开关类产品

1. 进入已创建好的项目的产品管理页面，创建一个新产品，填入信息。节点类型最好选择网关类，这样既可以用在 get-started example 里，也可以用在 mesh-with-aliyun example 里。

    <td><img src="docs/_static/create-product.png" width="600"></td>

1. 功能定义，使用默认配置即可。

    <td><img src="docs/_static/define-function.png" width="600"></td>

1. 设备调试，这里将看到测试设备列表，如果之前忘记记下四元组信息，可以在这里查看。

    <td><img src="docs/_static/debug-device.png"></td>

### 配置 example 以连接阿里云

#### get-started example

get-started example 是一个不具备 MESH 功能的设备连接阿里云的示例。

1. 配置 MDF 的环境变量，然后进入 menuconfig 界面
1. `Example Configuration` 里配置路由器的 SSID 和 password。
1. `Example Configuration` —> `aliyun config` 配置连接阿里云所需的四元组信息。
1. 编译运行，然后打开阿里云的设备调试界面，下发数据进行调试。

#### mesh-with-aliyun example

mesh-with-aliyun 是在 ESP-MESH 网络中连接阿里云，所以需要有一个设备配置成 root 节点。

1. 配置 MDF 的环境变量，然后进入 menuconfig 界面
1. `Example configuration` 选择设备类型（必须要有一个设备为 root）
1. 如果是 root 设备，需要在 `Example configuration`里配置路由器信息
1. `Example configuration`->`aliyun config`填入四元组信息
1. 编译运行，然后打开阿里云的设备调试界面，下发数据进行调试，还可以在 root 设备的子设备列表里看到所有子设备
