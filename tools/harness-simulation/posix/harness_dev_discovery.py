#!/usr/bin/env python3
#
#  Copyright (c) 2022, The OpenThread Authors.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. Neither the name of the copyright holder nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#

import argparse
import ctypes
import ctypes.util
import json
import logging
import os
import signal
import socket
import struct
import subprocess
import sys

GROUP = 'ff02::114'
PORT = 12345
MAX_OT11_NUM = 33
MAX_SNIFFER_NUM = 4
SNIFFER_SERVER_PORT_BASE = 50051


def if_nametoindex(ifname: str) -> int:
    libc = ctypes.CDLL(ctypes.util.find_library('c'))
    ret = libc.if_nametoindex(ifname.encode('ascii'))
    if not ret:
        raise RuntimeError('Invalid interface name')
    return ret


def get_ipaddr(ifname: str) -> str:
    for line in os.popen(f'ip addr list dev {ifname} | grep inet | grep global'):
        addr = line.strip().split()[1]
        return addr.split('/')[0]
    raise RuntimeError(f'No IP address on dev {ifname}')


def init_socket(ifname: str, group: str, port: int) -> socket.socket:
    # Look up multicast group address in name server and find out IP version
    addrinfo = socket.getaddrinfo(group, None)[0]
    assert addrinfo[0] == socket.AF_INET6

    # Create a socket
    s = socket.socket(addrinfo[0], socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_BINDTODEVICE, (ifname + '\0').encode('ascii'))

    # Bind it to the port
    s.bind((group, port))

    group_bin = socket.inet_pton(addrinfo[0], addrinfo[4][0])
    # Join group
    interface_index = if_nametoindex(ifname)
    mreq = group_bin + struct.pack('@I', interface_index)
    s.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_JOIN_GROUP, mreq)

    return s


def _advertise(s: socket.socket, dst, info):
    logging.info('Advertise: %r', info)
    s.sendto(json.dumps(info).encode('utf-8'), dst)


def advertise_ftd(s: socket.socket, dst, ven: str, ver: str, add: str, por: int, number: int):
    # Node ID of ot-cli-ftd is 1-indexed
    for i in range(1, number + 1):
        info = {
            'ven': ven,
            'mod': f'{ven}_{i}',
            'ver': ver,
            'add': f'{i}@{add}',
            'por': por,
        }
        _advertise(s, dst, info)


def advertise_sniffer(s: socket.socket, dst, add: str, number: int):
    for i in range(number):
        info = {
            'add': add,
            'por': i + SNIFFER_SERVER_PORT_BASE,
        }
        _advertise(s, dst, info)


def initiate_sniffer(addr: str, port: int) -> subprocess.Popen:
    cmd = ['python3', 'sniffer_sim/sniffer.py', '--address', addr, '--port', str(port)]
    logging.info('Executing command:  %s', ' '.join(cmd))
    return subprocess.Popen(cmd)


def main():
    logging.basicConfig(level=logging.INFO)

    # Parse arguments
    parser = argparse.ArgumentParser()

    # Determine the interface
    parser.add_argument('-i',
                        '--interface',
                        dest='ifname',
                        type=str,
                        required=True,
                        help='the interface used for discovery')

    # Determine the number of OpenThread 1.1 FTD simulations to be "detected" and then initiated
    parser.add_argument('--ot1.1',
                        dest='ot11_num',
                        type=int,
                        required=False,
                        default=0,
                        help=f'the number of OpenThread FTD simulations, no more than {MAX_OT11_NUM}')

    # Determine the number of sniffer simulations to be initiated and then detected
    parser.add_argument('-s',
                        '--sniffer',
                        dest='sniffer_num',
                        type=int,
                        required=False,
                        default=0,
                        help=f'the number of sniffer simulations, no more than {MAX_SNIFFER_NUM}')

    args = parser.parse_args()

    # Check validation of arguments
    if not 0 <= args.ot11_num <= MAX_OT11_NUM:
        raise ValueError(f'The number of FTDs should be between 0 and {MAX_OT11_NUM}')

    if not 0 <= args.sniffer_num <= MAX_SNIFFER_NUM:
        raise ValueError(f'The number of FTDs should be between 0 and {MAX_SNIFFER_NUM}')

    if args.ot11_num == args.sniffer_num == 0:
        raise ValueError('At least one device is required')

    # Get the local IP address on the specified interface
    addr = get_ipaddr(args.ifname)

    # Initiate the sniffer
    sniffer_procs = []
    for i in range(args.sniffer_num):
        sniffer_procs.append(initiate_sniffer(addr, i + SNIFFER_SERVER_PORT_BASE))

    s = init_socket(args.ifname, GROUP, PORT)

    logging.info('Advertising on interface %s group %s ...', args.ifname, GROUP)

    # Terminate all sniffer simulation server processes and then exit
    def exit_handler(signum, context):
        # Return code is non-zero if any return code of the processes is non-zero
        ret = 0
        for sniffer_proc in sniffer_procs:
            sniffer_proc.terminate()
            ret = max(ret, sniffer_proc.wait())
        sys.exit(ret)

    signal.signal(signal.SIGINT, exit_handler)
    signal.signal(signal.SIGTERM, exit_handler)

    # Loop, printing any data we receive
    while True:
        data, src = s.recvfrom(64)

        if data == b'BBR':
            logging.info('Received OpenThread simulation query, advertising')
            advertise_ftd(s, src, ven='OpenThread_Sim', ver='4', add=addr, por=22, number=args.ot11_num)

        elif data == b'Sniffer':
            logging.info('Received sniffer simulation query, advertising')
            advertise_sniffer(s, src, add=addr, number=args.sniffer_num)

        else:
            logging.warning('Received %r, but ignored', data)


if __name__ == '__main__':
    main()
