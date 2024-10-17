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
from enum import Enum


class TcatTLVType(Enum):
    RESPONSE_W_STATUS = 0x01
    RESPONSE_W_PAYLOAD = 0x02
    GET_NETWORK_NAME = 0x08
    DISCONNECT = 0x09
    PING = 0x0A
    GET_DEVICE_ID = 0x0B
    GET_EXT_PAN_ID = 0x0C
    GET_PROVISIONING_URL = 0x0D
    PRESENT_PSKD_HASH = 0x10
    PRESENT_PSKC_HASH = 0x11
    PRESENT_INSTALL_CODE_HASH = 0x12
    GET_RANDOM_NUMBER_CHALLENGE = 0x13
    GET_PSKD_HASH = 0x14
    ACTIVE_DATASET = 0x20
    DECOMMISSION = 0x60
    APPLICATION = 0x82
    THREAD_START = 0x27
    THREAD_STOP = 0x28

    @classmethod
    def from_value(cls, value: int):
        return cls._value2member_map_.get(value)

    def to_bytes(self):
        return bytes([self.value])
