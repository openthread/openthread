#!/usr/bin/env python
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

from enum import IntEnum
import io
import struct

from network_data import SubTlvsFactory

class TlvType(IntEnum):
    EXTENDED_PANID = 2
    NETWORK_NAME = 3
    STEERING_DATA = 8
    COMMISSIONER_UDP_PORT = 15
    STATE = 16
    JOINER_UDP_PORT = 18
    PROVISIONING_URL = 32
    VENDOR_NAME = 33
    VENDOR_MODEL = 34
    VENDOR_SW_VERSION = 35
    VENDOR_DATA = 36
    VENDOR_STACK_VERSION = 37
    DISCOVERY_REQUEST = 128
    DISCOVERY_RESPONSE = 129


class MeshCopMessageType(IntEnum):
    JOIN_FIN_REQ = 1
    JOIN_FIN_RSP = 2
    JOIN_ENT_NTF = 3
    JOIN_ENT_RSP = 4


def create_mesh_cop_message_type_set():
    return [ MeshCopMessageType.JOIN_FIN_REQ,
             MeshCopMessageType.JOIN_FIN_RSP,
             MeshCopMessageType.JOIN_ENT_NTF,
             MeshCopMessageType.JOIN_ENT_RSP ]


class State(object):

    def __init__(self, state):
        self._state = state

    @property
    def state(self):
        return self._state

    def __eq__(self, other):
        return self.state == other.state

    def __repr__(self):
        return "State(state={})".format(self.state)


class StateFactory:

    def parse(self, data):
        state = ord(data.read(1))
        return State(state)


class VendorName(object):

    def __init__(self, vendor_name):
        self._vendor_name = vendor_name

    @property
    def vendor_name(self):
        return self._vendor_name

    def __eq__(self, other):
        return self.vendor_name == other.vendor_name

    def __repr__(self):
        return "VendorName(vendor_name={})".format(self.vendor_name)


class VendorNameFactory:

    def parse(self, data):
        vendor_name = data.getvalue().decode('utf-8')
        return VendorName(vendor_name)


class VendorModel(object):

    def __init__(self, vendor_model):
        self._vendor_model = vendor_model

    @property
    def vendor_model(self):
        return self._vendor_model

    def __eq__(self, other):
        return self.vendor_model == other.vendor_model

    def __repr__(self):
        return "VendorModel(vendor_model={})".format(self.vendor_model)


class VendorModelFactory:

    def parse(self, data):
        vendor_model = data.getvalue().decode('utf-8')
        return VendorModel(vendor_model)

class VendorSWVersion(object):

    def __init__(self, vendor_sw_version):
        self._vendor_sw_version = vendor_sw_version

    @property
    def vendor_sw_version(self):
        return self._vendor_sw_version

    def __eq__(self, other):
        return self.vendor_sw_version == other.vendor_sw_version

    def __repr__(self):
        return "VendorName(vendor_sw_version={})".format(self.vendor_sw_version)


class VendorSWVersionFactory:

    def parse(self, data):
        vendor_sw_version = data.getvalue()
        return VendorSWVersion(vendor_sw_version)


# VendorStackVersion TLV (37)
class VendorStackVersion(object):

    def __init__(self, stack_vendor_oui, build, rev, minor, major):
        self._stack_vendor_oui = stack_vendor_oui
        self._build = build
        self._rev = rev
        self._minor = minor
        self._major = major
        return

    @property
    def stack_vendor_oui(self):
        return self._stack_vendor_oui

    @property
    def build(self):
        return self._build

    @property
    def rev(self):
        return self._rev

    @property
    def minor(self):
        return self._minor

    @property
    def major(self):
        return self._major

    def __repr__(self):
        return "VendorStackVersion(vendor_stack_version={}, build={}, rev={}, minor={}, major={})".format(self.stack_vendor_oui, self.build, self.rev, self.minor, self.major)


class VendorStackVersionFactory:

    def parse(self, data):
        stack_vendor_oui = struct.unpack(">H", data.read(2))[0]
        rest = struct.unpack(">BBBB", data.read(4))
        build = rest[1] << 4 | (0xf0 & rest[2])
        rev = 0xf & rest[2]
        minor = rest[3] & 0xf0
        major = rest[3] & 0xf
        return VendorStackVersion(stack_vendor_oui, build, rev, minor, major)


class ProvisioningUrl(object):

    def __init__(self, url):
        self._url = url

    @property
    def url(self):
        return self._url

    def __repr__(self):
        return "ProvisioningUrl(url={})".format(self.url)


class ProvisioningUrlFactory:

    def parse(self, data):
        url = data.decode('utf-8')
        return ProvisioningUrl(url)


class VendorData(object):

    def __init__(self, data):
        self._vendor_data = data
    @property
    def vendor_data(self):
        return self._vendor_data

    def __repr__(self):
        return "Vendor(url={})".format(self.vendor_data)


class VendorDataFactory(object):

    def parse(self, data):
        return VendorData(data)


class MeshCopCommand(object):

    def __init__(self, _type, tlvs):
        self._type = _type
        self._tlvs = tlvs

    @property
    def type(self):
        return self._type

    @property
    def tlvs(self):
        return self._tlvs

    def __repr__(self):
        tlvs_str = ", ".join(["{}".format(tlv) for tlv in self.tlvs])
        return "MeshCopCommand(type={}, tlvs=[{}])".format(self.type, tlvs_str)


def create_deault_mesh_cop_msg_type_map():
    return {
        'JOIN_FIN.req': MeshCopMessageType.JOIN_FIN_REQ,
        'JOIN_FIN.rsp': MeshCopMessageType.JOIN_FIN_RSP,
        'JOIN_ENT.ntf': MeshCopMessageType.JOIN_ENT_NTF,
        'JOIN_ENT.rsp': MeshCopMessageType.JOIN_ENT_RSP
    }


class MeshCopCommandFactory:

    def __init__(self, tlvs_factories):
        self._tlvs_factories = tlvs_factories
        self._mesh_cop_msg_type_map = create_deault_mesh_cop_msg_type_map()

    def _get_length(self, data):
        return ord(data.read(1))

    def _get_tlv_factory(self, _type):
        try:
            return self._tlvs_factories[_type]
        except KeyError:
            raise KeyError("Could not find TLV factory. Unsupported TLV type: {}".format(_type))

    def _parse_tlv(self, data):
        _type = TlvType(ord(data.read(1)))
        length = self._get_length(data)
        value = data.read(length)
        factory = self._get_tlv_factory(_type)
        if factory == None:
            return None
        return factory.parse(io.BytesIO(value))

    def _get_mesh_cop_msg_type(self, msg_type_str):
        tp = self._mesh_cop_msg_type_map[msg_type_str]
        if tp == None:
            raise RuntimeError('Mesh cop message type not found: {}'.format(msg_type_str))
        return tp

    def parse(self, cmd_type_str, data):
        cmd_type = self._get_mesh_cop_msg_type(cmd_type_str)

        tlvs = []
        while data.tell() < len(data.getvalue()):
            tlv = self._parse_tlv(data)
            tlvs.append(tlv)

        return MeshCopCommand(cmd_type, tlvs)


def create_default_mesh_cop_tlv_factories():
    return {
        TlvType.STATE: StateFactory(),
        TlvType.PROVISIONING_URL: ProvisioningUrlFactory(),
        TlvType.VENDOR_NAME: VendorNameFactory(),
        TlvType.VENDOR_MODEL: VendorModelFactory(),
        TlvType.VENDOR_SW_VERSION: VendorSWVersionFactory(),
        TlvType.VENDOR_DATA: VendorDataFactory(),
        TlvType.VENDOR_STACK_VERSION: VendorStackVersionFactory()
    }


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
