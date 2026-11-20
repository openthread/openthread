"""
  Copyright (c) 2026, The OpenThread Authors.
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
from collections import deque
from pathlib import Path
import ssl
import sys
import types
import unittest
from unittest.mock import patch

TCAT_CLIENT_DIR = Path(__file__).resolve().parents[1]
if str(TCAT_CLIENT_DIR) not in sys.path:
    sys.path.insert(0, str(TCAT_CLIENT_DIR))

# Allow running under standard unittest even in host environments without bleak installed.
try:
    import bleak  # noqa: F401
except ModuleNotFoundError:
    bleak_mod = types.ModuleType('bleak')
    bleak_mod.BleakClient = object
    backends_mod = types.ModuleType('bleak.backends')
    device_mod = types.ModuleType('bleak.backends.device')
    device_mod.BLEDevice = object
    char_mod = types.ModuleType('bleak.backends.characteristic')
    char_mod.BleakGATTCharacteristic = object
    sys.modules['bleak'] = bleak_mod
    sys.modules['bleak.backends'] = backends_mod
    sys.modules['bleak.backends.device'] = device_mod
    sys.modules['bleak.backends.characteristic'] = char_mod

from ble.ble_stream import BleConnectionClosed, BleStream  # noqa: E402
from ble.ble_stream_secure import BleStreamSecure  # noqa: E402

CERT_DIR = TCAT_CLIENT_DIR / 'auth-cert' / 'DeviceCert1'


class FakeGattCharacteristic:
    max_write_without_response_size = 512


class FakeGattService:

    def get_characteristic(self, _uuid):
        return FakeGattCharacteristic()


class FakeGattServices:

    def get_service(self, _uuid):
        return FakeGattService()


class FakeBleakClient:
    """Minimal BleakClient stand-in for testing BleStream directly."""

    def __init__(self, on_write=None):
        self.is_connected = True
        self.services = FakeGattServices()
        self._on_write = on_write

    async def write_gatt_char(self, _char, data: bytes):
        if self._on_write is not None:
            self._on_write(bytes(data))

    async def disconnect(self):
        self.is_connected = False


class FakeBleStream:
    """In-memory mock BLE transport supporting scripted fragments and link drops."""

    def __init__(self, server_peer=None):
        self._server_peer = server_peer
        self._rx_chunks = deque()
        self._on_recv = None
        self.is_connected = True
        self.recv_calls = 0

    def queue_rx(self, *chunks: bytes):
        for chunk in chunks:
            self._rx_chunks.append(chunk)

    def set_on_recv(self, callback):
        self._on_recv = callback

    async def send(self, data: bytes) -> int:
        if self._server_peer is not None:
            reply = self._server_peer.feed(data)
            if reply:
                self._rx_chunks.append(reply)
        return len(data)

    async def recv(self, bufsize: int) -> bytes:
        self.recv_calls += 1
        if self._on_recv is not None:
            result = self._on_recv(self, bufsize)
            if asyncio.iscoroutine(result):
                return await result
            if result is not None:
                return result
        if not self._rx_chunks:
            return b''
        chunk = self._rx_chunks.popleft()
        if len(chunk) > bufsize:
            self._rx_chunks.appendleft(chunk[bufsize:])
            chunk = chunk[:bufsize]
        return chunk


class TlsServerPeer:
    """In-memory TLS server endpoint paired with FakeBleStream."""

    def __init__(self):
        ctx = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH, cafile=str(CERT_DIR / 'ca_cert.pem'))
        ctx.maximum_version = ssl.TLSVersion.TLSv1_2
        ctx.verify_mode = ssl.CERT_REQUIRED
        ctx.load_cert_chain(
            certfile=str(CERT_DIR / 'device_cert.pem'),
            keyfile=str(CERT_DIR / 'device_key.pem'),
        )
        self.incoming = ssl.MemoryBIO()
        self.outgoing = ssl.MemoryBIO()
        self.ssl_object = ctx.wrap_bio(self.incoming, self.outgoing, server_side=True)
        self._handshake_done = False

    def feed(self, data: bytes) -> bytes:
        self.incoming.write(data)
        if not self._handshake_done:
            try:
                self.ssl_object.do_handshake()
                self._handshake_done = True
            except ssl.SSLWantReadError:
                pass
        return self.outgoing.read()

    def encrypt_record(self, plaintext: bytes) -> bytes:
        self.ssl_object.write(plaintext)
        return self.outgoing.read()


class TestBleStreamRecv(unittest.IsolatedAsyncioTestCase):

    def _make_ble_stream(self, on_write=None):
        client = FakeBleakClient(on_write=on_write)
        ble_stream = BleStream(client, 'svc-uuid', 'tx-uuid', 'rx-uuid')
        return client, ble_stream

    @staticmethod
    def _notify(ble_stream: BleStream, payload: bytes):
        ble_stream._BleStream__handle_rx(None, bytearray(payload))

    async def test_continuous_notifications_exit_at_max_wait(self):
        _client, ble_stream = self._make_ble_stream()
        self._notify(ble_stream, b'chunk0-')

        stop_drip = asyncio.Event()

        async def continuous_drip():
            idx = 1
            while not stop_drip.is_set():
                await asyncio.sleep(0.012)
                if not stop_drip.is_set():
                    self._notify(ble_stream, f'c{idx}-'.encode('ascii'))
                    idx += 1

        drip_task = asyncio.create_task(continuous_drip())
        try:
            with patch('ble.ble_stream.POLL_INTERVAL_SEC', 0.004):
                loop = asyncio.get_running_loop()
                start = loop.time()
                first_batch = await asyncio.wait_for(
                    ble_stream.recv(4096, recv_timeout=0.05, max_wait=0.07),
                    timeout=0.5,
                )
                elapsed = loop.time() - start

            stop_drip.set()
            await drip_task

            self.assertTrue(first_batch.startswith(b'chunk0-c1-'))
            self.assertGreaterEqual(elapsed, 0.06)
            self.assertLess(elapsed, 0.25)

            # Drain any notifications queued right before stop_drip was set.
            with patch('ble.ble_stream.POLL_INTERVAL_SEC', 0.004):
                remainder = await ble_stream.recv(4096, recv_timeout=0.02, max_wait=0.05)
                empty = await ble_stream.recv(4096, recv_timeout=0.02, max_wait=0.05)

            self.assertIsInstance(remainder, bytes)
            self.assertEqual(empty, b'')
        finally:
            stop_drip.set()
            await drip_task

    async def test_reaching_bufsize_exits_immediately(self):
        _client, ble_stream = self._make_ble_stream()
        self._notify(ble_stream, b'0123456789abcdefXYZ')

        with patch('ble.ble_stream.POLL_INTERVAL_SEC', 0.05):
            loop = asyncio.get_running_loop()
            start = loop.time()
            first = await asyncio.wait_for(
                ble_stream.recv(16, recv_timeout=5.0, max_wait=5.0),
                timeout=0.5,
            )
            elapsed = loop.time() - start

        self.assertEqual(first, b'0123456789abcdef')
        self.assertLess(elapsed, 0.05)

        with patch('ble.ble_stream.POLL_INTERVAL_SEC', 0.005):
            rest = await ble_stream.recv(16, recv_timeout=0.01, max_wait=0.05)
        self.assertEqual(rest, b'XYZ')

    async def test_disconnect_mid_burst_exits_immediately_and_drains(self):
        client, ble_stream = self._make_ble_stream()
        self._notify(ble_stream, b'partial-burst')

        async def drop_soon():
            await asyncio.sleep(0.015)
            client.is_connected = False

        drop_task = asyncio.create_task(drop_soon())
        try:
            with patch('ble.ble_stream.POLL_INTERVAL_SEC', 0.005):
                loop = asyncio.get_running_loop()
                start = loop.time()
                drained = await asyncio.wait_for(
                    ble_stream.recv(4096, recv_timeout=5.0, max_wait=5.0),
                    timeout=0.5,
                )
                elapsed = loop.time() - start

            self.assertEqual(drained, b'partial-burst')
            self.assertLess(elapsed, 0.15)
            self.assertFalse(ble_stream.is_connected)

            with self.assertRaises(BleConnectionClosed):
                await ble_stream.recv(4096, recv_timeout=0.01, max_wait=0.02)
        finally:
            await drop_task

    async def test_end_to_end_ble_stream_continuous_incomplete_tls_trickle_times_out(self):
        server = TlsServerPeer()
        ble_stream_holder = {}

        def on_write(tx_bytes: bytes):
            if ble_stream_holder.get('handshake_active', True):
                reply = server.feed(tx_bytes)
                if reply:
                    self._notify(ble_stream_holder['stream'], reply)

        _client, ble_stream = self._make_ble_stream(on_write=on_write)
        ble_stream_holder['stream'] = ble_stream

        secure_stream = BleStreamSecure(ble_stream)
        secure_stream.load_cert(
            certfile=str(CERT_DIR / 'commissioner_cert.pem'),
            keyfile=str(CERT_DIR / 'commissioner_key.pem'),
            cafile=str(CERT_DIR / 'ca_cert.pem'),
        )

        with patch('ble.ble_stream.POLL_INTERVAL_SEC', 0.002), \
                patch('ble.ble_stream_secure.POLL_INTERVAL_SEC', 0.002), \
                patch.object(BleStream.recv, '__defaults__', (0.005, 0.02)):
            ok = await secure_stream.do_handshake(timeout=2.0)
        self.assertTrue(ok)
        ble_stream_holder['handshake_active'] = False

        # Peer trickles a valid 229-byte TLS record 1 byte every 8 ms (faster than
        # recv_timeout=0.025 s), so BleStream.recv() must hit max_wait=0.035 s and
        # BleStreamSecure._recv() must cut off at TCAT_RESPONSE_TIMEOUT_SEC=0.09 s.
        record = server.encrypt_record(b'B' * 200)
        stop_trickle = asyncio.Event()

        async def trickle_notifications():
            idx = 0
            while not stop_trickle.is_set() and idx < len(record) - 10:
                self._notify(ble_stream, record[idx:idx + 1])
                idx += 1
                await asyncio.sleep(0.008)

        trickle_task = asyncio.create_task(trickle_notifications())
        try:
            with patch('ble.ble_stream.POLL_INTERVAL_SEC', 0.003), \
                    patch('ble.ble_stream_secure.POLL_INTERVAL_SEC', 0.003), \
                    patch('ble.ble_stream_secure.TCAT_RESPONSE_TIMEOUT_SEC', 0.09), \
                    patch.object(BleStream.recv, '__defaults__', (0.025, 0.035)):
                await asyncio.sleep(0.01)
                with self.assertLogs('ble.ble_stream_secure', level='WARNING') as cm:
                    async with secure_stream._recv_lock:
                        received = await asyncio.wait_for(secure_stream._recv(timeout=0.0), timeout=0.6)

            self.assertEqual(received, b'')
            self.assertTrue(any('Timed out waiting for complete TLS record' in msg for msg in cm.output))
        finally:
            stop_trickle.set()
            await trickle_task


class TestBleStreamSecureRecv(unittest.IsolatedAsyncioTestCase):

    async def asyncSetUp(self):
        self.server = TlsServerPeer()
        self.stream = FakeBleStream(server_peer=self.server)
        self.secure_stream = BleStreamSecure(self.stream)
        self.secure_stream.load_cert(
            certfile=str(CERT_DIR / 'commissioner_cert.pem'),
            keyfile=str(CERT_DIR / 'commissioner_key.pem'),
            cafile=str(CERT_DIR / 'ca_cert.pem'),
        )

        with patch('ble.ble_stream_secure.POLL_INTERVAL_SEC', 0.002):
            ok = await self.secure_stream.do_handshake(timeout=2.0)
        self.assertTrue(ok)
        # Detach automatic handshake responder and reset counters for the test body.
        self.stream._server_peer = None
        self.stream.recv_calls = 0

    async def test_multi_fragment_record_positive_timeout(self):
        payload = b'hello-multi-fragment-positive-timeout'
        record = self.server.encrypt_record(payload)
        self.assertGreater(len(record), 12)

        # Deliver initial empty poll, then 3 fragments separated by empty polls.
        self.stream.queue_rx(b'', record[:5], b'', b'', record[5:15], b'', record[15:])

        with patch('ble.ble_stream_secure.POLL_INTERVAL_SEC', 0.002), \
                patch('ble.ble_stream_secure.TCAT_RESPONSE_TIMEOUT_SEC', 0.2):
            async with self.secure_stream._recv_lock:
                received = await self.secure_stream._recv(timeout=0.2)

        self.assertEqual(received, payload)

    async def test_multi_fragment_record_zero_timeout(self):
        payload = b'hello-multi-fragment-zero-timeout'
        record = self.server.encrypt_record(payload)
        self.assertGreater(len(record), 12)

        # First BLE notification has arrived when _recv(timeout=0.0) polls;
        # remaining fragments arrive on later polls.
        self.stream.queue_rx(record[:4], b'', b'', record[4:14], b'', record[14:])

        with patch('ble.ble_stream_secure.POLL_INTERVAL_SEC', 0.002), \
                patch('ble.ble_stream_secure.TCAT_RESPONSE_TIMEOUT_SEC', 0.2):
            async with self.secure_stream._recv_lock:
                received = await self.secure_stream._recv(timeout=0.0)

        self.assertEqual(received, payload)

    async def test_recv_unsolicited_event_multi_fragment_record(self):
        payload = b'\x81\x04test'
        record = self.server.encrypt_record(payload)
        self.stream.queue_rx(record[:6], b'', record[6:])

        with patch('ble.ble_stream_secure.POLL_INTERVAL_SEC', 0.002), \
                patch('ble.ble_stream_secure.TCAT_RESPONSE_TIMEOUT_SEC', 0.2):
            event = await self.secure_stream.recv_unsolicited_event()

        self.assertEqual(event, payload)

    async def test_stall_mid_record_times_out_and_returns_empty(self):
        record = self.server.encrypt_record(b'stalled-record-payload')
        self.stream.queue_rx(record[:8])

        with patch('ble.ble_stream_secure.POLL_INTERVAL_SEC', 0.005), \
                patch('ble.ble_stream_secure.TCAT_RESPONSE_TIMEOUT_SEC', 0.05):
            with self.assertLogs('ble.ble_stream_secure', level='WARNING') as cm:
                async with self.secure_stream._recv_lock:
                    received = await asyncio.wait_for(self.secure_stream._recv(timeout=0.0), timeout=0.5)

            self.assertEqual(received, b'')
            self.assertTrue(any('Timed out waiting for complete TLS record' in msg for msg in cm.output))

            # A subsequent non-blocking poll with no new bytes must return immediately
            # instead of waiting another TCAT_RESPONSE_TIMEOUT_SEC.
            loop = asyncio.get_running_loop()
            start = loop.time()
            async with self.secure_stream._recv_lock:
                follow_up = await self.secure_stream._recv(timeout=0.0)
            elapsed = loop.time() - start
            self.assertEqual(follow_up, b'')
            self.assertLess(elapsed, 0.02)

    async def test_trickle_incomplete_record_times_out(self):
        record = self.server.encrypt_record(b'A' * 200)
        offset = 0

        async def slow_trickle(_stream, _bufsize):
            nonlocal offset
            await asyncio.sleep(0.01)
            byte = record[offset:offset + 1]
            offset += 1
            return byte

        self.stream.set_on_recv(slow_trickle)

        with patch('ble.ble_stream_secure.POLL_INTERVAL_SEC', 0.002), \
                patch('ble.ble_stream_secure.TCAT_RESPONSE_TIMEOUT_SEC', 0.04):
            with self.assertLogs('ble.ble_stream_secure', level='WARNING'):
                async with self.secure_stream._recv_lock:
                    received = await asyncio.wait_for(self.secure_stream._recv(timeout=0.0), timeout=0.5)

        self.assertEqual(received, b'')

    async def test_disconnect_immediately_after_partial_record(self):
        record = self.server.encrypt_record(b'drop-after-first-fragment')

        def recv_partial_then_drop(stream, _bufsize):
            if stream.recv_calls == 1:
                stream.is_connected = False
                return record[:8]
            return b''

        self.stream.set_on_recv(recv_partial_then_drop)

        with patch('ble.ble_stream_secure.POLL_INTERVAL_SEC', 0.05), \
                patch('ble.ble_stream_secure.TCAT_RESPONSE_TIMEOUT_SEC', 5.0):
            with self.assertNoLogs('ble.ble_stream_secure', level='WARNING'):
                loop = asyncio.get_running_loop()
                start = loop.time()
                async with self.secure_stream._recv_lock:
                    received = await asyncio.wait_for(self.secure_stream._recv(timeout=1.0), timeout=0.5)
                elapsed = loop.time() - start

        self.assertEqual(received, b'')
        self.assertEqual(self.stream.recv_calls, 1)
        self.assertLess(elapsed, 0.05)

    async def test_disconnect_while_polling_mid_record(self):
        record = self.server.encrypt_record(b'drop-while-polling-mid-record')

        def recv_then_drop_on_third_call(stream, _bufsize):
            if stream.recv_calls == 1:
                return record[:8]
            if stream.recv_calls == 3:
                stream.is_connected = False
            return b''

        self.stream.set_on_recv(recv_then_drop_on_third_call)

        with patch('ble.ble_stream_secure.POLL_INTERVAL_SEC', 0.005), \
                patch('ble.ble_stream_secure.TCAT_RESPONSE_TIMEOUT_SEC', 5.0):
            with self.assertNoLogs('ble.ble_stream_secure', level='WARNING'):
                loop = asyncio.get_running_loop()
                start = loop.time()
                async with self.secure_stream._recv_lock:
                    received = await asyncio.wait_for(self.secure_stream._recv(timeout=1.0), timeout=0.5)
                elapsed = loop.time() - start

        self.assertEqual(received, b'')
        self.assertLess(elapsed, 0.2)

    async def test_disconnect_while_waiting_for_initial_bytes(self):

        def drop_on_second_poll(stream, _bufsize):
            if stream.recv_calls == 2:
                stream.is_connected = False
            return b''

        self.stream.set_on_recv(drop_on_second_poll)

        with patch('ble.ble_stream_secure.POLL_INTERVAL_SEC', 0.005):
            loop = asyncio.get_running_loop()
            start = loop.time()
            async with self.secure_stream._recv_lock:
                received = await asyncio.wait_for(self.secure_stream._recv(timeout=5.0), timeout=0.5)
            elapsed = loop.time() - start

        self.assertEqual(received, b'')
        self.assertLess(elapsed, 0.2)

    async def test_coalesced_two_records_in_single_recv(self):
        msg1 = b'first-record'
        msg2 = b'second-record'
        rec1 = self.server.encrypt_record(msg1)
        rec2 = self.server.encrypt_record(msg2)
        self.stream.queue_rx(rec1 + rec2)

        async with self.secure_stream._recv_lock:
            first = await self.secure_stream._recv(timeout=0.0)
            second = await self.secure_stream._recv(timeout=0.0)
            third = await self.secure_stream._recv(timeout=0.0)

        self.assertEqual(first, msg1)
        self.assertEqual(second, msg2)
        self.assertEqual(third, b'')

    async def test_coalesced_record_plus_partial_second_record(self):
        msg1 = b'complete-first'
        msg2 = b'fragmented-second'
        rec1 = self.server.encrypt_record(msg1)
        rec2 = self.server.encrypt_record(msg2)
        self.stream.queue_rx(rec1 + rec2[:7], b'', rec2[7:])

        with patch('ble.ble_stream_secure.POLL_INTERVAL_SEC', 0.002), \
                patch('ble.ble_stream_secure.TCAT_RESPONSE_TIMEOUT_SEC', 0.2):
            async with self.secure_stream._recv_lock:
                first = await self.secure_stream._recv(timeout=0.0)
                second = await self.secure_stream._recv(timeout=0.0)

        self.assertEqual(first, msg1)
        self.assertEqual(second, msg2)

    async def test_pending_plaintext_in_ssl_object(self):
        payload = b'0123456789abcdef'
        record = self.server.encrypt_record(payload)
        self.stream.queue_rx(record)

        with patch('ble.ble_stream_secure.POLL_INTERVAL_SEC', 0.002), \
                patch('ble.ble_stream_secure.TCAT_RESPONSE_TIMEOUT_SEC', 0.2):
            async with self.secure_stream._recv_lock:
                part1 = await self.secure_stream._recv(buffersize=6, timeout=0.0)
                calls_after_part1 = self.stream.recv_calls
                part2 = await self.secure_stream._recv(buffersize=64, timeout=0.0)

        self.assertEqual(part1, b'012345')
        self.assertEqual(part2, b'6789abcdef')
        self.assertEqual(self.stream.recv_calls, calls_after_part1)


if __name__ == '__main__':
    unittest.main()
