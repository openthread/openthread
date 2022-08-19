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
import grpc
import logging
import signal
import pcap_codec
import threading

from proto import sniffer_pb2
from proto import sniffer_pb2_grpc
import sniffer_transport


class SnifferServicer(sniffer_pb2_grpc.Sniffer):
    """ Class representing the Sniffing node, whose main task is listening. """

    logger = logging.getLogger('sniffer.SnifferServicer')

    RECV_BUFFER_SIZE = 4096
    MAX_NODES_NUM = 33

    class State(enum.Enum):
        STOPPED = 0
        RUNNING = 1

    def _reset(self):
        self._state = SnifferServicer.State.STOPPED
        self._pcap = None
        self._allowed_nodeids = None
        self._transport = None
        self._thread = None
        self._thread_alive.clear()

    def __init__(self):
        self._thread_alive = threading.Event()
        self._mutex = threading.Lock()  # for self._allowed_nodeids
        self._reset()

    def _sniffer_main_loop(self):
        """ Sniffer main loop. """

        self.logger.debug('Sniffer started.')

        while self._thread_alive.is_set():
            # Avoid being blocked endlessly when there is no data
            if not self._transport.ready(0.1):
                continue
            data, nodeid = self._transport.recv(self.RECV_BUFFER_SIZE)

            with self._mutex:
                allowed_nodeids = self._allowed_nodeids

            # Equivalent to RF enclosure
            if allowed_nodeids is None or nodeid in allowed_nodeids:
                self._pcap.append(data)

        self.logger.debug('Sniffer stopped.')

    def Start(self, request, context):
        """ Start sniffing. """

        self.logger.debug('call Start')

        # Validate and change the state
        if self._state != SnifferServicer.State.STOPPED:
            return sniffer_pb2.StartResponse(status=sniffer_pb2.OPERATION_ERROR)
        self._state = SnifferServicer.State.RUNNING

        self._pcap = pcap_codec.PcapCodec(request.channel)

        # Sniffer all nodes in default, i.e. there is no RF enclosure
        # In this case, self._allowed_nodeids is set to None
        self._allowed_nodeids = None

        # Create transport
        transport_factory = sniffer_transport.SnifferTransportFactory()
        self._transport = transport_factory.create_transport()

        # Start the sniffer main loop thread
        self._thread = threading.Thread(target=self._sniffer_main_loop)
        self._thread.daemon = True
        self._transport.open()
        self._thread_alive.set()
        self._thread.start()

        return sniffer_pb2.StartResponse(status=sniffer_pb2.OK)

    def FilterNodes(self, request, context):
        """ Only sniffer the specified nodes. """

        self.logger.debug('call FilterNodes')

        # Validate the state
        if self._state != SnifferServicer.State.RUNNING:
            return sniffer_pb2.FilterNodesResponse(status=sniffer_pb2.OPERATION_ERROR)

        allowed_nodeids = set(request.nodeids)
        # Validate the node IDs
        for nodeid in allowed_nodeids:
            if not 1 <= nodeid <= self.MAX_NODES_NUM:
                return sniffer_pb2.FilterNodesResponse(status=sniffer_pb2.VALUE_ERROR)

        with self._mutex:
            self._allowed_nodeids = allowed_nodeids

        return sniffer_pb2.FilterNodesResponse(status=sniffer_pb2.OK)

    def Stop(self, request, context):
        """ Stop sniffing, and return the pcap bytes. """

        self.logger.debug('call Stop')

        # Validate and change the state
        if self._state != SnifferServicer.State.RUNNING:
            return sniffer_pb2.StopResponse(status=sniffer_pb2.OPERATION_ERROR, pcap_content=b'')
        self._state = SnifferServicer.State.STOPPED

        self._thread_alive.clear()
        self._thread.join(timeout=1)
        self._transport.close()

        pcap_content = self._pcap.pop_all()
        self._reset()

        return sniffer_pb2.StopResponse(status=sniffer_pb2.OK, pcap_content=pcap_content)


def serve(address_port):
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=1))
    sniffer_pb2_grpc.add_SnifferServicer_to_server(SnifferServicer(), server)
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
    args = parser.parse_args()

    serve(args.grpc_server)


if __name__ == '__main__':
    run_sniffer()
