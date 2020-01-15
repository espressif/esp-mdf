#!/usr/bin/env python3
# -*-coding:utf-8-*-
import os
import sys
import threading
import argparse
import re
import time


class MultiMinicom():
    """ open multiple minicom for ESP32 """
    RE_MAC = r"MAC: (([A-Fa-f0-9]{2}:){5}[A-Fa-f0-9]{2})"

    def __init__(self, ports, baudrate, esptools, dir):
        self.ports = ports
        self.baudrate = baudrate
        self.esptools = esptools
        if os.path.exists(dir) != True:
            os.makedirs(dir)
        self.dir = os.path.abspath(dir)
        if self.esptools is not None:
            self.re_mac = re.compile(self.RE_MAC)

    def _select_port(self):
        if 'all' in self.ports:
            dev_list = os.listdir('/dev')
            for dev in dev_list:
                if dev.find('ttyUSB') != -1:
                    yield f"/dev/{dev}"
        else:
            for i in self.ports:
                yield f"/dev/ttyUSB{i}"

    def _close_all_minicom(self):
        t = os.popen("""ps -ef | grep minicom | grep -v 'grep' | wc -l """)
        results = t.read()
        t.close()
        if int(results) > 0:
            print("close other minicom first")
            t = os.popen(f"""pkill minicom""")
            t.close()
            time.sleep(4)

    def _open_minicom_performance(self, port):
        name = str()
        if self.esptools is not None:
            t = os.popen(
                f"""python {self.esptools} -b 115200 -p {port} read_mac """)
            strings = t.read()
            result = self.re_mac.search(strings)
            name = result.group(1).replace(':', '-')
            t.close()
        t = os.popen(
            f""" gnome-terminal  --geometry 240x60 -x minicom -c on -D {port} -C "{self.dir}/{name}_{port.replace('/','_')}.log" """)
        t.close()

    def run(self):
        self._close_all_minicom()
        t_list = list()
        for port in self._select_port():
            t = threading.Thread(
                target=self._open_minicom_performance, args=(port,))
            t.start()
            t_list.append(t)
        for t in t_list:
            t.join()


def main():
    parser = argparse.ArgumentParser(
        description="ESP open multiple minicom tools")
    parser.add_argument('-p', '--ports', dest='ports', action='append', required=True, nargs='+',
                        help="uart port list, pass all for all uart port")
    parser.add_argument('-b', '--baudrate', dest='baudrate', action='store', default='460800',
                        help="uart baudrate")
    parser.add_argument('--esptools', dest='esptools', action='store',
                        help="if esptools is given, log file can be named by mac address, hardware flow control is needed, and ESP32 will be restart by flow control")
    parser.add_argument('--dir', dest='dir', action='store', default='log',
                        help="directory to save log file")
    args = parser.parse_args()
    ports = []
    for i in args.ports:
        ports += i

    tools = MultiMinicom(
        ports=ports, baudrate=args.baudrate, esptools=args.esptools, dir=args.dir)
    tools.run()


if __name__ == '__main__':
    if sys.version_info < (3,6):
        print('Error, need python version >= 3.6')
        sys.exit()
    try:
        main()
    except KeyboardInterrupt:
        quit()
