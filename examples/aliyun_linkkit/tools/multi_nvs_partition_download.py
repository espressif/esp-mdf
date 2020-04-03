#!/usr/bin/env python3
# -*-coding:utf-8-*-
import os
import sys
import argparse
import threading
import re
from copy import deepcopy


class MUltiLoader():
    """ Multi downloader for esp32 nvs partition """
    DOWNLOAD_SUCESS_STRING = "Hard resetting via RTS pin..."
    ERASE_SUCCESS_STRING = "Chip erase completed successfully"
    RE_MAC = r"MAC: (([A-Fa-f0-9]{2}:){5}[A-Fa-f0-9]{2})"

    def __init__(self, ports, baudrate, tools, nvs_addr, bin_dir, erase_flag):
        self._ports = ports
        self._baudrate = baudrate
        self._nvs_addr = nvs_addr
        self._tools = tools
        self._dir = bin_dir
        self._erase_flag = erase_flag
        self.re_mac = re.compile(self.RE_MAC)

    def _select_port(self):
        if 'all' in self._ports:
            dev_list = os.listdir('/dev')
            for dev in dev_list:
                if dev.find('ttyUSB') != -1:
                    yield f"/dev/{dev}"
        else:
            for i in self._ports:
                yield f"/dev/ttyUSB{i}"

    def _get_nvsbin_lists(self):
        files = os.listdir(self._dir)
        files_bak = deepcopy(files)
        for i in files_bak:
            if i[-4:] != '.bin':
                files.remove(i)
        return [os.path.join(self._dir,file) for file in files]

    def _download_performance(self, command):
        loader = os.popen(command)
        ret_string = loader.read()
        if ret_string.find(self.DOWNLOAD_SUCESS_STRING) != -1:
            print(f"{command} success")
        elif ret_string.find(self.ERASE_SUCCESS_STRING) != -1:
            print(f"{command} success")
        else:
            print(f"{command} failed")
        loader.close()

    def _erase(self, port):
        command = f"{self._tools} -p {port} -b {self._baudrate} erase_flash"
        print(f"start erase through {port}")
        t = threading.Thread(target=self._download_performance, args=(command,))
        t.start()
        return t

    def _download(self, port, files):
        """ Read MAC address from device """
        t = os.popen(f"{self._tools} -b 115200 -p {port} read_mac")
        strings = t.read()
        result = self.re_mac.search(strings)
        mac = result.group(1).replace(':','-')
        t.close()
        """ Match MAC address and bin file """
        file = str()
        for item in files:
            if item.find(mac) == -1:
                continue
            file = item
            break
        assert(len(file) != 0)
        """ Download bin fine to device """
        command = f"{self._tools} -p {port} -b {self._baudrate} write_flash {self._nvs_addr} {file}"
        print(f"start download bin through {port}")
        t = threading.Thread(
            target=self._download_performance, args=(command,))
        t.start()
        return t

    def run(self):
        t_list = list()
        if self._erase_flag:
            for port in self._select_port():
                t_list.append(self._erase(port))
            for t in t_list:
                t.join()

        files = self._get_nvsbin_lists()
        if len(files) == 0:
            print("can not find bin file")
            return False
        for port in self._select_port():
            t_list.append(self._download(port, files))
        for t in t_list:
            t.join()


def main():
    parser = argparse.ArgumentParser(
        description="ESP multi download tools for nvs partition bin")
    parser.add_argument('-p', '--port', dest='ports', action='append', required=True, nargs='+',
                        help="uart port list, pass all for all uart port")
    parser.add_argument('-b', '--baudrate', dest='baudrate', action='store', default='460800',
                        help="uart baudrate")
    parser.add_argument('--erase', dest='erase', action='store_true', default=False,
                        help="erase before flash if this is given")
    parser.add_argument('--addr', dest='addr', action='store', default='0x9000',
                        help="nvs address in partition table")
    parser.add_argument('--dir', dest='dir', action='store', required=True,
                        help="nvs bin file dir")
    parser.add_argument('--tools', dest='tools', action='store', required=True,
                        help="esptools path")
    args = parser.parse_args()
    ports = []
    for i in args.ports:
        ports += i

    downloader = MUltiLoader(
        ports=ports, baudrate=args.baudrate, erase_flag=args.erase, tools=args.tools, nvs_addr=args.addr, bin_dir=args.dir)
    downloader.run()


if __name__ == '__main__':
    if sys.version_info < (3, 6):
        print('Error, need python version >= 3.6')
        sys.exit()
    try:
        main()
    except KeyboardInterrupt:
        quit()

