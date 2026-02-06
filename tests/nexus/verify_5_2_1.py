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


def verify(pv):
    # 5.2.1 REED Attach
    #
    # 5.2.1.1 Topology
    # - Leader
    # - Router_1 (DUT)
    # - REED_1
    # - MED_1
    #
    # 5.2.1.2 Purpose & Description
    # The purpose of this test case is to show that the DUT is able to attach a REED and forward address solicits
    #   two hops away from the Leader.
    #
    # Spec Reference                               | V1.1 Section    | V1.3.0 Section
    # ---------------------------------------------|-----------------|---------------
    # Attaching to a Parent / Router ID Assignment | 4.7.1 / 5.9.10  | 4.5.1 / 5.9.10

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    LEADER_RLOC16 = pv.vars['LEADER_RLOC16']
    DUT = pv.vars['DUT']
    DUT_RLOC16 = pv.vars['DUT_RLOC16']
    REED_1 = pv.vars['REED_1']
    MED_1 = pv.vars['MED_1']

    # Step 1: Router_1 (DUT)
    # - Description: Attach to Leader and sends properly formatted MLE advertisements.
    # - Pass Criteria:
    #   - The DUT MUST send properly formatted MLE Advertisements.
    #   - MLE Advertisements MUST be sent with an IP Hop Limit of 255 to the Link-Local All Nodes multicast address
    #     (FF02::1).
    #   - The following TLVs MUST be present in MLE Advertisements:
    #     - Source Address TLV
    #     - Leader Data TLV
    #     - Route64 TLV
    print("Step 1: Router_1 (DUT)")
    pkts.filter_wpan_src64(DUT).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter(lambda p: {
                          consts.SOURCE_ADDRESS_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.ROUTE64_TLV
                          } <= set(p.mle.tlv.type) and\
               p.ipv6.hlim == 255).\
        must_next()

    # Step 2: REED_1
    # - Description: Attach REED_1 to DUT; REED_1 automatically sends MLE Parent Request.
    # - Pass Criteria: N/A
    print("Step 2: REED_1")
    pkts.filter_wpan_src64(REED_1).\
        filter_LLARMA().\
        filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        must_next()

    # Step 3: Router_1 (DUT)
    # - Description: Automatically sends an MLE Parent Response.
    # - Pass Criteria:
    #   - The DUT MUST respond with a MLE Parent Response.
    #   - The following TLVs MUST be present in the MLE Parent Response:
    #     - Challenge TLV
    #     - Connectivity TLV
    #     - Leader Data TLV
    #     - Link-layer Frame Counter TLV
    #     - Link Margin TLV
    #     - Response TLV
    #     - Source Address TLV
    #     - Version TLV
    #     - MLE Frame Counter TLV (optional)
    print("Step 3: Router_1 (DUT)")
    pkts.filter_wpan_src64(DUT).\
        filter_wpan_dst64(REED_1).\
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

    # Step 4: Router_1 (DUT)
    # - Description: Automatically sends an MLE Child ID Response.
    # - Pass Criteria:
    #   - The DUT MUST send a MLE Child ID Response.
    #   - The following TLVs MUST be present in the Child ID Response:
    #     - Address16 TLV
    #     - Leader Data TLV
    #     - Network Data TLV
    #     - Source Address TLV
    #     - Route64 TLV (optional)
    print("Step 4: Router_1 (DUT)")
    pkts.filter_wpan_src64(DUT).\
        filter_wpan_dst64(REED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter(lambda p: {
                          consts.ADDRESS16_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.NETWORK_DATA_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 6: MED_1
    # - Description: The harness attaches MED_1 to REED_1.
    # - Pass Criteria: N/A
    print("Step 6: MED_1")
    pkts.filter_wpan_src64(MED_1).\
        filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        must_next()

    # Step 7: REED_1
    # - Description: Automatically sends an Address Solicit Request to DUT.
    # - Pass Criteria: N/A
    print("Step 7: REED_1")
    _pkt_sol = pkts.filter_wpan_src64(REED_1).\
        filter_wpan_dst16(DUT_RLOC16).\
        filter_coap_request(consts.ADDR_SOL_URI).\
        must_next()

    # Step 8: Router_1 (DUT)
    # - Description: Automatically forwards Address Solicit Request to Leader, and forwards Leader's Address Solicit
    #     Response to REED_1.
    # - Pass Criteria:
    #   - The DUT MUST forward the Address Solicit Request to the Leader.
    #   - The DUT MUST forward the Leader's Address Solicit Response to REED_1.
    print("Step 8: Router_1 (DUT)")
    pkts.filter_wpan_src16(DUT_RLOC16).\
        filter_wpan_dst16(LEADER_RLOC16).\
        filter_coap_request(consts.ADDR_SOL_URI).\
        must_next()

    pkts.filter_wpan_src16(LEADER_RLOC16).\
        filter_wpan_dst16(DUT_RLOC16).\
        filter_coap_ack(consts.ADDR_SOL_URI).\
        must_next()

    pkts.filter_wpan_src16(DUT_RLOC16).\
        filter_wpan_dst16(_pkt_sol.wpan.src16).\
        filter_coap_ack(consts.ADDR_SOL_URI).\
        must_next()

    # Step 9: Leader
    # - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to REED_1.
    # - Pass Criteria:
    #   - REED_1 responds with ICMPv6 Echo Reply.
    print("Step 9: Leader")
    _pkt_ping = pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        must_next()

    pkts.filter_ping_reply(identifier=_pkt_ping.icmpv6.echo.identifier).\
        filter_wpan_src64(REED_1).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
