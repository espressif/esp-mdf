Mdebug API
============

``Mdebug`` (Mesh Network debug) is an important debugging solution used in ESP-MDF. It is designed to efficiently obtain ESP-MDF device logs through wireless espnow protocol, TCP protocol, serial port, etc., so that it can be read more conveniently and quickly. The device log information can then be analyzed according to the extracted logs.

.. ------------------------- Mdebug App Reference ---------------------------

Application Examples
---------------------

For ESP-MDF examples, please refer to the directory :example:`function_demo/Mdebug`, which includes:

   * The log information of the child node is transmitted to the root node through the `ESP-Mesh` network, and then outputted through the serial port of the root node or transmitted to the server through http;
   * Output the desired log information through the console.

.. -------------------------------------------------------- UART ENABLE -----------------------------------------------------------

1. UART enable
^^^^^^^^^^^^^^^^^^^^^

The serial port is enabled and the log information will be printed out via ``vprintf``. Enable the following program:

    .. code:: c

      if (config->log_uart_enable) { /**< Set log uart enable */
          mdebug_log_init();
      } else {
          mdebug_log_deinit();
      }

.. -------------------------------------------------------- Flash ENABLE -----------------------------------------------------------

2. Flash enable
^^^^^^^^^^^^^^^^^^^^^^

Write flash enable to store log information in flash. Enable the following program:

      .. code:: c

         if (config->log_flash_enable) { /**< Set log flash enable */
               mdebug_flash_init();
         } else {
               mdebug_flash_deinit();
         }

    2.1 Log information access

        1. The log is initialized, creating two text flash memory spaces, using the main functions as ``esp_vfs_spiffs_register``, ``esp_spiffs_info``, ``sprintf``, ``fopen``;

         .. code:: c

            /**< Create spiffs file and number of files */
            esp_vfs_spiffs_conf_t conf = {
                .base_path       = "/spiffs",
                .partition_label = NULL,
                .max_files       = MDEBUG_FLASH_FILE_MAX_NUM,
                .format_if_mount_failed = true
            };

        2. Write the log data, and write down the address of the write pointer. In order to address the next log write flash, use the main function as ``fwrite``. The following procedure:

         .. code:: c

            /**< Record the address of each write pointer */
            fseek(g_log_info[g_log_index].fp, g_log_info[g_log_index].size, SEEK_SET);
            ret = fwrite(data, 1, size, g_log_info[g_log_index].fp);

        3. Read the log data, and write down the address of the read pointer. For the next time to read the log from the flash, address the address, use the main function as ``fread``. The following procedure:

         .. code:: c

            /**< Record the pointer address for each read */
            flash_log_info_t *log_info = g_log_info + ((g_log_index + 1 + i) % MDEBUG_FLASH_FILE_MAX_NUM);
            fseek(log_info->fp, log_info->offset, SEEK_SET);

            ret = fread(data, 1, MIN(*size - read_size, log_info->size - log_info->offset), log_info->fp);

        4. The data is erased. When the data is full, the data pointer will be cleared, and the main function ``rewind`` will be restarted from the beginning or the end of the file header address. The following procedure:

         .. code:: c

            /**< Erase file address pointer */
            for (size_t i = 0; i < MDEBUG_FLASH_FILE_MAX_NUM; i++) {
                g_log_index          = 0;
                g_log_info[i].size   = 0;
                g_log_info[i].offset = 0;
                rewind(g_log_info[i].fp);
                mdf_info_save(MDEBUG_FLASH_STORE_KEY, g_log_info, sizeof(flash_log_info_t) * MDEBUG_FLASH_FILE_MAX_NUM);
            }

    .. Note::

        1. The header of the log data is an added timestamp. It is only used as an experiment, and there is no real-time calibration. Users can modify it according to their own needs. The following procedure:

        .. code:: c

          /**< Get the current timestamp */
          time(&now);
          localtime_r(&now, &timeinfo);
          strftime(strtime_buf, 32, "\n[%Y-%m-%d %H:%M:%S] ", &timeinfo);

        2. Extract and select valid string data information. Because the log information in the MDF has unnecessary data at the beginning and the end, it needs to be removed. The following procedure removes unwanted data from the head and tail:

        .. code:: c
        
          /**< Remove the header and tail that appear in the string in the log */
          if (log_data->data[0] == 0x1b) {
              data = log_data->data + 7;
              size = log_data->size - 12;
          }

.. -------------------------------------------------------- ESPNOW ENABLE -----------------------------------------------------------

3. ESPNOW enable
^^^^^^^^^^^^^^^^^^^^^

Espnow can be enabled to send the log data of the child node to the root node through espnow, so that the log information of the child node is read from the root node. Enable the following program:

.. code:: c

  /**< config espnow enable */
  if (config->log_espnow_enable) {
      mdebug_espnow_init();
  } else {
      mdebug_espnow_deinit();
  }

Write the log to espnow. The procedure is as follows:

.. code:: c

  /**< write log information by espnow */
  if (!g_log_config->log_espnow_enable && !MDEBUG_ADDR_IS_EMPTY(g_log_config->dest_addr)) {
      log_data->type |= MDEBUG_LOG_TYPE_ESPNOW;
  }
  
  if (log_data->type & MDEBUG_LOG_TYPE_ESPNOW) {
      mdebug_espnow_write(g_log_config->dest_addr, log_data->data,
                          log_data->size, MDEBUG_ESPNOW_LOG, pdMS_TO_TICKS(MDEBUG_LOG_TIMEOUT_MS));
  }

.. ------------------------- Mdebug API Reference ---------------------------

.. _api-reference-mdebug_console:

Mdebug Console
--------------

.. include:: /_build/inc/mdebug_console.inc

.. _api-reference-mdebug_espnow.inc:

Mdebug Espnow
--------------

.. include:: /_build/inc/mdebug_espnow.inc

.. _api-reference-mdebug_flash:

Mdebug Flash
--------------

.. include:: /_build/inc/mdebug_flash.inc

.. _api-reference-mdebug_log:

Mdebug Log
----------------

.. include:: /_build/inc/mdebug_log.inc

.. _api-reference-mdebug:

Mdebug Partially Referenced Header File
--------------------------------------------

.. include:: /_build/inc/mdebug.inc
