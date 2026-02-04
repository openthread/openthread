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
from pktverify.null_field import nullField


def verify(pv):
    # 5.1.1 Attaching
    #
    # 5.1.1.1 Topology
    # - Topology A
    # - Topology B
    #
    # 5.1.1.2 Purpose and Description
    # The purpose of this test case is to show that the DUT is able to both form and attach to a network.
    # This test case must be executed twice, first - where the DUT is a Leader and forms a network,
    # and second - where the DUT is a router and attaches to a network.
    #
    # Spec Reference: Attaching to a Parent
    # V1.1 Section: 4.7.1
    # V1.3.0 Section: 4.5.1

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    LEADER_RLOC16 = pv.vars['LEADER_RLOC16']
    ROUTER = pv.vars['ROUTER']

    # Step 1: Leader
    # - Description: Automatically transmits MLE advertisements.
    # - Pass Criteria:
    #   - Leader is sending properly formatted MLE Advertisements.
    #   - Advertisements MUST be sent with an IP Hop Limit of 255 to the Link-Local All Nodes multicast address (FF02::1).
    #   - The following TLVs MUST be present in the MLE Advertisements:
    #     - Leader Data TLV
    #     - Route64 TLV
    #     - Source Address TLV
    print("Step 1: Leader is sending properly formatted MLE Advertisements.")
    pkts.filter_wpan_src64(LEADER).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter(lambda p: {
                          consts.LEADER_DATA_TLV,
                          consts.ROUTE64_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type) and\
               p.ipv6.hlim == 255).\
        must_next()

    # Step 2: Router_1
    # - Description: Automatically begins the attach process by sending a multicast MLE Parent Request.
    # - Pass Criteria:
    #   - For DUT = Router: The DUT MUST send a MLE Parent Request with an IP Hop Limit of 255 to the Link-Local All Routers multicast address (FF02::2).
    #   - The following TLVs MUST be present in the MLE Parent Request:
    #     - Challenge TLV
    #     - Mode TLV
    #     - Scan Mask TLV = 0x80 (Active Routers)
    #     - Version TLV
    print("Step 2: Router sends a MLE Parent Request")
    pkts.filter_wpan_src64(ROUTER).\
        filter_LLARMA().\
        filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        filter(lambda p: {
                          consts.CHALLENGE_TLV,
                          consts.MODE_TLV,
                          consts.SCAN_MASK_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type) and\
               p.ipv6.hlim == 255 and\
               p.mle.tlv.scan_mask.r == 1 and\
               p.mle.tlv.scan_mask.e == 0).\
        must_next()

    # Step 3: Leader
    # - Description: Automatically responds with a MLE Parent Response.
    # - Pass Criteria:
    #   - For DUT = Leader: The DUT MUST unicast a MLE Parent Response to Router_1, including the following TLVs:
    #     - Challenge TLV
    #     - Connectivity TLV
    #     - Leader Data TLV
    #     - Link-layer Frame Counter TLV
    #     - Link Margin TLV
    #     - Response TLV
    #     - Source Address TLV
    #     - Version TLV
    #     - MLE Frame Counter TLV (optional)
    print("Step 3: Leader responds with a MLE Parent Response.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(ROUTER).\
        filter_mle_cmd(consts.MLE_PARENT_RESPONSE).\
        filter(lambda p: {
                          consts.CHALLENGE_TLV,
                          consts.CONNECTIVITY_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.LINK_LAYER_FRAME_COUNTER_TLV,
                          consts.LINK_MARGIN_TLV,
                          consts.RESPONSE_TLV,
                          consts.SOURCE_ADDRESS_TLV,
                          consts.VERSION_TLV
                           } <= set(p.mle.tlv.type)).\
               must_next()

    # Step 4: Router_1
    # - Description: Automatically responds to the MLE Parent Response by sending a MLE Child ID Request.
    # - Pass Criteria:
    #   - For DUT=Router: The DUT MUST unicast MLE Child ID Request to the Leader, including the following TLVs:
    #     - Link-layer Frame Counter TLV
    #     - Mode TLV
    #     - Response TLV
    #     - Timeout TLV
    #     - TLV Request TLV
    #       - Address16 TLV
    #       - Network Data TLV
    #       - Route64 TLV (optional)
    #     - Version TLV
    #     - MLE Frame Counter TLV (optional)
    #   - The following TLV MUST NOT be present in the MLE Child ID Request:
    #     - Address Registration TLV
    print("Step 4: Router sends a MLE Child ID Request.")
    _pkt = pkts.filter_wpan_src64(ROUTER).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter(lambda p: {
                          consts.LINK_LAYER_FRAME_COUNTER_TLV,
                          consts.MODE_TLV,
                          consts.RESPONSE_TLV,
                          consts.TIMEOUT_TLV,
                          consts.TLV_REQUEST_TLV,
                          consts.ADDRESS16_TLV,
                          consts.NETWORK_DATA_TLV,
                          consts.VERSION_TLV
                } <= set(p.mle.tlv.type) and\
               p.mle.tlv.addr16 is nullField and\
               p.thread_nwd.tlv.type is nullField).\
               must_next()
    _pkt.must_not_verify(lambda p: (consts.ADDRESS_REGISTRATION_TLV) in p.mle.tlv.type)

    # Step 5: Leader
    # - Description: Automatically unicasts a MLE Child ID Response.
    # - Pass Criteria:
    #   - For DUT=Leader: The DUT MUST unicast MLE Child ID Response to Router_1, including the following TLVs:
    #     - Address16 TLV
    #     - Leader Data TLV
    #     - Network Data TLV
    #     - Source Address TLV
    #     - Route64 TLV (if requested)
    print("Step 5: Leader responds with a Child ID Response.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(ROUTER).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter(lambda p: {
                          consts.ADDRESS16_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.NETWORK_DATA_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type)).\
               must_next()

    # Step 6: Router_1
    # - Description: Automatically sends an Address Solicit Request.
    # - Pass Criteria:
    #   - For DUT = Router: The DUT MUST send an Address Solicit Request with the following format:
    #     - CoAP Request URI: coap://<leader address>:MM/a/as
    #     - CoAP Payload:
    #       - MAC Extended Address TLV
    #       - Status TLV
    print("Step 6: Router sends an Address Solicit Request.")
    _pkt = pkts.filter_wpan_src64(ROUTER).\
        filter_wpan_dst16(LEADER_RLOC16).\
        filter_coap_request(consts.ADDR_SOL_URI).\
        filter(lambda p: {
                          consts.NL_MAC_EXTENDED_ADDRESS_TLV,
                          consts.NL_STATUS_TLV
                          } <= set(p.coap.tlv.type)\
               ).\
       must_next()

    # Step 7: Leader
    # - Description: Automatically sends an Address Solicit Response.
    # - Pass Criteria:
    #   - For DUT = Leader: The DUT MUST send an Address Solicit Response with the following format:
    #     - CoAP Response Code: 2.04 Changed
    #     - CoAP Payload:
    #       - Status TLV (value = 0 [Success])
    #       - RLOC16 TLV
    #       - Router Mask TLV
    print("Step 7: Leader sends an Address Solicit Response.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst16(_pkt.wpan.src16).\
        filter_coap_ack(consts.ADDR_SOL_URI).\
        filter(lambda p: {
                          consts.NL_STATUS_TLV,
                          consts.NL_RLOC16_TLV,
                          consts.NL_ROUTER_MASK_TLV
                          } <= set(p.coap.tlv.type) and\
               p.coap.code == consts.COAP_CODE_ACK and\
               p.coap.tlv.status == 0\
               ).\
        must_next()

    # Step 8: Router_1
    # - Description: Automatically multicasts a Link Request Message (optional).
    # - Pass Criteria:
    #   - For DUT = Router: The DUT MAY send a multicast Link Request Message, including the following TLVs:
    #     - Challenge TLV
    #     - Leader Data TLV
    #     - Source Address TLV
    #     - Version TLV
    #     - TLV Request TLV: Link Margin
    print("Step 8: Router MAY send a multicast Link Request Message (optional).")
    link_request = pkts.filter_wpan_src64(ROUTER).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_LINK_REQUEST).\
        filter(lambda p: {
                          consts.CHALLENGE_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.SOURCE_ADDRESS_TLV,
                          consts.VERSION_TLV,
                          consts.TLV_REQUEST_TLV
                          } <= set(p.mle.tlv.type)).\
        next()

    # Step 9: Leader
    # - Description: Automatically unicasts a Link Accept message (conditional).
    # - Pass Criteria:
    #   - For DUT = Leader: In the previous step, if Router_1 sent a Link Request Message, then the DUT MUST send a unicast Link Accept Message to Router_1 including the following TLVs:
    #     - Leader Data TLV
    #     - Link-layer Frame Counter TLV
    #     - Link Margin TLV
    #     - Response TLV
    #     - Source Address TLV
    #     - Version TLV
    #     - Challenge TLV (optional)
    #     - MLE Frame Counter TLV (optional)
    if link_request:
        print("Step 9: Leader responds with a Link Accept message.")
        pkts.filter_wpan_src64(LEADER).\
            filter_wpan_dst64(ROUTER).\
            filter_mle_cmd(consts.MLE_LINK_ACCEPT).\
            filter(lambda p: {
                              consts.LEADER_DATA_TLV,
                              consts.LINK_LAYER_FRAME_COUNTER_TLV,
                              consts.LINK_MARGIN_TLV,
                              consts.RESPONSE_TLV,
                              consts.SOURCE_ADDRESS_TLV,
                              consts.VERSION_TLV
                              } <= set(p.mle.tlv.type)).\
            must_next()
    else:
        print("Step 9: Link Request not sent, skipping Link Accept verification.")

    # Step 10: Router_1
    # - Description: Automatically transmits MLE advertisements.
    # - Pass Criteria:
    #   - For DUT = Router: The DUT MUST send MLE Advertisements with an IP Hop Limit of 255 to the Link-Local All Nodes multicast address (FF02::1). The following TLVs MUST be present in the MLE Advertisements:
    #     - Leader Data TLV
    #     - Route64 TLV
    #     - Source Address TLV
    print("Step 10: Router is sending properly formatted MLE Advertisements.")
    pkts.filter_wpan_src64(ROUTER).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter(lambda p: {
                          consts.LEADER_DATA_TLV,
                          consts.ROUTE64_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type) and\
               p.ipv6.hlim == 255).\
        must_next()

    # Step 11: Leader Or Router_1 (not the DUT)
    # - Description: Harness verifies connectivity by instructing the reference device to send a ICMPv6 Echo Request to the DUT link-local address.
    # - Pass Criteria:
    #   - The DUT MUST respond with ICMPv6 Echo Reply
    print("Step 11: ICMPv6 Echo Request/Reply")
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(ROUTER).\
        filter_wpan_dst64(LEADER).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst64(ROUTER).\
        must_next()

    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst64(ROUTER).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(ROUTER).\
        filter_wpan_dst64(LEADER).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
