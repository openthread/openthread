"""
  Copyright (c) 2024, The OpenThread Authors.
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

from itertools import count, takewhile
from typing import Iterator, Union
import logging
import time
from asyncio import sleep

from bleak import BleakClient
from bleak.backends.device import BLEDevice
from bleak.backends.characteristic import BleakGATTCharacteristic

logger = logging.getLogger(__name__)


class BleStream:

    def __init__(self, client, service_uuid, tx_char_uuid, rx_char_uuid):
        self.__receive_buffer = b''
        self.__last_recv_time = None
        self.client = client
        self.service_uuid = service_uuid
        self.tx_char_uuid = tx_char_uuid
        self.rx_char_uuid = rx_char_uuid

    async def __aenter__(self):
        return self

    async def __aexit__(self, exc_type, exc_value, traceback):
        if self.client.is_connected:
            await self.client.disconnect()

    def __handle_rx(self, _: BleakGATTCharacteristic, data: bytearray):
        logger.debug(f'received {len(data)} bytes')
        self.__receive_buffer += data
        self.__last_recv_time = time.time()

    @staticmethod
    def __sliced(data: bytes, n: int) -> Iterator[bytes]:
        return takewhile(len, (data[i:i + n] for i in count(0, n)))

    @classmethod
    async def create(cls, address_or_ble_device: Union[BLEDevice, str], service_uuid, tx_char_uuid, rx_char_uuid):
        client = BleakClient(address_or_ble_device)
        await client.connect()
        self = cls(client, service_uuid, tx_char_uuid, rx_char_uuid)
        await client.start_notify(self.tx_char_uuid, self.__handle_rx)
        return self

    async def send(self, data):
        logger.debug(f'sending {data}')
        services = self.client.services.get_service(self.service_uuid)
        rx_char = services.get_characteristic(self.rx_char_uuid)
        for s in BleStream.__sliced(data, rx_char.max_write_without_response_size):
            await self.client.write_gatt_char(rx_char, s)
        return len(data)

    async def recv(self, bufsize, recv_timeout=0.2):
        if not self.__receive_buffer:
            return b''

        while time.time() - self.__last_recv_time <= recv_timeout:
            await sleep(0.1)

        message = self.__receive_buffer[:bufsize]
        self.__receive_buffer = self.__receive_buffer[bufsize:]
        logger.debug(f'retrieved {message}')
        return message

    async def disconnect(self):
        if self.client.is_connected:
            await self.client.disconnect()
