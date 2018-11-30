Error Handling
================

Overview
---------

This document provides the introduction to the ESP-MDF related error debugging methods and to some common error handling patterns.

Error Debugging
----------------

Error Codes
^^^^^^^^^^^^

In most cases, the functions specific to ESP-MDF use the type :cpp:type:`mdf_err_t` to return error codes. This is a signed integer type. Success (no error) is indicated with the code ``MDF_OK``, which is defined as zero.

Various ESP-MDF header files define possible error codes. Usually, these definitions have the prefix ``MDF_ERR_``. Common error codes for generic failures (out of memory, timeout, invalid argument, etc.) are defined in the file ``mdf_err.h``. Various components in ESP-MDF can define additional error codes for specific situations. The macro definition that starts with ``MDF_ERROR_`` can be used to check the return value and return the file name and the line number in case of errors. After that, the value can be converted to an error code name with the function :cpp:func:`esp_err_to_name` to help you easier understand which error has happened.

For the complete list of error codes, please refer to :doc:`Error Codes <../api-reference/error-codes>`.

Common Configuration
^^^^^^^^^^^^^^^^^^^^^^^

The pattern given below is recommended for use during debugging and not in the production process. Please use the following configuration to identify the issue and then refer to: examples/sdkconfig.defaults::

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

In case of CPU exceptions or other fatal errors, the panic handler may print CPU registers and the backtrace.
A backtrace line contains PC:SP pairs, where PC stands for the program counter and SP is for the stack pointer. If a fatal error occurs inside an ISR (Interrupt Service Routine), the backtrace can include PC:SP pairs from both the ISR and the task that was interrupted.

For detailed instructions on diagnostics of unrecoverable errors, please refer to `Fatal Errors <https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/fatal-errors.html?highlight=fatal%20error>`_.

Use IDF Monitor
^^^^^^^^^^^^^^^^

If ``IDF Monitor`` is used, Program Counter values are converted into code locations, i.e., function name, file name, and line number::

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

To find the location where a fatal error has occurred, look at the lines which follow the ``Backtrace:`` line. The first line after the ``Backtrace:`` line indicates the fatal error location, and the subsequent lines show the call stack.

Do Not Use IDF Monitor
^^^^^^^^^^^^^^^^^^^^^^

If IDF Monitor is not used, only the backtrace can be acquired. You need to use the command ``xtensa-esp32-elf-addr2line`` to convert it to a code location::

    Guru Meditation Error: Core  0 panic'ed (StoreProhibited). Exception was unhandled.
    Core 0 register dump:
    PC      : 0x400d33ed  PS      : 0x00060f30  A0      : 0x800d1055  A1      : 0x3ffba950
    A2      : 0x00000000  A3      : 0x00000000  A4      : 0x00000000  A5      : 0x3ffbaaf4
    A6      : 0x00000001  A7      : 0x00000000  A8      : 0x800d2306  A9      : 0x3ffbaa10
    A10     : 0x3ffb0834  A11     : 0x3ffb3730  A12     : 0x40082784  A13     : 0x06ff1ff8
    A14     : 0x3ffafff4  A15     : 0x00060023  SAR     : 0x00000014  EXCCAUSE: 0x0000001d
    EXCVADDR: 0x00000000  LBEG    : 0x4000c46c  LEND    : 0x4000c477  LCOUNT  : 0xffffffff

    Backtrace: 0x400d33ed:0x3ffba950 0x400d1052:0x3ffbaa60

Open Terminal, navigate to your project directory, and execute the following command::

    xtensa-esp32-elf-addr2line -pfia -e build/*.elf Backtrace: 0x400d33ed:0x3ffba950 0x400d1052:0x3ffbaa60

The output would look as follows::

    0x00000bac: ?? ??:0
    0x400d33ed: app_main at ~/project/esp-mdf-master/examples/get_started/main/get_started.c:200
    0x400d1052: main_task at ~/project/esp-mdf-master/esp-idf/components/esp32/cpu_start.c:476

Heap Memory Debugging
^^^^^^^^^^^^^^^^^^^^^^^^

ESP-IDF integrates tools for requesting heap information, detecting heap corruption, and tracing memory leaks. For details, please refer to `Heap Memory Debugging <https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/system/heap_debug.html?highlight=Heap%20Memory%20Debugging>`_. 

If you use the APIs from ``mdf_mem.h``, you can also utilize these debugging tools. The function :cpp:func:`mdf_mem_print_record` can print all the unreleased memory and help quickly identify memory leak issues::

    I (1448) [mdf_mem, 95]: Memory record, num: 4
    I (1448) [mdf_mem, 100]: <mwifi : 181> ptr: 0x3ffc5f2c, size: 28
    I (1458) [mdf_mem, 100]: <mwifi : 401> ptr: 0x3ffc8fd4, size: 174
    I (1468) [mdf_mem, 100]: <get_started : 96> ptr: 0x3ffd3cd8, size: 1456
    I (1468) [mdf_mem, 100]: <get_started : 66> ptr: 0x3ffd5400, size: 1456


.. Note::

    1. Configuration: use ``make menuconfig`` to enable :envvar:`CONFIG_MDF_MEM_DEBUG`.
    2. Only the memory allocated and released with ``MDF_*ALLOC`` and ``MDF_FREE`` is logged.


Task Schedule
^^^^^^^^^^^^^^

The function :cpp:func:`mdf_mem_print_heap` can be used to acquire the running status, priority, and remaining stack space of all the tasks::

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

    1. The function :cpp:func:`mdf_mem_print_heap` can be called to suspend all the tasks, but it may take a while. For this reason, this function is recommended for use during debugging only.
    2. Configuration: use ``make menuconfig`` to enable :envvar:`CONFIG_FREERTOS_USE_TRACE_FACILITY` and :envvar:`CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS`.
    3. Status: ``R`` indicates the status `ready`, ``B`` indicates the status `blocked`.
    4. Remaining space: HWM (High Water Mark) must be set to no less than 512 bytes to avoid stack overflow.

Common Errors
--------------

Compilation Errors
^^^^^^^^^^^^^^^^^^^^

1. ``MDF_PATH`` **is not set**:

esp-mdf cannot be found if ``MDF_PATH`` environment variable is not set::

    Makefile:8: /project.mk: No such file or directory
    make: *** No rule to make target '/project.mk'.  Stop.


- Solution:
    Input the command given to configure::

        $ export MDF_PATH=~/project/esp-mdf

    Input the following command to verify::

        $ echo $MDF_PATH
        ~/project/esp-mdf


2. **The cloned project is not complete**

The option ``--recursive`` was missing at the time of getting the project with ``git clone``. For this reason, the submodules of esp-mdf were not cloned::

    ~/project/esp-mdf/project.mk:9: ~/project/esp-mdf/esp-idf/make/project.mk: No such file or directory
    make: *** No rule to make target '~/project/esp-mdf/esp-idf/make/project.mk'.  Stop.

- Solution:
    Run the command given below to re-get the submodules

    ```shell
    cd $MDF_PAHT
    git submodule update --init
    ```

Flashing Errors
^^^^^^^^^^^^^^^^^^

1. **Serial port permission is limited**

In Linux, text telephone devices (TTYs) belong to the dialout group to which regular users do not have access::

    serial.serialutil.SerialException: [Errno 13] could not open port /dev/ttyUSB0: [Errno 13] Permission denied: '/dev/ttyUSB0'


- **Solution:**

    1. Modify the permission directly::

        sudo chmod 0666 /dev/ttyUSB0

    2. Add the users with limited access to the dialout group, so that they may gain access to the TTYs and other similar devices::

        sudo gpasswd --add <user> dialout

2. ``make flash`` **Errors**

Incompatibility between python and pyserial::

    AttributeError: 'Serial' object has no attribute 'dtr'
    AttributeError: 'module' object has no attribute 'serial_for_url'

- Solution:
    Run the command given below. If the issue persists, you can go to `esptool issues <https://github.com/espressif/esptool/issues>`_ and search for any related issues::

        sudo apt-get update
        sudo apt-get upgrade
        sudo pip install esptool
        sudo pip install --ignore-installed pyserial


ESP-MESH Errors
^^^^^^^^^^^^^^^^^

1. **The device cannot connect to the router**

The router name and password have been configured correctly, but the device still cannot connect to the router. In this case, the log would look as follows::

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
    1. The ESP-MESH channel is not configured: For a quick network configuration, ESP-MESH scans only a fixed channel, and this channel must be configured.
    2. Connect to a hidden router: The router's BSSID must be configured when ESP-MESH connects to the hidden router.
    3. The router's channel is usually not fixed, so it switches channels in accordance with the changes in the network condition.

- Solution:
    1. Fixate the router's channel, and set the channel and the router's BSSID;
    2. Allow the device to automatically get the router information with :cpp:func:`mwifi_scan`. Although, it increases the network configuration time.