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
import ipaddress
import json
import logging
import os
import signal
import socket
import struct
import subprocess
import sys
from typing import Iterable

from config import (
    MAX_NODES_NUM,
    MAX_SNIFFER_NUM,
    SNIFFER_SERVER_PORT_BASE,
    OTBR_DOCKER_NAME_PREFIX,
)
from otbr_sim import otbr_docker

GROUP = 'ff02::114'
PORT = 12345


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


def advertise_devices(s: socket.socket,
                      dst,
                      ven: str,
                      ver: str,
                      add: str,
                      por: int,
                      nodeids: Iterable[int],
                      prefix: str = ''):
    for nodeid in nodeids:
        info = {
            'ven': ven,
            'mod': f'{ven}_{nodeid}',
            'ver': ver,
            'add': f'{prefix}{nodeid}@{add}',
            'por': por,
        }
        _advertise(s, dst, info)


def advertise_sniffers(s: socket.socket, dst, add: str, number: int):
    for i in range(number):
        info = {
            'add': add,
            'por': i + SNIFFER_SERVER_PORT_BASE,
        }
        _advertise(s, dst, info)


def start_sniffer(addr: str, port: int) -> subprocess.Popen:
    if isinstance(ipaddress.ip_address(addr), ipaddress.IPv6Address):
        server = f'[{addr}]:{port}'
    else:
        server = f'{addr}:{port}'

    cmd = ['python3', 'sniffer_sim/sniffer.py', '--grpc-server', server]
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

    # Determine the number of OpenThread FTD simulations to be "detected" and then started
    parser.add_argument('--ot',
                        dest='ot_num',
                        type=int,
                        required=False,
                        default=0,
                        help=f'the number of OpenThread FTD simulations')

    # Determine the number of OpenThread BR simulations to be initiated and then detected
    parser.add_argument('--otbr',
                        dest='otbr_num',
                        type=int,
                        required=False,
                        default=0,
                        help=f'the number of OpenThread BR simulations')

    # Determine the number of sniffer simulations to be started and then detected
    parser.add_argument('--sniffer',
                        dest='sniffer_num',
                        type=int,
                        required=False,
                        default=0,
                        help=f'the number of sniffer simulations, no more than {MAX_SNIFFER_NUM}')

    args = parser.parse_args()

    # Check validation of arguments
    if args.ot_num < 0:
        raise ValueError(f'The number of FTDs should be non-negative')

    if args.otbr_num < 0:
        raise ValueError(f'The number of OTBRs should be non-negative')

    if not 0 <= args.sniffer_num <= MAX_SNIFFER_NUM:
        raise ValueError(f'The number of sniffers should be between 0 and {MAX_SNIFFER_NUM}')

    if args.ot_num == args.otbr_num == args.sniffer_num == 0:
        raise ValueError('At least one device is required')

    if args.ot_num + args.otbr_num > MAX_NODES_NUM:
        raise ValueError(f'The number of all devices should be no more than {MAX_NODES_NUM}')

    # Get the local IP address on the specified interface
    addr = get_ipaddr(args.ifname)

    # Start the sniffer
    sniffer_procs = []
    for i in range(args.sniffer_num):
        sniffer_procs.append(start_sniffer(addr, i + SNIFFER_SERVER_PORT_BASE))

    # Start the BRs
    otbr_dockers = []
    for nodeid in range(args.ot_num + 1, args.ot_num + args.otbr_num + 1):
        otbr_dockers.append(otbr_docker.OtbrDocker(nodeid, OTBR_DOCKER_NAME_PREFIX + str(nodeid)))

    s = init_socket(args.ifname, GROUP, PORT)

    logging.info('Advertising on interface %s group %s ...', args.ifname, GROUP)

    # Terminate all sniffer simulation server processes and then exit
    def exit_handler(signum, context):
        # Return code is non-zero if any return code of the processes is non-zero
        ret = 0
        for sniffer_proc in sniffer_procs:
            sniffer_proc.terminate()
            ret = max(ret, sniffer_proc.wait())

        for otbr in otbr_dockers:
            otbr.close()

        sys.exit(ret)

    signal.signal(signal.SIGINT, exit_handler)
    signal.signal(signal.SIGTERM, exit_handler)

    # Loop, printing any data we receive
    while True:
        data, src = s.recvfrom(64)

        if data == b'BBR':
            logging.info('Received OpenThread simulation query, advertising')
            advertise_devices(s,
                              src,
                              ven='OpenThread_Sim',
                              ver='4',
                              add=addr,
                              por=22,
                              nodeids=range(1, args.ot_num + 1))
            advertise_devices(s,
                              src,
                              ven='OpenThread_BR_Sim',
                              ver='4',
                              add=addr,
                              por=22,
                              nodeids=range(args.ot_num + 1, args.ot_num + args.otbr_num + 1),
                              prefix=OTBR_DOCKER_NAME_PREFIX)

        elif data == b'Sniffer':
            logging.info('Received sniffer simulation query, advertising')
            advertise_sniffers(s, src, add=addr, number=args.sniffer_num)

        else:
            logging.warning('Received %r, but ignored', data)


if __name__ == '__main__':
    main()
