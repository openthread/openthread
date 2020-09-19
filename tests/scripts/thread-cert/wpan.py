#!/usr/bin/env python3
#
#  Copyright (c) 2019, The OpenThread Authors.
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
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS'
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

import struct

from collections import namedtuple
from enum import Enum


class DstAddrMode(Enum):
    NONE = 0
    RESERVED = 1
    SHORT = 2
    EXTENDED = 3


class FrameType(Enum):
    BEACON = 0
    DATA = 1
    ACK = 2
    COMMAND = 3


class WpanFrameInfo(namedtuple('WpanFrameInfo', ['fcf', 'seq_no', 'dst_extaddr', 'dst_short'])):

    @property
    def frame_type(self) -> FrameType:
        return FrameType(self.fcf & 0x7)

    @property
    def dst_addr_mode(self) -> DstAddrMode:
        return DstAddrMode((self.fcf & 0x0c00) >> 10)

    @property
    def is_broadcast(self) -> bool:
        return self.dst_addr_mode == DstAddrMode.SHORT and self.dst_short == 0xffff


def dissect(frame: bytes) -> WpanFrameInfo:
    fcf = struct.unpack("<H", frame[1:3])[0]

    seq_no = frame[3]

    dst_addr_mode = (fcf & 0x0c00) >> 10

    dst_extaddr, dst_short = None, None

    if dst_addr_mode == DstAddrMode.SHORT.value:
        dst_short = struct.unpack('<H', frame[6:8])[0]
    elif dst_addr_mode == DstAddrMode.EXTENDED.value:
        dst_extaddr = '%016x' % struct.unpack('<Q', frame[6:14])[0]

    return WpanFrameInfo(fcf=fcf, seq_no=seq_no, dst_extaddr=dst_extaddr, dst_short=dst_short)
