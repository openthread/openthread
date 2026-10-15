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
from typing import List, Optional


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
        self.__proc = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, encoding='utf-8')

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
        output = self.__proc.stdout.read() if self.__proc.stdout else ''
        returncode = self.__proc.returncode

        self.__release_proc()
        self.__remove_pcapng()

        raise RuntimeError(
            f'The sniffer stopped unexpectedly (exit code {returncode}, see the ot-extcap logs in syslog):'
            f' {output}')

    def __release_proc(self):
        if self.__proc.stdout is not None:
            self.__proc.stdout.close()

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
        """
        PCAPNG_EPB_TYPE = 6
        EPB_HEADER_FORMAT = '<IIIIIII'  # Type, total length, interface, timestamp high/low, captured/original length
        TAP_HEADER_FORMAT = '<HH'  # Version, length (including the TLVs)
        TAP_TLV_HEADER_FORMAT = '<HH'  # Type, length
        TAP_CHANNEL_TYPE = 3

        frames = []
        offset = 0

        while offset + 8 <= len(data):
            block_type, block_length = struct.unpack_from('<II', data, offset)

            if block_length < 12 or offset + block_length > len(data):
                break  # A truncated block, e.g. the capture was stopped while writing it.

            if block_type == PCAPNG_EPB_TYPE:
                _, _, _, ts_high, ts_low, captured_length, _ = struct.unpack_from(EPB_HEADER_FORMAT, data, offset)
                packet = data[offset + struct.calcsize(EPB_HEADER_FORMAT):][:captured_length]
                _, tap_length = struct.unpack_from(TAP_HEADER_FORMAT, packet, 0)
                channel = 0
                tlv_offset = struct.calcsize(TAP_HEADER_FORMAT)

                while tlv_offset + 4 <= tap_length:
                    tlv_type, tlv_length = struct.unpack_from(TAP_TLV_HEADER_FORMAT, packet, tlv_offset)
                    if tlv_type == TAP_CHANNEL_TYPE:
                        channel = struct.unpack_from('<H', packet, tlv_offset + 4)[0]
                    tlv_offset += 4 + ((tlv_length + 3) & ~3)

                frames.append(
                    SniffedFrame(timestamp_us=(ts_high << 32) | ts_low,
                                 channel=channel,
                                 psdu=packet[tap_length:].hex()))

            offset += block_length

        return frames
