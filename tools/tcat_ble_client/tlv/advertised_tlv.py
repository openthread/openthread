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


class AdvertisedTlv:
    """
    Represents a TCAT Advertisement TLV, used to advertise capabilities of a TCAT Device.
    """

    DEVICE_TYPE_STATUS_TLV = 4
    CAPABILITIES_TLV = 5

    def __init__(self, type: int = None, size: int = None, value: bytes = None):
        self._type: int = type
        self._size: int = size
        self._value: bytes = value

    def __str__(self) -> str:
        return f'TYPE 0x{self._type:02x} VALUE: {str(self._value)}'

    def set_from_bytes(self, data: bytes):
        self._type = (data[0] & 0xf0) >> 4
        self._size = data[0] & 0xf
        header_len = 1
        self._value = data[header_len:header_len + self._size]


class CapabilitiesTlv(AdvertisedTlv):
    G = 0x80
    L = 0x40

    def __str__(self) -> str:
        val = int.from_bytes(self._value, 'big')
        return f'TYPE 0x{self._type:02x} G: {is_set(val, self.G)} L: {is_set(val, self.L)}'


class DeviceTypeStatusTlv(AdvertisedTlv):
    D = 0x80
    R = 0x40
    B = 0x20
    T = 0x10
    C = 0x08
    S = 0x04
    M = 0x02

    def __str__(self) -> str:
        val = int.from_bytes(self._value, 'big')
        return f'TYPE 0x{self._type:02x} D: {is_set(val, self.D)} R: {is_set(val, self.R)} B: {is_set(val, self.B)} T: {is_set(val, self.T)} C: {is_set(val, self.C)} S: {is_set(val, self.S)} M: {is_set(val, self.M)}'


def is_set(val, flag):
    return 1 if val & flag else 0


def _create_tlv(data: bytes):
    res = None
    header_len = 1
    type = (data[0] & 0xf0) >> 4
    size = data[0] & 0xf
    value = data[header_len:header_len + size]

    if type == AdvertisedTlv.DEVICE_TYPE_STATUS_TLV:
        res = DeviceTypeStatusTlv(type, size, value)
    elif type == AdvertisedTlv.CAPABILITIES_TLV:
        res = CapabilitiesTlv(type, size, value)
    else:
        res = AdvertisedTlv(type, size, value)

    return res


def parse_tlvs(data: bytes):
    res = []
    while data:
        next_tlv = _create_tlv(data)
        header_len = 1
        data = data[next_tlv._size + header_len:]
        res.append(next_tlv)
    return res
