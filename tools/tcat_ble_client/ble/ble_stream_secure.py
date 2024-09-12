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

import _ssl
import asyncio
import ssl
import sys
import logging

from cryptography.x509 import load_der_x509_certificate
from cryptography.hazmat.primitives.serialization import (Encoding, PublicFormat)
from tlv.tlv import TLV
from tlv.tcat_tlv import TcatTLVType
from time import time
import utils

logger = logging.getLogger(__name__)


class BleStreamSecure:

    def __init__(self, stream):
        self.stream = stream
        self.ssl_context = ssl.create_default_context(ssl.Purpose.SERVER_AUTH)
        self.incoming = ssl.MemoryBIO()
        self.outgoing = ssl.MemoryBIO()
        self.ssl_object = None
        self.cert = ''
        self.peer_challenge = None
        self._peer_public_key = None

    def load_cert(self, certfile='', keyfile='', cafile=''):
        if certfile and keyfile:
            self.ssl_context.load_cert_chain(certfile=certfile, keyfile=keyfile)
            self.cert = utils.load_cert_pem(certfile)
        elif certfile:
            self.ssl_context.load_cert_chain(certfile=certfile)
            self.cert = utils.load_cert_pem(certfile)

        if cafile:
            self.ssl_context.load_verify_locations(cafile=cafile)

    async def do_handshake(self, timeout=30.0):
        is_debug = logger.getEffectiveLevel() <= logging.DEBUG
        self.ssl_object = self.ssl_context.wrap_bio(
            incoming=self.incoming,
            outgoing=self.outgoing,
            server_side=False,
            server_hostname=None,
        )
        start = time()
        while (time() - start) < timeout:
            try:
                if not is_debug:
                    print('.', end='')
                    sys.stdout.flush()
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
                await asyncio.sleep(0.02)

            # SSLWantRead means ssl wants to receive data from the link,
            # but might need to send first
            except ssl.SSLWantReadError:
                data = self.outgoing.read()
                if data:
                    await self.stream.send(data)
                output = await self.stream.recv(4096)
                if output:
                    self.incoming.write(output)
                await asyncio.sleep(0.02)
        else:
            print('TLS Connection timed out.')
            return False
        print('')
        cert = self.ssl_object.getpeercert(True)
        cert_obj = load_der_x509_certificate(cert)
        self._peer_public_key = cert_obj.public_key().public_bytes(Encoding.DER, PublicFormat.SubjectPublicKeyInfo)
        self.log_cert_identities()
        return True

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

    async def close(self):
        if self.ssl_object.session is not None:
            logger.debug('sending Disconnect command TLV')
            data = TLV(TcatTLVType.DISCONNECT.value, bytes()).to_bytes()
            self.peer_challenge = None
            self._peer_public_key = None
            await self.send(data)

    @property
    def peer_public_key(self):
        return self._peer_public_key

    @property
    def peer_challenge(self):
        return self._peer_challenge

    @peer_challenge.setter
    def peer_challenge(self, value):
        self._peer_challenge = value

    def log_cert_identities(self):
        # using the internal object of the ssl library is necessary to see the cert data in
        # case of handshake failure - see https://sethmlarson.dev/experimental-python-3.10-apis-and-trust-stores
        # Should work for Python >= 3.10
        try:
            cc = self.ssl_object._sslobj.get_unverified_chain()
            if cc is None:
                logger.info('No TCAT Device cert chain was received (yet).')
                return
            logger.info(f'TCAT Device cert chain: {len(cc)} certificates received.')
            for cert in cc:
                logger.info(f'  cert info:\n{cert.get_info()}')
                peer_cert_der_hex = utils.base64_string(cert.public_bytes(_ssl.ENCODING_DER))
                logger.info(f'  base64: (paste in https://lapo.it/asn1js/ to decode)\n{peer_cert_der_hex}')
            logger.info(f'TCAT Commissioner cert, PEM:\n{self.cert}')

        except Exception as e:
            logger.warning('Could not display TCAT client cert info (check Python version is >= 3.10?)')
