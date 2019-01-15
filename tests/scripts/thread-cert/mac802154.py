#!/usr/bin/env python
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

"""
    This module provides simple 802.15.4 MAC parser.
"""

import io
import struct

import config
from common import MacAddress, MacAddressType, MessageInfo
from net_crypto import AuxiliarySecurityHeader, CryptoEngine, MacCryptoMaterialCreator


class DeviceDescriptors:

    """Class representing 802.15.4 Device Descriptors."""

    device_descriptors = {}

    @classmethod
    def add(cls, short_address, extended_address):
        short_address = cls._get_short_address_value(short_address)
        cls.device_descriptors[short_address] = extended_address

    @classmethod
    def get_extended(cls, short_address):
        short_address = cls._get_short_address_value(short_address)
        return cls.device_descriptors[short_address]

    @staticmethod
    def _get_short_address_value(short_address):
        if isinstance(short_address, MacAddress):
            short_address = short_address.rloc
        return short_address


class MacHeader:

    """Class representing 802.15.4 MAC header."""

    class FrameType:
        BEACON = 0
        DATA = 1
        ACK = 2
        COMMAND = 3

    class AddressMode:
        NOT_PRESENT = 0
        SHORT = 2
        EXTENDED = 3

    class CommandIdentifier:
        DATA_REQUEST = 4

    def __init__(self, frame_type, frame_pending, ack_request, frame_version, seq,
                 dest_pan_id=None, dest_address=None, src_pan_id=None, src_address=None, command_type=None,
                 aux_sec_header=None, mic=None,
                 fcs=None):

        self.frame_type = frame_type
        self.frame_pending = frame_pending
        self.ack_request = ack_request
        self.frame_version = frame_version
        self.seq = seq

        self.dest_pan_id = dest_pan_id
        self.dest_address = dest_address
        self.src_pan_id = src_pan_id
        self.src_address = src_address
        self.command_type = command_type

        self.aux_sec_header = aux_sec_header
        self.mic = mic

        self.fcs = fcs


class MacPayload:

    """Class representing 802.15.4 MAC payload."""

    def __init__(self, data):
        self.data = bytearray(data)


class MacFrame:

    """Class representing 802.15.4 MAC frame."""

    def parse(self, data):
        mhr_start = data.tell()

        fc, seq = struct.unpack("<HB", data.read(3))

        frame_type = fc & 0x0007
        security_enabled = bool(fc & 0x0008)
        frame_pending = bool(fc & 0x0010)
        ack_request = bool(fc & 0x0020)
        panid_compression = bool(fc & 0x0040)
        dest_addr_mode = (fc & 0x0c00) >> 10
        frame_version = (fc & 0x3000) >> 12
        source_addr_mode = (fc & 0xc000) >> 14

        if frame_type == MacHeader.FrameType.ACK:
            fcs = self._parse_fcs(data, data.tell())
            self.header = MacHeader(frame_type, frame_pending, ack_request, frame_version, seq, fcs=fcs)
            self.payload = None
            return

        # Presence of PAN Ids is not fully implemented yet but should be enough for Thread.
        dest_pan_id = struct.unpack("<H", data.read(2))[0]

        dest_address = self._parse_address(data, dest_addr_mode)

        if not panid_compression:
            src_pan_id = struct.unpack("<H", data.read(2))[0]
        else:
            src_pan_id = dest_pan_id

        src_address = self._parse_address(data, source_addr_mode)

        mhr_end = data.tell()

        if security_enabled:
            aux_sec_header = self._parse_aux_sec_header(data)
            aux_sec_header_end = data.tell()
        else:
            aux_sec_header = None

        # Check end of MAC frame
        if frame_type == MacHeader.FrameType.COMMAND:
            command_type = ord(data.read(1))
        else:
            command_type = None

        payload_pos = data.tell()

        data.seek(-2, io.SEEK_END)
        fcs_start = data.tell()

        if aux_sec_header and aux_sec_header.security_level:
            mic, payload_end = self._parse_mic(data, aux_sec_header.security_level)
        else:
            payload_end = data.tell()
            mic = None

        fcs = self._parse_fcs(data, fcs_start)

        # Create Header object
        self.header = MacHeader(frame_type, frame_pending, ack_request, frame_version, seq,
                                dest_pan_id, dest_address, src_pan_id, src_address, command_type,
                                aux_sec_header, mic,
                                fcs)

        # Create Payload object
        payload_len = payload_end - payload_pos
        data.seek(payload_pos)

        payload = data.read(payload_len)

        if security_enabled:
            mhr_len = mhr_end - mhr_start
            data.seek(mhr_start)
            mhr_bytes = data.read(mhr_len)

            aux_sec_header_len = aux_sec_header_end - mhr_end
            aux_sec_hdr_bytes = data.read(aux_sec_header_len)

            non_payload_fields = bytearray([])

            if command_type is not None:
                non_payload_fields.append(command_type)

            message_info = MessageInfo()
            message_info.aux_sec_hdr = aux_sec_header
            message_info.aux_sec_hdr_bytes = aux_sec_hdr_bytes
            message_info.nonpayload_fields = non_payload_fields
            message_info.mhr_bytes = mhr_bytes
            if src_address.type == MacAddressType.SHORT:
                message_info.source_mac_address = DeviceDescriptors.get_extended(src_address).mac_address
            else:
                message_info.source_mac_address = src_address.mac_address

            sec_obj = CryptoEngine(MacCryptoMaterialCreator(config.DEFAULT_MASTER_KEY))
            self.payload = MacPayload(sec_obj.decrypt(payload, mic, message_info))

        else:
            self.payload = MacPayload(payload)

    def _parse_address(self, data, mode):
        if mode == MacHeader.AddressMode.SHORT:
            return MacAddress(data.read(2), MacAddressType.SHORT, big_endian=False)

        if mode == MacHeader.AddressMode.EXTENDED:
            return MacAddress(data.read(8), MacAddressType.LONG, big_endian=False)

        else:
            return None

    def _parse_aux_sec_header(self, data):
        security_control, frame_counter = struct.unpack("<BL", data.read(5))

        security_level = security_control & 0x07
        key_id_mode = (security_control & 0x18) >> 3

        if key_id_mode == 0:
            key_id = data.read(9)
        elif key_id_mode == 1:
            key_id = data.read(1)
        elif key_id_mode == 2:
            key_id = data.read(5)
        else:
            key_source = None
            key_index = None

        return AuxiliarySecurityHeader(key_id_mode, security_level, frame_counter, key_id, big_endian=False)

    def _parse_mic(self, data, security_level):
        if security_level in (1, 5):
            data.seek(-4, io.SEEK_CUR)
            payload_end = data.tell()
            mic = data.read(4)
        elif security_level in (2, 6):
            data.seek(-8, io.SEEK_CUR)
            payload_end = data.tell()
            mic = data.read(8)
        elif security_level in (3, 7):
            data.seek(-16, io.SEEK_CUR)
            payload_end = data.tell()
            mic = data.read(16)
        else:
            payload_end = data.tell()

        return mic, payload_end

    def _parse_fcs(self, data, fcs_start):
        data.seek(fcs_start)
        fcs = bytearray(data.read(2))
        return fcs
