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

import os
import socket


class SnifferTransport(object):
    """ Interface for transport that allows eavesdrop other nodes. """

    def open(self):
        """ Open transport.

        Raises:
            RuntimeError: when transport is already opened or when transport opening failed.
        """
        raise NotImplementedError

    def close(self):
        """ Close transport.

        Raises:
            RuntimeError: when transport is already closed.
        """
        raise NotImplementedError

    @property
    def is_opened(self):
        """ Check if transport is opened.

        Returns:
            bool: True if the transport is opened, False in otherwise
        """
        raise NotImplementedError

    def send(self, data, nodeid):
        """ Send data to the node with nodeid.

        Args:
            data (bytearray): outcoming data.
            nodeid (int): node id

        Returns:
            int: number of sent bytes
        """
        raise NotImplementedError

    def recv(self, bufsize, timeout):
        """ Receive data sent by other node.

        Args:
            bufsize (int): size of buffer for incoming data.
            timeout (float | None): socket timeout.

        Returns:
            A tuple contains data and node id.

            For example:
            (bytearray([0x00, 0x01...], 1)

        Raises:
            socket.timeout: when receiving the packets times out.
        """
        raise NotImplementedError


class SnifferSocketTransport(SnifferTransport):
    """ Socket based implementation of sniffer transport. """

    BASE_PORT = 9000

    MAX_NETWORK_SIZE = int(os.getenv('MAX_NETWORK_SIZE', '33'))

    PORT_OFFSET = int(os.getenv('PORT_OFFSET', '0'))

    RADIO_GROUP = '224.0.0.116'

    def __init__(self):
        self._socket = None

    def _nodeid_to_port(self, nodeid: int):
        return self.BASE_PORT + (self.PORT_OFFSET * (self.MAX_NETWORK_SIZE + 1)) + nodeid

    def _port_to_nodeid(self, port):
        return (port - self.BASE_PORT - (self.PORT_OFFSET * (self.MAX_NETWORK_SIZE + 1)))

    def open(self):
        if self.is_opened:
            raise RuntimeError('Transport is already opened.')

        self._socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        if not self.is_opened:
            raise RuntimeError('Transport opening failed.')

        self._socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
        self._socket.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 2 * 1024 * 1024)
        self._socket.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 2 * 1024 * 1024)
        self._socket.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP,
                                socket.inet_aton(self.RADIO_GROUP) + socket.inet_aton('127.0.0.1'))
        self._socket.bind((self.RADIO_GROUP, self._nodeid_to_port(0)))

    def close(self):
        if not self.is_opened:
            raise RuntimeError('Transport is closed.')

        self._socket.close()
        self._socket = None

    @property
    def is_opened(self):
        return bool(self._socket is not None)

    def send(self, data, nodeid):
        address = ('127.0.0.1', self._nodeid_to_port(nodeid))

        return self._socket.sendto(data, address)

    def recv(self, bufsize, timeout):
        self._socket.settimeout(timeout)
        data, address = self._socket.recvfrom(bufsize)

        nodeid = self._port_to_nodeid(address[1])

        return bytearray(data), nodeid


class SnifferTransportFactory(object):

    def create_transport(self):
        return SnifferSocketTransport()
