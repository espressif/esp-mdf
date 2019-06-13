#!/usr/bin/python
# -*- coding: utf-8 -*-
import json
import hashlib
import sys
import os
import csv
import datetime
import esptool as esptool
import platform

import nvs_partition_gen

CONFIG_CSV = 'config/config.csv'

KEY_CSV_FILE_PATH = "bin/key_csv/"
KEY_BIN_FILE_PATH = "bin/key_bin/"

class meshDevice:

    def mesh_read_mac(self, str_port):
        esp = esptool.ESP32ROM(port=str_port, baud=115200)
        esp.connect()
        mac = esp.read_mac()
        def mac_to_str(mac):
            return ('%s' % (':'.join(map(lambda x: '%02x' % x, mac))))
        str_mac = mac_to_str(mac)
        return str_mac

    def read_config_csv(self):
        try:
            csvFile = open(CONFIG_CSV, "r")
            reader = csv.reader(csvFile) # 返回的是迭代类型
            param = []
            for item in reader:
                param.append(item[1])
            csvFile.close()
            return param
        except:
            print("read config project error !!")

    def read_output(self, OutputList, productKey, str_mac):
        try:
            outputFile = open(OutputList, "r")
            reader = csv.reader(outputFile) # 返回的是迭代类型
            output = []
            for item in reader:
                if item[0] == productKey and item[5] == str_mac:
                    output.append(item[0])
                    output.append(item[1])
                    output.append(item[2])
                    output.append(item[3])
                    output.append(item[4])
                    output.append(item[5])
            outputFile.close()
            return output
        except:
            print("read output project error !!")

    def check_output_file(self, OutputList):
        if os.path.isfile(OutputList) == False:
            os.mknod(OutputList)
            device_info_out = open(OutputList,'a')
            csv_write = csv.writer(device_info_out, dialect='excel')
            write_list = ['ProductKey']
            write_list.append('DeviceName')
            write_list.append('DeviceSecret')
            write_list.append('ProductSecret')
            write_list.append('BurningTime')
            write_list.append('DeviceMac')
            csv_write.writerow(write_list)
            device_info_out.close()

    def read_source(self, productKey, RegistrationList, OutputList):
        try:
            sourceFile = open(RegistrationList,"r")
            source_reader = csv.reader(sourceFile) # 返回的是迭代类型

            outputFile = open(OutputList, "r")
            output_reader = csv.reader(outputFile) # 返回的是迭代类型
            source = []
            use_or_not = False

            for source_item  in source_reader :
                use_or_not = False
                for output_item in output_reader:
                    if source_item[0] == output_item[0] and source_item[1] == output_item[1]:
                            use_or_not = True
                            break
                if use_or_not == False:
                    print("source_item "+ source_item[0] + " " + source_item[1])
                    source.append(source_item[0])
                    source.append(source_item[1])
                    source.append(source_item[2])
                    return source

            sourceFile.close()
            outputFile.close()
            return source

        except:
            print("read source project error !!")

    def write_config(self, productKey, productSecret, RegistrationList, OutputList):
        f=open(CONFIG_CSV, 'w')
        f.write('ProductKey,'+ productKey + "\n")
        f.write('ProductSecret,'+ productSecret + "\n")
        f.write('RegistrationList,'+ RegistrationList + "\n")
        f.write('OutputList,'+ OutputList + "\n")
        f.close()

    def linkkit_key_bin(self, source, OutputList, productSecret, str_mac):
        print("=========")
        print(source[0])
        print(source[1])
        print(source[2])
        print(OutputList)
        print(productSecret)

        productKey = source[0]
        deviceName = source[1]
        deviceSecret = source[2]
        BurningTime = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')

        device_info_out = open(OutputList,'a')
        csv_write = csv.writer(device_info_out, dialect='excel')
        write_list = [productKey]
        write_list.append(deviceName)
        write_list.append(deviceSecret)
        write_list.append(productSecret)
        write_list.append(BurningTime)
        write_list.append(str_mac)
        csv_write.writerow(write_list)
        device_info_out.close()

        if os.path.exists(KEY_CSV_FILE_PATH) == False:
            os.makedirs(KEY_CSV_FILE_PATH)
        KEY_CSV_FILE = KEY_CSV_FILE_PATH+str_mac+".csv"  #file folder
        if os.path.isfile(KEY_CSV_FILE) == False:
            os.mknod(KEY_CSV_FILE)

        device_csv_out = open(KEY_CSV_FILE,'w+')
        csv_write = csv.writer(device_csv_out, dialect='excel')
        line_menu = ['key','type','encoding','value']
        line_namespace = ['ALIYUN-KEY','namespace','','']
        line_ProductKey = ['ProductKey','data','string']
        line_ProductSecret = ['ProductSecret','data','string']
        line_DeviceName = ['DeviceName','data','string']
        line_DeviceSecret = ['DeviceSecret','data','string']
        line_DeviceMac = ['DeviceMac','data','string']
        line_BurningTime = ['BurningTime','data','string']
        line_ProductKey.append(productKey)
        line_ProductSecret.append(productSecret)
        line_DeviceName.append(deviceName)
        line_DeviceSecret.append(deviceSecret)
        line_DeviceMac.append(str_mac)
        line_BurningTime.append(BurningTime)
        csv_write.writerow(line_menu)
        csv_write.writerow(line_namespace)
        csv_write.writerow(line_ProductKey)
        csv_write.writerow(line_ProductSecret)
        csv_write.writerow(line_DeviceName)
        csv_write.writerow(line_DeviceSecret)
        csv_write.writerow(line_DeviceMac)
        csv_write.writerow(line_BurningTime)
        device_csv_out.close()

        if os.path.exists(KEY_BIN_FILE_PATH) == False:
            os.makedirs(KEY_BIN_FILE_PATH)

        KEY_BIN_FILE = KEY_BIN_FILE_PATH +str_mac+".bin"

        nvs_partition_gen.nvs_part_gen(input_filename = KEY_CSV_FILE, \
        output_filename = KEY_BIN_FILE,input_size = "0x4000", \
        key_gen = False, encrypt_mode = False, key_file = None, version_no = "v2")


    def project_firmware_download(self, str_port, str_project, str_erase, str_mac):
        if str_erase == "e":
            erase_flash = "python esptool.py --chip esp32 --port "+ str_port +" erase_flash"
            os.system(erase_flash)

        project_download = "python esptool.py --chip esp32 --port " + str_port \
                    +" --baud 921600 --before default_reset --after hard_reset write_flash" \
                    +" -z --flash_mode dio --flash_freq 40m --flash_size detect" \
                    +" 0x1000 ../" + str_project + "/build/bootloader/bootloader.bin" \
                    +" 0x8000 ../" + str_project + "/build/partitions.bin" \
                    +" 0x10000 ../" + str_project + "/build/" + str_project +".bin" \
                    +" 0xd000 ../" + str_project + "/build/ota_data_initial.bin"

        if str_erase == "e":
            project_download  = project_download + " 0x9000 " + KEY_BIN_FILE_PATH + str_mac +".bin"

        print(project_download)
        os.system(project_download)

def main():
    try:
        os_platform = platform.system()
        mesh = meshDevice()
        config = mesh.read_config_csv()

        ProductKey        = config[0]
        ProductSecret     = config[1]
        RegistrationList  = config[2]
        OutputList        = config[3]

        if len(sys.argv) == 4:
            str_port = sys.argv[1]
            str_project = sys.argv[2]
            str_erase = sys.argv[3]
        elif len(sys.argv) == 2:
            str_port = sys.argv[1]
            str_project = "NULL"
            str_erase   = "NULL"
        else:
            print("Input parameter error")
            sys.exit()
        if str_port.isdigit():
            if os_platform == 'Linux':
                str_port = '/dev/ttyUSB' + str_port
            elif os_platform == 'Windows':
                str_port = 'COM' + str_port
        str_mac = mesh.mesh_read_mac(str_port)
        print("mesh_read_mac:"+str_mac)

        mesh.check_output_file(OutputList);

        LINKKIT_KEY_BIN = KEY_BIN_FILE_PATH + str_mac+".bin"

        if os.path.isfile(LINKKIT_KEY_BIN) == False:

            source = mesh.read_output(OutputList, ProductKey, str_mac)
            if source:
                mesh.linkkit_key_bin(source, OutputList, ProductSecret, str_mac)
            else:
                source = mesh.read_source(ProductKey, RegistrationList,OutputList)
                if source:
                    mesh.linkkit_key_bin(source, OutputList, ProductSecret, str_mac)
                    mesh.write_config(ProductKey, ProductSecret, RegistrationList, OutputList)
                else:
                    print("The device source registered info error !!!")
                    sys.exit(1)
        else:
            print("key file already exists, path:" + LINKKIT_KEY_BIN)
            mesh.write_config(ProductKey, ProductSecret, RegistrationList, OutputList)

        if str_project != "NULL":
            print("project_firmware_download")
            mesh.project_firmware_download(str_port,str_project,str_erase, str_mac)
    except SystemExit as e:
        print("Python exit !!!")
    except Exception as e:
        print("Abnormal exit !!!")
        sys.exit(1)

if __name__ == '__main__':
    main()