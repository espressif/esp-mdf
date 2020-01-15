#!/usr/bin/env python3
# -*-coding:utf-8-*-
import sys
import threading
import os
import argparse

ESP_TOOL = "components/esptool_py/esptool/esptool.py"


class MultiDownloader():
    """ Multi downloader for esp32 """
    CMAKE_BUILD_SUCESS_STRING = "Project build complete. To flash, run this command:"
    MAKE_BUILD_SUCESS_STRING = "To flash all build output, run 'make flash' or:"
    DOWNLOAD_SUCESS_STRING = "Hard resetting via RTS pin..."

    def __init__(self, build_type, ports, baudrate, erase_flag):
        self.type = build_type
        self.ports = ports
        self.baudrate = baudrate
        self.erase_flag = erase_flag

    def _select_port(self):
        if 'all' in self.ports:
            dev_list = os.listdir('/dev')
            for dev in dev_list:
                if dev.find('ttyUSB') != -1:
                    yield f"/dev/{dev}"
        else:
            for i in self.ports:
                yield f"/dev/ttyUSB{i}"

    def _download_performance(self, command):
        loader = os.popen(command)
        ret_string = loader.read()
        if ret_string.find(self.DOWNLOAD_SUCESS_STRING) == -1:
            print(f"{command} error")
        else:
            print(f"{command} sucess")
        loader.close()

    def _build(self):
        if self.type == 'cmake':
            build_command = 'idf.py build'
            sucess_string = self.CMAKE_BUILD_SUCESS_STRING
        elif self.type == 'make':
            build_command = 'make -j all'
            sucess_string = self.MAKE_BUILD_SUCESS_STRING
        else:
            raise ValueError(
                f"type should be 'make' or 'cmake', but it is {self.type}")

        print("start building target")
        loader = os.popen(build_command)
        results = loader.read()
        loader.close()
        if results.find(sucess_string) == -1:
            print(results)
            print("build failed")
            return False
        return True

    def _download(self, port):
        erase = ""
        if self.erase_flag:
            erase = "erase_flash"

        if self.type == 'cmake':
            download_command = f"idf.py -p {port} -b {self.baudrate} {erase} flash"
        elif self.type == 'make':
            download_command = f"make {erase} flash ESPPORT={port} ESPBAUD={self.baudrate}"
        else:
            raise ValueError(
                f"type should be 'make' or 'cmake', but it is {self.type}")

        print(f"start download through {port}")
        t = threading.Thread(
            target=self._download_performance, args=(download_command,))
        t.start()
        return t

    def run(self):
        if self._build() is not True:
            return False
        t_list = list()
        for port in self._select_port():
            t_list.append(self._download(port))
        for t in t_list:
            t.join()


def main():
    parser = argparse.ArgumentParser(description="ESP multi download tools")
    parser.add_argument('-t', '--type', dest='type', action='store', default='make',
                        help="build system type, can be make or cmake")
    parser.add_argument('-p', '--ports', dest='ports', action='append', required=True, nargs='+',
                        help="uart port list, pass all for all uart port")
    parser.add_argument('-b', '--baudrate', dest='baudrate', action='store', default='460800',
                        help="uart baudrate")
    parser.add_argument('--erase', dest='erase', action='store_true',default=False,
                        help="erase before flash if this is given")
    args = parser.parse_args()
    ports = []
    for i in args.ports:
        ports += i

    downloader = MultiDownloader(
        build_type=args.type, ports=ports, baudrate=args.baudrate,erase_flag=args.erase)
    downloader.run()


if __name__ == '__main__':
    if sys.version_info < (3,6):
        print('Error, need python version >= 3.6')
        sys.exit()
    try:
        main()
    except KeyboardInterrupt:
        quit()
