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
import logging
import signal
import time
import pcap_codec
import sys
import threading

import sniffer_transport


class Sniffer:
    """ Class representing the Sniffing node, whose main task is listening.
    """

    logger = logging.getLogger('sniffer.Sniffer')

    RECV_BUFFER_SIZE = 4096

    def __init__(self, filename, channel):
        self._pcap = pcap_codec.PcapCodec(filename, channel)

        # Create transport
        transport_factory = sniffer_transport.SnifferTransportFactory()
        self._transport = transport_factory.create_transport()

        self._thread = None
        self._thread_alive = threading.Event()
        self._thread_alive.clear()

    def _sniffer_main_loop(self):
        """ Sniffer main loop. """

        self.logger.debug('Sniffer started.')

        while self._thread_alive.is_set():
            data, nodeid = self._transport.recv(self.RECV_BUFFER_SIZE)
            self._pcap.append(data)

        self.logger.debug('Sniffer stopped.')

    def start(self):
        """ Start sniffing. """

        self._thread = threading.Thread(target=self._sniffer_main_loop)
        self._thread.daemon = True

        self._transport.open()

        self._thread_alive.set()
        self._thread.start()

    def stop(self):
        """ Stop sniffing. """

        self._thread_alive.clear()

        self._transport.close()

        self._thread.join(timeout=1)
        self._thread = None

    def close(self):
        """ Close the pcap file. """

        self._pcap.close()


def run_sniffer():
    parser = argparse.ArgumentParser()
    parser.add_argument('-o',
                        '--output',
                        dest='output',
                        type=str,
                        required=True,
                        help='the path of the output .pcap file')
    parser.add_argument('-c',
                        '--channel',
                        dest='channel',
                        type=int,
                        required=True,
                        help='the channel which is sniffered')
    args = parser.parse_args()

    sniffer = Sniffer(args.output, args.channel)
    sniffer.start()

    def atexit(signum, frame):
        sniffer.stop()
        sniffer.close()
        sys.exit(0)

    signal.signal(signal.SIGTERM, atexit)

    while sniffer._thread_alive.is_set():
        time.sleep(0.5)

    sniffer.stop()
    sniffer.close()


if __name__ == '__main__':
    run_sniffer()
