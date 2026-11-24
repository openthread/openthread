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
"""Unit tests of the delayed transmission analyzer, which do not need any device."""

import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))

from tx_delay_analyzer import MAX_INTERVAL_ERROR_US, measure_cca_offsets, measure_lateness

PERIOD_US = 200000
FRAMES = 21
START_US = 1700000000000000  # The sniffer timestamps are relative to the Unix epoch.
CCA_US = 288  # The timing error of a radio which does not compensate the CCA time


def make_timestamps(odd_offset_us: int = 0, even_offset_us: int = 0, drift_ppm: float = 0):
    """Returns the sniffer timestamps of frames sent by a radio with the given errors."""
    timestamps = {}

    for i in range(FRAMES):
        offset_us = odd_offset_us if i % 2 else even_offset_us
        timestamps[i] = round(START_US + (i * PERIOD_US + offset_us) * (1 + drift_ppm / 1e6))

    return timestamps


class TestMeasureCcaOffsets(unittest.TestCase):

    def assert_offsets(self, offsets, expected_us, count=FRAMES // 2):
        self.assertEqual(len(offsets), count)
        for offset in offsets:
            self.assertAlmostEqual(offset, expected_us, delta=1)

    def test_compensated_cca(self):
        self.assert_offsets(measure_cca_offsets(make_timestamps(), PERIOD_US), 0)

    def test_clock_drift_cancelled(self):
        # A drift of 80 ppm shifts the last frame by 320 us, which the offset of each frame cancels.
        self.assert_offsets(measure_cca_offsets(make_timestamps(drift_ppm=80), PERIOD_US), 0)
        self.assert_offsets(measure_cca_offsets(make_timestamps(drift_ppm=-80), PERIOD_US), 0)

    def test_uncompensated_cca(self):
        # The radio starts CCA at the scheduled time, so the frames with CCA are late.
        self.assert_offsets(measure_cca_offsets(make_timestamps(odd_offset_us=CCA_US, drift_ppm=40), PERIOD_US),
                            CCA_US)

    def test_constant_cca_compensation(self):
        # The radio starts every frame early by the CCA time, so the frames without CCA are early.
        self.assert_offsets(measure_cca_offsets(make_timestamps(even_offset_us=-CCA_US), PERIOD_US), CCA_US)

    def test_missing_frames(self):
        timestamps = make_timestamps()
        del timestamps[0]  # The first frame with CCA has no previous frame.
        del timestamps[9]  # A frame with CCA.
        del timestamps[20]  # The last frame with CCA has no next frame.
        self.assert_offsets(measure_cca_offsets(timestamps, PERIOD_US), 0, count=FRAMES // 2 - 3)

    def test_late_frame(self):
        timestamps = make_timestamps()
        timestamps[10] += MAX_INTERVAL_ERROR_US + 1  # The frames with CCA before and after it are not analyzed.
        timestamps[13] += 5000  # A frame with CCA sent late is analyzed.
        offsets = measure_cca_offsets(timestamps, PERIOD_US)
        self.assertEqual(len(offsets), FRAMES // 2 - 2)
        self.assertEqual(offsets.count(5000), 1)
        self.assertEqual(offsets.count(0), FRAMES // 2 - 3)

    def test_no_frames(self):
        self.assertEqual(measure_cca_offsets({}, PERIOD_US), [])
        self.assertEqual(measure_cca_offsets({1: START_US}, PERIOD_US), [])


class TestMeasureLateness(unittest.TestCase):

    def test_on_time(self):
        targets = {i: i * 1000 for i in range(5)}
        timestamps = {i: START_US + target for i, target in targets.items()}
        self.assertEqual(measure_lateness(timestamps, targets), {i: 0 for i in range(5)})

    def test_late_frames(self):
        targets = {0: -2000, 1: -1000, 2: 0, 3: 1000, 4: 2000}
        # Frames 0 and 1 are past due when the radio gets them at 500 us, so it sends them right away.
        timestamps = {0: START_US + 500, 1: START_US + 500, 2: START_US, 3: START_US + 1000, 4: START_US + 2000}
        self.assertEqual(measure_lateness(timestamps, targets), {0: 2500, 1: 1500, 2: 0, 3: 0, 4: 0})

    def test_missing_frames(self):
        targets = {0: -1000, 1: 0, 2: 1000}
        # Frame 0 is aborted, and the frame with index 9 is not sent by the DUT.
        timestamps = {1: START_US, 2: START_US + 1000, 9: START_US + 5000}
        self.assertEqual(measure_lateness(timestamps, targets), {1: 0, 2: 0})
        self.assertEqual(measure_lateness({}, targets), {})


if __name__ == '__main__':
    unittest.main()
