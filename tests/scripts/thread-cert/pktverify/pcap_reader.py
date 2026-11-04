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
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS'
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
import logging
import os
import subprocess
from typing import Optional

import pyshark
from pyshark.packet.fields import LayerField, LayerFieldsContainer

from pktverify import consts, utils
from pktverify.packet import Packet
from pktverify.packet_filter import PacketFilter


class PcapReader(object):
    """
    Implements Pcap reading utilities.
    """

    @classmethod
    def read(cls,
             filename: str,
             override_prefs: Optional[dict] = None,
             tshark_path: Optional[str] = None) -> PacketFilter:
        """
        Read packets from a given Pcap file.

        :param filename: The Pcap file.
        :param override_prefs: Preferences settings for wireshark
        :param tshark_path: The optional path to the `tshark`.
        :return: A PacketFilter containing all packets of the Pcap file.
        """
        if override_prefs is None:
            override_prefs = consts.WIRESHARK_OVERRIDE_PREFS
        if tshark_path is None:
            tshark_path = utils.which_tshark()

        logging.info("Using tshark path: %s", tshark_path)
        subprocess.check_call(f"{tshark_path} -v", shell=True)
        os.system(f"ls -l {filename}")
        filecap = pyshark.FileCapture(filename,
                                      tshark_path=tshark_path,
                                      override_prefs=override_prefs,
                                      decode_as=consts.WIRESHARK_DECODE_AS_ENTRIES)
        filecap.load_packets()
        cls._fix_coap_uri_path_recon(filecap._packets)
        return PacketFilter(tuple(map(Packet, filecap._packets)))

    @staticmethod
    def _fix_coap_uri_path_recon(packets) -> None:
        """
        Fix `coap.opt.uri_path_recon` on CoAP responses when a CoAP token is
        reused across multiple requests in the same conversation.

        Wireshark's CoAP dissector keys its per-conversation transaction state
        by token and only records `uri_path` when a token is first seen, so a
        later request reusing the same token causes its response to inherit the
        earlier request's `coap.opt.uri_path_recon`.
        """
        uri_by_frame = {}
        uri_by_token = {}

        for idx, raw_pkt in enumerate(packets, 1):
            if not hasattr(raw_pkt, 'coap'):
                continue

            frame_num = int(getattr(raw_pkt, 'number', idx))

            for coap_layer in raw_pkt.get_multiple_layers('coap'):
                token_field = coap_layer.get_field('coap.token')
                token = token_field.get_default_value() if token_field is not None else None

                if coap_layer.get_field('coap.opt.uri_path') is not None:
                    uri_field = coap_layer.get_field('coap.opt.uri_path_recon')
                    if uri_field is not None:
                        uri = uri_field.get_default_value()
                        uri_by_frame[frame_num] = uri
                        if token is not None:
                            uri_by_token[token] = uri
                else:
                    req_uri = None
                    resp_to_field = coap_layer.get_field('coap.response_to')
                    if resp_to_field is not None:
                        try:
                            req_uri = uri_by_frame.get(int(resp_to_field.get_default_value()))
                        except (ValueError, TypeError):
                            pass
                    if req_uri is None and token is not None:
                        req_uri = uri_by_token.get(token)
                    if req_uri is not None:
                        coap_layer._all_fields['coap.opt.uri_path_recon'] = LayerFieldsContainer(
                            main_field=LayerField(name='coap.opt.uri_path_recon', value=req_uri))


if __name__ == '__main__':
    pkts = PcapReader.read("../../../../merged.pcap")
    print(len(pkts), 'packets loaded')
    for p in pkts:
        if not p.mle:
            break

        print("mle.cmd=%r" % p.mle.cmd)
