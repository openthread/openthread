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
from pathlib import Path
import sys
import types
import unittest
from unittest.mock import AsyncMock, MagicMock, patch

TCAT_CLIENT_DIR = str(Path(__file__).resolve().parents[1])
if TCAT_CLIENT_DIR not in sys.path:
    sys.path.insert(0, TCAT_CLIENT_DIR)

from ble.ble_stream_secure import BleStreamSecure
from tlv.tcat_tlv import TcatTLVType
from tlv.tlv import TLV


class FakeUnderlyingStream:

    def __init__(self, connected: bool = False):
        self.is_connected = connected


class TestTcatTlvAndParser(unittest.TestCase):

    def test_is_unsolicited(self):
        unsolicited = [
            TcatTLVType.RESPONSE_EVENT.value,
            TcatTLVType.APPLICATION_DATA_1.value,
            TcatTLVType.APPLICATION_DATA_2.value,
            TcatTLVType.APPLICATION_DATA_3.value,
            TcatTLVType.APPLICATION_DATA_4.value,
        ]
        for tlv_type in unsolicited:
            self.assertTrue(TcatTLVType.is_unsolicited(tlv_type))

        non_unsolicited = [
            TcatTLVType.RESPONSE_W_STATUS.value,
            TcatTLVType.RESPONSE_W_PAYLOAD.value,
            TcatTLVType.PING.value,
            TcatTLVType.GET_APPLICATION_LAYERS.value,
            TcatTLVType.VENDOR_APPLICATION.value,
            0x00,
            0x85,
            0xFF,
        ]
        for tlv_type in non_unsolicited:
            self.assertFalse(TcatTLVType.is_unsolicited(tlv_type))

        # Ensure adding the classmethod did not alter enum iteration.
        for member in TcatTLVType:
            self.assertIsInstance(member.value, int)

    def test_validate_unsolicited_tlv(self):
        try:
            import bbtc
        except ModuleNotFoundError:
            fake_bleak = types.ModuleType('bleak')
            fake_bleak.__path__ = []
            fake_bleak.BLEDevice = MagicMock()
            fake_bleak.BleakClient = MagicMock()
            fake_bleak.BleakScanner = MagicMock()
            fake_backends = types.ModuleType('bleak.backends')
            fake_backends.__path__ = []
            fake_device = types.ModuleType('bleak.backends.device')
            fake_device.BLEDevice = MagicMock()
            fake_char = types.ModuleType('bleak.backends.characteristic')
            fake_char.BleakGATTCharacteristic = MagicMock()
            fake_uuids = types.ModuleType('bleak.uuids')
            fake_uuids.normalize_uuid_str = lambda s: s
            with patch.dict(
                    sys.modules,
                {
                    'bleak': fake_bleak,
                    'bleak.backends': fake_backends,
                    'bleak.backends.device': fake_device,
                    'bleak.backends.characteristic': fake_char,
                    'bleak.uuids': fake_uuids,
                },
            ):
                import bbtc

        with self.assertLogs(bbtc.logger, level='INFO') as cm_event:
            bbtc.validate_unsolicited_tlv(TLV(TcatTLVType.RESPONSE_EVENT.value, b'\x00'))
        self.assertTrue(any('Response Event 0x3' in msg for msg in cm_event.output))

        with self.assertLogs(bbtc.logger, level='INFO') as cm_app:
            bbtc.validate_unsolicited_tlv(TLV(TcatTLVType.APPLICATION_DATA_2.value, b'\xaa'))
        self.assertTrue(any('Send Application Data 2 0x82' in msg for msg in cm_app.output))

        with self.assertLogs(bbtc.logger, level='ERROR') as cm_err:
            bbtc.validate_unsolicited_tlv(TLV(TcatTLVType.RESPONSE_W_STATUS.value, b'\x00'))
        self.assertTrue(any('Illegal unsolicited TLV type' in msg for msg in cm_err.output))

    def test_parse_tlvs_short_and_extended_headers(self):
        short_tlv = TLV(TcatTLVType.RESPONSE_W_STATUS.value, b'\x00').to_bytes()
        # Extended header (0xFF) carrying a short (< 254 byte) payload, as used by HandleGetDiagnosticTlvs.
        ext_small_payload = b'\x00\x08\x11\x22\x33\x44\x55\x66\x77\x88'
        ext_small_tlv = bytes([TcatTLVType.RESPONSE_W_PAYLOAD.value, 0xFF, 0x00,
                               len(ext_small_payload)]) + (ext_small_payload)
        long_payload = bytes(range(256)) * 2
        long_tlv = TLV(TcatTLVType.RESPONSE_W_PAYLOAD.value, long_payload).to_bytes()

        parsed = TLV.parse_tlvs(short_tlv + ext_small_tlv + long_tlv)
        self.assertEqual(len(parsed), 3)
        self.assertEqual(parsed[0].type, TcatTLVType.RESPONSE_W_STATUS.value)
        self.assertEqual(parsed[0].value, b'\x00')
        self.assertEqual(parsed[1].type, TcatTLVType.RESPONSE_W_PAYLOAD.value)
        self.assertEqual(parsed[1].value, ext_small_payload)
        self.assertEqual(parsed[2].type, TcatTLVType.RESPONSE_W_PAYLOAD.value)
        self.assertEqual(parsed[2].value, long_payload)

    def test_truncated_tlvs_raise_value_error(self):
        for bad_data in (
                b'\x01',
                b'\x02\xff\x00',
                b'\x02\x05\xaa\xbb',
                b'\x02\xff\x00\x04\xaa\xbb',
        ):
            with self.assertRaises(ValueError):
                TLV.from_bytes(bad_data)
            with self.assertRaises(ValueError):
                TLV.parse_tlvs(bad_data)


class TestBleStreamSecureSendWithResp(unittest.IsolatedAsyncioTestCase):

    def _make_secure_stream(self, pre_send_chunks=None, post_send_chunks=None):
        bless = BleStreamSecure(FakeUnderlyingStream(connected=False))
        pre_iter = iter(pre_send_chunks or [])
        post_iter = iter(post_send_chunks or [])
        sent_flag = False

        async def fake_send(data: bytes, buffersize: int = 0):
            nonlocal sent_flag
            sent_flag = True

        async def fake_recv(buffersize: int = 0, timeout: float = 0.0) -> bytes:
            if not sent_flag:
                return next(pre_iter, b'')
            item = next(post_iter, b'')
            if callable(item):
                return await item(timeout)
            return item

        bless._send = AsyncMock(side_effect=fake_send)
        bless._recv = AsyncMock(side_effect=fake_recv)
        return bless

    async def _drain_events(self, bless: BleStreamSecure):
        events = []
        while True:
            ev = await bless.recv_unsolicited_event()
            if not ev:
                break
            events.append(ev)
        return events

    async def test_response_w_status_returned_directly(self):
        cmd = TLV(TcatTLVType.THREAD_START.value, b'').to_bytes()
        resp = TLV(TcatTLVType.RESPONSE_W_STATUS.value, b'\x00').to_bytes()
        bless = self._make_secure_stream(post_send_chunks=[resp])

        res = await bless.send_with_resp(cmd, timeout=1.0)

        self.assertEqual(res, resp)
        self.assertTrue(bless._async_events_queue.empty())
        self.assertEqual(await self._drain_events(bless), [])

    async def test_response_w_payload_returned_directly(self):
        payload = bytes(range(256)) + b'\xaa\xbb'
        cmd = TLV(TcatTLVType.PING.value, payload).to_bytes()
        resp = TLV(TcatTLVType.RESPONSE_W_PAYLOAD.value, payload).to_bytes()
        bless = self._make_secure_stream(post_send_chunks=[resp])

        res = await bless.send_with_resp(cmd, timeout=1.0)

        self.assertEqual(res, resp)
        self.assertTrue(bless._async_events_queue.empty())

    async def test_unsolicited_tlvs_queued_in_fifo_order_before_response(self):
        cmd = TLV(TcatTLVType.GET_NETWORK_NAME.value, b'').to_bytes()
        unsolicited_tlvs = [
            TLV(TcatTLVType.RESPONSE_EVENT.value, b'\x01').to_bytes(),
            TLV(TcatTLVType.APPLICATION_DATA_1.value, b'\x11\x12').to_bytes(),
            TLV(TcatTLVType.APPLICATION_DATA_2.value, b'\x21\x22').to_bytes(),
            TLV(TcatTLVType.APPLICATION_DATA_3.value, b'\x31\x32').to_bytes(),
            TLV(TcatTLVType.APPLICATION_DATA_4.value, b'\x41\x42').to_bytes(),
        ]
        resp = TLV(TcatTLVType.RESPONSE_W_STATUS.value, b'\x00').to_bytes()
        bless = self._make_secure_stream(post_send_chunks=[*unsolicited_tlvs, resp])

        res = await bless.send_with_resp(cmd, timeout=1.0)

        self.assertEqual(res, resp)
        self.assertEqual(await self._drain_events(bless), unsolicited_tlvs)

    async def test_coalesced_tls_read_demuxes_unsolicited_and_response(self):
        cmd = TLV(TcatTLVType.PING.value, b'\xde\xad').to_bytes()
        ev_before = TLV(TcatTLVType.APPLICATION_DATA_1.value, b'\xaa\xbb').to_bytes()
        resp = TLV(TcatTLVType.RESPONSE_W_PAYLOAD.value, b'\xde\xad').to_bytes()
        ev_after = TLV(TcatTLVType.RESPONSE_EVENT.value, b'\x02').to_bytes()

        bless = self._make_secure_stream(post_send_chunks=[ev_before + resp + ev_after])

        res = await bless.send_with_resp(cmd, timeout=1.0)

        self.assertEqual(res, resp)
        self.assertEqual(await self._drain_events(bless), [ev_before, ev_after])

    async def test_pending_pre_send_and_in_flight_events_preserve_fifo_order(self):
        cmd = TLV(TcatTLVType.PING.value, b'\x01').to_bytes()
        ev_pre_1 = TLV(TcatTLVType.APPLICATION_DATA_1.value, b'\x01').to_bytes()
        ev_pre_2 = TLV(TcatTLVType.APPLICATION_DATA_2.value, b'\x02').to_bytes()
        ev_post = TLV(TcatTLVType.RESPONSE_EVENT.value, b'\x03').to_bytes()
        resp = TLV(TcatTLVType.RESPONSE_W_PAYLOAD.value, b'\x01').to_bytes()

        bless = self._make_secure_stream(
            pre_send_chunks=[ev_pre_1 + ev_pre_2],
            post_send_chunks=[ev_post, resp],
        )

        res = await bless.send_with_resp(cmd, timeout=1.0)

        self.assertEqual(res, resp)
        self.assertEqual(await self._drain_events(bless), [ev_pre_1, ev_pre_2, ev_post])

    async def test_only_unsolicited_events_until_timeout_queues_events_and_returns_empty(self):
        cmd = TLV(TcatTLVType.GET_DEVICE_ID.value, b'').to_bytes()
        ev1 = TLV(TcatTLVType.RESPONSE_EVENT.value, b'\x00').to_bytes()
        ev2 = TLV(TcatTLVType.APPLICATION_DATA_3.value, b'\xca\xfe').to_bytes()

        async def wait_and_timeout(timeout: float) -> bytes:
            await asyncio.sleep(min(timeout, 0.02))
            return b''

        bless = self._make_secure_stream(post_send_chunks=[ev1, ev2, wait_and_timeout])

        with self.assertLogs('ble.ble_stream_secure', level='ERROR') as cm:
            res = await bless.send_with_resp(cmd, timeout=0.05)

        self.assertEqual(res, b'')
        self.assertTrue(any('No response when response TLV/line expected' in msg for msg in cm.output))
        self.assertEqual(await self._drain_events(bless), [ev1, ev2])

    async def test_continuous_unsolicited_stream_stops_at_deadline(self):
        cmd = TLV(TcatTLVType.GET_DEVICE_ID.value, b'').to_bytes()
        ev = TLV(TcatTLVType.APPLICATION_DATA_1.value, b'\x99').to_bytes()

        async def slow_unsolicited(_timeout: float) -> bytes:
            await asyncio.sleep(0.02)
            return ev

        bless = self._make_secure_stream(post_send_chunks=[slow_unsolicited] * 10)

        with self.assertLogs('ble.ble_stream_secure', level='ERROR'):
            res = await bless.send_with_resp(cmd, timeout=0.05)

        self.assertEqual(res, b'')
        drained = await self._drain_events(bless)
        self.assertGreaterEqual(len(drained), 2)
        self.assertLess(len(drained), 10)
        self.assertTrue(all(item == ev for item in drained))

    async def test_fragmented_tlv_reassembled_across_recv_calls(self):
        cmd = TLV(TcatTLVType.PING.value, b'\x01\x02\x03\x04').to_bytes()
        resp = TLV(TcatTLVType.RESPONSE_W_PAYLOAD.value, b'\x01\x02\x03\x04').to_bytes()
        bless = self._make_secure_stream(post_send_chunks=[resp[:3], resp[3:]])

        res = await bless.send_with_resp(cmd, timeout=1.0)

        self.assertEqual(res, resp)

    async def test_truncated_bytes_handled_safely_without_crash(self):
        cmd = TLV(TcatTLVType.GET_EXT_PAN_ID.value, b'').to_bytes()
        ev = TLV(TcatTLVType.APPLICATION_DATA_2.value, b'\xab').to_bytes()
        truncated = b'\x02\x10\xde\xad'

        bless = self._make_secure_stream(post_send_chunks=[ev + truncated, b''])

        with self.assertLogs('ble.ble_stream_secure', level='WARNING') as cm:
            res = await bless.send_with_resp(cmd, timeout=0.05)

        self.assertEqual(res, b'')
        self.assertTrue(any('Discarding incomplete or malformed TLV data' in msg for msg in cm.output))
        self.assertTrue(any('No response when response TLV/line expected' in msg for msg in cm.output))
        self.assertEqual(await self._drain_events(bless), [ev])

    async def test_pre_send_partial_unsolicited_tlv_completed_after_send(self):
        cmd = TLV(TcatTLVType.PING.value, b'\x42').to_bytes()
        ev1 = TLV(TcatTLVType.APPLICATION_DATA_1.value, b'\xaa\xbb').to_bytes()
        ev2 = TLV(TcatTLVType.APPLICATION_DATA_2.value, b'\x11\x22\x33\x44').to_bytes()
        resp = TLV(TcatTLVType.RESPONSE_W_PAYLOAD.value, b'\x42').to_bytes()

        bless = self._make_secure_stream(
            pre_send_chunks=[ev1 + ev2[:3]],
            post_send_chunks=[ev2[3:] + resp],
        )

        res = await bless.send_with_resp(cmd, timeout=1.0)

        self.assertEqual(res, resp)
        self.assertEqual(await self._drain_events(bless), [ev1, ev2])

    async def test_pre_send_valid_tlv_preserved_when_trailing_bytes_malformed(self):
        cmd = TLV(TcatTLVType.GET_NETWORK_NAME.value, b'').to_bytes()
        ev1 = TLV(TcatTLVType.RESPONSE_EVENT.value, b'\x01').to_bytes()
        malformed = b'\x81\x20\xde\xad'
        next_resp = TLV(TcatTLVType.RESPONSE_W_STATUS.value, b'\x00').to_bytes()

        bless = self._make_secure_stream(
            pre_send_chunks=[ev1 + malformed],
            post_send_chunks=[b'', b'', next_resp],
        )

        with self.assertLogs('ble.ble_stream_secure', level='WARNING') as cm:
            res = await bless.send_with_resp(cmd, timeout=0.05)

        self.assertEqual(res, b'')
        self.assertEqual(len(bless._rx_tlv_buf), 0)
        self.assertTrue(any('Discarding incomplete or malformed TLV data' in msg for msg in cm.output))
        self.assertEqual(await self._drain_events(bless), [ev1])

        # Subsequent command succeeds cleanly after malformed bytes were cleared on timeout.
        res2 = await bless.send_with_resp(cmd, timeout=1.0)
        self.assertEqual(res2, next_resp)

    async def test_recv_unsolicited_event_demuxes_coalesced_tlvs(self):
        bless = BleStreamSecure(FakeUnderlyingStream(connected=True))
        bless._peer_public_key = b'peer-key'
        bless.ssl_object = MagicMock()

        ev1 = TLV(TcatTLVType.RESPONSE_EVENT.value, b'\x05').to_bytes()
        ev2 = TLV(TcatTLVType.APPLICATION_DATA_1.value, b'\xaa\xbb').to_bytes()
        ev3 = TLV(TcatTLVType.APPLICATION_DATA_4.value, b'\xcc').to_bytes()
        chunks = iter([ev1 + ev2 + ev3, b'', b''])

        async def fake_recv(buffersize: int = 0, timeout: float = 0.0) -> bytes:
            return next(chunks, b'')

        bless._recv = AsyncMock(side_effect=fake_recv)

        self.assertEqual(await bless.recv_unsolicited_event(), ev1)
        self.assertEqual(await bless.recv_unsolicited_event(), ev2)
        self.assertEqual(await bless.recv_unsolicited_event(), ev3)
        self.assertEqual(await bless.recv_unsolicited_event(), b'')

    async def test_recv_unsolicited_event_reassembles_fragmented_tlv(self):
        bless = BleStreamSecure(FakeUnderlyingStream(connected=True))
        bless._peer_public_key = b'peer-key'
        bless.ssl_object = MagicMock()

        ev = TLV(TcatTLVType.APPLICATION_DATA_3.value, b'\xde\xad\xbe\xef').to_bytes()
        batches = [iter([ev[:3], b'']), iter([ev[3:], b''])]
        batch_idx = 0

        async def fake_recv(buffersize: int = 0, timeout: float = 0.0) -> bytes:
            return next(batches[batch_idx], b'')

        bless._recv = AsyncMock(side_effect=fake_recv)

        # First call receives only partial TLV bytes and returns empty without crashing.
        self.assertEqual(await bless.recv_unsolicited_event(), b'')

        # Second call receives remaining bytes and returns the complete reassembled TLV.
        batch_idx = 1
        self.assertEqual(await bless.recv_unsolicited_event(), ev)
        self.assertEqual(await bless.recv_unsolicited_event(), b'')

    async def test_extended_header_wire_bytes_preserved(self):
        cmd = TLV(TcatTLVType.GET_DIAGNOSTIC_TLVS.value, b'\x00').to_bytes()
        ext_ev_pre = bytes([TcatTLVType.APPLICATION_DATA_1.value, 0xFF, 0x00, 0x03]) + b'\x11\x22\x33'
        ext_ev_post = bytes([TcatTLVType.RESPONSE_EVENT.value, 0xFF, 0x00, 0x01]) + b'\x00'
        ext_resp = bytes([TcatTLVType.RESPONSE_W_PAYLOAD.value, 0xFF, 0x00, 0x04]) + b'\xaa\xbb\xcc\xdd'

        bless = self._make_secure_stream(
            pre_send_chunks=[ext_ev_pre],
            post_send_chunks=[ext_ev_post + ext_resp],
        )

        res = await bless.send_with_resp(cmd, timeout=1.0)

        self.assertEqual(res, ext_resp)
        self.assertEqual(await self._drain_events(bless), [ext_ev_pre, ext_ev_post])

        bless_connected = BleStreamSecure(FakeUnderlyingStream(connected=True))
        bless_connected._peer_public_key = b'peer-key'
        bless_connected.ssl_object = MagicMock()
        ext_ev_direct = bytes([TcatTLVType.APPLICATION_DATA_2.value, 0xFF, 0x00, 0x02]) + b'\xde\xad'
        chunks = iter([ext_ev_direct, b'', b''])

        async def fake_recv(buffersize: int = 0, timeout: float = 0.0) -> bytes:
            return next(chunks, b'')

        bless_connected._recv = AsyncMock(side_effect=fake_recv)

        self.assertEqual(await bless_connected.recv_unsolicited_event(), ext_ev_direct)
        self.assertEqual(await bless_connected.recv_unsolicited_event(), b'')


if __name__ == '__main__':
    unittest.main()
