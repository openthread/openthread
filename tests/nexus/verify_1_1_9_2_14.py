#!/usr/bin/env python3
#
#  Copyright (c) 2026, The OpenThread Authors.
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

import sys
import os

# Add the current directory to sys.path to find verify_utils
CUR_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(CUR_DIR)

import verify_utils
from pktverify import consts

# Channel Mask for Channel 20 (1 << (31 - 20) = 1 << 11 = 0x800).
# The implementation uses a big-endian bit mask where bit 0 is MSB.
CONFLICTING_CHANNEL_MASK = bytes([0x00, 0x04, 0x00, 0x00, 0x08, 0x00])


def verify(pv):
    # 9.2.14 PAN ID Query Requests
    #
    # 9.2.14.1 Topology
    #   - Leader_2 forms a separate network on the Secondary channel, with the same PAN ID.
    #
    # 9.2.14.2 Purpose & Description
    #   The purpose of this test case is to ensure that the DUT is able to properly accept and process PAN ID
    #   Query requests and properly respond when a conflict is found.
    #
    # Spec Reference            | V1.1 Section | V1.3.0 Section
    # --------------------------|--------------|---------------
    # Avoiding PAN ID Conflicts | 8.7.9        | 8.7.9

    pkts = pv.pkts
    pv.summary.show()

    ROUTER_1 = pv.vars['ROUTER_1']
    COMMISSIONER = pv.vars['COMMISSIONER']
    ROUTER_1_RLOC = pv.vars['ROUTER_1_RLOC']
    COMMISSIONER_RLOC = pv.vars['COMMISSIONER_RLOC']
    COMMISSIONER_MLEID = pv.vars['COMMISSIONER_MLEID']
    ROUTER_1_MLEID = pv.vars['ROUTER_1_MLEID']

    # Step 1: All
    #   - Description: Topology Ensure topology is formed correctly.
    #   - Pass Criteria: N/A
    print("Step 1: All")

    # Step 2: Commissioner
    #   - Description: Harness instructs the Commissioner to send a unicast MGMT_PANID_QUERY.qry to Router_1. For
    #     DUT = Commissioner: Through implementation-specific means, the user instructs the DUT to send a
    #     MGMT_PANID_QUERY.qry to Router_1.
    #   - Pass Criteria: For DUT = Commissioner: The DUT MUST send a unicast MGMT_PANID_QUERY.qry unicast to
    #     Router_1:
    #     - CoAP Request URI: coap://[R]:MM/c/pq
    #     - CoAP Payload:
    #       - Commissioner Session ID TLV
    #       - Channel Mask TLV
    #       - PAN ID TLV
    print("Step 2: Commissioner")
    pkts.filter_wpan_src64(COMMISSIONER).\
        filter_ipv6_dst(ROUTER_1_RLOC).\
        filter_coap_request(consts.MGMT_PANID_QUERY).\
        filter(lambda p: {
                          consts.NM_COMMISSIONER_SESSION_ID_TLV,
                          consts.NM_CHANNEL_MASK_TLV,
                          consts.NM_PAN_ID_TLV
                          } <= set(p.coap.tlv.type)).\
        must_next()

    # Step 3: Router_1
    #   - Description: Automatically sends a MGMT_PANID_CONFLICT.ans reponse to the Commissioner.
    #   - Pass Criteria: For DUT = Router: The DUT MUST send MGMT_ED_REPORT.ans to the Commissioner:
    #     - CoAP Request URI: coap://[Commissioner]:MM/c/pc
    #     - CoAP Payload:
    #       - Channel Mask TLV
    #       - PAN ID TLV
    print("Step 3: Router_1")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_ipv6_dst(COMMISSIONER_RLOC).\
        filter_coap_request(consts.MGMT_PANID_CONFLICT).\
        filter(lambda p: {
                          consts.NM_CHANNEL_MASK_TLV,
                          consts.NM_PAN_ID_TLV
                          } <= set(p.coap.tlv.type)).\
        filter(lambda p: p.coap.tlv.channel_mask == CONFLICTING_CHANNEL_MASK).\
        must_next()

    # Step 4: Commissioner
    #   - Description: Harness instructs Commissioner to send MGMT_PANID_QUERY.qry to All Thread Node Multicast
    #     Address: FF33:0040:<mesh local prefix>::1. For DUT = Commissioner: Through implementation-specific
    #     means, the user instructs the DUT to send a MGMT_PANID_QUERY.qry.
    #   - Pass Criteria: For DUT = Commissioner: The DUT MUST send a multicast MGMT_PANID_QUERY.qry
    #     - CoAP Request URI: coap://[Destination]:MM/c/pq
    #     - CoAP Payload:
    #       - Commissioner Session ID TLV
    #       - Channel Mask TLV
    #       - PAN ID TLV
    print("Step 4: Commissioner")
    pkts.filter_wpan_src64(COMMISSIONER).\
        filter_coap_request(consts.MGMT_PANID_QUERY).\
        filter(lambda p: str(p.ipv6.dst).startswith('ff33:0040:')).\
        filter(lambda p: {
                          consts.NM_COMMISSIONER_SESSION_ID_TLV,
                          consts.NM_CHANNEL_MASK_TLV,
                          consts.NM_PAN_ID_TLV
                          } <= set(p.coap.tlv.type)).\
        must_next()

    # Step 5: Router_1
    #   - Description: Automatically sends a MGMT_PANID_CONFLICT.ans reponse to the Commissioner.
    #   - Pass Criteria: For DUT = Router: The DUT MUST send MGMT_PANID_CONFLICT.ans to the Commissioner:
    #     - CoAP Request URI: coap://[Commissioner]:MM/c/pc
    #     - CoAP Payload:
    #       - Channel Mask TLV
    #       - PAN ID TLV
    print("Step 5: Router_1")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_ipv6_dst(COMMISSIONER_RLOC).\
        filter_coap_request(consts.MGMT_PANID_CONFLICT).\
        filter(lambda p: {
                          consts.NM_CHANNEL_MASK_TLV,
                          consts.NM_PAN_ID_TLV
                          } <= set(p.coap.tlv.type)).\
        filter(lambda p: p.coap.tlv.channel_mask == CONFLICTING_CHANNEL_MASK).\
        must_next()

    # Step 6: Commissioner
    #   - Description: Verify connectivity by sending an ICMPv6 Echo Request to the DUT mesh local address.
    #   - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
    print("Step 6: Commissioner")
    _pkt = pkts.filter_ping_request().\
        filter_ipv6_src(COMMISSIONER_MLEID).\
        filter_ipv6_dst(ROUTER_1_MLEID).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_ipv6_src(ROUTER_1_MLEID).\
        filter_ipv6_dst(COMMISSIONER_MLEID).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
