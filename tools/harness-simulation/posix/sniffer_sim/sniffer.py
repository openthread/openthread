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
from concurrent import futures
import enum
import fcntl
import grpc
import logging
import os
import signal
import socket
import subprocess
import tempfile
import threading
import time

import pcap_codec
from proto import sniffer_pb2
from proto import sniffer_pb2_grpc
import sniffer_transport


class CaptureState(enum.Flag):
    NONE = 0
    THREAD = enum.auto()
    ETHERNET = enum.auto()


class SnifferServicer(sniffer_pb2_grpc.Sniffer):
    """ Class representing the Sniffing node, whose main task is listening. """

    logger = logging.getLogger('sniffer.SnifferServicer')

    RECV_BUFFER_SIZE = 4096
    TIMEOUT = 0.1

    def _reset(self):
        self._state = CaptureState.NONE
        self._pcap = None
        self._denied_nodeids = None
        self._transport = None
        self._thread = None
        self._thread_alive.clear()
        self._file_sync_done.clear()
        self._tshark_proc = None

    def __init__(self, max_nodes_num):
        self._max_nodes_num = max_nodes_num
        self._thread_alive = threading.Event()
        self._file_sync_done = threading.Event()
        self._nodeids_mutex = threading.Lock()  # for `self._denied_nodeids`
        self._reset()

    def Start(self, request, context):
        """ Start sniffing. """

        self.logger.debug('call Start')

        # Validate and change the state
        if self._state != CaptureState.NONE:
            return sniffer_pb2.StartResponse(status=sniffer_pb2.OPERATION_ERROR)
        self._state = CaptureState.THREAD

        # Create a temporary named pipe
        tempdir = tempfile.mkdtemp()
        fifo_name = os.path.join(tempdir, 'pcap.fifo')
        os.mkfifo(fifo_name)

        cmd = ['tshark', '-i', fifo_name]
        if request.includeEthernet:
            self._state |= CaptureState.ETHERNET
            cmd += ['-i', 'docker0']
        cmd += ['-w', '-', '-q', 'not ip and not tcp and not arp and not ether proto 0x8899']

        self.logger.debug('Running command:  %s', ' '.join(cmd))
        self._tshark_proc = subprocess.Popen(cmd, stdout=subprocess.PIPE)
        self._set_nonblocking(self._tshark_proc.stdout.fileno())

        # Construct pcap codec after initiating tshark to avoid blocking
        self._pcap = pcap_codec.PcapCodec(request.channel, fifo_name)

        # Sniffer all nodes in default, i.e. there is no RF enclosure
        self._denied_nodeids = set()

        # Create transport
        transport_factory = sniffer_transport.SnifferTransportFactory()
        self._transport = transport_factory.create_transport()

        # Start the sniffer main loop thread
        self._thread = threading.Thread(target=self._sniffer_main_loop)
        self._thread.setDaemon(True)
        self._transport.open()
        self._thread_alive.set()
        self._thread.start()

        return sniffer_pb2.StartResponse(status=sniffer_pb2.OK)

    def _sniffer_main_loop(self):
        """ Sniffer main loop. """

        while self._thread_alive.is_set():
            try:
                data, nodeid = self._transport.recv(self.RECV_BUFFER_SIZE, self.TIMEOUT)
            except socket.timeout:
                continue

            with self._nodeids_mutex:
                denied_nodeids = self._denied_nodeids

            # Equivalent to RF enclosure
            if nodeid not in denied_nodeids:
                self._pcap.append(data)

    def TransferPcapng(self, request, context):
        """ Transfer the capture file. """

        # Validate the state
        if self._state == CaptureState.NONE:
            return sniffer_pb2.FilterNodesResponse(status=sniffer_pb2.OPERATION_ERROR)

        # Synchronize the capture file
        while True:
            content = self._tshark_proc.stdout.read()
            if content is None:
                # Currently no captured packets
                time.sleep(self.TIMEOUT)
            elif content == b'':
                # Reach EOF when tshark terminates
                break
            else:
                # Forward the captured packets
                yield sniffer_pb2.TransferPcapngResponse(content=content)

        self._file_sync_done.set()

    def _set_nonblocking(self, fd):
        flags = fcntl.fcntl(fd, fcntl.F_GETFL)
        if flags < 0:
            raise RuntimeError('fcntl(F_GETFL) failed')

        flags |= os.O_NONBLOCK
        if fcntl.fcntl(fd, fcntl.F_SETFL, flags) < 0:
            raise RuntimeError('fcntl(F_SETFL) failed')

    def FilterNodes(self, request, context):
        """ Only sniffer the specified nodes. """

        self.logger.debug('call FilterNodes')

        # Validate the state
        if not (self._state & CaptureState.THREAD):
            return sniffer_pb2.FilterNodesResponse(status=sniffer_pb2.OPERATION_ERROR)

        denied_nodeids = set(request.nodeids)
        # Validate the node IDs
        for nodeid in denied_nodeids:
            if not 1 <= nodeid <= self._max_nodes_num:
                return sniffer_pb2.FilterNodesResponse(status=sniffer_pb2.VALUE_ERROR)

        with self._nodeids_mutex:
            self._denied_nodeids = denied_nodeids

        return sniffer_pb2.FilterNodesResponse(status=sniffer_pb2.OK)

    def Stop(self, request, context):
        """ Stop sniffing, and return the pcap bytes. """

        self.logger.debug('call Stop')

        # Validate and change the state
        if not (self._state & CaptureState.THREAD):
            return sniffer_pb2.StopResponse(status=sniffer_pb2.OPERATION_ERROR)
        self._state = CaptureState.NONE

        self._thread_alive.clear()
        self._thread.join()
        self._transport.close()
        self._pcap.close()

        self._tshark_proc.terminate()
        self._file_sync_done.wait()
        # `self._tshark_proc` becomes None after the next statement
        self._tshark_proc.wait()

        self._reset()

        return sniffer_pb2.StopResponse(status=sniffer_pb2.OK)


def serve(address_port, max_nodes_num):
    # One worker is used for `Start`, `FilterNodes` and `Stop`
    # The other worker is used for `TransferPcapng`, which will be kept running by the client in a background thread
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=2))
    sniffer_pb2_grpc.add_SnifferServicer_to_server(SnifferServicer(max_nodes_num), server)
    # add_secure_port requires a web domain
    server.add_insecure_port(address_port)
    logging.info('server starts on %s', address_port)
    server.start()

    def exit_handler(signum, context):
        server.stop(1)

    signal.signal(signal.SIGINT, exit_handler)
    signal.signal(signal.SIGTERM, exit_handler)

    server.wait_for_termination()


def run_sniffer():
    logging.basicConfig(level=logging.INFO)

    parser = argparse.ArgumentParser()
    parser.add_argument('--grpc-server',
                        dest='grpc_server',
                        type=str,
                        required=True,
                        help='the address of the sniffer server')
    parser.add_argument('--max-nodes-num',
                        dest='max_nodes_num',
                        type=int,
                        required=True,
                        help='the maximum number of nodes')
    args = parser.parse_args()

    serve(args.grpc_server, args.max_nodes_num)


if __name__ == '__main__':
    run_sniffer()
