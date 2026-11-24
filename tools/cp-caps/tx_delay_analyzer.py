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
"""Analyzes the timing of delayed transmissions captured by the sniffer."""

from typing import Dict, List

# The maximum error (in microseconds) of the interval between two frames scheduled two periods apart. It covers the
# clock drift between the DUT and the sniffer (e.g. ±40 ppm each over a few hundred milliseconds) and the timestamp
# jitter. The frame between two frames which are not as far apart as scheduled (e.g. one was sent late, after its
# scheduled time) is not analyzed.
MAX_INTERVAL_ERROR_US = 100


def measure_cca_offsets(timestamps_us: Dict[int, int], period_us: int) -> List[float]:
    """Measures how much later the delayed frames with CCA are sent than the delayed frames without CCA.

    Frame `i` is scheduled `i * period_us` after a common base time, with CCA disabled if `i` is even and enabled if
    `i` is odd. As `mTxDelayBaseTime + mTxDelay` is the time of the end of the SFD, a radio performing CCA must start it
    earlier, so that frames with and without CCA are sent at their scheduled times.

    The offset of odd frame `i` is `T(i) - (T(i - 1) + T(i + 1)) / 2`, where `T` is the receive timestamp of the
    sniffer, which cancels a constant clock drift between the DUT and the sniffer.

    :param timestamps_us: The receive timestamps (in microseconds) of the captured frames, by frame index.
    :param period_us: The scheduled interval (in microseconds) between two consecutive frames.

    :returns: The offset (in microseconds) of each odd frame captured with both of its neighbors, which are two periods
        apart within MAX_INTERVAL_ERROR_US.
    """
    offsets = []

    for i, timestamp in sorted(timestamps_us.items()):
        prev_timestamp = timestamps_us.get(i - 1)
        next_timestamp = timestamps_us.get(i + 1)

        if i % 2 == 0 or prev_timestamp is None or next_timestamp is None:
            continue

        if abs(next_timestamp - prev_timestamp - 2 * period_us) > MAX_INTERVAL_ERROR_US:
            continue

        offsets.append(timestamp - (prev_timestamp + next_timestamp) / 2)

    return offsets
