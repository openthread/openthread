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
    # 5.5.1 Leader Reboot < timeout
    #
    # 5.5.1.1 Topology
    # - Leader
    # - Router_1
    #
    # 5.5.1.2 Purpose & Description
    # The purpose of this test case is to show that when the Leader is rebooted for a time period shorter than the
    #   leader timeout, it does not trigger network partitioning and remains the leader when it reattaches to the
    #   network.
    #
    # Spec Reference      | V1.1 Section | V1.3.0 Section
    # --------------------|--------------|---------------
    # Losing Connectivity | 5.16.1       | 5.16.1

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A
    print("Step 1: All")

    # Step 2: Leader, Router_1
    # - Description: Transmit MLE advertisements.
    # - Pass Criteria:
    #   - The devices MUST send properly formatted MLE Advertisements.
    #   - Advertisements MUST be sent with an IP Hop Limit of 255 to the Link-Local All Nodes multicast address
    #     (FF02::1).
    #   - The following TLVs MUST be present in MLE Advertisements:
    #     - Leader Data TLV
    #     - Route64 TLV
    #     - Source Address TLV
    #   - Non-DUT device: Harness instructs device to send a ICMPv6 Echo Request to the DUT to help differentiate
    #     between Link Requests sent before and after reset.
    print("Step 2: Leader, Router_1")
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

    pkts.filter_wpan_src64(ROUTER_1).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter(lambda p: {
                          consts.LEADER_DATA_TLV,
                          consts.ROUTE64_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type) and\
               p.ipv6.hlim == 255).\
        must_next()

    pkts.filter_ping_request().\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(LEADER).\
        must_next()
    lstart = pkts.index

    # Step 3: Leader
    # - Description: Reset Leader.
    #   - If DUT=Leader and testing is manual, this is a UI pop-up box interaction.
    #   - Allowed Leader reboot time is 125 seconds (must be greater than Leader Timeout value [default 120 seconds]).
    # - Pass Criteria:
    #   - For DUT = Leader: The Leader MUST stop sending MLE advertisements.
    #   - The Leader reboot time MUST be less than Leader Timeout value (default 120 seconds).
    print("Step 3: Leader")

    # Step 4: Leader
    # - Description: Automatically performs Synchronization after Reset, sends Link Request.
    # - Pass Criteria:
    #   - For DUT = Leader: The Leader MUST send a multicast Link Request.
    #   - The following TLVs MUST be present in the Link Request:
    #     - Challenge TLV
    #     - TLV Request TLV: Address16 TLV, Route64 TLV
    #     - Version TLV
    print("Step 4: Leader")
    pkts.filter_wpan_src64(LEADER).\
        filter_LLARMA().\
        filter_mle_cmd(consts.MLE_LINK_REQUEST).\
        filter(lambda p: {
                          consts.CHALLENGE_TLV,
                          consts.TLV_REQUEST_TLV,
                          consts.VERSION_TLV,
                          consts.ADDRESS16_TLV,
                          consts.ROUTE64_TLV
                          } <= set(p.mle.tlv.type) and\
               p.mle.tlv.addr16 is nullField and\
               p.mle.tlv.route64.id_mask is nullField).\
        must_next()
    lend = pkts.index

    # Step 3 (verify): Leader MUST stop sending MLE advertisements.
    pkts.range(lstart, lend).\
        filter_wpan_src64(LEADER).\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        must_not_next()

    # Step 5: Router_1
    # - Description: Automatically responds with a Link Accept.
    # - Pass Criteria:
    #   - For DUT = Router: Router_1 MUST reply with a Link Accept.
    #   - The following TLVs MUST be present in the Link Accept:
    #     - Address16 TLV
    #     - Leader Data TLV
    #     - Link-Layer Frame Counter TLV
    #     - Response TLV
    #     - Route64 TLV
    #     - Source Address TLV
    #     - Version TLV
    #     - MLE Frame Counter TLV (optional)
    #     - Challenge TLV (situational - MUST be included if the response is an Accept and Request message)
    print("Step 5: Router_1")
    _pkt = pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd2(consts.MLE_LINK_ACCEPT, consts.MLE_LINK_ACCEPT_AND_REQUEST).\
        filter(lambda p: {
                          consts.ADDRESS16_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.LINK_LAYER_FRAME_COUNTER_TLV,
                          consts.RESPONSE_TLV,
                          consts.ROUTE64_TLV,
                          consts.SOURCE_ADDRESS_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type) and\
               p.mle.tlv.addr16 is not nullField and\
               p.mle.tlv.route64.id_mask is not nullField).\
        must_next()
    if _pkt.mle.cmd == consts.MLE_LINK_ACCEPT_AND_REQUEST:
        _pkt.must_verify(lambda p: {consts.CHALLENGE_TLV} <= set(p.mle.tlv.type))

    # Step 7 (prepare): ICMPv6 Echo Request to Router_1
    print("Step 7: All")
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst64(ROUTER_1).\
        must_next()
    lconnect = pkts.index

    # Step 7 (verify): Router_1 MUST respond with an ICMPv6 Echo Reply
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(LEADER).\
        must_next()

    # Step 6: Leader
    # - Description: Does NOT send a Parent Request.
    # - Pass Criteria:
    #   - For DUT = Leader: The Leader MUST NOT send a Parent Request after it is re-enabled.
    print("Step 6: Leader")
    pkts.range(lend, lconnect).\
        filter_wpan_src64(LEADER).\
        filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        must_not_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
