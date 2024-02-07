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


class MeshcopTlvType(Enum):
    CHANNEL = 0
    PANID = 1
    EXTPANID = 2
    NETWORKNAME = 3
    PSKC = 4
    NETWORKKEY = 5
    NETWORK_KEY_SEQUENCE = 6
    MESHLOCALPREFIX = 7
    STEERING_DATA = 8
    BORDER_AGENT_RLOC = 9
    COMMISSIONER_ID = 10
    COMM_SESSION_ID = 11
    SECURITYPOLICY = 12
    GET = 13
    ACTIVETIMESTAMP = 14
    COMMISSIONER_UDP_PORT = 15
    STATE = 16
    JOINER_DTLS = 17
    JOINER_UDP_PORT = 18
    JOINER_IID = 19
    JOINER_RLOC = 20
    JOINER_ROUTER_KEK = 21
    PROVISIONING_URL = 32
    VENDOR_NAME_TLV = 33
    VENDOR_MODEL_TLV = 34
    VENDOR_SW_VERSION_TLV = 35
    VENDOR_DATA_TLV = 36
    VENDOR_STACK_VERSION_TLV = 37
    UDP_ENCAPSULATION_TLV = 48
    IPV6_ADDRESS_TLV = 49
    PENDINGTIMESTAMP = 51
    DELAYTIMER = 52
    CHANNELMASK = 53
    COUNT = 54
    PERIOD = 55
    SCAN_DURATION = 56
    ENERGY_LIST = 57
    DISCOVERYREQUEST = 128
    DISCOVERYRESPONSE = 129
    JOINERADVERTISEMENT = 241

    @classmethod
    def from_value(cls, value: int):
        return cls._value2member_map_.get(value)

    def to_bytes(self):
        return bytes([self.value])
