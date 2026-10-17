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
"""Captures IEEE 802.15.4 frames with an OpenThread RCP sniffer driven by `ot-extcap` (from ot-br-posix)."""

import atexit
import logging
import os
import signal
import struct
import subprocess
import tempfile
import time

from dataclasses import dataclass
from typing import IO, List, Optional


@dataclass
class SniffedFrame:
    """Represents a frame captured by the sniffer.

    Attributes:
      timestamp_us: The receive timestamp in microseconds, taken by the sniffer radio (end of SFD).
      channel: The channel the frame was received on.
      psdu: The PSDU of the frame in hex, including the FCS.
    """
    timestamp_us: int
    channel: int
    psdu: str


class Sniffer(object):
    """Captures frames on a channel by running `ot-extcap --capture` with an RCP as the sniffer radio."""

    DEFAULT_START_DELAY = 2.0  # Time (in seconds) for `ot-extcap` to initialize the RCP and start capturing.
    DEFAULT_STOP_TIMEOUT = 5.0

    def __init__(self, extcap: str, radio_url: str, start_delay: float = DEFAULT_START_DELAY):
        """
        :param extcap: The path to the `ot-extcap` executable.
        :param radio_url: The radio URL of the sniffer RCP, e.g. `spinel+hdlc+uart:///dev/ttyACM2?uart-baudrate=115200`.
        :param start_delay: The time (in seconds) to wait for `ot-extcap` to start capturing.
        """
        self.__extcap = extcap
        self.__radio_url = radio_url
        self.__start_delay = start_delay
        self.__proc: Optional[subprocess.Popen] = None
        self.__log: Optional[IO[bytes]] = None
        self.__pcapng_path: Optional[str] = None

    def start(self, channel: int):
        """Starts capturing frames on the channel."""
        if self.__proc is not None:
            raise RuntimeError('The sniffer is already capturing')

        fd, self.__pcapng_path = tempfile.mkstemp(prefix='cp-caps-sniffer-', suffix='.pcapng')
        os.close(fd)

        args = [
            self.__extcap, '--capture', '--extcap-interface', 'spinel_radio_url', '--radio-url', self.__radio_url,
            '--channel',
            str(channel), '--fifo', self.__pcapng_path
        ]
        logging.debug('Sniffer: %s', ' '.join(args))

        # The output of `ot-extcap` goes to a file rather than a pipe, which is not read while capturing and could
        # block `ot-extcap` once full. It is only read if `ot-extcap` stops unexpectedly.
        self.__log = tempfile.TemporaryFile(prefix='cp-caps-sniffer-', suffix='.log')
        self.__proc = subprocess.Popen(args, stdout=self.__log, stderr=subprocess.STDOUT)

        # Stops `ot-extcap` (which holds the sniffer RCP) even if the caller exits without stopping the sniffer.
        atexit.register(self.close)

        time.sleep(self.__start_delay)

        if self.__proc.poll() is not None:
            self.__raise_stopped_unexpectedly()

    def stop(self) -> List[SniffedFrame]:
        """Stops capturing frames and returns the captured frames.

        Raises RuntimeError if `ot-extcap` stopped before (e.g. it failed to initialize the sniffer RCP).
        """
        if self.__proc is None:
            raise RuntimeError('The sniffer is not capturing')

        if self.__proc.poll() is not None:
            self.__raise_stopped_unexpectedly()

        self.__proc.send_signal(signal.SIGTERM)

        try:
            self.__proc.wait(self.DEFAULT_STOP_TIMEOUT)
        except subprocess.TimeoutExpired:
            self.__proc.kill()
            self.__proc.wait()

        self.__release_proc()

        try:
            with open(self.__pcapng_path, 'rb') as f:
                frames = self.parse_pcapng(f.read())
        finally:
            self.__remove_pcapng()

        return frames

    def close(self):
        """Stops capturing frames if capturing, and discards the captured frames."""
        if self.__proc is not None:
            if self.__proc.poll() is None:
                self.__proc.kill()
                self.__proc.wait()

            self.__release_proc()
            self.__remove_pcapng()

    def __raise_stopped_unexpectedly(self):
        self.__log.seek(0)
        output = self.__log.read().decode('utf-8', errors='replace').strip()
        returncode = self.__proc.returncode

        self.__release_proc()
        self.__remove_pcapng()

        raise RuntimeError(
            f'The sniffer stopped unexpectedly (exit code {returncode}, see the ot-extcap logs in syslog):'
            f' {output}')

    def __release_proc(self):
        self.__log.close()
        self.__log = None
        self.__proc = None
        atexit.unregister(self.close)

    def __remove_pcapng(self):
        if self.__pcapng_path is not None:
            os.remove(self.__pcapng_path)
            self.__pcapng_path = None

    @staticmethod
    def parse_pcapng(data: bytes) -> List[SniffedFrame]:
        """Parses the frames of a little-endian PCAPNG capture written by `ot-extcap`.

        The frames are Enhanced Packet Blocks of the `IEEE802_15_4_TAP` link type, with timestamps in microseconds.
        Malformed blocks are skipped, and the parsing stops at a truncated block (e.g. the capture was stopped while
        writing it).
        """
        BLOCK_HEADER_FORMAT = '<II'  # Type, total length
        BLOCK_TRAILER_LENGTH = 4  # Total length
        PCAPNG_EPB_TYPE = 6

        frames = []
        offset = 0

        while offset + struct.calcsize(BLOCK_HEADER_FORMAT) <= len(data):
            block_type, block_length = struct.unpack_from(BLOCK_HEADER_FORMAT, data, offset)

            if block_length < struct.calcsize(BLOCK_HEADER_FORMAT) + BLOCK_TRAILER_LENGTH or \
                    offset + block_length > len(data):
                break

            if block_type == PCAPNG_EPB_TYPE:
                frame = Sniffer.__parse_epb(data[offset:offset + block_length - BLOCK_TRAILER_LENGTH])
                if frame is not None:
                    frames.append(frame)

            offset += block_length

        return frames

    @staticmethod
    def __parse_epb(block: bytes) -> Optional[SniffedFrame]:
        """Parses an Enhanced Packet Block (without its trailer), or returns None if it is malformed."""
        EPB_HEADER_FORMAT = '<IIIIIII'  # Type, total length, interface, timestamp high/low, captured/original length
        TAP_HEADER_FORMAT = '<HH'  # Version, length (including the TLVs)
        TAP_TLV_HEADER_FORMAT = '<HH'  # Type, length
        TAP_CHANNEL_TYPE = 3
        TAP_CHANNEL_FORMAT = '<H'  # Channel (followed by the channel page)

        if len(block) < struct.calcsize(EPB_HEADER_FORMAT):
            return None

        _, _, _, ts_high, ts_low, captured_length, _ = struct.unpack_from(EPB_HEADER_FORMAT, block, 0)
        packet = block[struct.calcsize(EPB_HEADER_FORMAT):]

        if captured_length > len(packet) or captured_length < struct.calcsize(TAP_HEADER_FORMAT):
            return None

        packet = packet[:captured_length]
        _, tap_length = struct.unpack_from(TAP_HEADER_FORMAT, packet, 0)

        if tap_length < struct.calcsize(TAP_HEADER_FORMAT) or tap_length > len(packet):
            return None

        channel = 0
        tlv_offset = struct.calcsize(TAP_HEADER_FORMAT)

        while tlv_offset + struct.calcsize(TAP_TLV_HEADER_FORMAT) <= tap_length:
            tlv_type, tlv_length = struct.unpack_from(TAP_TLV_HEADER_FORMAT, packet, tlv_offset)
            value_offset = tlv_offset + struct.calcsize(TAP_TLV_HEADER_FORMAT)

            if value_offset + tlv_length > tap_length:
                return None

            if tlv_type == TAP_CHANNEL_TYPE and tlv_length >= struct.calcsize(TAP_CHANNEL_FORMAT):
                channel = struct.unpack_from(TAP_CHANNEL_FORMAT, packet, value_offset)[0]

            # TLV values are padded to 4-byte boundaries.
            tlv_offset = value_offset + ((tlv_length + 3) & ~3)

        return SniffedFrame(timestamp_us=(ts_high << 32) | ts_low, channel=channel, psdu=packet[tap_length:].hex())
