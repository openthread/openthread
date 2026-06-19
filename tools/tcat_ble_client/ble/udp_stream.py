"""
  Copyright (c) 2024-2026, The OpenThread Authors.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
  3. Neither the name of the copyright holder nor the
     names of its contributors may be used to endorse or promote products
     derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
"""

import logging
import socket
import select

from ble.ble_stream import BleConnectionClosed

logger = logging.getLogger(__name__)


class UdpStream:
    BASE_PORT = 10000
    MAX_SERVER_TIMEOUT_SEC = 0.010

    def __init__(self, address, node_id):
        self.__receive_buffer = b''
        self.__last_recv_time = None
        self.__connected = True
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.setblocking(False)
        self.address = (address, self.BASE_PORT + node_id)

    def __str__(self):
        return f"UdpStream[{self.address[0]}:{self.address[1]}]"

    async def send(self, data):
        logger.debug(f'tx {len(data)} bytes')
        if not self.__connected:
            raise BleConnectionClosed('BLE connection (simulation) was closed')
        return self.socket.sendto(data, self.address)

    async def recv(self, bufsize):
        if not self.__connected:
            raise BleConnectionClosed('BLE connection (simulation) was closed')
        ready = select.select([self.socket], [], [], self.MAX_SERVER_TIMEOUT_SEC)
        if ready[0]:
            data = self.socket.recv(bufsize)
            # A received 0-byte datagram simulates the peer dropping the BLE link
            if len(data) == 0:
                logger.debug('rx: BLE link disconnection was simulated (0-byte UDP packet)')
                self.__connected = False
                return b''
            logger.debug(f'rx {len(data)} bytes')
            return data
        else:
            return b''

    async def simulation_ble_disconnect(self):
        # Simulate a BLE link break (e.g. peer out of range) by sending a zero-length UDP
        # datagram. Unlike `disconnect`, this does not send a Disconnect TLV and does not
        # perform a clean TLS shutdown.
        self.socket.sendto(b'', self.address)

    async def disconnect(self):
        self.__connected = False
        if self.socket is not None:
            self.socket.close()

    @property
    def is_connected(self):
        return self.__connected and self.socket is not None
