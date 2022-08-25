#!/usr/bin/env python3
#
#  Copyright (c) 2022, The OpenThread Authors.
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
""" Module to provide codec utilities for .pcap formatters. """

import struct
import time

# https://www.tcpdump.org/linktypes.html
DLT_IEEE802_15_4_WITHFCS = 195

PCAP_MAGIC_NUMBER = 0xA1B2C3D4
PCAP_VERSION_MAJOR = 2
PCAP_VERSION_MINOR = 4


class PcapCodec(object):
    """ Utility class for .pcap formatters. """

    def __init__(self, channel, filename):
        self._dlt = DLT_IEEE802_15_4_WITHFCS
        self._channel = channel

        self._pcap_writer = open(filename, 'wb')
        self._write(self._encode_header())

    def _write(self, content):
        self._pcap_writer.write(content)
        self._pcap_writer.flush()

    def _encode_header(self):
        """ Return a pcap file header. """

        return struct.pack(
            '<LHHLLLL',
            PCAP_MAGIC_NUMBER,
            PCAP_VERSION_MAJOR,
            PCAP_VERSION_MINOR,
            0,
            0,
            256,
            self._dlt,
        )

    def _encode_frame(self, frame, sec, usec):
        """ Return a pcap encapsulation of the given frame. """

        # Ignore the first byte storing channel.
        frame = frame[1:]

        length = len(frame)
        pcap_frame = struct.pack('<LLLL', sec, usec, length, length)
        pcap_frame += frame
        return pcap_frame

    def _get_timestamp(self):
        """ Return the internal timestamp. """

        timestamp = time.time()
        timestamp_sec = int(timestamp)
        timestamp_usec = int((timestamp - timestamp_sec) * 1000000)
        return timestamp_sec, timestamp_usec

    def append(self, frame):
        """ Append a frame. """

        # Filter channel.
        if frame[0] != self._channel:
            return

        timestamp = self._get_timestamp()
        self._write(self._encode_frame(frame, *timestamp))

    def close(self):
        self._pcap_writer.close()
