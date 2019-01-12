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

import io
import math
import struct

from enum import IntEnum
from network_data import SubTlvsFactory


class TlvType(IntEnum):
    EXTENDED_PANID = 2
    NETWORK_NAME = 3
    STEERING_DATA = 8
    COMMISSIONER_UDP_PORT = 15
    JOINER_UDP_PORT = 18
    REQUEST = 128
    RESPONSE = 129


class ThreadDiscoveryTlvsFactory(SubTlvsFactory):

    def __init__(self, sub_tlvs_factories):
        super(ThreadDiscoveryTlvsFactory, self).__init__(sub_tlvs_factories)


class DiscoveryRequest(object):

    def __init__(self, version, joiner_flag):
        self._version = version
        self._joiner_flag = joiner_flag

    @property
    def version(self):
        return self._version

    @property
    def joiner_flag(self):
        return self._joiner_flag

    def __eq__(self, other):
        return (type(self) is type(other)
            and self.version == other.version
            and self.joiner_flag == other.joiner_flag)

    def __repr__(self):
        return "DiscoveryRequest(version={}, joiner_flag={})".format(
            self.version, self.joiner_flag)


class DiscoveryRequestFactory(object):

    def parse(self, data, message_info):
        data_byte = struct.unpack(">B", data.read(1))[0]
        version = (data_byte & 0xf0) >> 4
        joiner_flag = (data_byte & 0x08) >> 3

        return DiscoveryRequest(version, joiner_flag)


class DiscoveryResponse(object):

    def __init__(self, version, native_flag):
        self._version = version
        self._native_flag = native_flag

    @property
    def version(self):
        return self._version

    @property
    def native_flag(self):
        return self._native_flag

    def __eq__(self, other):
        return (type(self) is type(other)
            and self.version == other.version
            and self.native_flag == other.native_flag)

    def __repr__(self):
        return "DiscoveryResponse(version={}, native_flag={})".format(
            self.version, self.native_flag)


class DiscoveryResponseFactory(object):

    def parse(self, data, message_info):
        data_byte = struct.unpack(">B", data.read(1))[0]
        version = (data_byte & 0xf0) >> 4
        native_flag = (data_byte & 0x08) >> 3

        return DiscoveryResponse(version, native_flag)


class ExtendedPanid(object):

    def __init__(self, extended_panid):
        self._extended_panid = extended_panid

    @property
    def extended_panid(self):
        return self._extended_panid

    def __eq__(self, other):
        return (type(self) is type(other)
            and self.extended_panid == other.extended_panid)

    def __repr__(self):
        return "ExtendedPanid(extended_panid={})".format(self.extended_panid)


class ExtendedPanidFactory(object):

    def parse(self, data, message_info):
        extended_panid = struct.unpack(">Q", data.read(8))[0]
        return ExtendedPanid(extended_panid)


class NetworkName(object):

    def __init__(self, network_name):
        self._network_name = network_name

    @property
    def network_name(self):
        return self._network_name

    def __eq__(self, other):
        return (type(self) is type(other)
            and self.network_name == other.network_name)

    def __repr__(self):
        return "NetworkName(network_name={})".format(self.network_name)


class NetworkNameFactory(object):

    def parse(self, data, message_info):
        len = message_info.length
        network_name = struct.unpack("{}s".format(10), data.read(len))[0]
        return NetworkName(network_name)


class JoinerUdpPort(object):

    def __init__(self, udp_port):
        self._udp_port = udp_port

    @property
    def udp_port(self):
        return self._udp_port

    def __eq__(self, other):
        return type(self) is type(other) and self.udp_port == other.udp_port

    def __repr__(self):
        return "JoinerUdpPort(udp_port={})".format(self.udp_port)


class JoinerUdpPortFactory(object):

    def parse(self, data, message_info):
        udp_port = struct.unpack(">H", data.read(2))[0]
        return JoinerUdpPort(udp_port)
