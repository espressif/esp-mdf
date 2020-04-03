#!/usr/bin/env python3
# -*-coding:utf-8-*-
import sys
import json
import argparse
import csv
import os
from copy import deepcopy


class Credcsv():
    """ activation credentials to csv for nvs """
    HEADER = ['key', 'type', 'encoding', 'value']
    NAMESPACE = ['', 'namespace']

    def __init__(self, product, device, namespace):
        """
        :param product: product key and secret format as (key, secret)
        :param device: device name and secret format as (name,secret)
        :param namespace: nvs csv namespace
        """
        self._product_key = product[0]
        self._product_secret = product[1]
        self._device_name = device[0]
        self._device_secret = device[1]
        self._namespace = namespace

    def format2csv(self):
        """ format activation credentials to csv data """
        self._nvs_csv = {}
        self._nvs_csv['headers'] = self.HEADER
        namespace = deepcopy(self.NAMESPACE)
        namespace[0] = self._namespace
        rows = self._nvs_csv['rows'] = []
        rows.append(tuple(namespace))
        rows.append(('ProductKey', 'data', 'string', f"{self._product_key}"))
        rows.append(('ProductSecret', 'data', 'string',
                     f"{self._product_secret}"))
        rows.append(('DeviceName', 'data', 'string', f"{self._device_name}"))
        rows.append(('DeviceSecret', 'data', 'string',
                     f"{self._device_secret}"))

    def write2csv(self, file):
        """ write csv data to file
        param file: target file to save csv data
        """
        with open(file, 'w+') as f:
            f_csv = csv.writer(f)
            f_csv.writerow(self._nvs_csv['headers'])
            f_csv.writerows(self._nvs_csv['rows'])

    def __str__(self):
        string = (f"ProductKey: {self._product_key}\n"
                  f"ProductSecret: {self._product_secret}\n"
                  f"DeviceName: {self._device_name}\n"
                  f"DeviceSecret: {self._device_secret}")
        return string


def main():
    parser = argparse.ArgumentParser(
        description="generate nvs partition csv file for deferent activation credentials in aliyun application")
    parser.add_argument('--input', action='store', required=True, dest='input',
                        help="input triplet information cvs file")
    parser.add_argument('--outdir', action='store', required=True, dest='outdir',
                        help="dir for deferent output cvs file. if dir doesn't exist, then create it")
    parser.add_argument('--namespace', action='store', required=True, dest='namespace',
                        help="namespace for nvs partition")
    parser.add_argument('--tools', action='store', dest='tools', default=None,
                        help="if nvs_partition_gen.py path is given, then it will generate nvs bin in outdir automaticly, only support idf3.x version")
    parser.add_argument('--size', action='store', dest='size', default='0x6000',
                        help="nvs partition size")
    parser.add_argument('--secret', action='store', dest='product_secret', required=True,
                        help="Product secret")
    args = parser.parse_args()

    with open(args.input, 'r') as f:
        creds = csv.reader(f)
        next(creds)
        for row in creds:
            nvs = Credcsv((row[2], args.product_secret),
                          (row[0], row[1]), args.namespace)
            nvs.format2csv()
            csv_name = f"{row[0]}_{row[3]}.csv"
            bin_name = f"{row[0]}_{row[3]}.bin"
            nvs.write2csv(os.path.join(args.outdir, csv_name))
            print(nvs)
            if args.tools is not None:
                tools = os.popen(
                    f"{args.tools} --input {args.outdir}/{csv_name} --output {args.outdir}/{bin_name} --size {args.size}")
                print(tools.read())
                tools.close()


if __name__ == '__main__':
    if sys.version_info < (3, 6):
        print('Error, need python version >= 3.6')
        sys.exit()
    try:
        main()
    except KeyboardInterrupt:
        quit()
