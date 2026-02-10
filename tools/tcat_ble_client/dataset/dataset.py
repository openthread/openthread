"""
  Copyright (c) 2025, The OpenThread Authors.
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

from typing import Dict, List

from tlv.tlv import TLV
from tlv.dataset_tlv import MeshcopTlvType
from dataset.dataset_entries import DatasetEntry, create_dataset_entry, ENTRY_CLASSES

initial_dataset = bytes([
    0x0E, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x12, 0x35, 0x06, 0x00, 0x04,
    0x00, 0x1F, 0xFF, 0xE0, 0x02, 0x08, 0xEF, 0x13, 0x98, 0xC2, 0xFD, 0x50, 0x4B, 0x67, 0x07, 0x08, 0xFD, 0x35, 0x34,
    0x41, 0x33, 0xD1, 0xD7, 0x3E, 0x05, 0x10, 0xFD, 0xA7, 0xC7, 0x71, 0xA2, 0x72, 0x02, 0xE2, 0x32, 0xEC, 0xD0, 0x4C,
    0xF9, 0x34, 0xF4, 0x76, 0x03, 0x0F, 0x4F, 0x70, 0x65, 0x6E, 0x54, 0x68, 0x72, 0x65, 0x61, 0x64, 0x2D, 0x63, 0x36,
    0x34, 0x65, 0x01, 0x02, 0xC6, 0x4E, 0x04, 0x10, 0x5E, 0x9B, 0x9B, 0x36, 0x0F, 0x80, 0xB8, 0x8B, 0xE2, 0x60, 0x3F,
    0xB0, 0x13, 0x5C, 0x8D, 0x65, 0x0C, 0x04, 0x02, 0xA0, 0xF7, 0xF8
])


class ThreadDataset:

    def __init__(self):
        self.entries: Dict[MeshcopTlvType, DatasetEntry] = {}
        self.set_from_bytes(initial_dataset)

    def print_content(self):
        for type, entry in self.entries.items():
            print(f'{type.name}:')
            entry.print_content(indent=1)
            print()

    def set_from_bytes(self, bytes):
        for tlv in TLV.parse_tlvs(bytes):
            type = MeshcopTlvType.from_value(tlv.type)
            self.entries[type] = create_dataset_entry(type)
            self.entries[type].set_from_tlv(tlv)

    def to_bytes(self):
        res = bytes()
        for entry in self.entries.values():
            res += entry.to_tlv().to_bytes()
        return res

    def get_entry(self, type: MeshcopTlvType):
        return self.entries[type]

    def set_entry(self, type: MeshcopTlvType, args: List[str]):
        if type in ENTRY_CLASSES:
            if type in self.entries:
                self.entries[type].set(args)
            else:
                self.entries[type] = create_dataset_entry(type, args)
            return
        raise KeyError(f'Key {type} not available in the dataset.')

    def clear(self):
        self.entries.clear()
