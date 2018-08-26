[[English]](../../en/application-notes/mdf_lan_protocol_guide_en.md)

#  MDF 设备与 ESP-Mesh App 之间的通信协议

## 1. 准备工作

ESP-Mesh app 与 ESP-MDF 设备之间进行局域网通信的前提是：设备已经组网成功，且手机和 Mesh 网络处于同一个局域网中。

ESP-MDF 设备在配网成功后会自动进入[组网阶段](https://esp-idf.readthedocs.io/en/latest/api-guides/mesh.html#mesh-networking)，对用户来说，在 ESP-Mesh app 的设备页面中能够扫描到所以已配网的设备即可表示组网已经成功。

<div align=center>
<img src="../../_static/esp32_mesh_topology_lan.png" width="400">
<p> ESP-Mesh Network Topology </p>
</div>

> 下文中的 app 都表示 Android 版本的 [ESP-Mesh app](https://www.espressif.com/zh-hans/support/download/apps?keys=&field_technology_tid%5B%5D=18)。

## 2. 通信流程

根节点是一个 Mesh 网络与外部通信的唯一出口，app 想要控制 Mesh 网络内设备，首先需要在该网络中找到根节点，之后通过根节点获取该 Mesh 网络内的设备列表，最后才能与该 Mesh 网络中任意设备进行通信。整个过程主要分为以下三步：

1. App 获取根节点的 IP 地址，局域网通信端口和 mac 地址
2. App 询问根节点获得 Mesh 网络设备列表
3. App 与 Mesh 网络中任意设备进行通信

<div align=center>
<img src="../../_static/mesh_comm_procedure.png" width="600" />
<p> App 与设备局域网通信流程 </p>
</div>

## 3. 通信协议

本章节从通信流程角度，分别介绍这三个步骤中所涉及的通信协议格式。其中，app 获取 Mesh 网络设备列表，app 与 Mesh 网络中任意设备的通信采用标准的 HTTP 或 HTTPS 通信协议。此外，协议还包括以下两部分：

1. 设备状态通知：为了方便用户通过 app 查看设备的实时状态，在现有协议中增加了设备通知功能，即在设备状态发生变化时主动发 UDP 广播通知 app，之后由 app 来获取设备详细状态的过程，详见章节 `3.5. 设备状态通知`。
2. 本地互联控制：实现局域网内设备间的互联控制。协议说明请参考章节 `3.6 本地互联控制`。

### 3.1. App 获取根节点的 IP 地址，局域网通信端口和 MAC 地址

现阶段，根节点会同时启动 mDNS 服务和接收 UDP 广播两种方式用于设备发现。设备发现表示 app 获取根节点的 IP 地址，通信端口和 MAC 地址的过程。

#### 3.1.1. mDNS 设备发现服务

    mDNS Service Info：
        hostname: "esp32_mesh"
        instance_name: "mesh"
        service_type: "_mesh-https"
        proto: "_tcp"
        port: 80
        txt:
            key: "mac"
            value: "112233445566"

当 app 查询到相关的 mDNS 服务时，便获取了根节点的 `IP`，再通过该服务中 `port` 和 `txt` 字段可知根节点的 `局域网通信端口` 和 `mac 地址`。

#### 3.1.2. 接收 UDP 广播

在设备发现阶段，app 会主动发送 UDP 广播包，并根据根节点的回复获取根节点的信息。

**Request:**

    "Are You Espressif IOT Smart Device?"

**Response:**

    "ESP32 Mesh 112233445566 http 80"

> * `112233445566` 为根节点的 MAC 地址
> * `80` 是 http 服务器端口
>
> 此外，app 通过 UDP 回复包获取根节点的 IP 地址。

### 3.2. App 获取设备列表

**Request:**

    GET /mesh_info HTTP/1.1
    Host: 192.168.1.1:80

**Response:**

    HTTP/1.1 200 OK
    Content-Length: ??
    Mesh-Node-Mac: aabbccddeeff,112233445566,18fe34a1090c
    Host: 192.168.1.1:80

> * `/mesh_info` app 获取设备列表的命令，通过 http 的 URL 字段实现
> * `Mesh-Node-Mac` 节点的 Station Mac 地址，以逗号分隔
> * `Host` HTTP/1.1 协议必须携带的字段，表示控制端 IP 地址和通信端口

### 3.3. App 与 ESP-MDF 设备通信格式说明

#### 3.3.1. App 请求格式

**Request:**

    POST /device_request HTTP/1.1
    Content-Length: ??
    Content-Type: application/json
    Root-Response:??
    Mesh-Node-Mac: aabbccddeeff,112233445566
    Host: 192.168.1.1:80
    \r\n
    **content_json**

> * `/device_request` app 对设备的控制操作，包括状态的设置和获取，通过 http 请求的 URL 字段实现
> * `Content-Length` http 请求的 body 数据长度
> * `Content-Type` http 请求的 body 数据类型，现阶段采用 `application/json` 格式
> * `Root-Response` 是否只需要根节点回复。如果只需要根节点回复，则命令不会转发到 Mesh 网络内的设备。`1` 表示需要回复，`0` 表示不需要回3
> * `Host` HTTP/1.1 协议必须携带的字段，表示控制端 IP 地址和通信端口
> * `**content_json**` http 请求的 body 数据，表示章节 `2.4. App 对 ESP-MDF 设备的控制操作` 中的 `Request` 部分

#### 3.3.2. 设备回复格式

**Response:**

    HTTP/1.1 200 OK
    Content-Length: ??
    Content-Type: application/json
    Mesh-Node-Mac: 30aea4062ca0
    Mesh-Parent-Mac: aabbccddeeff
    Host: 192.168.1.1:80
    \r\n
    **content_json**

> * `Content-Length` http 回复的 body 数据长度
> * `Content-Type` http 回复的 body 数据类型，现阶段采用 `application/json` 格式
> * `Mesh-Node-Mac` 设备自身的 mac 地址
> * `Mesh-Parent-Mac` 父节点的 mac 地址
> * `Host` HTTP/1.1 协议必须携带的字段，表示控制端 IP 地址和通信端口
> * `**content_json**` http 请求的 body 数据，表示章节 `3.4. App 对 ESP-MDF 设备的控制操作` 中的 `Response` 部分

### 3.4. App 对 ESP-MDF 设备的控制操作

#### 3.4.1. 获取设备信息：get_device_info

**Request:**

    {
        "request": "get_device_info"
    }

> * `"request"` 设备的操作字段，后接具体的操作命令

**Response:**

    {
        "tid": "1",
        "name": "light_064414",
        "version": "v0.8.5.1-Jan 17 2018",
        "characteristics": [
            {
                "cid": 0,
                "name": "on",
                "format": "int",
                "perms": 7,
                "value": 1,
                "min": 0,
                "max": 1,
                "step": 1
            },
            {
                "cid": 1,
                "name": "hue",
                "format": "int",
                "perms": 7,
                "value": 0,
                "min": 0,
                "max": 360,
                "step": 1
            },
            {
                "cid": 2,
                "name": "saturation",
                "format": "int",
                "perms": 7,
                "value": 0,
                "min": 0,
                "max": 100,
                "step": 1
            },
            {
                "cid": 3,
                "name": "value",
                "format": "int",
                "perms": 7,
                "value": 100,
                "min": 0,
                "max": 100,
                "step": 1
            },
            {
                "cid": 4,
                "name": "color_temperature",
                "format": "int",
                "perms": 7,
                "value": 0,
                "min": 0,
                "max": 100,
                "step": 1
            },
            {
                "cid": 5,
                "name": "brightness",
                "format": "int",
                "perms": 7,
                "value": 100,
                "min": 0,
                "max": 100,
                "step": 1
            }
        ],
        "status_code": 0
    }

> * `"tid"` 设备的 type ID，用于区分灯，插座，空调等不同类型的 ESP-MDF 设备
> * `"name"` 设备名称
> * `"version"` 设备固件版本
> * `"characteristics"` 设备的属性信息，通过 json 列表表示
>   * `"cid"` 设备属性身份（characteristic ID），用于区分亮度，明暗，开关等不同的的设备属性
>   * `"name"` 属性名称
>   * `"format"` 数据类型，支持 `"int"`，`"double"`，`"string"`，`"json"` 四种数据类型
>   * `"value"` 属性值
>   * `"min"` 属性值的最小值或支持的字符串的最小长度
>   * `"max"` 属性值的最大值或支持的字符串的最大长度
>   * `"step"` 属性值的最小变化值
>       * 当 `"format"` 为 `"int"` 或 `"double"` 数据类型时，`"min"`，`"max"` 和 `"step"` 分别表示属性值的最小值，最大值和最小变化值
>       * 当 `"format"` 为 `"string"` 或 `"json"` 数据类型时，`"min"` 和 `"max"` 分别表示支持的字符串的最小长度和最大长度，无关键字 `"step"`
>   * `"perms"` 属性的操作权限，以二进制整数解析，第一位表示 `读权限`，第二位表示 `写权限`，第三位表示 `执行权限`，`0` 表示禁止，`1` 表示允许
>       * 若参数没有读权限，则无法获得对应的值
>       * 若参数没有写权限，则无法修改对应的值
>       * 若参数没有执行权限，则无法执行设置对应的互联事件
> * `"status_code"` 请求命令的回复值， `0` 表示正常，`-1` 表示错误

#### 3.4.2. 获取设备状态：get_status

**Request:**

    {
        "request": "get_status",
        "cids": [
            0,
            1,
            2
        ]
    }

> * `"cids"` 设备属性字段，后接请求的 CID 值列表。

**Response:**

    {
        "characteristics": [
            {
                "cid": 0,
                "value": 0
            },
            {
                "cid": 1,
                "value": 0
            },
            {
                "cid": 2,
                "value": 100
            }
        ],
        "status_code": 0
    }

> * `"status_code"` 请求命令的回复值，`0` 表示正常，`-1` 表示请求中有非法参数，包括设备无对应的 CID 或 `"cids"` 列表中包含没有读权限的值。

#### 3.4.3. 修改设备状态

**Request:**

    {
        "request": "set_status",
        "characteristics": [
            {
                "cid": 0,
                "value": 0
            },
            {
                "cid": 1,
                "value": 0
            },
            {
                "cid": 2,
                "value": 100
            }
        ]
    }

**Response:**

    {
        "status_code": 0
    }

> * `"status_code"` 请求命令的回复值，`0` 表示正常，`-1` 表示请求中有非法参数，包括设备无对应的 CID 或 `"cids"` 列表中包含没有写权限的值。

#### 3.4.4. 进入配网模式

**Request:**

    {
        "request": "config_network"
    }

**Response:**

    {
        "status_code": 0
    }

> * `"status_code"` 请求命令的回复值，`0` 表示正常，`-1` 表示错误。

#### 3.4.5. 重启设备：reboot

**Request:**

    {
        "request": "reboot",
        "delay": 50
    }

> `"delay"` 命令延时执行时间，该字段非必需，若不设，使用设备端默认延时 `2s`。


**Response:**

    {
        "status_code": 0
    }

> * `"status_code"` 请求命令的回复值，`0` 表示正常，`-1` 表示错误。

#### 3.4.6. 恢复出厂设置：reset

**Request:**

    {
        "request": "reset",
        "delay": 50
    }

> `"delay"` 命令延时执行时间，该字段非必需，若不设，使用设备端默认延时 `2s`。

**Response:**

    {
        "status_code": 0
    }

> * `"status_code"` 请求命令的回复值，`0` 表示正常，`-1` 表示错误。

### 3.5. 设备状态通知

当 ESP-MDF 设备状态（打开或关闭）、网络连接关系（连接或断开）、路由表（route table）信息等发生变化时，根节点会主动发送 UDP 广播，通知 app 来获取设备的最新状态。


**UDP Broadcast:**

    mac=112233445566
    flag=1234
    type=***

> * `"mac"` 状态发生变化的设备的 mac 地址
> * `"flag"` 为一个随机整数值，用于区分不同时刻的通知
> * `"type"` 状态变化的类型，包含以下几种：
>   * `"status"` 设备属性状态发生变化
>   * `"https"` 设备拓扑结构发生变化，并要求采用 https 通信协议获取设备拓扑结构
>   * `"http"` 设备拓扑结构发生变化，并要求采用 http 通信协议获取设备拓扑结构
>   * `"sniffer"` 监听到新的联网设备

### 3.6 本地互联控制

#### 3.6.1. 实现步骤

1. `获取设备信息`：获取设备已存在的互联控制信息。

2. `关联设备`：对关联设备（发起互联控制操作的设备）进行关联设置（如设定光敏传感器，当检测到光照强度低于某一个阈值时发送控制命令打开特定的电灯），关联设置完成后 app 会将关联数据按协议格式转成控制指令，发送给关联设备。

3. `触发互联控制`：当关联设备接收到关联设置指令后，将指令解析成 `触发条件` 和 `执行指令`。之后设备便会定时检查自身状态，当状态满足 `触发条件` 时，主动向目标设备发送 `执行指令`。

#### 3.6.2. ESP-MDF 本地互联控制协议说明

#### 3.6.2.1. 获取已有互联事件

**Request:**

    {
        "request": "get_event"
    }

> * `"get_event"` 获取设备互联控制数据的命令。
**Response:**

    {
        "events": {
            "name": "on",
            "trigger_content": {
                "request": "contorl"
            },
            "trigger_cid": 2,
            "trigger_compare": {
                ">": 1,
                "~": 10
            },
            "execute_mac": [
                "30aea4064060"
            ]
        },
        "status_code": 0
    }

> * `"events"` 互联控制事件
>   * `"name"` 互联事件的名称，是互联事件的唯一标识
>   * `"trigger_content"` 互联事件的内容
>       * `"request"` 互联事件的类型
>           * `"sync"` 同步，同步两个设备，保持状态一致
>           * `"linkage"` 关联，一般的关联控制，当触发条件满足时发送触发命令
>           * `"delay"` 延时，表示在触发条件满足时，延迟给定的时间（单位秒）后发送触发命令。
>   * `"trigger_cid"` 关联设备的属性值，是互联事件的检测对象
>   * `"trigger_compare"` 互联事件的判断条件，用于比较关联设备的实际值与设置的触发值
>       * `">"` 大于
>       * `"<"` 小于
>       * `"=="` 等于
>       * `"!="` 不等于
>       * `"~"` 单位时间（秒）内的变化量
>       * `"/"` 单位时间（秒）上升到某个值
>       * `"\\"` 单位时间（秒）下降到某个值
>   * `"execute_mac"` 互联事件的目标地址，及执行指令的执行设备地址
> * `"status_code"` 请求命令的回复值， `0` 表示正常，`-1` 表示错误

#### 3.6.2.2. 设置互联事件：set_event

互联控制的协议主要在于对触发条件的理解和转换，即如何将用户设置的条件转成计算机可理解的条件判断。

**Request:**

    {
        "request": "set_event",
        "events": {
            "name": "on",
            "trigger_cid": 0,
            "trigger_content": {
                "request": "linkage"
            },
            "trigger_compare": {
                "==": 0,
                "~": 1
            },
            "execute_mac": [
                "30aea457e200",
                "30aea457dfe0"
            ],
            "execute_content": {
                "request": "set_status",
                "characteristics": [
                    {
                        "cid": 0,
                        "value": 1
                    }
                ]
            }
        }
    }

> 注：如果设备端无此事件则添加该事件，若存在则修改事件。
>
> * `"set_event"` 设置设备互联控制数据的命令
> * `"events"` 互联控制事件
>   * `"name"` 互联事件的名称，是互联事件的唯一标识
>   * `"trigger_cid"` 关联设备的属性值，是互联事件的检测对象
>   * `"trigger_content"` 互联事件的内容，其中互联事件类型包括以下三种：`sync`、`linkage` 和 `delay`
>   * `"trigger_compare"` 互联事件的判断条件，用于比较关联设备的实际值与设置的触发值，包括以下七种：`>`、`<`、`==`、`!=`、`~`、`/` 和 `\\`
>   * `"execute_mac"` 互联事件的目标地址，即执行指令的设备地址
>   * `"execute_content"` 执行指令内容，当触发条件满足时，发送该命令到 `"execute_mac"`

**Response:**

    {
        "status_code": 0
    }

#### 3.6.2.3. 删除互联事件：remove_event

**Request:**

    {
        "request": "remove_event",
        "events": [
            {
                "name": "on"
            },
            {
                "name": "off"
            }
        ]
    }

**Response:**

    {
        "status_code": 0
    }
