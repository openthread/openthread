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
import struct
from typing import Tuple, List

from pktverify.addrs import ExtAddr, Ipv6Addr
from pktverify.consts import COAP_CODE_POST, COAP_CODE_ACK
from pktverify.layers import Layer


class CoapTlvParser(object):

    @staticmethod
    def _parse_0(v: bytearray) -> List[Tuple[str, str]]:
        """parse Target EID TLV"""
        return [('target_eid', CoapTlvParser._parse_ipv6_address(v))]

    @staticmethod
    def _parse_1(v: bytearray) -> List[Tuple[str, str]]:
        """parse MAC Extended Address TLV"""
        return [('ext_mac_addr', CoapTlvParser._parse_ext_mac_addr(v))]

    @staticmethod
    def _parse_2(v: bytearray) -> List[Tuple[str, str]]:
        """parse RLOC16 TLV"""
        return [('rloc16', CoapTlvParser._parse_uint16(v))]

    @staticmethod
    def _parse_3(v: bytearray) -> List[Tuple[str, str]]:
        """parse ML-EID TLV"""
        return [('ml_eid', CoapTlvParser._parse_ext_mac_addr(v))]

    @staticmethod
    def _parse_4(v: bytearray) -> List[Tuple[str, str]]:
        """parse Status TLV"""
        assert len(v) == 1
        return [('status', hex(v[0]))]

    @staticmethod
    def _parse_6(v: bytearray) -> List[Tuple[str, str]]:
        """parse Time Since Last Transaction TLV"""
        return [('last_transaction_time', CoapTlvParser._parse_uint32(v))]

    @staticmethod
    def _parse_7(v: bytearray) -> List[Tuple[str, str]]:
        """parse Router Mask TLV"""
        assert len(v) == 9
        return [
            ('router_mask_id_seq', hex(v[0])),
            ('router_mask_assigned', CoapTlvParser._parse_uint64(v[1:])),
        ]

    @staticmethod
    def _parse_10(v: bytearray) -> List[Tuple[str, str]]:
        """parse Thread Network Data TLV"""
        # TODO: Thread Network Data can not be parsed by COAP TLVs yet
        return []

    @staticmethod
    def _parse_12(v: bytearray) -> List[Tuple[str, str]]:
        """parse Network Name TLV"""
        return [('net_name', CoapTlvParser._parse_utf8_str(v))]

    @staticmethod
    def _parse_uint16(v: bytearray) -> str:
        assert len(v) == 2
        return hex(v[0] * 256 + v[1])

    @staticmethod
    def _parse_uint32(v: bytearray) -> str:
        assert len(v) == 4
        return hex(struct.unpack(">I", v)[0])

    @staticmethod
    def _parse_uint64(v: bytearray) -> str:
        assert len(v) == 8
        return hex(struct.unpack(">Q", v)[0])

    @staticmethod
    def _parse_ipv6_address(s: bytearray):
        assert len(s) == 16
        a = Ipv6Addr(s)
        return a.format_hextets()

    @staticmethod
    def _parse_utf8_str(v: bytearray) -> str:
        return v.decode('utf-8')

    @staticmethod
    def parse(t, v: bytearray) -> str:
        assert isinstance(v, bytearray)
        try:
            parse_func = getattr(CoapTlvParser, f'_parse_{t}')
        except AttributeError:
            raise NotImplementedError(f"Please implement _parse_{t} for COAP TLV: type={t}")

        return parse_func(v)

    @staticmethod
    def _parse_ext_mac_addr(v: bytearray) -> str:
        assert len(v) == 8
        return ExtAddr(v).format_octets()


class CoapLayer(Layer):
    """
    Represents a COAP layer.
    """

    def __init__(self, packet, layer_name):
        super().__init__(packet, layer_name)

    @property
    def is_post(self) -> bool:
        """
        Returns if the COAP layer is using code POST.
        """
        return self.code == COAP_CODE_POST

    @property
    def is_ack(self) -> bool:
        """
        Returns if the COAP layer is using code ACK.
        """
        return self.code == COAP_CODE_ACK

    def __getattr__(self, name):
        super_attr = super().__getattr__(name)
        if name == 'tlv':
            self._parse_coap_payload()

        return super_attr

    def _parse_coap_payload(self):
        payload = self.payload

        r = 0
        while True:
            t, tvs, r = self._parse_next_tlv(payload, r)
            if t is None:
                break

            self._add_field('coap.tlv.type', hex(t))
            for k, v in tvs:
                assert isinstance(k, str), (t, k, v)
                assert isinstance(v, str), (t, k, v)
                self._add_field('coap.tlv.' + k, v)

    @staticmethod
    def _parse_next_tlv(payload, read_pos) -> tuple:
        assert read_pos <= len(payload)
        if read_pos == len(payload):
            return None, None, read_pos

        t = payload[read_pos]
        len_ = payload[read_pos + 1]
        assert (len(payload) - read_pos - 2 >= len_)
        kvs = CoapTlvParser.parse(t, payload[read_pos + 2:read_pos + 2 + len_])
        return t, kvs, read_pos + len_ + 2
