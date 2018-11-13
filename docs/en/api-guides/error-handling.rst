Error Handling
================

Overview
---------

This document introduces the ESP-MDF related error debugging methods, and provides some common error handling patterns.

Error Debugging
----------------

Error Codes
^^^^^^^^^^^^

Most of the functions specific to ESP-MDF use :cpp:type:`mdf_err_t` type to return error codes. :cpp:type:`mdf_err_t` is a signed integer type. Success (no error) is indicated with ``MDF_OK`` code, which is defined as zero.

Various ESP-MDF header files define possible error codes. Usually these definitions start with ``MDF_ERR_``  prefix. Common error codes for generic failures (out of memory, timeout, invalid argument, etc.) are defined in ``mdf_err.h`` file. Various components in ESP-MDF may define additional error codes for specific situations. The macro definition that starts with ``MDF_ERROR_`` prefix can be used to check the return value, and return the file name and the line number in case of errors. Then the value can be converted to an error code name with :cpp:func:`esp_err_to_name`, which makes it easier to understand which error has happened.

For the complete list of the error codes, please refer to :doc:`Error Codes <../api-reference/error-codes>`.

Common Configuration
^^^^^^^^^^^^^^^^^^^^^^^

It is recommended to use this pattern only when ``Debugging``, but not in the production process. Please use the following configuration to identify the issue and refer to: examples/sdkconfig.defaults::

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

Fatal Errors
^^^^^^^^^^^^^

In case of CPU exceptions or other fatal errors, panic handler may print CPU registers and the backtrace.
Backtrace line contains PC:SP pairs, where PC is the Program Counter and SP is Stack Pointer. If a fatal error happens inside an ISR (Interrupt Service Routine), the backtrace may include PC:SP pairs both from the task which was interrupted, and from the ISR.

For instructions on diagnosing unrecoverable errors, please refer to `Fatal Errors <https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/fatal-errors.html?highlight=fatal%20error>`_.

Use IDF Monitor
^^^^^^^^^^^^^^^^

If ``IDF Monitor`` is used, Program Counter values will be converted to code locations (function name, file name, and line number)::

    Guru Meditation Error: Core  0 panic'ed (StoreProhibited). Exception was unhandled.
    Core 0 register dump:
    PC      : 0x400d33ed  PS      : 0x00060f30  A0      : 0x800d1055  A1      : 0x3ffba950
    0x400d33ed: app_main at /home/zzc/project/esp-mdf-master/examples/get_started/main/get_started.c:200

    A2      : 0x00000000  A3      : 0x00000000  A4      : 0x00000000  A5      : 0x3ffbaaf4
    A6      : 0x00000001  A7      : 0x00000000  A8      : 0x800d2306  A9      : 0x3ffbaa10
    A10     : 0x3ffb0834  A11     : 0x3ffb3730  A12     : 0x40082784  A13     : 0x06ff1ff8
    0x40082784: _calloc_r at /home/zzc/project/esp-mdf-master/esp-idf/components/newlib/syscalls.c:51

    A14     : 0x3ffafff4  A15     : 0x00060023  SAR     : 0x00000014  EXCCAUSE: 0x0000001d
    EXCVADDR: 0x00000000  LBEG    : 0x4000c46c  LEND    : 0x4000c477  LCOUNT  : 0xffffffff

    Backtrace: 0x400d33ed:0x3ffba950 0x400d1052:0x3ffbaa60
    0x400d33ed: app_main at /home/zzc/project/esp-mdf-master/examples/get_started/main/get_started.c:200

    0x400d1052: main_task at /home/zzc/project/esp-mdf-master/esp-idf/components/esp32/cpu_start.c:476

To find the location where a fatal error has happened, look at the lines which follow the "Backtrace" line. Fatal error location is the top line, and subsequent lines show the call stack.

Don't Use IDF Monitor
^^^^^^^^^^^^^^^^^^^^^^

If IDF Monitor is not used, only the backtrace may be acquired, and you need to use ``xtensa-esp32-elf-addr2line`` command to convert it to code location::

    Guru Meditation Error: Core  0 panic'ed (StoreProhibited). Exception was unhandled.
    Core 0 register dump:
    PC      : 0x400d33ed  PS      : 0x00060f30  A0      : 0x800d1055  A1      : 0x3ffba950
    A2      : 0x00000000  A3      : 0x00000000  A4      : 0x00000000  A5      : 0x3ffbaaf4
    A6      : 0x00000001  A7      : 0x00000000  A8      : 0x800d2306  A9      : 0x3ffbaa10
    A10     : 0x3ffb0834  A11     : 0x3ffb3730  A12     : 0x40082784  A13     : 0x06ff1ff8
    A14     : 0x3ffafff4  A15     : 0x00060023  SAR     : 0x00000014  EXCCAUSE: 0x0000001d
    EXCVADDR: 0x00000000  LBEG    : 0x4000c46c  LEND    : 0x4000c477  LCOUNT  : 0xffffffff

    Backtrace: 0x400d33ed:0x3ffba950 0x400d1052:0x3ffbaa60

Navigate to your project, and input the following command in your terminal::

    xtensa-esp32-elf-addr2line -pfia -e build/*.elf Backtrace: 0x400d33ed:0x3ffba950 0x400d1052:0x3ffbaa60

The output would look like this::

    0x00000bac: ?? ??:0
    0x400d33ed: app_main at /home/zzc/project/esp-mdf-master/examples/get_started/main/get_started.c:200
    0x400d1052: main_task at /home/zzc/project/esp-mdf-master/esp-idf/components/esp32/cpu_start.c:476

Heap Memory Debugging
^^^^^^^^^^^^^^^^^^^^^^^^

ESP-IDF integrates tools for requesting heap information, detecting heap corruption, and tracing memory leaks. For details, please refer to: `Heap Memory Debugging <https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/system/heap_debug.html?highlight=Heap%20Memory%20Debugging>`_, If you use the APIs in ``mdf_mem.h``, you can also use these tools. :cpp:func:`mdf_mem_print_record` may print all the unreleased memory, and help quickly identify the memory leak issue::

    I (1448) [mdf_mem, 95]: Memory record, num: 4
    I (1448) [mdf_mem, 100]: <mwifi : 181> ptr: 0x3ffc5f2c, size: 28
    I (1458) [mdf_mem, 100]: <mwifi : 401> ptr: 0x3ffc8fd4, size: 174
    I (1468) [mdf_mem, 100]: <get_started : 96> ptr: 0x3ffd3cd8, size: 1456
    I (1468) [mdf_mem, 100]: <get_started : 66> ptr: 0x3ffd5400, size: 1456


.. Note::

    1. Configuration: enable :envvar:`CONFIG_MDF_MEM_DEBUG` with ``make menuconfig``;
    2. Only the memory allocated and released with ``MDF_*ALLOC`` and ``MDF_FREE`` respectively will be logged.


Task Schedule
^^^^^^^^^^^^^^

:cpp:func:`mdf_mem_print_heap` can be used to acquire the running status, priority and remaining stack space of all the tasks::

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

    1. :cpp:func:`mdf_mem_print_heap` can be called to suspend all the tasks, which may take a while. Therefore, it is recommended to use this function only for debugging;
    2. Configuration: enable :envvar:`CONFIG_FREERTOS_USE_TRACE_FACILITY` and :envvar:`CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS` with ``make menuconfig``;
    3. Status: R indicates ready status, and B indicates blocked status;
    4. Remaining space: HWM (High Water Mark) must be no less than 512 bytes, to avoid stack overflow.

Common Errors
--------------

Compilation Errors
^^^^^^^^^^^^^^^^^^^^

1. ``MDF_PATH`` **is not set**：

esp-mdf cannot be found if ``MDF_PATH`` environment variables are not set::

    Makefile:8: /project.mk: No such file or directory
    make: *** No rule to make target '/project.mk'.  Stop.


- Solution:
    Input the below command to configure::

        $ export MDF_PATH=/home/zzc/project/esp-mdf

    Input the below command to verify::

        $ echo $MDF_PATH
        /home/zzc/project/esp-mdf


2. **The acquired project is not complete**

``--recursive`` option is missed when getting the project with `git clone`, so the submodules under esp-mdf are not cloned::

    /home/zzc/project/esp-mdf/project.mk:9: /home/zzc/project/esp-mdf/esp-idf/make/project.mk: No such file or directory
    make: *** No rule to make target '/home/zzc/project/esp-mdf/esp-idf/make/project.mk'.  Stop.

- Solution:
    Run the command below to re-get the submodules

    ```shell
    cd $MDF_PAHT
    git submodule update --init
    ```

Flashing Errors
^^^^^^^^^^^^^^^^^^

1. **Serial port permission is limited**

In linux, Text telephone devices (TTYs) belong to the dialout group, and ordinary users don't have the access::

    serial.serialutil.SerialException: [Errno 13] could not open port /dev/ttyUSB0: [Errno 13] Permission denied: '/dev/ttyUSB0'


- **Solution:**

    1. Modify the permission directly::

        sudo chmod 0666 /dev/ttyUSB0

    2. Add the users with limited access to the dialout group, so they may have access to the TTYs and other similar devices::

        sudo gpasswd --add <user> dialout

2. ``make flash`` **Errors**

Incompatibility between python and pyserial::

    AttributeError: 'Serial' object has no attribute 'dtr'
    AttributeError: 'module' object has no attribute 'serial_for_url'

- Solution:
    Run the below command. If the issue is still pending, you may go to `esptool issues <https://github.com/espressif/esptool/issues>`_ and search for any related issues::

        sudo apt-get update
        sudo apt-get upgrade
        sudo pip install esptool
        sudo pip install --ignore-installed pyserial


ESP-MESH Errors
^^^^^^^^^^^^^^^^^

1. **The device cannot connect to the router**

The router name and password are configured correctly, but still the device cannot connect to the router. In this case, the log would look like this::

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


- Possible Reasons:
    1. The ESP-MESH channel is not configured: For a quick network configuration, ESP-MESH scans only on a fixed channel, and this channel must be configured;
    2. Connect to a hidden router: The router's BSSID must be configured when ESP-MESH connects with a hidden router;
    3. The router's channel is usually not fixed, and it will switch channels accordingly if the network condition changes.

- Solution:
    1. Fixate the router's channel, and set the channel and the router's BSSID;
    2. Allow the device automatically get the router information with :cpp:func:`mwifi_scan`, but it will increase the network configuration time.