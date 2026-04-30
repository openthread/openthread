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
    # 5.1.13 Router Synchronization after Reset
    #
    # 5.1.13.1 Topology
    # - Topology A
    # - Topology B
    #
    # 5.1.13.2 Purpose & Description
    # The purpose of this test case is to validate that when a router resets, it will synchronize upon returning by
    #   using the Router Synchronization after Reset procedure.
    #
    # Spec Reference                     | V1.1 Section | V1.3.0 Section
    # -----------------------------------|--------------|---------------
    # Router Synchronization after Reset | 4.7.7.3      | 4.7.1.3

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']

    # Step 1: All
    # - Description: Verify topology is formed correctly.
    # - Pass Criteria: N/A
    print("Step 1: All")

    # Step 2: Router_1 / Leader
    # - Description: Automatically transmit MLE advertisements.
    # - Pass Criteria:
    #   - Devices MUST send properly formatted MLE Advertisements with an IP Hop Limit of 255 to the Link-Local All
    #     Nodes multicast address (FF02::1).
    #   - The following TLVs MUST be present in the Advertisements:
    #     - Leader Data TLV
    #     - Route64 TLV
    #     - Source Address TLV
    print("Step 2: Router_1 / Leader")

    # Leader Advertisements
    pkts.filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter_LLANMA().\
        filter_wpan_src64(LEADER).\
        filter(lambda p: {
                          consts.LEADER_DATA_TLV,
                          consts.ROUTE64_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type) and\
               p.ipv6.hlim == 255 and\
               p.mle.tlv.route64.id_mask is not nullField).\
        must_next()

    # Router_1 Advertisements
    pkts.filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter_LLANMA().\
        filter_wpan_src64(ROUTER_1).\
        filter(lambda p: {
                          consts.LEADER_DATA_TLV,
                          consts.ROUTE64_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type) and\
               p.ipv6.hlim == 255 and\
               p.mle.tlv.route64.id_mask is not nullField).\
        must_next()

    # Step 3: Router_1
    # - Description: Harness silently resets the device.
    # - Pass Criteria: N/A
    print("Step 3: Router_1")

    # Step 4: Router_1
    # - Description: Automatically sends multicast Link Request message.
    # - Pass Criteria:
    #   - For DUT = Router: The Link Request message MUST be sent to the Link-Local All Routers multicast address
    #     (FF02::2).
    #   - The following TLVs MUST be present in the Link Request message:
    #     - Challenge TLV
    #     - TLV Request TLV
    #       - Address16 TLV
    #       - Route64 TLV
    #     - Version TLV
    print("Step 4: Router_1")

    # Note: Address16 and Route64 TLVs are requested in TLV Request TLV, and Wireshark dissects their types into
    # mle.tlv.type list. We also verify that they are NOT present as top-level TLVs (since the router has reset).
    pkt_lq = pkts.filter_mle_cmd(consts.MLE_LINK_REQUEST).\
        filter_LLARMA().\
        filter_wpan_src64(ROUTER_1).\
        filter(lambda p: {
                          consts.CHALLENGE_TLV,
                          consts.TLV_REQUEST_TLV,
                          consts.VERSION_TLV,
                          consts.ADDRESS16_TLV,
                          consts.ROUTE64_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()

    pkt_lq.must_verify(lambda p: p.mle.tlv.addr16 is nullField and\
                                 p.mle.tlv.route64.id_mask is nullField)

    # Step 5: Leader
    # - Description: Automatically replies to Router_1 with Link Accept message.
    # - Pass Criteria:
    #   - For DUT = Leader: The following TLVs MUST be present in the Link Accept Message:
    #     - Address16 TLV
    #     - Leader Data TLV
    #     - Link-layer Frame Counter TLV
    #     - Response TLV
    #     - Route64 TLV
    #     - Source Address TLV
    #     - Version TLV
    #     - Challenge TLV (situational - MUST be included if the response is an Accept and Request message)
    #     - MLE Frame Counter TLV (optional)
    #   - Responses to multicast Link Requests MUST be delayed by a random time of up to MLE_MAX_RESPONSE_DELAY (1
    #     second).
    print("Step 5: Leader")

    ROUTER_1_RLOC16 = pv.vars['ROUTER_1_RLOC16']

    pkt_la = pkts.filter_mle_cmd2(consts.MLE_LINK_ACCEPT, consts.MLE_LINK_ACCEPT_AND_REQUEST).\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst64(ROUTER_1).\
        filter(lambda p: p.sniff_timestamp - pkt_lq.sniff_timestamp < consts.MLE_MAX_RESPONSE_DELAY + 0.1 and\
                         {
                          consts.ADDRESS16_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.LINK_LAYER_FRAME_COUNTER_TLV,
                          consts.RESPONSE_TLV,
                          consts.ROUTE64_TLV,
                          consts.SOURCE_ADDRESS_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type) and\
               p.mle.tlv.addr16 == ROUTER_1_RLOC16 and\
               p.mle.tlv.route64.id_mask is not nullField).\
        must_next()

    if pkt_la.mle.cmd == consts.MLE_LINK_ACCEPT_AND_REQUEST:
        pkt_la.must_verify(lambda p: consts.CHALLENGE_TLV in p.mle.tlv.type)


if __name__ == '__main__':
    verify_utils.run_main(verify)
