错误处理
========

概述
----

本指南介绍了 ESP-MDF 错误调试方式，并提供了一些常见的错误处理方式。

错误调试
---------

错误代码
^^^^^^^^^

大多数特定于 ESP-MDF 的函数使用 :cpp:type:`mdf_err_t` 类型来返回错误代码。:cpp:type:`mdf_err_t` 是有符号整数类型。``MDF_OK`` 代码表示成功（无错误），代码定义为零。
各种 ESP-MDF 头文件定义可能的错误代码。通常这些定义以``MDF_ERR_`` 前缀开头。通用故障的常见错误代码（内存不足，超时，无效参数等）在 ``mdf_err.h`` 文件中定义。ESP-MDF 中的各种组件可以为特定情况定义附加的错误代码。使用 ``MDF_ERROR_`` 前缀开头的宏定义去检查返回值，出错时会直接返回文件名和行号，并通过 :cpp:func:`esp_err_to_name` 将值转换为错误代码的名称，以便更容易理解发生了哪个错误。

有关错误代码的完整列表，请参阅 :doc:`错误代码 <../api-reference/error-codes>`。

常用配置
^^^^^^^^^

建议仅在 ``调试`` 时启用此模式，而不是在生产中，请使用如下配置以便定位问题，参见: examples/sdkconfig.defaults::

    #
    # FreeRTOS
    #
    CONFIG_FREERTOS_UNICORE=y              # Debug portMUX Recursion
    CONFIG_TIMER_TASK_STACK_DEPTH=3072     # FreeRTOS timer task stack size

    #
    # ESP32-specific
    #
    CONFIG_TASK_WDT_PANIC=y                # Invoke panic handler on Task Watchdog timeout
    CONFIG_TASK_WDT_TIMEOUT_S=10           # Task Watchdog timeout period (seconds)
    CONFIG_ESP32_ENABLE_COREDUMP=y         # Core dump destination
    CONFIG_TIMER_TASK_STACK_SIZE=4096      # High-resolution timer task stack size
    CONFIG_ESP32_ENABLE_COREDUMP_TO_FLASH=y # Select flash to store core dump

致命错误
^^^^^^^^^

CPU 异常或其他致命错误发生时，panic 处理程序会输出记录一些 CPU 寄存器和 Backtrace。
Backtrace 包含 PC:SP 对。其中 PC 是程序计数器，SP 是堆栈指针。如果在中断内发生致命错误，则 Backtrace 可能包括来自被中断的任务和中断的 PC:SP 对。
有关诊断不可恢复错误的说明，请参阅 `致命错误 <https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/fatal-errors.html?highlight=fatal%20error>`_。

使用 IDF Monitor
^^^^^^^^^^^^^^^^

如果使用 ``IDF Monitor``，程序计数器值将转换为代码位置（函数名称，文件名和行号）::

    Guru Meditation Error: Core  0 panic'ed (StoreProhibited). Exception was unhandled.
    Core 0 register dump:
    PC      : 0x400d33ed  PS      : 0x00060f30  A0      : 0x800d1055  A1      : 0x3ffba950
    0x400d33ed: app_main at ~/project/esp-mdf-master/examples/get_started/main/get_started.c:200

    A2      : 0x00000000  A3      : 0x00000000  A4      : 0x00000000  A5      : 0x3ffbaaf4
    A6      : 0x00000001  A7      : 0x00000000  A8      : 0x800d2306  A9      : 0x3ffbaa10
    A10     : 0x3ffb0834  A11     : 0x3ffb3730  A12     : 0x40082784  A13     : 0x06ff1ff8
    0x40082784: _calloc_r at ~/project/esp-mdf-master/esp-idf/components/newlib/syscalls.c:51

    A14     : 0x3ffafff4  A15     : 0x00060023  SAR     : 0x00000014  EXCCAUSE: 0x0000001d
    EXCVADDR: 0x00000000  LBEG    : 0x4000c46c  LEND    : 0x4000c477  LCOUNT  : 0xffffffff

    Backtrace: 0x400d33ed:0x3ffba950 0x400d1052:0x3ffbaa60
    0x400d33ed: app_main at ~/project/esp-mdf-master/examples/get_started/main/get_started.c:200

    0x400d1052: main_task at ~/project/esp-mdf-master/esp-idf/components/esp32/cpu_start.c:476

要查找发生致命错误的位置，请查看“Backtrace”行后面的行。致命错误位置是紧跟“Backtrace”行的后一行，后续行则显示调用堆栈。

未使用 IDF Monitor
^^^^^^^^^^^^^^^^^^

如果未使用 IDF Monitor，您将只能获取到 Backtrace，您需要使用 ``xtensa-esp32-elf-addr2line`` 命令转换为代码位置::

    Guru Meditation Error: Core  0 panic'ed (StoreProhibited). Exception was unhandled.
    Core 0 register dump:
    PC      : 0x400d33ed  PS      : 0x00060f30  A0      : 0x800d1055  A1      : 0x3ffba950
    A2      : 0x00000000  A3      : 0x00000000  A4      : 0x00000000  A5      : 0x3ffbaaf4
    A6      : 0x00000001  A7      : 0x00000000  A8      : 0x800d2306  A9      : 0x3ffbaa10
    A10     : 0x3ffb0834  A11     : 0x3ffb3730  A12     : 0x40082784  A13     : 0x06ff1ff8
    A14     : 0x3ffafff4  A15     : 0x00060023  SAR     : 0x00000014  EXCCAUSE: 0x0000001d
    EXCVADDR: 0x00000000  LBEG    : 0x4000c46c  LEND    : 0x4000c477  LCOUNT  : 0xffffffff

    Backtrace: 0x400d33ed:0x3ffba950 0x400d1052:0x3ffbaa60

跳转到您的工程下，并在终端输入如下命令::

    xtensa-esp32-elf-addr2line -pfia -e build/*.elf Backtrace: 0x400d33ed:0x3ffba950 0x400d1052:0x3ffbaa60

转换结果如下::

    0x00000bac: ?? ??:0
    0x400d33ed: app_main at ~/project/esp-mdf-master/examples/get_started/main/get_started.c:200
    0x400d1052: main_task at ~/project/esp-mdf-master/esp-idf/components/esp32/cpu_start.c:476

内存调试
^^^^^^^^^

ESP-IDF 集成了用于请求堆信息，检测堆损坏和跟踪内存泄漏的工具，详见：`Heap Memory Debugging <https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/system/heap_debug.html?highlight=Heap%20Memory%20Debugging>`_，如果您使用的是 ``mdf_mem.h`` 中相关的 APIs，也能使用。 :cpp:func:`mdf_mem_print_record` 可以打印所有未释放的内存，快速定位内存泄露的问题::

    I (1448) [mdf_mem, 95]: Memory record, num: 4
    I (1448) [mdf_mem, 100]: <mwifi : 181> ptr: 0x3ffc5f2c, size: 28
    I (1458) [mdf_mem, 100]: <mwifi : 401> ptr: 0x3ffc8fd4, size: 174
    I (1468) [mdf_mem, 100]: <get_started : 96> ptr: 0x3ffd3cd8, size: 1456
    I (1468) [mdf_mem, 100]: <get_started : 66> ptr: 0x3ffd5400, size: 1456


.. Note::

    1. 配置：通过 ``make menuconfig`` 开启 :envvar:`CONFIG_MDF_MEM_DEBUG`；
    2. 仅记录使用 ``MDF_*ALLOC`` 和 ``MDF_FREE`` 申请和释放的内存空间


任务调度
^^^^^^^^^

使用 :cpp:func:`mdf_mem_print_heap` 可以获取所有任务的运行状态、优先级和栈的剩余空间::

    Task Name       Status  Prio    HWM     Task
    main            R       1       1800    3
    IDLE            R       0       1232    4
    node_write_task B       6       2572    16
    node_read_task  B       6       2484    17
    Tmr Svc         B       1       1648    5
    tiT             B       18      1576    7
    MEVT            B       20      2080    10
    eventTask       B       20      2032    8
    MTXBLK          B       7       2068    11
    MTX             B       10      1856    12
    MTXON           B       11      2012    13
    MRX             B       13      2600    14
    MNWK            B       15      2700    15
    mdf_event_loop  B       10      2552    6
    esp_timer       B       22      3492    1
    wifi            B       23      1476    9
    ipc0            B       24      636     2

    Current task, Name: main, HWM: 1800
    Free heap, current: 170884, minimum: 169876

.. Note::

    1. 调用 :cpp:func:`mdf_mem_print_heap` 会挂起所有任务，这一过程可能持续较长时间，因此建议本函数仅在调试时使用；
    2. 配置：通过 ``make menuconfig`` 开启 :envvar:`CONFIG_FREERTOS_USE_TRACE_FACILITY` 和 :envvar:`CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS`；
    3. 状态：R（Ready）代表准备态，B（blocked）代表阻塞态；
    4. 剩余空间：HWM（High Water Mark）应不小于 512 byte，防止栈溢出。

常见错误
--------

编译错误
^^^^^^^^^

1. ``MDF_PATH`` **未设置**：

未设置 ``MDF_PATH`` 环境变量编译时找不到 esp-mdf::

    Makefile:8: /project.mk: No such file or directory
    make: *** No rule to make target '/project.mk'.  Stop.


- 解决方法：
    输入如下命令进行配置::

        $ export MDF_PATH=~/project/esp-mdf

    输入如下命令进行验证::

        $ echo $MDF_PATH
        ~/project/esp-mdf


2. **获取工程不完整**

通过 `git clone` 获取工程时，没有带有 ``--recursive`` 标志，以至于 esp-mdf 的子工程没有被获取::

    ~/project/esp-mdf/project.mk:9: ~/project/esp-mdf/esp-idf/make/project.mk: No such file or directory
    make: *** No rule to make target '~/project/esp-mdf/esp-idf/make/project.mk'.  Stop.

- 解决方法：
    运行如下命令重新获取子工程

    ```shell
    cd $MDF_PAHT
    git submodule update --init
    ```

烧录错误
^^^^^^^^^

1. **串口权限不足**

在 linux 系统下，TTY 设备隶属于 dialout 用户组，普通用户没有权限访问::

    serial.serialutil.SerialException: [Errno 13] could not open port /dev/ttyUSB0: [Errno 13] Permission denied: '/dev/ttyUSB0'


- **解决方法：**

    1. 直接修改串口权限::

        sudo chmod 0666 /dev/ttyUSB0

    2. 将用户添加至 dialout 用户组，该用户就会具备访问 TTY 等串口设备的权限::

        sudo gpasswd --add <user> dialout

2. ``make flash`` **错误**

python 与 pyserial 版本不兼容::

    AttributeError: 'Serial' object has no attribute 'dtr'
    AttributeError: 'module' object has no attribute 'serial_for_url'

- 解决方法：
    运行如下命令，如果问题还未解决，你可以到 `esptool issues <https://github.com/espressif/esptool/issues>`_ 查找是否有相同的问题::

        sudo apt-get update
        sudo apt-get upgrade
        sudo pip install esptool
        sudo pip install --ignore-installed pyserial


ESP-MESH 错误
^^^^^^^^^^^^^^

1. **设备无法连接路由器**

路由器名称与密码配置正确，但设备无法连接路由器，日志如下::

    I (2917) mesh: [SCAN][ch:1]AP:1, otherID:0, MAP:0, idle:0, candidate:0, root:0, topMAP:0[c:0,i:0]<>
    I (2917) mesh: [FAIL][1]root:0, fail:1, normal:0, <pre>backoff:0

    I (3227) mesh: [SCAN][ch:1]AP:1, otherID:0, MAP:0, idle:0, candidate:0, root:0, topMAP:0[c:0,i:0]<>
    I (3227) mesh: [FAIL][2]root:0, fail:2, normal:0, <pre>backoff:0

    I (3527) mesh: [SCAN][ch:1]AP:2, otherID:0, MAP:0, idle:0, candidate:0, root:0, topMAP:0[c:0,i:0]<>
    I (3527) mesh: [FAIL][3]root:0, fail:3, normal:0, <pre>backoff:0

    I (3837) mesh: [SCAN][ch:1]AP:2, otherID:0, MAP:0, idle:0, candidate:0, root:0, topMAP:0[c:0,i:0]<>
    I (3837) mesh: [FAIL][4]root:0, fail:4, normal:0, <pre>backoff:0

    I (4137) mesh: [SCAN][ch:1]AP:2, otherID:0, MAP:0, idle:0, candidate:0, root:0, topMAP:0[c:0,i:0]<>
    I (4137) mesh: [FAIL][5]root:0, fail:5, normal:0, <pre>backoff:0


- 原因：
    1. 未配置信道：ESP-MESH 为了更快速的进行组网，因此只在固定的一个信道上进行扫描，因此必须配置 ESP-MESH 的工作信道；
    2. 连接隐藏路由器：当 ESP-MESH 连接隐藏路由器时，必须配置路由器的 BSSID；
    3. 路由器信道通常是非固定的，路由器会根据网络情况进行信道迁移。

- 解决方式：
    1. 固定路由器的信道并配置路由器的信道和 BSSID；
    2. 通过 :cpp:func:`mwifi_scan` 让设备自动去获取路由器信息，但这会增加组网时间。