#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import socket
import struct
import threading
import string
import datetime

# typedef struct {
#     uint8_t mac[6];
#     char version[20];
#     uint8_t flag;
#     size_t  size;
#     uint8_t data[0];
# } mesh_coredump_data_t;

pkt_head_size = 32
pkt_type_index = 26
pkt_ver_len = 6 # v2.1.2

pkt_start = 0
pkt_transferring = 1
pkt_end = 2


def tcplink(sock, addr):
    print('Accept new connection from %s:%s...' % addr)
    data_buffer = bytes()
    while True:
        data = sock.recv(2048)
        print("recv data len: %d" % len(data))
        if data:
            data_buffer += data
            while True:
                if len(data_buffer) < pkt_head_size:
                    print("pkt header is incomplete, size: %d byte" % len(data_buffer))
                    break

                mac_0, mac_1, mac_2, mac_3, mac_4, mac_5, ver, flag, pad, size = struct.unpack('<6B20sBBI', data_buffer[:pkt_head_size])
                if flag == pkt_start:
                    print("start recv coredump_pkt, size: %d" % size)
                    data_buffer = data_buffer[pkt_head_size:]
                    break
                elif flag == pkt_end:
                    print("end recv coredump_pkt")
                    data_buffer = data_buffer[pkt_head_size:]
                    sock.close()
                    print('Connection from %s:%s closed.' % addr)
                    return
                elif flag == pkt_transferring:
                    if len(data_buffer) < pkt_head_size + size:
                        print("pkt is incomplete, size: %d byte" % len(data_buffer))
                        break
                    mac_str = '%02x%02x%02x%02x%02x%02x' % (mac_0, mac_1, mac_2, mac_3, mac_4, mac_5)
                    ver_str = ver[:pkt_ver_len]
                    base_dir = "../received_coredump_files/"
                    file_name = mac_str + '_' + str(ver_str) + '_' + str(str(datetime.datetime.now())[0:16]).replace(' ', '-').replace(':', '-') +'.dump'
                    file_path = str(base_dir + file_name)
                    print("file: %s" % file_path)

                    file = open(file_path,'a+')
                    file.write(data_buffer[pkt_head_size:pkt_head_size+size])
                    data_buffer = data_buffer[pkt_head_size+size:]
                else:
                    print("coredump pkt type error ...")
                    break


if __name__ == '__main__':
    # create one socket:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    # bind ip, port:
    ip = '0.0.0.0'
    port = 8766
    print("ip: {}, port: {}".format(ip, port))
    s.bind((ip, port))

    # set max client num:
    s.listen(50)
    print('Waiting for connection...')

    # wait for client to connect:
    while True:
        # accept client connect:
        sock, addr = s.accept()
        # create a new thread to deal with the tcp connection:
        t = threading.Thread(target=tcplink, args=(sock, addr))
        t.start()
