#
#  Copyright (c) 2018, The OpenThread Authors.
#  All rights reserved.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
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
