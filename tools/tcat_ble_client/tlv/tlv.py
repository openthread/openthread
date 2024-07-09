"""
  Copyright (c) 2024, The OpenThread Authors.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
  3. Neither the name of the copyright holder nor the
     names of its contributors may be used to endorse or promote products
     derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
"""

from __future__ import annotations
from typing import List


class TLV():

    def __init__(self, type: int = None, value: bytes = None):
        self.type: int = type
        self.value: bytes = value

    def __str__(self):
        return f'TLV\n\tTYPE:\t0x{self.type:02x}\n\tVALUE:\t{self.value}'

    @staticmethod
    def parse_tlvs(data: bytes) -> List[TLV]:
        res: List[TLV] = []
        while data:
            next_tlv = TLV.from_bytes(data)
            next_tlv_size = len(next_tlv.to_bytes())
            data = data[next_tlv_size:]
            res.append(next_tlv)
        return res

    @staticmethod
    def from_bytes(data: bytes) -> TLV:
        res = TLV()
        res.set_from_bytes(data)
        return res

    def set_from_bytes(self, data: bytes):
        self.type = data[0]
        header_len = 2
        size_offset = 1
        if data[1] == 0xFF:
            header_len = 4
            size_offset = 2
        length = int.from_bytes(data[size_offset:header_len], byteorder='big')
        self.value = data[header_len:header_len + length]

    def to_bytes(self) -> bytes:
        has_long_header = len(self.value) >= 254
        header_len = 4 if has_long_header else 2
        len_bytes = len(self.value).to_bytes(header_len // 2, byteorder='big')
        type = self.type
        if has_long_header:
            type = type << 8 | 255
        header = type.to_bytes(header_len // 2, byteorder='big') + len_bytes
        return header + self.value
