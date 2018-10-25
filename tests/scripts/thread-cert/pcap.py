#!/usr/bin/env python
#
#  Copyright (c) 2018, The OpenThread Authors.
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

DLT_IEEE802_15_4 = 195
PCAP_MAGIC_NUMBER = 0xa1b2c3d4
PCAP_VERSION_MAJOR = 2
PCAP_VERSION_MINOR = 4


class PcapCodec(object):
    """ Utility class for .pcap formatters. """

    def __init__(self, filename):
        self._pcap_file = open('%s.pcap' % filename, 'wb')
        self._pcap_file.write(self.encode_header())
        self._epoch = time.time()

    def encode_header(self):
        """ Returns a pcap file header. """
        return struct.pack("<LHHLLLL",
                           PCAP_MAGIC_NUMBER,
                           PCAP_VERSION_MAJOR,
                           PCAP_VERSION_MINOR,
                           0, 0, 256,
                           DLT_IEEE802_15_4)

    def encode_frame(self, frame, sec, usec):
        """ Returns a pcap encapsulation of the given frame. """
        frame = frame[1:]
        length = len(frame)
        pcap_frame = struct.pack("<LLLL", sec, usec, length, length)
        pcap_frame += frame
        return pcap_frame

    def _get_timestamp(self):
        """ Returns the internal timestamp. """
        timestamp = time.time() - self._epoch
        timestamp_sec = int(timestamp)
        timestamp_usec = int((timestamp - timestamp_sec) * 1000000)
        return timestamp_sec, timestamp_usec

    def append(self, frame, timestamp=None):
        """ Appends a frame. """
        if timestamp is None:
            timestamp = self._get_timestamp()
        pkt = self.encode_frame(frame, *timestamp)
        self._pcap_file.write(pkt)

    def __del__(self):
        self._pcap_file.close()
