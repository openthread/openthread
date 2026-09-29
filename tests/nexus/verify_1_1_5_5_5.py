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
    # 5.5.5 Split and Merge with REED
    #
    # 5.5.5.1 Topology
    # - Test topology has a total of 16 active routers, including the Leader. Router_1 is restricted only to
    #   communicate with Router_3 and the DUT.
    #
    # 5.5.5.2 Purpose & Description
    # The purpose of this test case is to show that the DUT will upgrade to a Router when Router_3 is eliminated.
    #
    # Spec Reference             | V1.1 Section | V1.3.0 Section
    # ---------------------------|--------------|---------------
    # Thread Network Partitions  | 5.16         | 5.16

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_2 = pv.vars['ROUTER_2']
    REED_1 = pv.vars['REED_1']

    # Step 1: All
    # - Description: Ensure topology is formed correctly without the DUT.
    # - Pass Criteria: N/A
    print("Step 1: All")

    # Step 2: REED_1 (DUT)
    # - Description: The DUT is added to the topology. Harness filters are set to limit the DUT to attach to Router_2.
    # - Pass Criteria: The DUT MUST NOT attempt to become an active router by sending an Address Solicit Request.
    print("Step 2: REED_1 (DUT)")
    pkts.filter_wpan_src64(REED_1).\
      filter_wpan_dst64(ROUTER_2).\
      filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
      must_next()
    step2_end = pkts.index

    # Step 3: Router_3
    # - Description: Harness instructs the device to powerdown – removing it from the network.
    # - Pass Criteria: N/A
    print("Step 3: Router_3")

    # Step 4: Router_1
    # - Description: Automatically attempt to re-attach to the partition by sending multicast Parent Requests to the
    #   Routers and REEDs address.
    # - Pass Criteria: N/A
    print("Step 4: Router_1")
    step4_start = pkts.index
    pkts.filter_wpan_src64(ROUTER_1).\
      filter_LLARMA().\
      filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
      must_next()

    # Step 2 (verify): The DUT MUST NOT attempt to become an active router by sending an Address Solicit Request.
    pkts.range(step2_end, step4_start).\
      filter_wpan_src64(REED_1).\
      filter_coap_request(consts.ADDR_SOL_URI).\
      must_not_next()

    # Step 5: REED_1 (DUT)
    # - Description: Automatically sends MLE Parent Response to Router_1.
    # - Pass Criteria:
    #   - The DUT MUST send MLE Parent response to Router_1.
    #   - The MLE Parent Response to Router_1 MUST be properly formatted.
    print("Step 5: REED_1 (DUT)")
    pkts.filter_wpan_src64(REED_1).\
      filter_wpan_dst64(ROUTER_1).\
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

    # Step 6: Router_1
    # - Description: Automatically sends MLE Child ID Request to the DUT.
    # - Pass Criteria: N/A
    print("Step 6: Router_1")
    pkts.filter_wpan_src64(ROUTER_1).\
      filter_wpan_dst64(REED_1).\
      filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
      must_next()

    # Step 7: REED_1 (DUT)
    # - Description: Automatically sends an Address Solicit Request to Leader, receives a short address and becomes a
    #   router.
    # - Pass Criteria:
    #   - The Address Solicit Request MUST be properly formatted:
    #     - CoAP Request URI: coap://[<leader address>]:MM/a/as
    #     - CoAP Payload:
    #       - MAC Extended Address TLV
    #       - Status TLV
    #       - RLOC16 TLV (optional)
    print("Step 7: REED_1 (DUT)")
    pkts.range(step4_start, cascade=False).\
      filter_wpan_src64(REED_1).\
      filter_coap_request(consts.ADDR_SOL_URI).\
      filter(lambda p: {
        consts.NL_MAC_EXTENDED_ADDRESS_TLV,
        consts.NL_STATUS_TLV
      } <= set(p.coap.tlv.type)).\
      must_next()

    # Step 8: REED_1 (DUT)
    # - Description: Automatically (optionally) sends multicast Link Request.
    # - Pass Criteria:
    #   - The DUT MAY send a multicast Link Request Message.
    #   - If sent, the following TLVs MUST be present in the Multicast Link Request Message:
    #     - Challenge TLV
    #     - Leader Data TLV
    #     - TLV Request TLV: Link Margin
    #     - Source Address TLV
    #     - Version TLV
    print("Step 8: REED_1 (DUT)")
    link_req = pkts.range(step4_start, cascade=False).\
      filter_wpan_src64(REED_1).\
      filter_LLANMA().\
      filter_mle_cmd(consts.MLE_LINK_REQUEST).\
      filter(lambda p: {
        consts.CHALLENGE_TLV,
        consts.LEADER_DATA_TLV,
        consts.TLV_REQUEST_TLV,
        consts.SOURCE_ADDRESS_TLV,
        consts.VERSION_TLV
      } <= set(p.mle.tlv.type)).\
      next()

    # Step 9: REED_1 (DUT)
    # - Description: Automatically sends Child ID Response to Router_1.
    # - Pass Criteria:
    #   - The DUT MUST send MLE Child ID Response to Router_1.
    #   - The Child ID Response MUST be properly formatted.
    print("Step 9: REED_1 (DUT)")
    pkts.filter_wpan_src64(REED_1).\
      filter_wpan_dst64(ROUTER_1).\
      filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
      filter(lambda p: {
        consts.ADDRESS16_TLV,
        consts.LEADER_DATA_TLV,
        consts.NETWORK_DATA_TLV,
        consts.SOURCE_ADDRESS_TLV
      } <= set(p.mle.tlv.type)).\
      must_next()

    # Step 10: Router_1
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request to the Leader.
    # - Pass Criteria:
    #   - The DUT MUST route the ICMPv6 Echo request to the Leader.
    #   - The DUT MUST route the ICMPv6 Echo reply back to Router_1.
    ROUTER_1_RLOC16 = pv.vars['ROUTER_1_RLOC16']
    ROUTER_2_RLOC16 = pv.vars['ROUTER_2_RLOC16']
    REED_1_RLOC16 = pv.vars['REED_1_RLOC16']

    print("Step 10: Router_1")
    _pkt = pkts.filter_ping_request().\
      filter_wpan_src64(ROUTER_1).\
      filter_wpan_dst16(REED_1_RLOC16).\
      must_next()
    pkts.filter_ping_request().\
      filter_wpan_src64(REED_1).\
      filter_wpan_dst16(ROUTER_2_RLOC16).\
      must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
      filter_wpan_src64(REED_1).\
      filter_wpan_dst16(ROUTER_1_RLOC16).\
      must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
