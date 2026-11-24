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
"""Analyzes sniffed IEEE 802.15.4 frames to tell CSL transmitter issues from CSL receiver issues."""

import math
import statistics
import struct

from dataclasses import dataclass, field
from typing import Collection, Dict, List, Optional, Tuple

from sniffer import SniffedFrame

FRAME_TYPE_DATA = 1
FRAME_TYPE_ACK = 2
FRAME_TYPE_COMMAND = 3
FRAME_VERSION_2015 = 2

ADDR_MODE_NONE = 0
ADDR_MODE_SHORT = 2
ADDR_MODE_EXT = 3
BROADCAST_ADDR = 'ffff'

HEADER_IE_ID_CSL = 0x1a
HEADER_IE_ID_TERMINATION_1 = 0x7e
HEADER_IE_ID_TERMINATION_2 = 0x7f

FCS_SIZE = 2
TEN_SYMBOLS_US = 160  # The unit of the CSL Phase and CSL Period
PHR_PSDU_BYTE_US = 32  # The duration of a PHR or PSDU byte

# The receiver must send the ACK right after the end of the frame (AIFS is 192 us). Allows some slack for the
# maximum Enh-ACK wait duration and the timestamping of the sniffer.
ACK_MAX_DELAY_US = 2000

# A CSL receiver stays in receive mode for up to this time after a data poll (OPENTHREAD_CONFIG_MAC_DATA_POLL_TIMEOUT),
# so the data frames sent in this time are not CSL transmissions.
DATA_POLL_TIMEOUT_US = 100000

# The CSL transmitter must start the MAC header at the CSL Phase time, which is the time the CSL receiver must be
# ready to receive at (Thread 1.4, section 3.2.6.3.4). The CSL Phase is truncated to units of 10 symbols, so the
# sample times derived from different frames differ by up to 160 us. Also allows the SHR + PHR duration (192 us) for
# the timestamping and TX scheduling accuracy of the radios.
CSL_TX_TOLERANCE_US = TEN_SYMBOLS_US + 192

# The CSL sample times are derived on the sniffer clock from a frame of the CSL receiver, so they drift by up to 40 ppm
# (IEEE 802.15.4 O-QPSK PHY) of each of the sniffer and the CSL receiver since that frame.
MAX_CLOCK_DRIFT_PPM = 80

# The clock accuracy of the sniffer (IEEE 802.15.4 O-QPSK PHY).
SNIFFER_ACCURACY_PPM = 40

# The clock drift of the CSL transmitter relative to the sniffer is only estimated if the data frames timed from the
# same CSL IE (e.g. the retransmissions of an unacknowledged frame in the next CSL sample windows) span this time.
MIN_DRIFT_SPAN_US = 600000


@dataclass
class CslIe:
    """Represents a CSL IE (IEEE 802.15.4-2015, section 7.4.2.3), in units of 10 symbols."""
    phase: int
    period: int


@dataclass
class MacFrame:
    """Represents the MAC header fields of a frame used by the CSL analysis.

    Addresses are hex strings as shown by the OpenThread CLI: 16 digits for extended addresses (e.g. `extaddr`),
    and 4 digits for short addresses (e.g. `rloc16`). The Frame Counter and the Key Index are from the Auxiliary
    Security Header, and are None if the frame is not secured or the field is not present.
    """
    frame_type: int
    ack_request: bool
    seq: Optional[int]
    dst_addr: Optional[str]
    src_addr: Optional[str]
    csl_ie: Optional[CslIe]
    frame_counter: Optional[int] = None
    key_index: Optional[int] = None


def parse_mac_frame(psdu: bytes) -> Optional[MacFrame]:
    """Parses the MAC header of a PSDU (including the FCS), or returns None if it is malformed.

    Header IEs are not encrypted, so the CSL IE is also parsed from secured frames.
    """
    end = len(psdu) - FCS_SIZE

    if end < 2:
        return None

    fcf = struct.unpack_from('<H', psdu, 0)[0]
    frame_type = fcf & 0x7
    security_enabled = bool(fcf & (1 << 3))
    ack_request = bool(fcf & (1 << 5))
    panid_compression = bool(fcf & (1 << 6))
    seq_suppressed = bool(fcf & (1 << 8))
    ie_present = bool(fcf & (1 << 9))
    dst_mode = (fcf >> 10) & 0x3
    version = (fcf >> 12) & 0x3
    src_mode = (fcf >> 14) & 0x3
    offset = 2

    seq = None
    if not seq_suppressed:
        if offset + 1 > end:
            return None
        seq = psdu[offset]
        offset += 1

    if offset + _get_addr_fields_size(version, dst_mode, src_mode, panid_compression) > end:
        return None

    if _is_dst_panid_present(version, dst_mode, src_mode, panid_compression):
        offset += 2
    dst_addr, offset = _parse_addr(psdu, offset, dst_mode)

    if _is_src_panid_present(version, dst_mode, src_mode, panid_compression):
        offset += 2
    src_addr, offset = _parse_addr(psdu, offset, src_mode)

    frame_counter = None
    key_index = None
    if security_enabled:
        if offset + 1 > end:
            return None
        security_control = psdu[offset]
        header_size = _get_security_header_size(security_control)
        end -= _get_mic_size(security_control)
        if offset + header_size > end:
            return None
        frame_counter, key_index = _parse_security_header(psdu, offset)
        offset += header_size

    csl_ie = None
    if ie_present and version == FRAME_VERSION_2015:
        while offset + 2 <= end:
            descriptor = struct.unpack_from('<H', psdu, offset)[0]
            if descriptor & (1 << 15):
                break
            ie_length = descriptor & 0x7f
            ie_id = (descriptor >> 7) & 0xff
            offset += 2

            if offset + ie_length > end:
                return None

            if ie_id == HEADER_IE_ID_CSL and ie_length >= 4:
                phase, period = struct.unpack_from('<HH', psdu, offset)
                csl_ie = CslIe(phase=phase, period=period)
            elif ie_id in (HEADER_IE_ID_TERMINATION_1, HEADER_IE_ID_TERMINATION_2):
                break

            offset += ie_length

    return MacFrame(frame_type=frame_type,
                    ack_request=ack_request,
                    seq=seq,
                    dst_addr=dst_addr,
                    src_addr=src_addr,
                    csl_ie=csl_ie,
                    frame_counter=frame_counter,
                    key_index=key_index)


@dataclass
class CslTxAttempt:
    """Represents a data frame sent by the CSL transmitter to the CSL receiver.

    Attributes:
      timestamp_us: The sniffer timestamp of the frame (end of SFD).
      acked: Whether the CSL receiver acknowledged the frame.
      offset_us: The offset of the frame from the CSL sample time advertised by the CSL receiver (positive if late),
          or None if the sniffer captured no CSL IE from the CSL receiver to time the frame.
      drift_us: The uncertainty of `offset_us` for the clock drift since the CSL IE that the frame is timed from.
      csl_ie_age_us: The time between the frame and the CSL IE that the frame is timed from.
      csl_ie_timestamp_us: The sniffer timestamp of the CSL IE that the frame is timed from.
      schedule_changed: Whether the CSL receiver changed its CSL sample time between the CSL IEs before and after the
          frame, and the frame is in the CSL sample window of only one of them.
      around_schedule_change: Whether the CSL receiver changed its CSL sample time between the CSL IEs before and after
          the frame, so the CSL sample time of the frame is uncertain even if both put it in or out of the window.
    """
    timestamp_us: int
    acked: bool
    offset_us: Optional[int] = None
    drift_us: int = 0
    csl_ie_age_us: int = 0
    csl_ie_timestamp_us: int = 0
    schedule_changed: bool = False
    around_schedule_change: bool = False

    @property
    def is_timed(self) -> bool:
        return self.offset_us is not None

    @property
    def is_in_window(self) -> bool:
        """Whether the frame was sent in the CSL sample window, even with the worst clock drift."""
        return self.is_timed and not self.schedule_changed and _is_in_window(self.offset_us, self.drift_us)

    @property
    def is_out_of_window(self) -> bool:
        """Whether the frame was sent out of the CSL sample window, even with the worst clock drift."""
        return self.is_timed and not self.schedule_changed and _is_out_of_window(self.offset_us, self.drift_us)


@dataclass
class RetransmittedFrame:
    """Represents a secured data frame sent more than once by the CSL transmitter to the CSL receiver.

    A retransmission of a secured frame must use the same Frame Counter and Key Index as the first transmission
    (`mIsHeaderUpdated` of the retransmitted frame is set), instead of assigning a new Frame Counter or Key Index.

    Attributes:
      seq: The sequence number of the frame.
      frame_counters: The Frame Counters of the captured transmissions, in order.
      key_indexes: The Key Indexes of the captured transmissions, in order.
    """
    seq: int
    frame_counters: List[int]
    key_indexes: List[Optional[int]]

    @property
    def num_retx(self) -> int:
        return len(self.frame_counters) - 1

    @property
    def num_changed(self) -> int:
        """The number of retransmissions whose Frame Counter or Key Index differ from the first transmission."""
        first = (self.frame_counters[0], self.key_indexes[0])

        return sum(1 for retx in zip(self.frame_counters[1:], self.key_indexes[1:]) if retx != first)

    def describe(self) -> str:
        parts = []

        if any(frame_counter != self.frame_counters[0] for frame_counter in self.frame_counters):
            parts.append(f'expected frame counter {self.frame_counters[0]}, observed'
                         f' {", ".join(str(frame_counter) for frame_counter in self.frame_counters)}')

        if any(key_index != self.key_indexes[0] for key_index in self.key_indexes):
            parts.append(f'expected key index {self.key_indexes[0]}, observed'
                         f' {", ".join(str(key_index) for key_index in self.key_indexes)}')

        return f'seq {self.seq}: {" and ".join(parts)}'


@dataclass
class CslTxTiming:
    """Represents the timing of the data frames sent by the CSL transmitter, whether acknowledged or not.

    The offset of a frame is modeled as `bias + drift * CSL IE age + jitter`. The drift is only estimated from the
    frames timed from the same CSL IE, so that how the CSL transmitter timestamped each CSL IE is not taken as drift.

    Attributes:
      offsets_us: The offsets of the frames from the CSL sample time.
      ages_us: The times of the frames since the CSL IEs that they are timed from (negative if timed from a later one).
      bias_us: The median offset after removing the drift.
      jitter_us: The half range of the offsets after removing the drift.
      max_offset_us: The largest absolute offset after removing the drift.
      drift_ppm: The clock drift of the CSL transmitter relative to the sniffer (positive if the offsets grow with the
          CSL IE age), or None if the frames timed from the same CSL IE span too little time to estimate it.
      drift_se_ppm: The standard error of `drift_ppm`.
    """
    offsets_us: List[int]
    ages_us: List[int]
    bias_us: float
    jitter_us: float
    max_offset_us: float
    drift_ppm: Optional[float] = None
    drift_se_ppm: Optional[float] = None

    def describe(self, accuracy_ppm: Optional[int] = None, uncertainty_us: Optional[int] = None) -> List[str]:
        """Describes the timing, and warns if it exceeds the CSL accuracy and uncertainty advertised by the CSL
        transmitter, which the CSL receiver uses to widen its CSL sample window."""
        max_drift_ppm = (accuracy_ppm or 0) + SNIFFER_ACCURACY_PPM

        if self.drift_ppm is None:
            drift = 'drift vs. sniffer unknown (insufficient span)'
        else:
            drift = f'drift vs. sniffer {self.drift_ppm:+.0f} ± {self.drift_se_ppm:.0f} ppm'

        lines = [
            f'CSL TX timing: {len(self.offsets_us)} data frames, offsets min {min(self.offsets_us):+d} / median'
            f' {statistics.median(self.offsets_us):+.0f} / max {max(self.offsets_us):+d} us, bias {self.bias_us:+.0f}'
            f' us, jitter ±{self.jitter_us:.0f} us, {drift}'
        ]

        if uncertainty_us is not None and self.drift_ppm is not None and self.max_offset_us > uncertainty_us:
            lines.append(f'CSL TX timing warning: offsets up to {self.max_offset_us:.0f} us after removing the drift'
                         f' exceed the CSL uncertainty of the CSL transmitter ({uncertainty_us} us)')

        if uncertainty_us is not None and self.drift_ppm is None:
            # Without a drift estimate, allows the worst clock drift of the CSL transmitter and the sniffer since the
            # CSL IE.
            exceeding = [
                abs(offset_us)
                for offset_us, age_us in zip(self.offsets_us, self.ages_us)
                if abs(offset_us) > uncertainty_us + max_drift_ppm * abs(age_us) / 1000000
            ]
            if exceeding:
                lines.append(f'CSL TX timing warning: offsets up to {max(exceeding)} us exceed the CSL uncertainty of'
                             f' the CSL transmitter ({uncertainty_us} us) plus a clock drift of {max_drift_ppm} ppm'
                             ' since the CSL IEs')

        # Only warns if the drift exceeds the limit by more than its standard error.
        if accuracy_ppm is not None and self.drift_ppm is not None and \
                abs(self.drift_ppm) - self.drift_se_ppm > max_drift_ppm:
            lines.append(f'CSL TX timing warning: {drift} exceeds the CSL accuracy of the CSL transmitter'
                         f' ({accuracy_ppm} ppm) plus the sniffer accuracy ({SNIFFER_ACCURACY_PPM} ppm)')

        return lines


@dataclass
class CslTxReport:
    """Represents the CSL frame exchanges between a CSL transmitter and a CSL receiver captured on a channel."""
    channel: int
    attempts: List[CslTxAttempt] = field(default_factory=list)
    retransmitted_frames: List[RetransmittedFrame] = field(default_factory=list)
    receiver_attached: bool = False
    num_excluded: int = 0  # The number of data frames sent while the CSL receiver polled data or attached

    @property
    def is_untimed(self) -> bool:
        """Whether no frame can be timed because the sniffer captured no CSL IE from the CSL receiver."""
        return bool(self.attempts) and not any(attempt.is_timed for attempt in self.attempts)

    @property
    def timing(self) -> Optional[CslTxTiming]:
        """The timing of the timed data frames whose CSL sample time is certain, or None if there is none."""
        samples = [(attempt.csl_ie_timestamp_us, attempt.timestamp_us - attempt.csl_ie_timestamp_us, attempt.offset_us)
                   for attempt in self.attempts
                   if attempt.is_timed and not attempt.around_schedule_change]

        if not samples:
            return None

        drift_ppm, drift_se_ppm = _estimate_drift(samples)
        adjusted = [offset_us - (drift_ppm or 0) * age_us / 1000000 for _, age_us, offset_us in samples]

        return CslTxTiming(offsets_us=[offset_us for _, _, offset_us in samples],
                           ages_us=[age_us for _, age_us, _ in samples],
                           bias_us=statistics.median(adjusted),
                           jitter_us=(max(adjusted) - min(adjusted)) / 2,
                           max_offset_us=max(abs(offset_us) for offset_us in adjusted),
                           drift_ppm=drift_ppm,
                           drift_se_ppm=drift_se_ppm)

    def describe(self) -> List[str]:
        """Describes the CSL issues found from the frames on the channel."""
        unacked = [attempt for attempt in self.attempts if not attempt.acked]
        timed = [attempt for attempt in unacked if attempt.is_timed]
        out_of_window = [attempt for attempt in timed if attempt.is_out_of_window]
        in_window = [attempt for attempt in timed if attempt.is_in_window]
        schedule_changed = [attempt for attempt in timed if attempt.schedule_changed]
        drifted = [
            attempt for attempt in timed
            if not attempt.schedule_changed and not attempt.is_in_window and not attempt.is_out_of_window
        ]
        untimed = [attempt for attempt in unacked if not attempt.is_timed]
        total = len(self.attempts)
        lines = []

        if not self.attempts:
            lines.append(
                f'CSL transmitter issue (no TX): no CSL data frame to the CSL receiver on channel {self.channel}')

        if out_of_window:
            lines.append(f'CSL transmitter issue (TX timing miss): {len(out_of_window)}/{total} data frames out of'
                         f' the CSL sample window ({_describe_timing(out_of_window)})')

        if in_window:
            lines.append(f'CSL receiver issue (missing Enh-ACK): {len(in_window)}/{total} data frames in the CSL'
                         f' sample window were not acknowledged ({_describe_timing(in_window)})')

        if drifted:
            lines.append(f'CSL timing inconclusive (clock drift): {len(drifted)}/{total} data frames were not'
                         ' acknowledged, and the clock drift since the CSL IE they are timed from makes it unknown'
                         f' whether they were in the CSL sample window ({_describe_timing(drifted)})')

        if schedule_changed:
            lines.append(f'CSL timing inconclusive (CSL schedule changed): {len(schedule_changed)}/{total} data frames'
                         ' were not acknowledged, and the CSL receiver changed its CSL sample time between the CSL IEs'
                         ' before and after them (e.g. it attached again)')

        if untimed:
            lines.append(f'{len(untimed)}/{total} data frames on channel {self.channel} were not acknowledged, and no'
                         f' CSL IE from the CSL receiver was captured on channel {self.channel} to time them')

        changed = [frame for frame in self.retransmitted_frames if frame.num_changed]
        if changed:
            num_changed = sum(frame.num_changed for frame in changed)
            num_retx = sum(frame.num_retx for frame in self.retransmitted_frames)
            lines.append(f'CSL transmitter issue (retransmission with new frame counter): {num_changed}/{num_retx}'
                         ' retransmitted data frames did not reuse the frame counter and key index of the first'
                         f' transmission ({"; ".join(frame.describe() for frame in changed)})')

        if not lines:
            lines.append(
                f'No CSL issue found: {total}/{total} data frames on channel {self.channel} were acknowledged')

        if self.receiver_attached:
            lines.append(f'The CSL receiver attached again on channel {self.channel}')

        if self.num_excluded:
            lines.append(f'{self.num_excluded} data frames sent while the CSL receiver polled data or attached are not'
                         ' analyzed')

        return lines


@dataclass
class _CslAnchor:
    """Represents a valid CSL IE from the CSL receiver."""
    timestamp_us: int
    csl_ie: CslIe
    session: int  # The number of times the CSL receiver started attaching before the CSL IE

    def time(self, timestamp_us: int) -> Tuple[int, int, int]:
        """Returns the offset, the clock drift and the CSL IE age of a frame timed from this CSL IE."""
        period_us = self.csl_ie.period * TEN_SYMBOLS_US
        sample_us = self.timestamp_us + self.csl_ie.phase * TEN_SYMBOLS_US
        offset_us = (timestamp_us - sample_us + period_us // 2) % period_us - period_us // 2
        age_us = abs(timestamp_us - self.timestamp_us)

        return offset_us, _get_drift(age_us), age_us


def _get_drift(age_us: int) -> int:
    return -(-age_us * MAX_CLOCK_DRIFT_PPM // 1000000)


def _is_in_window(offset_us: int, drift_us: int) -> bool:
    return abs(offset_us) + drift_us <= CSL_TX_TOLERANCE_US


def _is_out_of_window(offset_us: int, drift_us: int) -> bool:
    return abs(offset_us) > CSL_TX_TOLERANCE_US + drift_us


def _classify(offset_us: int, drift_us: int) -> Tuple[bool, bool]:
    return _is_in_window(offset_us, drift_us), _is_out_of_window(offset_us, drift_us)


def _is_schedule_changed(before: _CslAnchor, after: _CslAnchor) -> bool:
    """Returns whether the CSL receiver changed its CSL sample time between two of its CSL IEs."""
    offset_us, drift_us, _ = before.time(after.timestamp_us + after.csl_ie.phase * TEN_SYMBOLS_US)

    return before.csl_ie.period != after.csl_ie.period or _is_out_of_window(offset_us, drift_us)


def _describe_timing(attempts: List[CslTxAttempt]) -> str:
    offsets = ', '.join(f'{attempt.offset_us:+d}' for attempt in attempts)
    drift_us = max(attempt.drift_us for attempt in attempts)
    age_ms = max(attempt.csl_ie_age_us for attempt in attempts) // 1000

    return f'offsets {offsets} us, tolerance {CSL_TX_TOLERANCE_US} us, clock drift up to {drift_us} us for CSL IEs' \
        f' up to {age_ms} ms old'


def _estimate_drift(samples: List[Tuple[int, int, int]]) -> Tuple[Optional[float], Optional[float]]:
    """Estimates the clock drift and its standard error from (CSL IE timestamp, CSL IE age, offset) samples.

    Fits `offset = intercept of the CSL IE + drift * CSL IE age` with ordinary least squares, so that only the
    frames timed from the same CSL IE contribute to the drift. The frames before the first CSL IE, which the CSL
    transmitter timed from an uncaptured CSL IE, are grouped separately from the frames after it.
    """
    groups: Dict[Tuple[int, bool], List[Tuple[float, float]]] = {}
    for csl_ie_timestamp_us, age_us, offset_us in samples:
        groups.setdefault((csl_ie_timestamp_us, age_us < 0), []).append((age_us / 1000000, offset_us))

    deviations = []
    span_us = 0
    for group in groups.values():
        mean_age_s = statistics.fmean(age_s for age_s, _ in group)
        mean_offset_us = statistics.fmean(offset_us for _, offset_us in group)
        deviations += [(age_s - mean_age_s, offset_us - mean_offset_us) for age_s, offset_us in group]
        span_us = max(span_us, (max(age_s for age_s, _ in group) - min(age_s for age_s, _ in group)) * 1000000)

    # Needs at least 2 degrees of freedom left for the standard error.
    dof = len(samples) - len(groups) - 1
    if span_us < MIN_DRIFT_SPAN_US or dof < 2:
        return None, None

    sxx = sum(dx * dx for dx, _ in deviations)
    drift_ppm = sum(dx * dy for dx, dy in deviations) / sxx
    rss = sum((dy - drift_ppm * dx)**2 for dx, dy in deviations)

    return drift_ppm, math.sqrt(rss / dof / sxx)


def analyze_csl_tx(frames: List[SniffedFrame],
                   channel: int,
                   transmitter: Collection[str],
                   receiver: Collection[str],
                   start_us: int = 0,
                   csl_period_us: Optional[int] = None) -> CslTxReport:
    """Analyzes the CSL frame exchanges between a CSL transmitter and a CSL receiver captured on a channel.

    The CSL sample times of the receiver are derived from the CSL IEs in the frames it sent. The CSL Phase is the time
    from the MAC header of the frame containing the CSL IE to the MAC header of the frame to transmit (Thread 1.4,
    section 3.2.6.3.4). The sniffer timestamps all frames at the end of SFD, which is the PHR duration before the MAC
    header, so the offset of a data frame is directly the difference of the timestamps.

    A data frame is timed from the last CSL IE before it, which is the CSL schedule the CSL transmitter follows, or the
    first CSL IE after it if there is none. CSL IEs from before the CSL receiver started attaching again are not used.
    If the CSL receiver changed its CSL sample time between the CSL IEs before and after a data frame (e.g. it attached
    again on another channel), the data frame is timed from both of them.

    A data frame with the same sequence number and length as the previous data frame to the same address of the CSL
    receiver, in the same attachment of the CSL receiver, is a retransmission of it. The CSL transmitter may send other
    frames with new sequence numbers between the retransmissions, e.g. MLE messages to the extended address of the CSL
    receiver while it attaches again, or frames to other devices.

    :param frames: The frames captured by the sniffer on the channel.
    :param channel: The channel of the frames.
    :param transmitter: The addresses (extended address and RLOC16) of the CSL transmitter.
    :param receiver: The addresses (extended address and RLOC16) of the CSL receiver.
    :param start_us: Only data frames captured since this timestamp are analyzed.
    :param csl_period_us: The CSL period configured on the CSL receiver, or None to accept the CSL IEs of any period.
    """
    parsed = []
    for frame in frames:
        mac_frame = parse_mac_frame(bytes.fromhex(frame.psdu))
        if mac_frame is not None:
            parsed.append((frame, mac_frame))

    anchors = []
    report = CslTxReport(channel=channel)
    data_poll_us = None
    attaching = False
    session = 0
    attempts = []
    last_txs = {}  # The sequence number, PSDU length, session and retransmissions of the last data frame to an address

    for i, (frame, mac_frame) in enumerate(parsed):
        # An SSED is assumed to send broadcast frames only when attaching (e.g. MLE Parent Requests), and MAC command
        # frames only to poll its parent. In both cases it stays in receive mode, so the data frames sent to it are not
        # CSL transmissions.
        if mac_frame.src_addr in receiver:
            if mac_frame.dst_addr == BROADCAST_ADDR:
                session += 1
                if frame.timestamp_us >= start_us:
                    attaching = True
                    report.receiver_attached = True
            elif mac_frame.frame_type == FRAME_TYPE_COMMAND:
                data_poll_us = frame.timestamp_us

        if _is_csl_anchor(mac_frame, transmitter, receiver, csl_period_us):
            anchors.append(_CslAnchor(frame.timestamp_us, mac_frame.csl_ie, session))
            attaching = False

        if frame.timestamp_us < start_us or mac_frame.frame_type != FRAME_TYPE_DATA or \
                mac_frame.src_addr not in transmitter or mac_frame.dst_addr not in receiver:
            continue

        tx = (mac_frame.seq, len(frame.psdu), session)
        last_tx, retransmitted = last_txs.get(mac_frame.dst_addr, (None, None))
        if mac_frame.seq is None or mac_frame.frame_counter is None:
            retransmitted = None
        elif tx == last_tx and retransmitted is not None:
            if not retransmitted.num_retx:
                report.retransmitted_frames.append(retransmitted)
            retransmitted.frame_counters.append(mac_frame.frame_counter)
            retransmitted.key_indexes.append(mac_frame.key_index)
        else:
            retransmitted = RetransmittedFrame(mac_frame.seq, [mac_frame.frame_counter], [mac_frame.key_index])
        last_txs[mac_frame.dst_addr] = (tx, retransmitted)

        if attaching or (data_poll_us is not None and frame.timestamp_us - data_poll_us <= DATA_POLL_TIMEOUT_US):
            report.num_excluded += 1
            continue

        attempt = CslTxAttempt(timestamp_us=frame.timestamp_us,
                               acked=mac_frame.ack_request and _is_acked(parsed, i, transmitter))
        attempts.append((attempt, len(anchors), session))
        report.attempts.append(attempt)

    for attempt, num_prior_anchors, attempt_session in attempts:
        before = anchors[num_prior_anchors - 1] if num_prior_anchors > 0 else None
        after = anchors[num_prior_anchors] if num_prior_anchors < len(anchors) else None

        if before is not None and before.session != attempt_session:
            before = None

        if after is not None and after.session != attempt_session:
            after = None

        if before is None and after is None:
            continue

        attempt.offset_us, attempt.drift_us, attempt.csl_ie_age_us = (before or after).time(attempt.timestamp_us)
        attempt.csl_ie_timestamp_us = (before or after).timestamp_us

        if before is not None and after is not None and _is_schedule_changed(before, after):
            offset_us, drift_us, _ = after.time(attempt.timestamp_us)
            attempt.around_schedule_change = True
            attempt.schedule_changed = _classify(offset_us, drift_us) != _classify(attempt.offset_us, attempt.drift_us)

    return report


def _is_csl_anchor(mac_frame: MacFrame, transmitter: Collection[str], receiver: Collection[str],
                   csl_period_us: Optional[int]) -> bool:
    """Returns whether the frame is from the CSL receiver and has a valid CSL IE (Enh-ACKs have no source address)."""
    csl_ie = mac_frame.csl_ie

    if csl_ie is None or csl_ie.phase >= csl_ie.period:
        return False

    if csl_period_us is not None and csl_ie.period * TEN_SYMBOLS_US != csl_period_us:
        return False

    return mac_frame.src_addr in receiver or (mac_frame.frame_type == FRAME_TYPE_ACK and
                                              mac_frame.dst_addr in transmitter)


def _is_acked(parsed: List[Tuple[SniffedFrame, MacFrame]], index: int, transmitter: Collection[str]) -> bool:
    frame, mac_frame = parsed[index]
    deadline_us = frame.timestamp_us + (1 + len(frame.psdu) // 2) * PHR_PSDU_BYTE_US + ACK_MAX_DELAY_US

    for ack_frame, ack_mac_frame in parsed[index + 1:]:
        if ack_frame.timestamp_us > deadline_us:
            break

        # Imm-ACKs have no destination address.
        if ack_mac_frame.frame_type == FRAME_TYPE_ACK and ack_mac_frame.seq == mac_frame.seq and \
                ack_mac_frame.dst_addr in {None, *transmitter}:
            return True

    return False


def _is_dst_panid_present(version: int, dst_mode: int, src_mode: int, panid_compression: bool) -> bool:
    if version != FRAME_VERSION_2015:
        return dst_mode != ADDR_MODE_NONE

    # IEEE 802.15.4-2015, table 7-2
    if dst_mode == ADDR_MODE_NONE:
        return src_mode == ADDR_MODE_NONE and panid_compression

    if src_mode == ADDR_MODE_NONE or (dst_mode == ADDR_MODE_EXT and src_mode == ADDR_MODE_EXT):
        return not panid_compression

    return True


def _is_src_panid_present(version: int, dst_mode: int, src_mode: int, panid_compression: bool) -> bool:
    if src_mode == ADDR_MODE_NONE:
        return False

    if version != FRAME_VERSION_2015:
        return not panid_compression

    # IEEE 802.15.4-2015, table 7-2
    if dst_mode == ADDR_MODE_EXT and src_mode == ADDR_MODE_EXT:
        return False

    return not panid_compression


def _get_addr_size(mode: int) -> int:
    return {ADDR_MODE_SHORT: 2, ADDR_MODE_EXT: 8}.get(mode, 0)


def _get_addr_fields_size(version: int, dst_mode: int, src_mode: int, panid_compression: bool) -> int:
    size = _get_addr_size(dst_mode) + _get_addr_size(src_mode)

    if _is_dst_panid_present(version, dst_mode, src_mode, panid_compression):
        size += 2

    if _is_src_panid_present(version, dst_mode, src_mode, panid_compression):
        size += 2

    return size


def _parse_addr(psdu: bytes, offset: int, mode: int):
    size = _get_addr_size(mode)

    if size == 0:
        return None, offset

    # Addresses are sent in little-endian byte order.
    return psdu[offset:offset + size][::-1].hex(), offset + size


def _get_security_header_size(security_control: int) -> int:
    frame_counter_suppressed = bool(security_control & (1 << 5))
    key_id_mode = (security_control >> 3) & 0x3

    return 1 + (0 if frame_counter_suppressed else 4) + (0, 1, 5, 9)[key_id_mode]


def _get_mic_size(security_control: int) -> int:
    return (0, 4, 8, 16)[security_control & 0x3]


def _parse_security_header(psdu: bytes, offset: int) -> Tuple[Optional[int], Optional[int]]:
    """Returns the Frame Counter and the Key Index of the Auxiliary Security Header at the offset.

    The Key Identifier field is the Key Source (0, 4 or 8 bytes) followed by the Key Index (IEEE 802.15.4-2015,
    section 9.4.4).
    """
    security_control = psdu[offset]
    frame_counter_suppressed = bool(security_control & (1 << 5))
    key_id_mode = (security_control >> 3) & 0x3
    frame_counter = None
    key_index = None
    offset += 1

    if not frame_counter_suppressed:
        frame_counter = struct.unpack_from('<I', psdu, offset)[0]
        offset += 4

    if key_id_mode != 0:
        key_index = psdu[offset + (0, 0, 4, 8)[key_id_mode]]

    return frame_counter, key_index
