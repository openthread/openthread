"""
  Copyright (c) 2024-2025, The OpenThread Authors.
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
import logging
import ssl
from time import time
from typing import Optional, Callable

from cryptography.x509 import load_der_x509_certificate
from cryptography.hazmat.primitives.serialization import (Encoding, PublicFormat)

from tlv.tlv import TLV
from tlv.tcat_tlv import TcatTLVType
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
        self._recv_lock = asyncio.Lock()
        self._is_closed_by_local = False
        self._async_events_queue = asyncio.Queue()

    def load_cert(self, certfile='', keyfile='', cafile=''):
        if certfile and keyfile:
            self.ssl_context.load_cert_chain(certfile=certfile, keyfile=keyfile)
            self.cert = utils.load_cert_pem(certfile)
        elif certfile:
            self.ssl_context.load_cert_chain(certfile=certfile)
            self.cert = utils.load_cert_pem(certfile)

        if cafile:
            self.ssl_context.load_verify_locations(cafile=cafile)

    async def do_handshake(self,
                           buffersize: int = 4096,
                           timeout: float = 30.0,
                           progress_callback: Optional[Callable[[bool], None]] = None) -> bool:
        """
        Performs a TLS handshake with a TCAT Device, reporting progress via an optional callback.

        Args:
            timeout: The maximum time in seconds to wait for the handshake to complete.
            progress_callback: A function that accepts one boolean argument:
                               - False (is_concluded=False): The handshake attempt is ongoing.
                               - True (is_concluded=True): The handshake attempt has concluded. This call is made
                                 before any results/errors in do_handshake are logged.

        Returns:
            True if the TLS handshake was successful, False otherwise.
        """
        self._is_closed_by_local = False
        self._peer_public_key = None
        self.ssl_object = self.ssl_context.wrap_bio(
            incoming=self.incoming,
            outgoing=self.outgoing,
            server_side=False,
            server_hostname=None,
        )

        try:
            start = time()
            while (time() - start) < timeout:
                try:
                    if progress_callback:
                        progress_callback(False)
                    self.ssl_object.do_handshake()
                    break

                # SSLWantWrite means ssl wants to send data over the link,
                # but might need to receive first
                except ssl.SSLWantWriteError:
                    recv_data = await self.stream.recv(buffersize)
                    if recv_data:
                        self.incoming.write(recv_data)
                    send_data = self.outgoing.read()
                    if send_data:
                        await self.stream.send(send_data)
                    await asyncio.sleep(0.020)

                # SSLWantRead means ssl wants to receive data from the link,
                # but might need to send first
                except ssl.SSLWantReadError:
                    send_data = self.outgoing.read()
                    if send_data:
                        await self.stream.send(send_data)
                    recv_data = await self.stream.recv(buffersize)
                    if recv_data:
                        self.incoming.write(recv_data)
                    await asyncio.sleep(0.020)

                except ssl.SSLCertVerificationError as err:
                    if progress_callback:
                        progress_callback(True)
                    logger.error(
                        f'SSLCertVerificationError reason={err.reason} verify_code={err.verify_code} verify_msg="{err.verify_message}"'
                    )
                    return False

                except ssl.SSLError as err:
                    if progress_callback:
                        progress_callback(True)
                    logger.error(f"SSLError reason={err.reason}")
                    return False

            else:
                if progress_callback:
                    progress_callback(True)
                logger.error(f'TLS Connection timed out (timeout={timeout}s).')
                return False

        # also catch Exceptions here which may be raised in SSL-specific Exception handlers
        except Exception as err:
            if progress_callback:
                progress_callback(True)
            raise err

        if progress_callback:
            progress_callback(True)
        cert = self.ssl_object.getpeercert(True)
        cert_obj = load_der_x509_certificate(cert)
        self._peer_public_key = cert_obj.public_key().public_bytes(Encoding.DER, PublicFormat.SubjectPublicKeyInfo)
        self.log_cert_identities()
        return True

    # Precondition: caller must handle all exceptions raised
    async def _send(self, data: bytes, buffersize: int = 4096) -> None:
        hexdump_str = utils.hexdump_ot("Tx", data) if len(data) > 0 else ''
        logger.debug(f"tx {len(data)} bytes\n{hexdump_str}")
        self.ssl_object.write(data)
        while self.outgoing.pending > 0:
            encrypted_chunk = self.outgoing.read(buffersize)
            await self.stream.send(encrypted_chunk)

    # Precondition: caller must acquire _recv_lock before calling this method
    # Precondition: caller must handle all exceptions raised
    async def _recv(self, buffersize: int = 4096, timeout: float = 1.0) -> bytes:
        # Sleep time for the data polling loop is set to 1/10th of the total timeout, capped at 20ms.
        # This ensures a default responsiveness for longer timeout values, and a higher responsiveness when a
        # short timeout value is given.
        slp_time = min(timeout / 10.0, 0.020)
        end_time = asyncio.get_event_loop().time() + timeout

        data = await self.stream.recv(buffersize)
        while not data and asyncio.get_event_loop().time() < end_time:
            await asyncio.sleep(slp_time)
            data = await self.stream.recv(buffersize)
        if not data:
            return b''

        self.incoming.write(data)
        while True:
            try:
                decode = self.ssl_object.read(buffersize)
                break
            # if _recv called before entire message was received from the link
            except ssl.SSLWantReadError:
                more = await self.stream.recv(buffersize)
                while not more:
                    await asyncio.sleep(slp_time)
                    more = await self.stream.recv(buffersize)
                self.incoming.write(more)

        hexdump_str = utils.hexdump_ot("Rx", decode) if len(decode) > 0 else ''
        logger.debug(f"rx {len(decode)} bytes\n{hexdump_str}")

        # ssl_object.read returns 0 bytes when peer side sent TLS Alert close/notify.
        if len(decode) == 0:
            await self._closed_by_peer()

        return decode

    async def send_with_resp(self, data: bytes, timeout: float = 5.0) -> bytes:
        """
        Send data to the server over the secure TLS connection and wait for response data.

        Args:
            data: The data to send to the server.
            timeout: The maximum time in seconds to wait for the response data. Defaults to 5.0 seconds.

        Returns:
            bytes: The received response data as bytes, or empty b'' if no data TLV/line was received
                   within the timeout.
        """
        async with self._recv_lock:
            # first receive any pending unsolicited events and store in FIFO queue
            while True:
                pend_data = await self._recv(timeout=0.0)
                if len(pend_data) == 0:
                    break
                await self._async_events_queue.put(pend_data)

            # then send the data (command) and wait for the response (1 TLV or line)
            await self._send(data)
            res = await self._recv(timeout=timeout)
            if len(res) == 0:
                logger.error(f'No response when response TLV/line expected (timeout={timeout}s).')
            return res

    async def recv_unsolicited_event(self) -> bytes:
        """
        Receive unsolicited event data, if any, from the server over the secure TLS connection.

        This method is non-blocking and returns immediately if no unsolicited event data is available.
        Events are returned in FIFO order of reception. To receive multiple events, call this method
        repeatedly until no more data is available.

        Returns:
            bytes: The received event data as bytes, or empty b'' if no data is available.
        """

        # dequeue the next event, if any
        try:
            data = self._async_events_queue.get_nowait()
            return data
        except asyncio.QueueEmpty:
            pass

        # when connected, receive any pending unsolicited events incoming from the peer (and queue these)
        while True:
            if not self.is_connected:
                break
            async with self._recv_lock:
                data = await self._recv(timeout=0.0)
            if len(data) == 0:
                break
            await self._async_events_queue.put(data)

        # dequeue the next event, if any
        try:
            data = self._async_events_queue.get_nowait()
            return data
        except asyncio.QueueEmpty:
            return b''

    async def close(self, timeout: float = 3.0):
        try:
            if self.is_connected and not self._is_closed_by_local:
                logger.debug('sending Disconnect command TLV')
                data = TLV(TcatTLVType.DISCONNECT.value, bytes()).to_bytes()
                self._is_closed_by_local = True
                try:
                    await self._send(data)
                except asyncio.CancelledError:
                    raise
                except Exception as err:
                    logger.warning(f'Failed to send Disconnect command TLV: {err}')
                    logger.debug(err, exc_info=True)

            # wait until peer sends close/notify alert, setting is_connected to False, or until timeout
            async with self._recv_lock:
                t0: float = time()
                while self.is_connected:
                    if len(await self._recv(timeout=0.0)) == 0:
                        break  # alert received
                    if (time() - t0) >= timeout:
                        logger.warning(f'TLS connection close() timeout ({timeout}s). Forcing connection to close.')
                        break
                    await asyncio.sleep(0.100)
                self._peer_public_key = None
                await self._close_tls_gracefully()

        finally:
            self._peer_public_key = None
            self.peer_challenge = None
            self.ssl_object = None

    # Precondition: must only be called by _recv()
    async def _closed_by_peer(self):
        if self.is_connected:
            if not self._is_closed_by_local:
                logger.warning('TLS connection closed by peer.')
            else:
                logger.debug('TLS connection closed by local, confirmed by peer.')

        self._peer_public_key = None
        await self._close_tls_gracefully()
        self.peer_challenge = None
        self.ssl_object = None

    # Precondition: must only be called by close(), with _recv_lock acquired, or by _closed_by_peer()
    async def _close_tls_gracefully(self, buffersize: int = 4096):
        if self.ssl_object is None:
            return

        while True:
            try:
                self.ssl_object.unwrap()
                break
            except ssl.SSLWantReadError:
                recv_data = await self.stream.recv(buffersize)
                if recv_data:
                    self.incoming.write(recv_data)
            except ssl.SSLWantWriteError:
                send_data = self.outgoing.read()
                if send_data:
                    await self.stream.send(send_data)
            except ssl.SSLError as err:
                logger.warning(f'TLS closing procedure failed: {err}')
                logger.debug(err, exc_info=True)
                break
            await asyncio.sleep(0.010)

    @property
    def is_connected(self):
        return self._peer_public_key is not None and self.ssl_object is not None and \
            hasattr(self.ssl_object, 'session') and self.ssl_object.session

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
                logger.warning('No TCAT Device cert chain was received (yet).')
                return
            logger.info(f'TCAT Device cert chain: {len(cc)} certificates received.')
            for cert in cc:
                logger.info(f'  cert info:\n{cert.get_info()}')
                peer_cert_der_hex = utils.base64_string(cert.public_bytes(_ssl.ENCODING_DER))
                logger.info(f'  base64: (paste in https://lapo.it/asn1js/ to decode)\n{peer_cert_der_hex}')
            logger.info(f'TCAT Commissioner cert, PEM:\n{self.cert}')

        except asyncio.CancelledError:
            raise
        except Exception as e:
            logger.warning('Could not display TCAT client cert info (check Python version is >= 3.10?)')
