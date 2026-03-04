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

import verify_utils
from pktverify import consts


def verify(pv):
    # 5.3.6 Router supports SSED transition to MED
    #
    # 5.3.6.1 Topology
    # - Leader
    # - Router_1 (DUT)
    # - SSED_1
    #
    # 5.3.6.2 Purpose & Description
    # - The purpose of this test case is to validate that a Router can support a child transitioning from a SSED to a
    #   MED and back.
    #
    # Spec Reference                          | V1.2 Section
    # ----------------------------------------|-------------
    # Router supports SSED transition to MED  | 4.6.5.2

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    SSED_1 = pv.vars['SSED_1']
    LEADER_MLEID = pv.vars['LEADER_MLEID']
    SSED_1_MLEID = pv.vars['SSED_1_MLEID']
    SSED_1_RLOC16 = pv.vars['SSED_1_RLOC16']
    ROUTER_1_RLOC16 = pv.vars['ROUTER_1_RLOC16']
    LEADER_RLOC16 = pv.vars['LEADER_RLOC16']

    # Step 0: SSED_1
    # - Description: Preconditions: Set CSL Synchronized Timeout = 20s, Set CSL Period = 500ms.
    # - Pass Criteria: N/A
    print("Step 0: SSED_1")

    # Step 1: All
    # - Description: Topology formation: Leader, DUT, SSED_1.
    # - Pass Criteria: N/A
    print("Step 1: All")

    # Step 2: SSED_1
    # - Description: Automatically attaches to the DUT and establishes CSL synchronization.
    # - Pass Criteria:
    #   - 2.1: The DUT MUST send MLE Child ID Response to SSED_1.
    #   - 2.2: The DUT MUST unicast MLE Child Update Response to SSED_1.
    print("Step 2: SSED_1")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        must_next()

    _response_pkt_1 = pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        must_next()

    # Step 3: Leader
    # - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
    #   SSED mesh-local address.
    # - Pass Criteria:
    #   - 3.1: SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
    print("Step 3: Leader")
    _pkt_echo_req_1 = pkts.filter_ping_request().\
        filter_ipv6_src(LEADER_MLEID).\
        filter_ipv6_dst(SSED_1_MLEID).\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst16(SSED_1_RLOC16).\
        must_next()

    # SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
    pkts.range((_response_pkt_1.number, _response_pkt_1.number), (_pkt_echo_req_1.number, _pkt_echo_req_1.number)).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        filter_wpan_cmd(consts.WPAN_DATA_REQUEST).\
        filter(lambda p: p.wpan.header_ie.csl.period == 0).\
        must_not_next()

    # Step 4: SSED_1
    # - Description: Automatically replies with ICMPv6 Echo Reply.
    # - Pass Criteria: Verify that the SSED replies with an ICMPv6 Echo Reply and is received by the Router.
    print("Step 4: SSED_1")
    pkts.filter_ping_reply(identifier=_pkt_echo_req_1.icmpv6.echo.identifier).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        must_next()

    # Step 5: SSED_1 (MED_1)
    # - Description: Harness instructs the device to transition to a MED and send an MLE Child Update Request and
    #   properly sets the Mode TLV.
    # - Pass Criteria: N/A
    print("Step 5: SSED_1 (MED_1)")
    pkts.filter_wpan_src64(SSED_1).\
        filter_wpan_dst64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        filter(lambda p:
               p.mle.tlv.mode.receiver_on_idle == 1 and\
               p.mle.tlv.mode.device_type_bit == 0).\
        must_next()

    # Step 6: Router_1 (DUT)
    # - Description: Automatically responds with a MLE Child Update Response.
    # - Pass Criteria:
    #   - 6.1: The DUT MUST unicast MLE Child Update Response to SSED_1, including the following:
    #     - Mode TLV
    #     - R bit = 1
    #     - D bit = 0
    print("Step 6: Router_1 (DUT)")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        filter(lambda p:
               p.mle.tlv.mode.receiver_on_idle == 1 and\
               p.mle.tlv.mode.device_type_bit == 0).\
        must_next()

    # Step 7: Leader
    # - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to
    #   SSED_1’s mesh-local address.
    # - Pass Criteria:
    #   - 7.1: The DUT MUST forward the ICMPv6 Echo Reply from the SSED to the Leader.
    print("Step 7: Leader")
    _pkt_echo_req_2 = pkts.filter_ping_request().\
        filter_ipv6_src(LEADER_MLEID).\
        filter_ipv6_dst(SSED_1_MLEID).\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst16(SSED_1_RLOC16).\
        must_next()

    pkts.filter_ping_reply(identifier=_pkt_echo_req_2.icmpv6.echo.identifier).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        must_next()

    # Verify that Router_1 forwards the reply to Leader
    pkts.filter_ping_reply(identifier=_pkt_echo_req_2.icmpv6.echo.identifier).\
        filter_wpan_src16(ROUTER_1_RLOC16).\
        filter_wpan_dst16(LEADER_RLOC16).\
        must_next()

    # Step 8: MED_1 (SSED_1)
    # - Description: Harness instructs the device to transition back to a SSED and send an MLE Child Update
    #   Request.
    # - Pass Criteria: N/A
    print("Step 8: MED_1 (SSED_1)")
    pkts.filter_wpan_src64(SSED_1).\
        filter_wpan_dst64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        filter(lambda p:
               p.mle.tlv.mode.receiver_on_idle == 0 and\
               p.mle.tlv.mode.device_type_bit == 0).\
        must_next()

    # Step 9: Router_1 (DUT)
    # - Description: Automatically responds with a MLE Child Update Response.
    # - Pass Criteria:
    #   - 9.1: The DUT MUST unicast MLE Child Update Response to SSED_1, including the following:
    #     - Mode TLV
    #     - R bit set = 0
    #     - D bit set = 0
    print("Step 9: Router_1 (DUT)")
    _response_pkt_2 = pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        filter(lambda p:
               p.mle.tlv.mode.receiver_on_idle == 0 and\
               p.mle.tlv.mode.device_type_bit == 0).\
        must_next()

    # Step 10: SSED_1
    # - Description: Automatically begins sending synchronization messages containing CSL IEs.
    # - Pass Criteria: N/A
    print("Step 10: SSED_1")

    # Step 11: Leader
    # - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
    #   SSED_1 mesh-local address.
    # - Pass Criteria:
    #   - 11.1: SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
    print("Step 11: Leader")
    _pkt_echo_req_3 = pkts.filter_ping_request().\
        filter_ipv6_src(LEADER_MLEID).\
        filter_ipv6_dst(SSED_1_MLEID).\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst16(SSED_1_RLOC16).\
        must_next()

    # SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
    pkts.range((_response_pkt_2.number, _response_pkt_2.number), (_pkt_echo_req_3.number, _pkt_echo_req_3.number)).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        filter_wpan_cmd(consts.WPAN_DATA_REQUEST).\
        filter(lambda p: p.wpan.header_ie.csl.period == 0).\
        must_not_next()

    # Step 12: SSED_1
    # - Description: Automatically replies with ICMPv6 Echo Reply.
    # - Pass Criteria:
    #   - 12.1: SSED_1 MUST send an ICMPv6 Echo Reply.
    print("Step 12: SSED_1")
    pkts.filter_ping_reply(identifier=_pkt_echo_req_3.icmpv6.echo.identifier).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        filter(lambda p: p.wpan.header_ie.csl.period > 0).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
