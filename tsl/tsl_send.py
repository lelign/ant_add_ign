#!/usr/bin/env python3

import argparse
import socket
from struct import *

def get_tsl(addr, tally_red, tally_green, preset_num, text):
    header = 0x80 + addr
    control = tally_red+(tally_green<<1) + preset_num

    return pack('BB16s', header, control, str.encode(text.ljust(16)))

parser = argparse.ArgumentParser(description='TSL UMD test program')
parser.add_argument('IP', metavar='IP', type=str,
                    help='IP address')
parser.add_argument('PORT', metavar='PORT', type=int,
                    help='port')
parser.add_argument('--txt', type=str, 
                    help='Channel name', default="Ch 1")
parser.add_argument('--all',
                    help='Send all', action='store_true')
parser.add_argument('-a',
                    help='Send all', type=int, default=0)
parser.add_argument('-r',
                    help='Tally red', action='store_const', const=1, default=0)
parser.add_argument('-g',
                    help='Tally green', action='store_const', const=1, default=0)
parser.add_argument('-p',
                    help='layout Preset Num', type=int, default=0)

args = parser.parse_args()

sock = socket.socket(socket.AF_INET,
    socket.SOCK_DGRAM)

if(args.all):
    r = b''
    for i in range(36):
        r += get_tsl(i, args.r, args.g, args.p,"Ch {}".format(i+1))
    sock.sendto(r, (args.IP, args.PORT))
else:
    sock.sendto(get_tsl(args.a, args.r, args.g, args.p, args.txt), (args.IP, args.PORT))
    # buffer = get_tsl(args.a, args.r, args.g, args.p, args.txt)
    # print("Buffer:", buffer)
    # for byte in buffer:
    #     print(hex(byte), end=" ")
    # # print()