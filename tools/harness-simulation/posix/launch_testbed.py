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
import yaml

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


def advertise_devices(s: socket.socket, dst, ven: str, add: str, nodeids: Iterable[int], tag: str):
    for nodeid in nodeids:
        info = {
            'ven': ven,
            'mod': 'OpenThread',
            'ver': '4',
            'add': f'{tag}_{nodeid}@{add}',
            'por': 22,
        }
        _advertise(s, dst, info)


def advertise_sniffers(s: socket.socket, dst, add: str, ports: Iterable[int]):
    for port in ports:
        info = {
            'add': add,
            'por': port,
        }
        _advertise(s, dst, info)


def start_sniffer(addr: str, port: int, ot_path: str, max_nodes_num: int) -> subprocess.Popen:
    if isinstance(ipaddress.ip_address(addr), ipaddress.IPv6Address):
        server = f'[{addr}]:{port}'
    else:
        server = f'{addr}:{port}'

    cmd = [
        'python3',
        os.path.join(ot_path, 'tools/harness-simulation/posix/sniffer_sim/sniffer.py'),
        '--grpc-server',
        server,
        '--max-nodes-num',
        str(max_nodes_num),
    ]
    logging.info('Executing command:  %s', ' '.join(cmd))
    return subprocess.Popen(cmd)


def main():
    logging.basicConfig(level=logging.INFO)

    # Parse arguments
    parser = argparse.ArgumentParser()
    parser.add_argument('-c',
                        '--config',
                        dest='config',
                        type=str,
                        required=True,
                        help='the path of the configuration JSON file')
    args = parser.parse_args()
    with open(args.config, 'rt') as f:
        config = yaml.safe_load(f)

    ot_path = config['ot_path']
    ot_build = config['ot_build']
    max_nodes_num = ot_build['max_number']
    # No test case requires more than 2 sniffers
    MAX_SNIFFER_NUM = 2

    ot_devices = [(item['tag'], item['number']) for item in ot_build['ot']]
    otbr_devices = [(item['tag'], item['number']) for item in ot_build['otbr']]
    ot_nodes_num = sum(x[1] for x in ot_devices)
    otbr_nodes_num = sum(x[1] for x in otbr_devices)
    nodes_num = ot_nodes_num + otbr_nodes_num
    sniffer_num = config['sniffer']['number']

    # Check validation of numbers
    if not all(0 <= x[1] <= max_nodes_num for x in ot_devices):
        raise ValueError(f'The number of devices of each OT version should be between 0 and {max_nodes_num}')

    if not all(0 <= x[1] <= max_nodes_num for x in otbr_devices):
        raise ValueError(f'The number of devices of each OTBR version should be between 0 and {max_nodes_num}')

    if not 1 <= nodes_num <= max_nodes_num:
        raise ValueError(f'The number of devices should be between 1 and {max_nodes_num}')

    if not 1 <= sniffer_num <= MAX_SNIFFER_NUM:
        raise ValueError(f'The number of sniffers should be between 1 and {MAX_SNIFFER_NUM}')

    # Get the local IP address on the specified interface
    ifname = config['discovery_ifname']
    addr = get_ipaddr(ifname)

    # Start the sniffer
    sniffer_server_port_base = config['sniffer']['server_port_base']
    sniffer_procs = []
    for i in range(sniffer_num):
        sniffer_procs.append(start_sniffer(addr, i + sniffer_server_port_base, ot_path, max_nodes_num))

    # OTBR firewall scripts create rules inside the Docker container
    # Run modprobe to load the kernel modules for iptables
    subprocess.run(['sudo', 'modprobe', 'ip6table_filter'])
    # Start the BRs
    otbr_dockers = []
    nodeid = ot_nodes_num
    for item in ot_build['otbr']:
        tag = item['tag']
        ot_rcp_path = os.path.join(ot_path, item['rcp_subpath'], 'examples/apps/ncp/ot-rcp')
        docker_image = item['docker_image']
        for _ in range(item['number']):
            nodeid += 1
            otbr_dockers.append(
                otbr_docker.OtbrDocker(nodeid=nodeid,
                                       ot_path=ot_path,
                                       ot_rcp_path=ot_rcp_path,
                                       docker_image=docker_image,
                                       docker_name=f'{tag}_{nodeid}'))

    s = init_socket(ifname, GROUP, PORT)

    logging.info('Advertising on interface %s group %s ...', ifname, GROUP)

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

            nodeid = 1
            for ven, devices in [('OpenThread_Sim', ot_devices), ('OpenThread_BR_Sim', otbr_devices)]:
                for tag, number in devices:
                    advertise_devices(s, src, ven=ven, add=addr, nodeids=range(nodeid, nodeid + number), tag=tag)
                    nodeid += number

        elif data == b'Sniffer':
            logging.info('Received sniffer simulation query, advertising')
            advertise_sniffers(s,
                               src,
                               add=addr,
                               ports=range(sniffer_server_port_base, sniffer_server_port_base + sniffer_num))

        else:
            logging.warning('Received %r, but ignored', data)


if __name__ == '__main__':
    main()
