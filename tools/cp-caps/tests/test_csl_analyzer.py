#!/usr/bin/env python3
#
#  Copyright (c) 2026, The OpenThread Authors.
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
"""Unit tests of the CSL analyzer, which do not need any device."""

import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))

import csl_analyzer

from csl_analyzer import CSL_TX_TOLERANCE_US, MAX_CLOCK_DRIFT_PPM, CslIe, analyze_csl_tx, parse_mac_frame
from sniffer import SniffedFrame

CHANNEL = 20
DUT = {'a2bc7ebbdd2da161', '5c00'}
SSED = {'3edf345f6c63fa01', '5c01'}

# Frames captured on the simulated testbed, including the FCS.
#
# Secured 2015 data frame from 0x5c00 to 0x5c01, sequence 0x7b.
DATA_2015 = ('69a87b6f95015c005c0d0100000001962758b6d335ec71fc8f719688db129faaf2e16f9b031001d4dcc91afe38915ab965b4'
             '3b22f9760063')
# Secured 2015 data frame from 0x5c01 to 0x5c00, sequence 0x3e, with a CSL IE and a header termination IE.
DATA_2015_CSL_IE = (
    '69aa3e6f95005c015c0d0300000001040dcf07d007803f2c18588cfdb6c755b347e8d55ef4de09064dc749dab780f2738860'
    '1e3306b9b60575f8fc227528187f')
# Secured 2015 data frame between extended addresses, sequence 0x3d, with a CSL IE and a header termination IE.
DATA_2015_EXT_CSL_IE = (
    '29ee3d6f9561a12dddbb7ebca201fa636c5f34df3e0d0100000001040d5b05d007803fe87bcf0a7da7432eaf9abd9a1d4394'
    'c7d01ef31c7fcd2db47592316eb77919fafb4d79690f32030ab795ecc41c1f7bf3df598119aa29dea0b71bdd99e2610b'
    '195eb26259f22730325fc1796be3977e5a')
# Secured Enh-ACK to 0x5c00, sequence 0x7b, with a CSL IE.
ENH_ACK_CSL_IE = '1a2a7b6f95005c0d0200000001040d0000d007d41753cfdc74'
# Imm-ACK with the frame pending bit, sequence 0x78.
IMM_ACK = '121078735a'
# Secured 2006 MAC command frame between extended addresses, sequence 0x3c.
COMMAND_2006 = '6bdc3c6f9561a12dddbb7ebca201fa636c5f34df3e0d0000000001044ac109dad21e'
# 2006 broadcast data frame (MLE Parent Request) from 3edf345f6c63fa01, sequence 0x3a.
PARENT_REQUEST_2006 = (
    '41d83a6f95ffff01fa636c5f34df3e7f3b02f04d4c4d4cc4c700150000000000000000014369a8ecf57d68cb789b1c31c1b9'
    'ae32265a2f405d18b5be40d690')


class TestParseMacFrame(unittest.TestCase):

    def test_data_frame(self):
        frame = parse_mac_frame(bytes.fromhex(DATA_2015))

        self.assertEqual(frame.frame_type, 1)
        self.assertTrue(frame.ack_request)
        self.assertEqual(frame.seq, 0x7b)
        self.assertEqual(frame.dst_addr, '5c01')
        self.assertEqual(frame.src_addr, '5c00')
        self.assertIsNone(frame.csl_ie)

    def test_data_frame_with_csl_ie(self):
        frame = parse_mac_frame(bytes.fromhex(DATA_2015_CSL_IE))

        self.assertEqual(frame.seq, 0x3e)
        self.assertEqual(frame.dst_addr, '5c00')
        self.assertEqual(frame.src_addr, '5c01')
        self.assertEqual(frame.csl_ie, CslIe(phase=1999, period=2000))

        frame = parse_mac_frame(bytes.fromhex(DATA_2015_EXT_CSL_IE))

        self.assertEqual(frame.dst_addr, 'a2bc7ebbdd2da161')
        self.assertEqual(frame.src_addr, '3edf345f6c63fa01')
        self.assertEqual(frame.csl_ie, CslIe(phase=1371, period=2000))

    def test_enh_ack_with_csl_ie(self):
        frame = parse_mac_frame(bytes.fromhex(ENH_ACK_CSL_IE))

        self.assertEqual(frame.frame_type, 2)
        self.assertEqual(frame.seq, 0x7b)
        self.assertEqual(frame.dst_addr, '5c00')
        self.assertIsNone(frame.src_addr)
        self.assertEqual(frame.csl_ie, CslIe(phase=0, period=2000))

    def test_imm_ack(self):
        frame = parse_mac_frame(bytes.fromhex(IMM_ACK))

        self.assertEqual(frame.frame_type, 2)
        self.assertEqual(frame.seq, 0x78)
        self.assertIsNone(frame.dst_addr)
        self.assertIsNone(frame.src_addr)

    def test_2006_frames(self):
        frame = parse_mac_frame(bytes.fromhex(COMMAND_2006))

        self.assertEqual(frame.frame_type, 3)
        self.assertEqual(frame.dst_addr, 'a2bc7ebbdd2da161')
        self.assertEqual(frame.src_addr, '3edf345f6c63fa01')

        frame = parse_mac_frame(bytes.fromhex(PARENT_REQUEST_2006))

        self.assertFalse(frame.ack_request)
        self.assertEqual(frame.seq, 0x3a)
        self.assertEqual(frame.dst_addr, 'ffff')
        self.assertEqual(frame.src_addr, '3edf345f6c63fa01')

    def test_truncated_frames(self):
        # Truncated within the addressing fields, the security header and the CSL IE.
        for psdu, length in ((DATA_2015, 7), (DATA_2015_CSL_IE, 12), (ENH_ACK_CSL_IE, 21)):
            self.assertIsNone(parse_mac_frame(bytes.fromhex(psdu)[:length] + b'\xff\xff'))

        self.assertIsNone(parse_mac_frame(bytes.fromhex('12')))

    def test_panid_presence(self):
        # IEEE 802.15.4-2015, table 7-2: (dst addr mode, src addr mode, PAN ID compression) -> (dst PAN ID, src PAN ID)
        NONE, SHORT, EXT = 0, 2, 3
        table = {
            (NONE, NONE, False): (False, False),
            (NONE, NONE, True): (True, False),
            (SHORT, NONE, False): (True, False),
            (EXT, NONE, True): (False, False),
            (NONE, EXT, False): (False, True),
            (NONE, SHORT, True): (False, False),
            (EXT, EXT, False): (True, False),
            (EXT, EXT, True): (False, False),
            (SHORT, SHORT, False): (True, True),
            (SHORT, EXT, False): (True, True),
            (EXT, SHORT, False): (True, True),
            (SHORT, EXT, True): (True, False),
            (EXT, SHORT, True): (True, False),
            (SHORT, SHORT, True): (True, False),
        }

        for (dst_mode, src_mode, panid_compression), expected in table.items():
            self.assertEqual((csl_analyzer._is_dst_panid_present(2, dst_mode, src_mode, panid_compression),
                              csl_analyzer._is_src_panid_present(2, dst_mode, src_mode, panid_compression)), expected)

    def test_security_header(self):
        # (security control, security header size, MIC size)
        for security_control, header_size, mic_size in ((0x0d, 6, 4), (0x05, 5, 4), (0x2d, 2, 4), (0x16, 10, 8),
                                                        (0x1f, 14, 16), (0x04, 5, 0)):
            self.assertEqual(csl_analyzer._get_security_header_size(security_control), header_size)
            self.assertEqual(csl_analyzer._get_mic_size(security_control), mic_size)

    def test_security_header_fields(self):
        # (Auxiliary Security Header, Frame Counter, Key Index)
        for header, frame_counter, key_index in (
            ('0d0801000002', 264, 2),  # Key ID Mode 1
            ('0508010000', 264, None),  # Key ID Mode 0
            ('2d02', None, 2),  # Frame Counter suppressed
            ('1508010000ffffffff03', 264, 3),  # Key ID Mode 2, 4-byte Key Source
            ('1d08010000111111111111111104', 264, 4),  # Key ID Mode 3, 8-byte Key Source
        ):
            self.assertEqual(csl_analyzer._parse_security_header(bytes.fromhex(header), 0), (frame_counter, key_index))

        for psdu, frame_counter, key_index in ((DATA_2015, 1, 1), (ENH_ACK_CSL_IE, 2, 1), (COMMAND_2006, 0, 1),
                                               (PARENT_REQUEST_2006, None, None), (IMM_ACK, None, None)):
            frame = parse_mac_frame(bytes.fromhex(psdu))
            self.assertEqual((frame.frame_counter, frame.key_index), (frame_counter, key_index))


class TestAnalyzeCslTx(unittest.TestCase):

    PERIOD = 2000  # 320 ms in units of 10 symbols
    PERIOD_US = PERIOD * 160
    ANCHOR_US = 1000000
    PHASE = 100  # The first CSL sample is at ANCHOR_US + 16 ms

    def _anchor(self, timestamp_us=ANCHOR_US, phase=PHASE, period=PERIOD):
        psdu = bytes.fromhex(DATA_2015_CSL_IE)
        psdu = psdu[:17] + phase.to_bytes(2, 'little') + period.to_bytes(2, 'little') + psdu[21:]
        return SniffedFrame(timestamp_us=timestamp_us, channel=CHANNEL, psdu=psdu.hex())

    def _sample_us(self, n):
        return self.ANCHOR_US + self.PHASE * 160 + n * self.PERIOD_US

    @staticmethod
    def _data(timestamp_us, seq=0x7b, frame_counter=None, key_index=None):
        psdu = bytes.fromhex(DATA_2015)
        psdu = psdu[:2] + bytes([seq]) + psdu[3:]
        if frame_counter is not None:
            psdu = psdu[:10] + frame_counter.to_bytes(4, 'little') + psdu[14:]
        if key_index is not None:
            psdu = psdu[:14] + bytes([key_index]) + psdu[15:]
        return SniffedFrame(timestamp_us=timestamp_us, channel=CHANNEL, psdu=psdu.hex())

    @staticmethod
    def _unsecured_data(timestamp_us, seq):
        # Unsecured 2015 data frame from 0x5c00 to 0x5c01 (e.g. an MLE message, which is secured by MLE).
        psdu = bytes.fromhex(f'61a8{seq:02x}6f95015c005c') + bytes(40) + bytes.fromhex('ffff')
        return SniffedFrame(timestamp_us=timestamp_us, channel=CHANNEL, psdu=psdu.hex())

    @staticmethod
    def _ack(timestamp_us, seq=0x7b):
        return SniffedFrame(timestamp_us=timestamp_us, channel=CHANNEL, psdu=f'0200{seq:02x}1234')

    def test_no_tx(self):
        report = analyze_csl_tx([self._anchor()], CHANNEL, DUT, SSED)

        self.assertEqual(report.attempts, [])
        self.assertFalse(report.is_untimed)
        self.assertEqual(report.describe(),
                         ['CSL transmitter issue (no TX): no CSL data frame to the CSL receiver on channel 20'])

    def test_in_window_and_acked(self):
        frames = [self._anchor()]
        for n in range(1, 4):
            frames += [self._data(self._sample_us(n) + 30, seq=n), self._ack(self._sample_us(n) + 1500, seq=n)]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

        self.assertEqual([attempt.offset_us for attempt in report.attempts], [30, 30, 30])
        self.assertTrue(all(attempt.acked and attempt.is_in_window for attempt in report.attempts))
        self.assertEqual(report.describe(), ['No CSL issue found: 3/3 data frames on channel 20 were acknowledged'])

    def test_tx_timing_miss(self):
        # Sent without waiting for the CSL sample time, e.g. the radio does not support `mTxDelay`.
        frames = [self._anchor(), self._data(self._sample_us(1) - 150000), self._data(self._sample_us(2) + 1000)]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

        self.assertEqual([attempt.offset_us for attempt in report.attempts], [-150000, 1000])
        self.assertFalse(any(attempt.is_in_window for attempt in report.attempts))
        self.assertEqual(len(report.describe()), 1)
        self.assertTrue(report.describe()[0].startswith('CSL transmitter issue (TX timing miss): 2/2 data frames'))

    def test_rx_missing_ack(self):
        frames = [self._anchor(), self._data(self._sample_us(3) - 100), self._data(self._sample_us(4) + 100)]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

        self.assertTrue(all(attempt.is_in_window and not attempt.acked for attempt in report.attempts))
        self.assertEqual(report.describe(), [
            'CSL receiver issue (missing Enh-ACK): 2/2 data frames in the CSL sample window were not acknowledged'
            ' (offsets -100, +100 us, tolerance 352 us, clock drift up to 104 us for CSL IEs up to 1296 ms old)'
        ])

    def test_late_ack_is_missing(self):
        data = self._data(self._sample_us(1))
        frames = [self._anchor(), data, self._ack(data.timestamp_us + (1 + len(data.psdu) // 2) * 32 + 2001)]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

        self.assertFalse(report.attempts[0].acked)

    def test_ack_to_other_device(self):
        # Enh-ACK with the same sequence number, but sent by the DUT to the SSED.
        enh_ack = bytes.fromhex(ENH_ACK_CSL_IE)
        enh_ack = enh_ack[:5] + bytes.fromhex('015c') + enh_ack[7:]
        data = self._data(self._sample_us(1))
        frames = [self._anchor(), data, SniffedFrame(data.timestamp_us + 100, CHANNEL, enh_ack.hex())]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

        self.assertFalse(report.attempts[0].acked)

    def test_clock_drift(self):
        # The window membership of a frame is only certain if it holds with the worst clock drift since the CSL IE.
        for n in (1, 10):
            sample_us = self._sample_us(n)
            drift_us = -(-(sample_us - self.ANCHOR_US) * MAX_CLOCK_DRIFT_PPM // 1000000)
            frames = [
                self._anchor(),
                self._data(sample_us - CSL_TX_TOLERANCE_US + drift_us),
                self._data(sample_us + CSL_TX_TOLERANCE_US - drift_us),
                self._data(sample_us - CSL_TX_TOLERANCE_US),
                self._data(sample_us + CSL_TX_TOLERANCE_US + drift_us),
                self._data(sample_us - CSL_TX_TOLERANCE_US - drift_us - 10),
                self._data(sample_us + CSL_TX_TOLERANCE_US + drift_us + 10),
            ]

            report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

            self.assertEqual([attempt.is_in_window for attempt in report.attempts],
                             [True, True, False, False, False, False])
            self.assertEqual([attempt.is_out_of_window for attempt in report.attempts],
                             [False, False, False, False, True, True])

    def test_old_csl_ie(self):
        # A frame sent +542 us late is a TX timing miss 1.4 s after the CSL IE, but it is inconclusive whether the
        # same frame is in the CSL sample window 9 s after the CSL IE, so it must not be blamed on the CSL receiver.
        frames = [self._anchor(), self._data(self._sample_us(4) + 542), self._data(self._sample_us(28) + 896)]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

        self.assertEqual([(attempt.is_in_window, attempt.is_out_of_window) for attempt in report.attempts],
                         [(False, True), (False, False)])
        self.assertEqual(len(report.describe()), 2)
        self.assertTrue(report.describe()[0].startswith('CSL transmitter issue (TX timing miss): 1/2 data frames'))
        self.assertEqual(
            report.describe()[1], 'CSL timing inconclusive (clock drift): 1/2 data frames were not acknowledged,'
            ' and the clock drift since the CSL IE they are timed from makes it unknown whether they were in the CSL'
            ' sample window (offsets +896 us, tolerance 352 us, clock drift up to 719 us for CSL IEs up to 8976 ms'
            ' old)')

    def test_inconclusive_with_recent_csl_ie(self):
        # A frame near the edge of the CSL sample window is inconclusive even with a recent CSL IE.
        frames = [self._anchor(), self._data(self._sample_us(4) - 400)]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

        self.assertEqual(report.attempts[0].drift_us, 104)
        self.assertTrue(report.describe()[0].startswith('CSL timing inconclusive (clock drift): 1/1 data frames'))

    def test_schedule_changed(self):
        # The CSL receiver attached again on the PAN channel, which the sniffer on the CSL channel does not capture,
        # and changed its CSL sample time. The frames sent before are timed from the CSL IE before them, but are only
        # conclusive if the CSL IE after them gives the same result.
        new_phase = self.PHASE + 1000
        new_anchor_us = self._sample_us(6) + 200000
        frames = [
            self._anchor(),
            self._data(self._sample_us(5) + 120),
            self._data(self._sample_us(6) - 150000),
            self._anchor(timestamp_us=new_anchor_us, phase=new_phase),
            self._data(new_anchor_us + new_phase * 160 + self.PERIOD_US - 50),
        ]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

        self.assertEqual([attempt.offset_us for attempt in report.attempts], [120, -150000, -50])
        self.assertEqual([attempt.schedule_changed for attempt in report.attempts], [True, False, False])
        self.assertEqual(report.attempts[0].csl_ie_age_us, self._sample_us(5) + 120 - self.ANCHOR_US)
        self.assertEqual([line.split(':')[0] for line in report.describe()], [
            'CSL transmitter issue (TX timing miss)',
            'CSL receiver issue (missing Enh-ACK)',
            'CSL timing inconclusive (CSL schedule changed)',
        ])

    def test_csl_ie_before_attaching(self):
        # The CSL IEs from before the CSL receiver started attaching again are not used. The sniffer missed the CSL IE
        # at the end of the attaching, so the frames after `start_us` are timed from no CSL IE but still analyzed.
        parent_request = SniffedFrame(timestamp_us=self._sample_us(1), channel=CHANNEL, psdu=PARENT_REQUEST_2006)
        frames = [self._anchor(), parent_request, self._data(self._sample_us(3))]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED, start_us=self._sample_us(2))

        self.assertEqual(len(report.attempts), 1)
        self.assertTrue(report.is_untimed)
        self.assertFalse(report.receiver_attached)
        self.assertEqual(report.num_excluded, 0)

    def test_first_csl_ie_after_frame(self):
        # Without a CSL IE before the frame, the frame is timed from the first CSL IE after it, unless the CSL receiver
        # attached again in between.
        parent_request = SniffedFrame(timestamp_us=self._sample_us(1), channel=CHANNEL, psdu=PARENT_REQUEST_2006)
        frames = [self._data(self.ANCHOR_US - 100000), self._anchor()]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

        self.assertEqual(report.attempts[0].offset_us, -100000 - self.PHASE * 160)

        frames = [self._data(self._sample_us(1) - 100000), parent_request, self._anchor(self._sample_us(2))]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

        self.assertFalse(report.attempts[0].is_timed)

    def test_invalid_csl_ie(self):
        # An SSED attaching again sent an Enh-ACK with a corrupted CSL IE, which must not be used to time frames nor
        # end the attaching.
        parent_request = SniffedFrame(timestamp_us=self._sample_us(1), channel=CHANNEL, psdu=PARENT_REQUEST_2006)
        frames = [
            self._anchor(),
            parent_request,
            self._anchor(timestamp_us=self._sample_us(1) + 10000, phase=8195, period=36896),
            self._data(self._sample_us(2)),
            self._anchor(timestamp_us=self._sample_us(3) - self.PHASE * 160),
            self._data(self._sample_us(4)),
        ]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED, csl_period_us=self.PERIOD_US)

        self.assertEqual(report.num_excluded, 1)
        self.assertEqual([attempt.offset_us for attempt in report.attempts], [0])

        for phase, period in ((self.PERIOD + 1, self.PERIOD), (0, 0)):
            report = analyze_csl_tx([self._anchor(phase=phase, period=period),
                                     self._data(self._sample_us(1))], CHANNEL, DUT, SSED)
            self.assertTrue(report.is_untimed)

    def test_enh_ack_anchor(self):
        enh_ack = SniffedFrame(timestamp_us=self.ANCHOR_US, channel=CHANNEL, psdu=ENH_ACK_CSL_IE)
        frames = [enh_ack, self._data(self.ANCHOR_US + self.PERIOD_US)]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

        self.assertEqual(report.attempts[0].offset_us, 0)

    def test_ignores_csl_ie_of_other_devices(self):
        other_data = bytes.fromhex(DATA_2015_CSL_IE)
        other_data = other_data[:7] + bytes.fromhex('025c') + other_data[9:]
        other_ack = bytes.fromhex(ENH_ACK_CSL_IE)
        other_ack = other_ack[:5] + bytes.fromhex('025c') + other_ack[7:]
        frames = [
            SniffedFrame(self.ANCHOR_US, CHANNEL, other_data.hex()),
            SniffedFrame(self.ANCHOR_US + 100, CHANNEL, other_ack.hex()),
            self._data(self._sample_us(1)),
        ]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

        self.assertEqual(len(report.attempts), 1)
        self.assertTrue(report.is_untimed)

    def test_data_poll_response(self):
        data_poll = SniffedFrame(timestamp_us=self._sample_us(1) - 50000, channel=CHANNEL, psdu=COMMAND_2006)
        frames = [self._anchor(), data_poll, self._data(data_poll.timestamp_us + 1000), self._data(self._sample_us(2))]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

        self.assertEqual([attempt.timestamp_us for attempt in report.attempts], [self._sample_us(2)])
        self.assertEqual(report.num_excluded, 1)

    def test_receiver_attaching(self):
        # The SSED sends a broadcast frame (e.g. an MLE Parent Request) to attach again, and the DUT sends frames to it
        # without CSL until it sends a CSL IE again.
        parent_request = SniffedFrame(timestamp_us=self._sample_us(1), channel=CHANNEL, psdu=PARENT_REQUEST_2006)
        frames = [
            self._anchor(),
            parent_request,
            self._data(self._sample_us(1) + 20000),
            self._anchor(timestamp_us=self._sample_us(2)),
            self._data(self._sample_us(3)),
        ]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

        self.assertEqual([attempt.timestamp_us for attempt in report.attempts], [self._sample_us(3)])
        self.assertTrue(report.receiver_attached)
        self.assertEqual(report.describe()[-2:], [
            'The CSL receiver attached again on channel 20',
            '1 data frames sent while the CSL receiver polled data or attached are not analyzed',
        ])

    def test_untimed(self):
        # The CSL channel is not the PAN channel, and the SSED sent no Enh-ACK on the CSL channel.
        report = analyze_csl_tx([self._data(1000), self._data(2000)], CHANNEL, DUT, SSED)

        self.assertTrue(report.is_untimed)
        self.assertEqual(report.describe(), [
            '2/2 data frames on channel 20 were not acknowledged, and no CSL IE from the CSL receiver was captured on'
            ' channel 20 to time them'
        ])

    def test_start_time(self):
        frames = [self._anchor(), self._data(self._sample_us(1)), self._data(self._sample_us(2))]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED, start_us=self._sample_us(2))

        self.assertEqual(len(report.attempts), 1)
        self.assertEqual(report.attempts[0].timestamp_us, self._sample_us(2))

    def _retx(self, frame_counters, key_indexes=None, seq=206, first_sample=1):
        # The CSL transmitter retransmits an unacknowledged frame in the next CSL sample windows.
        key_indexes = key_indexes or [1] * len(frame_counters)
        return [
            self._data(self._sample_us(first_sample + n), seq, frame_counter, key_index)
            for n, (frame_counter, key_index) in enumerate(zip(frame_counters, key_indexes))
        ]

    def test_retx_security_reused(self):
        frames = [self._anchor(), *self._retx([8, 8, 8, 8]), self._data(self._sample_us(5), 207, 9)]
        frames.append(self._ack(self._sample_us(5) + 1500, 207))

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

        self.assertEqual(len(report.retransmitted_frames), 1)
        self.assertEqual(report.retransmitted_frames[0].seq, 206)
        self.assertEqual(report.retransmitted_frames[0].num_retx, 3)
        self.assertEqual(report.retransmitted_frames[0].num_changed, 0)
        self.assertEqual(report.describe(), [
            'CSL receiver issue (missing Enh-ACK): 4/5 data frames in the CSL sample window were not acknowledged'
            ' (offsets +0, +0, +0, +0 us, tolerance 352 us, clock drift up to 104 us for CSL IEs up to 1296 ms old)'
        ])

    def test_retx_frame_counter_changed(self):
        # The radio secured each retransmission again with a new frame counter. It is reported in addition to the
        # timing of the frames, which it does not change. The broadcast frame from the CSL transmitter between the
        # retransmissions has a new sequence number, and does not end the retransmissions.
        broadcast = bytes.fromhex(PARENT_REQUEST_2006)
        broadcast = broadcast[:2] + bytes([207]) + broadcast[3:7] + bytes.fromhex('61a12dddbb7ebca2') + broadcast[15:]
        frames = [self._anchor(), *self._retx([8, 9, 10, 11])]
        frames.insert(3, SniffedFrame(self._sample_us(2) + 100000, CHANNEL, broadcast.hex()))

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

        self.assertEqual(report.retransmitted_frames[0].frame_counters, [8, 9, 10, 11])
        self.assertEqual(report.retransmitted_frames[0].num_changed, 3)
        self.assertFalse(report.receiver_attached)
        self.assertEqual(report.describe(), [
            'CSL receiver issue (missing Enh-ACK): 4/4 data frames in the CSL sample window were not acknowledged'
            ' (offsets +0, +0, +0, +0 us, tolerance 352 us, clock drift up to 104 us for CSL IEs up to 1296 ms old)',
            'CSL transmitter issue (retransmission with new frame counter): 3/3 retransmitted data frames did not reuse the'
            ' frame counter and key index of the first transmission (seq 206: expected frame counter 8, observed 8,'
            ' 9, 10, 11)',
        ])

    def test_retx_key_index_changed(self):
        frames = [self._anchor(), *self._retx([8, 8, 9], [1, 1, 2]), *self._retx([20, 21], seq=207, first_sample=4)]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

        self.assertEqual([frame.num_changed for frame in report.retransmitted_frames], [1, 1])
        self.assertTrue(report.describe()[-1].endswith(
            ': 2/3 retransmitted data frames did not reuse the frame counter and key index of the first transmission'
            ' (seq 206: expected frame counter 8, observed 8, 8, 9 and expected key index 1, observed 1, 1, 2; seq 207:'
            ' expected frame counter 20, observed 20, 21)'))

    def test_retx_frame_counter_changed_only_issue(self):
        # The sniffer captured the Enh-ACKs, which the CSL transmitter missed.
        frames = [self._anchor()]
        for frame in self._retx([8, 9]):
            frames += [frame, self._ack(frame.timestamp_us + 1500, 206)]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

        self.assertEqual(len(report.describe()), 1)
        self.assertTrue(
            report.describe()[0].startswith('CSL transmitter issue (retransmission with new frame counter): 1/1'))

    def test_retx_not_detected(self):
        # The same sequence number is not a retransmission after another data frame to the CSL receiver (e.g. the
        # sequence number wrapped around), with another PSDU length (another frame after a wrap-around whose other
        # frames were not captured), or after the CSL receiver attached again.
        other_length = self._data(self._sample_us(3), 206, 300)
        other_length.psdu = other_length.psdu[:-4] + '0000' + other_length.psdu[-4:]
        parent_request = SniffedFrame(timestamp_us=self._sample_us(4) + 50000,
                                      channel=CHANNEL,
                                      psdu=PARENT_REQUEST_2006)
        frames = [
            self._anchor(),
            self._data(self._sample_us(1), 206, 8),
            self._data(self._sample_us(2), 207, 9),
            self._data(self._sample_us(2) + 50000, 206, 10),
            other_length,
            self._data(self._sample_us(4), 207, 11),
            parent_request,
            self._anchor(timestamp_us=self._sample_us(5)),
            self._data(self._sample_us(6), 207, 400),
        ]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

        self.assertEqual(report.retransmitted_frames, [])
        self.assertFalse(any('retransmi' in line for line in report.describe()))

    def test_retx_unsecured_and_other_devices_ignored(self):
        # Retransmissions of unsecured frames (e.g. MLE messages) have no frame counter, and secured frames with a
        # repeated sequence number from or to other devices are not checked.
        other_dst = bytes.fromhex(self._data(0, 206, 9).psdu)
        other_dst = other_dst[:5] + bytes.fromhex('025c') + other_dst[7:]
        other_src = bytes.fromhex(self._data(0, 206, 10).psdu)
        other_src = other_src[:7] + bytes.fromhex('025c') + other_src[9:]
        frames = [
            self._anchor(),
            *[self._unsecured_data(self._sample_us(n), 202) for n in range(1, 4)],
            self._data(self._sample_us(4), 206, 8),
            SniffedFrame(self._sample_us(4) + 50000, CHANNEL, other_dst.hex()),
            SniffedFrame(self._sample_us(4) + 60000, CHANNEL, other_src.hex()),
        ]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

        self.assertEqual(len(report.attempts), 4)
        self.assertEqual(report.retransmitted_frames, [])

        # An unsecured frame to the CSL receiver ends the retransmissions of a secured frame.
        frames = [self._anchor(), self._data(self._sample_us(1), 206, 8)]
        frames += [self._unsecured_data(self._sample_us(2), 206), self._data(self._sample_us(3), 206, 9)]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

        self.assertEqual(report.retransmitted_frames, [])

    def test_retx_seq_suppressed_ignored(self):
        # Secured data frames without a sequence number (Sequence Number Suppression) cannot be matched to a first
        # transmission.
        frames = [self._anchor()]
        for n, frame in enumerate(self._retx([8, 9]), 1):
            psdu = bytes.fromhex(frame.psdu)
            psdu = bytes([psdu[0], psdu[1] | 0x01]) + psdu[3:]
            frames.append(SniffedFrame(self._sample_us(n), CHANNEL, psdu.hex()))

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

        self.assertEqual([parse_mac_frame(bytes.fromhex(frame.psdu)).seq for frame in frames[1:]], [None, None])
        self.assertEqual(len(report.attempts), 2)
        self.assertEqual(report.retransmitted_frames, [])

    def test_retx_interleaved_frame_to_extended_address(self):
        # While the CSL receiver attaches again, the CSL transmitter sends an MLE message to its extended address
        # between the retransmissions to its short address.
        mle = bytes.fromhex('61ecd2' + '01fa636c5f34df3e' + '61a12dddbb7ebca2') + bytes(40) + bytes.fromhex('ffff')
        frames = [self._anchor(), *self._retx([9, 9, 9, 9])]
        frames.insert(2, SniffedFrame(self._sample_us(1) + 100000, CHANNEL, mle.hex()))

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

        self.assertEqual(len(report.attempts), 5)
        self.assertEqual(report.retransmitted_frames[0].frame_counters, [9, 9, 9, 9])
        self.assertEqual(report.retransmitted_frames[0].num_changed, 0)

    def test_retx_start_time(self):
        frames = [self._anchor(), *self._retx([8, 9, 9])]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED, start_us=self._sample_us(2))

        self.assertEqual(report.retransmitted_frames[0].frame_counters, [9, 9])
        self.assertEqual(report.retransmitted_frames[0].num_changed, 0)

    def _timed_data(self, n, offset_us, drift_ppm=0, anchor_us=ANCHOR_US, phase=PHASE, seq=0x7b):
        # A data frame in the n-th CSL sample window after a CSL IE, with a bias and a clock drift since the CSL IE.
        sample_us = anchor_us + phase * 160 + n * self.PERIOD_US
        return self._data(sample_us + round(offset_us + drift_ppm * (sample_us - anchor_us) / 1000000), seq)

    def test_timing_perfect(self):
        frames = [self._anchor()]
        for n in range(1, 5):
            frames += [self._timed_data(n, 0, seq=n), self._ack(self._sample_us(n) + 1500, seq=n)]

        timing = analyze_csl_tx(frames, CHANNEL, DUT, SSED).timing

        self.assertEqual(timing.offsets_us, [0, 0, 0, 0])
        self.assertEqual((timing.bias_us, timing.jitter_us, timing.max_offset_us), (0, 0, 0))
        self.assertEqual((timing.drift_ppm, timing.drift_se_ppm), (0, 0))
        self.assertEqual(timing.describe(20, 100), [
            'CSL TX timing: 4 data frames, offsets min +0 / median +0 / max +0 us, bias +0 us, jitter ±0 us, drift vs.'
            ' sniffer +0 ± 0 ppm'
        ])

    def test_timing_late_but_acked(self):
        # The frames are acknowledged, so there is no CSL issue, but they are later than the CSL uncertainty of the
        # CSL transmitter, e.g. the radio does not compensate the SHR duration.
        frames = [self._anchor()]
        for n in range(1, 6):
            frames += [self._timed_data(n, 220, seq=n), self._ack(self._sample_us(n) + 1500, seq=n)]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)
        timing = report.timing

        self.assertEqual(report.describe(), ['No CSL issue found: 5/5 data frames on channel 20 were acknowledged'])
        self.assertEqual((timing.bias_us, timing.drift_ppm), (220, 0))
        self.assertEqual(
            timing.describe(20, 100)[1:], [
                'CSL TX timing warning: offsets up to 220 us after removing the drift exceed the CSL uncertainty of the'
                ' CSL transmitter (100 us)'
            ])
        self.assertEqual(len(timing.describe(20, 250)), 1)
        self.assertEqual(len(timing.describe()), 1)

    def test_timing_bias_and_drift(self):
        # The CSL transmitter is late by a fixed 520 us, and its clock drifts by 41 ppm relative to the sniffer, which
        # shows in the retransmissions timed from the same CSL IE.
        frames = [self._anchor(), *[self._timed_data(n, 520, 41) for n in range(1, 5)]]
        anchor_us = self._sample_us(6) - self.PHASE * 160
        frames += [self._anchor(anchor_us), *[self._timed_data(n, 520, 41, anchor_us, seq=0x7c) for n in range(1, 5)]]

        timing = analyze_csl_tx(frames, CHANNEL, DUT, SSED).timing

        self.assertEqual(timing.offsets_us, [534, 547, 560, 573] * 2)
        self.assertAlmostEqual(timing.drift_ppm, 41, delta=0.5)
        self.assertLess(timing.drift_se_ppm, 1)
        self.assertAlmostEqual(timing.bias_us, 520, delta=1)
        self.assertLess(timing.jitter_us, 1)
        self.assertTrue(
            timing.describe(20, 200)[0].endswith(', bias +520 us, jitter ±0 us, drift vs. sniffer +41 ± 0 ppm'))
        self.assertEqual(timing.describe(0, 1000), [
            timing.describe()[0],
            'CSL TX timing warning: drift vs. sniffer +41 ± 0 ppm exceeds the CSL accuracy of the CSL transmitter'
            ' (0 ppm) plus the sniffer accuracy (40 ppm)',
        ])

    def test_timing_short_span(self):
        # The retransmissions timed from the same CSL IE span less than MIN_DRIFT_SPAN_US, so the noise of the offsets
        # must not be shown as clock drift.
        frames = []
        for i, offsets_us in enumerate(((100, 160), (150, 90), (120, 180))):
            anchor_us = self.ANCHOR_US + i * 7 * self.PERIOD_US
            frames.append(self._anchor(anchor_us))
            frames += [
                self._timed_data(n, offset_us, 0, anchor_us, seq=i) for n, offset_us in enumerate(offsets_us, 1)
            ]

        timing = analyze_csl_tx(frames, CHANNEL, DUT, SSED).timing

        self.assertIsNone(timing.drift_ppm)
        self.assertEqual((timing.bias_us, timing.jitter_us, timing.max_offset_us), (135, 45, 180))
        self.assertTrue(timing.describe()[0].endswith(', drift vs. sniffer unknown (insufficient span)'))

    def test_timing_each_frame_timed_from_new_csl_ie(self):
        # Each frame is timed from a new CSL IE (e.g. in the Enh-ACK of the previous frame), so no frames are timed from
        # the same CSL IE and the clock drift is unknown. The offsets are within the CSL uncertainty of the CSL
        # transmitter.
        frames = []
        for i, offset_us in enumerate((158, 136, 170, 176, 149, 168, 136, 180, 170, 187)):
            anchor_us = self.ANCHOR_US + i * 4 * self.PERIOD_US
            frames += [self._anchor(anchor_us), self._timed_data(1 + i % 3, offset_us, 0, anchor_us, seq=i)]

        timing = analyze_csl_tx(frames, CHANNEL, DUT, SSED).timing

        self.assertIsNone(timing.drift_ppm)
        self.assertEqual((timing.bias_us, timing.jitter_us, timing.max_offset_us), (169, 25.5, 187))
        self.assertEqual(timing.describe(20, 200), [
            'CSL TX timing: 10 data frames, offsets min +136 / median +169 / max +187 us, bias +169 us, jitter ±26 us,'
            ' drift vs. sniffer unknown (insufficient span)'
        ])

    def test_timing_attached_again(self):
        # The CSL transmitter timestamped the CSL IEs before and after the CSL receiver attached again differently, which
        # changes the offsets of the frames timed from each of them but not the clock drift.
        parent_request = SniffedFrame(timestamp_us=self._sample_us(5), channel=CHANNEL, psdu=PARENT_REQUEST_2006)
        anchor_us = self._sample_us(6)
        frames = [self._anchor(), *[self._timed_data(n, 100, 30) for n in range(1, 5)], parent_request]
        frames += [self._data(self._sample_us(5) + 20000, 0x7c)]
        frames += [self._anchor(anchor_us), *[self._timed_data(n, 300, 30, anchor_us, seq=0x7d) for n in range(1, 5)]]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)
        timing = report.timing

        self.assertEqual(report.num_excluded, 1)
        self.assertEqual(len(timing.offsets_us), 8)
        self.assertAlmostEqual(timing.drift_ppm, 30, delta=0.5)
        self.assertAlmostEqual(timing.jitter_us, 100, delta=1)
        self.assertAlmostEqual(timing.max_offset_us, 300, delta=1)

    def test_timing_excluded_frames(self):
        self.assertIsNone(analyze_csl_tx([self._data(1000)], CHANNEL, DUT, SSED).timing)

        # The frames between CSL IEs with different CSL sample times are not used, even if both CSL IEs put them out of
        # the CSL sample window.
        new_anchor_us = self._sample_us(6) + 200000
        frames = [
            self._anchor(),
            self._data(self._sample_us(5) + 120),
            self._data(self._sample_us(6) - 150000),
            self._anchor(timestamp_us=new_anchor_us, phase=self.PHASE + 1000),
            self._timed_data(1, -50, 0, new_anchor_us, self.PHASE + 1000),
        ]

        report = analyze_csl_tx(frames, CHANNEL, DUT, SSED)

        self.assertEqual([attempt.schedule_changed for attempt in report.attempts], [True, False, False])
        self.assertEqual([attempt.around_schedule_change for attempt in report.attempts], [True, True, False])
        self.assertEqual(report.timing.offsets_us, [-50])

    def test_timing_frame_before_first_csl_ie(self):
        # A frame before the first captured CSL IE was timed by the CSL transmitter from an uncaptured CSL IE, so its
        # offset from the CSL IE after it is not taken as drift.
        frames = [self._timed_data(-2, 300), self._anchor(), *[self._timed_data(n, 520, 41) for n in range(1, 5)]]

        timing = analyze_csl_tx(frames, CHANNEL, DUT, SSED).timing

        self.assertEqual(timing.offsets_us, [300, 534, 547, 560, 573])
        self.assertEqual(timing.ages_us[0], -2 * self.PERIOD_US + self.PHASE * 160 + 300)
        self.assertAlmostEqual(timing.drift_ppm, 41, delta=0.5)
        self.assertLess(timing.drift_se_ppm, 1)

    def test_timing_warnings_without_drift(self):
        # Without a drift estimate, the worst clock drift of the CSL transmitter and the sniffer since the CSL IE is
        # allowed on top of the CSL uncertainty: (20 + 40) ppm * 976 ms = 58 us.
        frames = [self._anchor(), self._timed_data(3, 250)]

        timing = analyze_csl_tx(frames, CHANNEL, DUT, SSED).timing

        self.assertIsNone(timing.drift_ppm)
        self.assertEqual(len(timing.describe(20, 200)), 1)
        self.assertEqual(
            timing.describe(20, 180)[1:], [
                'CSL TX timing warning: offsets up to 250 us exceed the CSL uncertainty of the CSL transmitter (180 us)'
                ' plus a clock drift of 60 ppm since the CSL IEs'
            ])

    def test_timing_drift_warning_significance(self):
        # The drift only warns if it exceeds the CSL accuracy plus the sniffer accuracy by more than its standard
        # error.
        frames = [self._anchor()]
        for n, offset_us in enumerate((0, 60, 0, 60, 60, 120), 1):
            frames.append(self._timed_data(n, offset_us, seq=n))

        timing = analyze_csl_tx(frames, CHANNEL, DUT, SSED).timing

        self.assertGreater(abs(timing.drift_ppm), 40)
        self.assertLess(abs(timing.drift_ppm) - timing.drift_se_ppm, 40)
        self.assertEqual(len(timing.describe(0, 1000)), 1)


if __name__ == '__main__':
    unittest.main()
