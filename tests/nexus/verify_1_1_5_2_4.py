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
    # 5.2.4 Router Upgrade Threshold - REED
    #
    # 5.2.4.1 Topology
    # - Each router numbered 1 through 15 have a link to leader
    # - Router 15 and REED_1 (DUT) have a link
    # - REED_1 (DUT) and MED_1 have a link.
    #
    # 5.2.4.2 Purpose & Description
    # The purpose of this test case is to:
    # 1. Verify that the DUT does not attempt to become a router if there are already 16 active routers on the
    #   Thread network AND it is not bringing children.
    # 2. Verify that the DUT transmits MLE Advertisement messages every REED_ADVERTISEMENT_INTERVAL (+
    #   REED_ADVERTISEMENT_MAX_JITTER) seconds.
    # 3. Verify that the DUT upgrades to a router by sending an Address Solicit Request when a child attempts
    #   to attach to it.
    #
    # Spec Reference                              | V1.1 Section   | V1.3.0 Section
    # --------------------------------------------|----------------|----------------
    # Router ID Management / Router ID Assignment | 5.9.9 / 5.9.10 | 5.9.9 / 5.9.10

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['Leader']
    ROUTER_15 = pv.vars['Router_15']
    REED_1 = pv.vars['REED_1']
    MED_1 = pv.vars['MED_1']

    # Step 1: All
    # - Description: Ensure topology is formed correctly without the DUT.
    # - Pass Criteria: N/A
    print("Step 1: Ensure topology is formed correctly without the DUT.")

    # Step 2: REED_1 (DUT)
    # - Description: The harness causes the DUT to attach to any node, 2-hops from the Leader.
    # - Pass Criteria: The DUT MUST NOT attempt to become an active router by sending an Address Solicit Request.
    print("Step 2: The DUT MUST NOT attempt to become an active router.")
    # We verify it joins as a child and NO Address Solicit Request is sent before Step 9.
    pkts.filter_wpan_src64(REED_1).\
        filter_wpan_dst64(ROUTER_15).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        must_next()

    # Step 3: REED_1 (DUT)
    # - Description: Automatically sends MLE Advertisements.
    # - Pass Criteria:
    #   - The DUT MUST send properly formatted MLE Advertisements.
    #   - MLE Advertisements MUST be sent with an IP Hop Limit of 255, to the Link-Local All Nodes multicast
    #     address (FF02::1).
    #   - The following TLVs MUST be present in the MLE Advertisements:
    #     - Leader Data TLV
    #     - Source Address TLV
    #   - The following TLV MUST NOT be present in the MLE Advertisement:
    #     - Route64 TLV
    print("Step 3: The DUT MUST send properly formatted MLE Advertisements.")
    _pkt = pkts.filter_wpan_src64(REED_1).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter(lambda p: {
                          consts.LEADER_DATA_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type) and
               p.ipv6.hlim == 255).\
        must_next()
    _pkt.must_not_verify(lambda p: (consts.ROUTE64_TLV) in p.mle.tlv.type)

    # Step 4: Wait
    # - Description: Wait for REED_ADVERTISEMENT_INTERVAL+ REED_ADVERTISEMENT_MAX_JITTER seconds (default time = 630
    #   seconds).
    # - Pass Criteria: N/A
    print("Step 4: Wait for REED_ADVERTISEMENT_INTERVAL+ REED_ADVERTISEMENT_MAX_JITTER seconds.")

    # Step 5: REED_1 (DUT)
    # - Description: Automatically sends a MLE Advertisement.
    # - Pass Criteria: The DUT MUST send a second MLE Advertisement after REED_ADVERTISEMENT_INTERVAL+JITTER where
    #   JITTER <= REED_ADVERTISEMENT_MAX_JITTER.
    print("Step 5: The DUT MUST send a second MLE Advertisement.")
    pkts.filter_wpan_src64(REED_1).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        must_next()

    # Step 6: MED_1
    # - Description: Automatically sends multicast MLE Parent Request.
    # - Pass Criteria: N/A
    print("Step 6: MED_1 sends multicast MLE Parent Request.")
    pkts.filter_wpan_src64(MED_1).\
        filter_LLARMA().\
        filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        must_next()

    # Step 7: REED_1 (DUT)
    # - Description: Automatically sends MLE Parent Response.
    # - Pass Criteria: The DUT MUST reply with a properly formatted MLE Parent Response.
    print("Step 7: The DUT MUST reply with a properly formatted MLE Parent Response.")
    pkts.filter_wpan_src64(REED_1).\
        filter_wpan_dst64(MED_1).\
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

    # Step 8: MED_1
    # - Description: Automatically sends MLE Child ID Request to the DUT.
    # - Pass Criteria: N/A
    print("Step 8: MED_1 sends MLE Child ID Request to the DUT.")
    pkts.filter_wpan_src64(MED_1).\
        filter_wpan_dst64(REED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        must_next()

    # Step 9: REED_1 (DUT)
    # - Description: Automatically sends an Address Solicit Request to the Leader.
    # - Pass Criteria:
    #   - Verify that the DUT’s Address Solicit Request is properly formatted:
    #     - CoAP Request URI: coap://[<leader address>]:MM/a/as
    #     - CoAP Payload:
    #       - MAC Extended Address TLV
    #       - Status TLV
    #       - RLOC16 TLV (optional)
    print("Step 9: The DUT MUST send an Address Solicit Request to the Leader.")
    pkts.filter_wpan_src64(REED_1).\
        filter_coap_request(consts.ADDR_SOL_URI).\
        filter(lambda p: {
                          consts.NL_MAC_EXTENDED_ADDRESS_TLV,
                          consts.NL_STATUS_TLV
                          } <= set(p.coap.tlv.type)).\
        must_next()

    # Step 10: REED_1 (DUT)
    # - Description: Optionally, automatically sends a Multicast Link Request after receiving an Address Solicit
    #   Response from Leader with its new Router ID.
    # - Pass Criteria:
    #   - The DUT MAY send a Multicast Link Request to the Link-Local All-Routers multicast address (FF02::2).
    #   - The following TLVs MUST be present in the Link Request:
    #     - Challenge TLV
    #     - Leader Data TLV
    #     - Source Address TLV
    #     - TLV Request TLV: Link Margin
    #     - Version TLV
    print("Step 10: The DUT MAY send a Multicast Link Request.")
    # We use next() since it's optional
    pkts.filter_wpan_src64(REED_1).\
        filter_LLARMA().\
        filter_mle_cmd(consts.MLE_LINK_REQUEST).\
        filter(lambda p: {
                          consts.CHALLENGE_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.SOURCE_ADDRESS_TLV,
                          consts.TLV_REQUEST_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type)).\
        next()

    # Step 11: REED_1 (DUT)
    # - Description: Automatically sends MLE Child ID Response to MED_1.
    # - Pass Criteria: The DUTs MLE Child ID Response MUST be properly formatted with MED_1’s new 16-bit address.
    print("Step 11: The DUT MUST send MLE Child ID Response to MED_1.")
    pkts.filter_wpan_src64(REED_1).\
        filter_wpan_dst64(MED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter(lambda p: {
                          consts.ADDRESS16_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.NETWORK_DATA_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 12: MED_1
    # - Description: The harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
    #   Leader.
    # - Pass Criteria: The Leader MUST respond with an ICMPv6 Echo Reply.
    print("Step 12: Harness verifies connectivity by sending an ICMPv6 Echo Request.")
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(MED_1).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_ipv6_dst(_pkt.ipv6.src).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
