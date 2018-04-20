# 0. Overview
Considering the actual LED products, the Wi-Fi module firmware cannot be updated through the serial port due to product structure design and security. In this scenario, we provide a way to ensure that even if there is a serious operational abnormality in the device and an online upgrade cannot be performed (OTA mode), it can be restored to the initial factory mode. The implementation process is as follows.

# 1. Principle
In the ʻESP32` secondary bootloader stage, the image file to be run needs to be loaded from flash into memory, which takes about 500ms. The user can use this mechanism to achieve: when switching power (3 to 10 times, each time within 500ms), switch the system to the factory partition again to restore the device to the factory mode.

| Name | Type | SubType | Offset | Size |
| ---- | :--- | :------ | :----- | :--- |
| nvs      | data |      nvs |   0x9000 |   16k |
| otadata  | data |      ota |   0xd000 |    8k |
| phy_init | data |      phy |   0xf000 |    4k |
| coredump | data | coredump |  0x10000 |   32K |
| flag     | data |     0x99 |  0x18000 |   28K |
| rsa      | data |     0x9a |  0x1f000 |    4K |
| reserved | data |     0x9b |  0x20000 |   64K |
| factory  |  app |  factory |  0x30000 |  576K |
| ota_0    |  app |    ota_0 |  0xc0000 | 1664K |
| ota_1    |  app |    ota_1 | 0x260000 | 1664K |

The above is the contents of the partitions.csv file in the system partition table. During the system compilation process, the file will produce the partition.bin file, which will be burned into the flash along with the burning process, and will not be changed afterwards. The OTA process only modifies the ota_0 and ota_1 partitions and switches between them.

# 2. Process
> The commit of bootloader（when copied to this project's components directory) in ESP-IDF is：`0a83733d2d630e52fd2bdd33d5d9876fa7c1c5d3`

The specific approach is to copy the `components/bootloader` diredtory in [esp-idf] (https://github.com/espressif/esp-idf) to the `components` directory of the project so that it can overwrite the bootloader module with the same name as ESP-IDF. You can customize it by modifying the bootloader module in this project directory. The Boodloader module's execution flow is as follows:

```
|----------------------------|
|  Hardware initialization   |
|----------------------------|
              ||
              \/
|----------------------------|
|    Load bootloader.bin     |
|----------------------------|
              ||
              \/
|----------------------------------|
| Check&update flash configuration |
|----------------------------------|
              ||
              ||                  |----------------- `Modifications` ---------------|
              \/                  |                                                 |
|----------------------------|    |   |-------------------------------------------| |
|      Load partition.bin    | =====> |       Insert Code Seg(##3.2) to get       | |
|----------------------------|    |   |        addresss of flag partition         | |
                                  |   |-------------------------------------------| |
                                  |                      ||                         |
                                  |                      \/                         |
                                  |   |------------------------------------|        |
              || <=================== |    update `bootloader_cnt` (##3.5) |        |
              ||                  |   |------------------------------------|        |
              \/                  |                                                 |
|------------------------------|  |   |-------------------------------|             |
| Get boot partition and load  | <=== | set factory partition as boot |             |
|------------------------------|  |   |-------------------------------|             |
              ||                  |                    /\                           |
              ||                  |                    || Y                         |
              ||                  |   |----------------------------------------|    |
              || ===================> |    Check `bootloader_cnt` value,       |    |
                                  |   |       whether or not in [3,10]         |    |
                                  |   |----------------------------------------|    |
                                  |                    || N                         |
              || ===================================== ||                           |
              ||                  |-------------------------------------------------|
              \/
|----------------------------|
|          Start APP         |
|----------------------------|
```

# 3. Detail
Detail changes in file `bootloader_start.c`

## 3.1. Define macro and global variable in beginning
```c
    #define TRIGGER_CNT_MIN   (3)
    #define TRIGGER_CNT_MAX   (10)

    static uint32_t secure_flag;
    static uint32_t bootloader_cnt = 0;
    static uint32_t flag_prt_addr  = 0;
```

## 3.2. Insert code segment get `flag` partition address
```c
    /*!< get flag partition address */
    if (!memcmp(partition->label, "flag", sizeof("flag"))) {
        flag_prt_addr = partition->pos.offset;
    }
```

## 3.3. Data protection mechanisms in flash
Implementation of variable save and load of `bootloader_cnt` base on `bootloader_flash_*` (read/write), and use three section of flash to ensure safety in data updating progress.

```c
    // -----------------------------------------------------
    // | bootloader_cnt A | bootloader_cnt B | secure_flag |
    // -----------------------------------------------------
    static void bootlooder_cnt_save(void)
    {
        bootloader_flash_read(flag_prt_addr + SPI_SEC_SIZE * 2, &secure_flag, sizeof(secure_flag), false);

        if (secure_flag == 0) {
            bootloader_flash_erase_sector(flag_prt_addr / SPI_SEC_SIZE + 1);
            bootloader_flash_write(flag_prt_addr + SPI_SEC_SIZE * 1, &bootloader_cnt, sizeof(bootloader_cnt), false);

            secure_flag = 1;
            bootloader_flash_erase_sector(flag_prt_addr / SPI_SEC_SIZE + 2);
            bootloader_flash_write(flag_prt_addr + SPI_SEC_SIZE * 2, &secure_flag, sizeof(secure_flag), false);
        } else {
            bootloader_flash_erase_sector(flag_prt_addr / SPI_SEC_SIZE);
            bootloader_flash_write(flag_prt_addr, &bootloader_cnt, sizeof(bootloader_cnt), false);

            secure_flag = 0;
            bootloader_flash_erase_sector(flag_prt_addr / SPI_SEC_SIZE + 2);
            bootloader_flash_write(flag_prt_addr + SPI_SEC_SIZE * 2, &secure_flag, sizeof(secure_flag), false);
        }

        return true;
    }

    static void bootlooder_cnt_load(void)
    {
        bootloader_flash_read(flag_prt_addr + SPI_SEC_SIZE * 2, &secure_flag, sizeof(secure_flag), false);

        if (secure_flag == 0) {
            bootloader_flash_read(flag_prt_addr, &bootloader_cnt, sizeof(bootloader_cnt), false);
        } else {
            bootloader_flash_read(flag_prt_addr + SPI_SEC_SIZE * 1, &bootloader_cnt, sizeof(bootloader_cnt), false);
        }
    }
```

## 3.4. Bootloader_cnt update implementation
```c
    // esp32-mesh use the flag to decide whether return to factory partition,
    // must be after load_partition_table(), because flag_prt_addr was assigned in it.
    static void update_bootloader_cnt(void)
    {
        bootlooder_cnt_load();

        // if customer want to disable factory partition, set bootloader_cnt 0
        if (bootloader_cnt) {
            if (bootloader_cnt == 0xffffffff) {
                // first time power-on or been erased
                bootloader_cnt = 1;
            } else {
                bootloader_cnt++;
            }

            ESP_LOGI(TAG, "bootloader_cnt: %d", bootloader_cnt);
            bootlooder_cnt_save();
        }
    }
```

## 3.5. Calling update_bootloader_cnt after `load_partition_table(...)` to update bootloader_cnt
```c
    update_bootloader_cnt();
```

## 3.6. Check bootloader_cnt after `load_boot_image(...)` to decide whether goto factory partition
```c
    bootlooder_cnt_load();

    if (bootloader_cnt) {
        if (bootloader_cnt >= TRIGGER_CNT_MIN && bootloader_cnt <= TRIGGER_CNT_MAX) {
            ESP_LOGI(TAG, "bootloader_cnt: %d, enter factory app", bootloader_cnt);

            memset(&image_data, 0, sizeof(esp_image_metadata_t));

            if (!load_boot_image(&bs, FACTORY_INDEX, &image_data)) {
                return;
            }
        }

        // reset bootloader_cnt
        bootloader_cnt = 0xffffffff;
        bootlooder_cnt_save();
    }
```
