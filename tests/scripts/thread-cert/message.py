#!/usr/bin/python
#
#  Copyright (c) 2016, The OpenThread Authors.
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
import ipaddress
import struct
import sys

import coap
import common
import ipv6
import lowpan
import mac802154
import mle

from enum import IntEnum

class MessageType(IntEnum):
    MLE = 0
    COAP = 1
    ICMP = 2
    ACK = 3 
    BEACON = 4
    DATA = 5


class Message(object):

    def __init__(self):
        self._type = None
        self._channel = None
        self._mac_header = None
        self._ipv6_packet = None
        self._coap = None
        self._mle = None
        self._icmp = None

    def _extract_udp_datagram(self, udp_datagram):
        if isinstance(udp_datagram.payload, mle.MleMessage):
            self._type = MessageType.MLE
            self._mle = udp_datagram.payload

        elif isinstance(udp_datagram.payload, (coap.CoapMessage, coap.CoapMessageProxy)):
            self._type = MessageType.COAP
            self._coap = udp_datagram.payload

    def _extract_upper_layer_protocol(self, upper_layer_protocol):
        if isinstance(upper_layer_protocol, ipv6.ICMPv6):
            self._type = MessageType.ICMP
            self._icmp = upper_layer_protocol

        elif isinstance(upper_layer_protocol, ipv6.UDPDatagram):
            self._extract_udp_datagram(upper_layer_protocol)

    @property
    def type(self):
        return self._type

    @type.setter
    def type(self, value):
        self._type = value

    @property
    def channel(self):
        return self._channel

    @channel.setter
    def channel(self, value):
        self._channel = value

    @property
    def mac_header(self):
        return self._mac_header

    @mac_header.setter
    def mac_header(self, value):
        self._mac_header = value

        if self._mac_header.frame_type == mac802154.MacHeader.FrameType.BEACON:
            self._type = MessageType.BEACON

        elif self._mac_header.frame_type == mac802154.MacHeader.FrameType.ACK:
            self._type = MessageType.ACK

        elif self._mac_header.frame_type == mac802154.MacHeader.FrameType.DATA:
            self._type = MessageType.DATA

    @property
    def ipv6_packet(self):
        return self._ipv6_packet

    @ipv6_packet.setter
    def ipv6_packet(self, value):
        self._ipv6_packet = value
        self._extract_upper_layer_protocol(value.upper_layer_protocol)

    @property
    def coap(self):
        return self._coap

    @property
    def mle(self):
        return self._mle

    @property
    def icmp(self):
        return self._icmp

    @icmp.setter
    def icmp(self, value):
        self._icmp = value

    def get_mle_message_tlv(self, tlv_class_type):
        if self.type != MessageType.MLE:
            raise ValueError("Invalid message type. Expected MLE message.")

        for tlv in self.mle.command.tlvs:
            if isinstance(tlv, tlv_class_type):
                return tlv

    def assertMleMessageIsType(self, command_type):
        if self.type != MessageType.MLE:
            raise ValueError("Invalid message type. Expected MLE message.")

        assert(self.mle.command.type == command_type)

    def assertMleMessageContainsTlv(self, tlv_class_type):
        if self.type != MessageType.MLE:
            raise ValueError("Invalid message type. Expected MLE message.")

        contains_tlv = False
        for tlv in self.mle.command.tlvs:
            if isinstance(tlv, tlv_class_type):
                contains_tlv = True
                break

        assert(contains_tlv == True)

    def assertMleMessageDoesNotContainTlv(self, tlv_class_type):
        if self.type != MessageType.MLE:
            raise ValueError("Invalid message type. Expected MLE message.")

        contains_tlv = False
        for tlv in self.mle.command.tlvs:
            if isinstance(tlv, tlv_class_type):
                contains_tlv = True
                break

        assert(contains_tlv == False)

    def assertMleMessageContainsOptionalTlv(self, tlv_class_type):
        if self.type != MessageType.MLE:
            raise ValueError("Invalid message type. Expected MLE message.")

        contains_tlv = False
        for tlv in self.mle.command.tlvs:
            if isinstance(tlv, tlv_class_type):
                contains_tlv = True
                break

        print("MleMessage doesn't contain optional TLV: {}".format(tlv_class_type))

    def get_coap_message_tlv(self, tlv_class_type):
        if self.type != MessageType.COAP:
            raise ValueError("Invalid message type. Expected CoAP message.")

        for tlv in self.coap.payload:
            if isinstance(tlv, tlv_class_type):
                return tlv

    def assertCoapMessageContainsTlv(self, tlv_class_type):
        if self.type != MessageType.COAP:
            raise ValueError("Invalid message type. Expected CoAP message.")

        contains_tlv = False
        for tlv in self.coap.payload:
            if isinstance(tlv, tlv_class_type):
                contains_tlv = True
                break

        assert(contains_tlv == True)

    def assertCoapMessageContainsOptionalTlv(self, tlv_class_type):
        if self.type != MessageType.COAP:
            raise ValueError("Invalid message type. Expected CoAP message.")

        contains_tlv = False
        for tlv in self.coap.payload:
            if isinstance(tlv, tlv_class_type):
                contains_tlv = True
                break

        print("CoapMessage doesn't contain optional TLV: {}".format(tlv_class_type))

    def assertCoapMessageRequestUriPath(self, uri_path):
        if self.type != MessageType.COAP:
            raise ValueError("Invalid message type. Expected CoAP message.")

        assert(uri_path == self.coap.uri_path)

    def assertCoapMessageCode(self, code):
        if self.type != MessageType.COAP:
            raise ValueError("Invalid message type. Expected CoAP message.")

        assert(code == self.coap.code)

    def assertSentToNode(self, node):
        sent_to_node = False
        dst_addr = self.ipv6_packet.ipv6_header.destination_address

        for addr in node.get_addrs():
            if dst_addr == ipaddress.ip_address(addr):
                sent_to_node = True

        assert sent_to_node == True

    def assertSentToDestinationAddress(self, ipv6_address):
        if sys.version_info[0] == 2:
            ipv6_address = ipv6_address.decode("utf-8")

        assert self.ipv6_packet.ipv6_header.destination_address == ipaddress.ip_address(ipv6_address)

    def assertSentWithHopLimit(self, hop_limit):
        assert self.ipv6_packet.ipv6_header.hop_limit == hop_limit

    def __repr__(self):
        return "Message(type={})".format(MessageType(self.type).name)


class MessagesSet(object):

    def __init__(self, messages):
        self._messages = messages

    @property
    def messages(self):
        return self._messages

    def next_coap_message(self, code, uri_path=None, assert_enabled=True):
        message = None

        while self.messages:
            m = self.messages.pop(0)

            if m.type != MessageType.COAP:
                continue

            if uri_path is not None and m.coap.uri_path != uri_path:
                continue

            else:
                if not m.coap.code.is_equal_dotted(code):
                    continue

            message = m
            break

        if assert_enabled:
            assert message is not None, "Could not find CoapMessage with code: {}".format(code)

        return message

    def next_mle_message(self, command_type, assert_enabled=True):
        message = self.next_mle_message_of_one_of_command_types(command_type,)

        if assert_enabled:
            assert message is not None, "Could not find MleMessage of the type: {}".format(command_type)

        return message

    def next_mle_message_of_one_of_command_types(self, *command_types):
        message = None

        while self.messages:
            m = self.messages.pop(0)

            if m.type != MessageType.MLE:
                continue

            command_found = False

            for command_type in command_types:
                if m.mle.command.type == command_type:
                    command_found = True
                    break

            if command_found:
                message = m
                break

        return message

    def contains_mle_message(self, command_type):
        for m in self.messages:
            if m.type != MessageType.MLE:
                continue

            if m.mle.command.type == command_type:
                return True

        return False

    def does_not_contain_coap_message(self):
        for m in self.messages:
            if m.type != MessageType.COAP:
                continue

            return False

        return True


class MessageFactory:

    def __init__(self, lowpan_parser):
        self._lowpan_parser = lowpan_parser

    def _add_device_descriptors(self, message):
        for tlv in message.mle.command.tlvs:

            if isinstance(tlv, mle.SourceAddress):
                mac802154.DeviceDescriptors.add(tlv.address, message.mac_header.src_address)

            if isinstance(tlv, mle.Address16):
                mac802154.DeviceDescriptors.add(tlv.address, message.mac_header.dest_address)

    def _parse_mac_frame(self, data):
        mac_frame = mac802154.MacFrame()
        mac_frame.parse(data)
        return mac_frame

    def create(self, data):
        message = Message()
        message.channel = struct.unpack(">B", data.read(1))

        # Parse MAC header
        mac_frame = self._parse_mac_frame(data)
        message.mac_header = mac_frame.header

        if message.mac_header.frame_type != mac802154.MacHeader.FrameType.DATA:
            return message

        message_info = common.MessageInfo()
        message_info.source_mac_address = message.mac_header.src_address
        message_info.destination_mac_address = message.mac_header.dest_address

        # Create stream with 6LoWPAN datagram
        lowpan_payload = io.BytesIO(mac_frame.payload.data)

        ipv6_packet = self._lowpan_parser.parse(lowpan_payload, message_info)
        if ipv6_packet is None:
            return message

        message.ipv6_packet = ipv6_packet

        if message.type == MessageType.MLE:
            self._add_device_descriptors(message)

        return message
