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

import asyncio
import ssl
import logging

logger = logging.getLogger(__name__)


class BleStreamSecure:

    def __init__(self, stream):
        self.stream = stream
        self.ssl_context = ssl.create_default_context(ssl.Purpose.SERVER_AUTH)
        self.incoming = ssl.MemoryBIO()
        self.outgoing = ssl.MemoryBIO()
        self.ssl_object = None

    def load_cert(self, certfile='', keyfile='', cafile=''):
        if certfile and keyfile:
            self.ssl_context.load_cert_chain(certfile=certfile, keyfile=keyfile)
        elif certfile:
            self.ssl_context.load_cert_chain(certfile=certfile)

        if cafile:
            self.ssl_context.load_verify_locations(cafile=cafile)

    async def do_handshake(self, hostname):
        self.ssl_object = self.ssl_context.wrap_bio(
            incoming=self.incoming,
            outgoing=self.outgoing,
            server_side=False,
            server_hostname=hostname,
        )
        while True:
            try:
                self.ssl_object.do_handshake()
                break
            # SSLWantWrite means ssl wants to send data over the link,
            # but might need a receive first
            except ssl.SSLWantWriteError:
                output = await self.stream.recv(4096)
                if output:
                    self.incoming.write(output)
                data = self.outgoing.read()
                if data:
                    await self.stream.send(data)
                await asyncio.sleep(0.1)

            # SSLWantRead means ssl wants to receive data from the link,
            # but might need to send first
            except ssl.SSLWantReadError:
                data = self.outgoing.read()
                if data:
                    await self.stream.send(data)
                output = await self.stream.recv(4096)
                if output:
                    self.incoming.write(output)
                await asyncio.sleep(0.1)

    async def send(self, bytes):
        self.ssl_object.write(bytes)
        encode = self.outgoing.read(4096)
        await self.stream.send(encode)

    async def recv(self, buffersize, timeout=1):
        end_time = asyncio.get_event_loop().time() + timeout
        data = await self.stream.recv(buffersize)
        while not data and asyncio.get_event_loop().time() < end_time:
            await asyncio.sleep(0.1)
            data = await self.stream.recv(buffersize)
        if not data:
            logger.warning('No response when response expected.')
            return b''

        self.incoming.write(data)
        while True:
            try:
                decode = self.ssl_object.read(4096)
                break
            # if recv called before entire message was received from the link
            except ssl.SSLWantReadError:
                more = await self.stream.recv(buffersize)
                while not more:
                    await asyncio.sleep(0.1)
                    more = await self.stream.recv(buffersize)
                self.incoming.write(more)
        return decode

    async def send_with_resp(self, bytes):
        await self.send(bytes)
        res = await self.recv(buffersize=4096, timeout=5)
        return res
