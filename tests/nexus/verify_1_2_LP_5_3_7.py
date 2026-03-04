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
    # 5.3.7 Router supports SSED modifying CSL Synchronized Timeout TLV
    #
    # 5.3.7.1 Topology
    #   - Leader
    #   - Router_1 (DUT)
    #   - SSED_1
    #
    # 5.3.7.2 Purpose & Description
    #   The purpose of this test case is to validate that a Router can support a SSED that modifies the CSL Synchronized
    #   Timeout TLV.
    #
    # Spec Reference   | V1.2 Section
    # -----------------|--------------
    # SSED Attachment  | 4.6.5.2

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    LEADER_RLOC16 = pv.vars['LEADER_RLOC16']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_1_RLOC16 = pv.vars['ROUTER_1_RLOC16']
    SSED_1 = pv.vars['SSED_1']
    SSED_1_RLOC16 = pv.vars['SSED_1_RLOC16']

    # Step 0: SSED_1
    #   Description: Preconditions: Set CSL Synchronized Timeout = 10s. Set CSL Period = 500ms.
    #   Pass Criteria: N/A.
    print("Step 0: SSED_1: Preconditions: Set CSL Synchronized Timeout = 10s. Set CSL Period = 500ms.")

    # Step 1: All
    #   Description: Topology formation: Leader, DUT, SSED_1.
    #   Pass Criteria: N/A.
    print("Step 1: All: Topology formation: Leader, DUT, SSED_1.")

    # Step 2: SSED_1
    #   Description: Automatically attaches to the DUT and establishes CSL synchronization.
    #   Pass Criteria:
    #   - 2.1: The DUT MUST unicast MLE Child ID Response to SSED_1.
    #   - 2.2: The DUT MUST unicast MLE Child Update Response to SSED_1.
    print("Step 2: SSED_1: Automatically attaches to the DUT and establishes CSL synchronization.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        must_next()

    _step2_pkt = pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        filter(lambda p: p.wpan.version == 2).\
        must_next()

    # Step 3: Harness
    #   Description: Harness waits for 2 seconds.
    #   Pass Criteria: N/A.
    print("Step 3: Harness: Harness waits for 2 seconds.")

    # Step 4: Leader
    #   Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
    #     SSED_1 mesh-local address.
    #   Pass Criteria:
    #   - 4.1: SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
    #   - Verify that the Leader receives the ICMPv6 Echo Reply.
    print("Step 4: Leader: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request.")
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst16(SSED_1_RLOC16).\
        filter(lambda p: p.wpan.version == 2).\
        must_next()

    # Skip first 10 packets after Step 2 to ignore initial resync polls
    pkts.range((_step2_pkt.number + 10, 0), (_pkt.number, 0)).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        filter_wpan_cmd(consts.WPAN_DATA_REQUEST).\
        must_not_next()

    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        must_next()

    # Step 5: SSED_1
    #   Description: Harness instructs the device to modify its CSL Synchronized Timeout TLV to 20s and send an MLE
    #     Child Update Request.
    #   Pass Criteria: N/A.
    print("Step 5: SSED_1: Modify its CSL Synchronized Timeout TLV to 20s and send an MLE Child Update Request.")
    pkts.filter_wpan_src64(SSED_1).\
        filter_wpan_dst64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        filter(lambda p: {consts.CSL_SYNCHRONIZED_TIMEOUT} <= set(p.mle.tlv.type)).\
        must_next()

    # Step 6: Router_1 (DUT)
    #   Description: Automatically responds with MLE Child Update Response.
    #   Pass Criteria:
    #   - 6.1: The DUT MUST unicast MLE Child Update Response to SSED_1.
    #   - Note: Following this response, Harness instructs SSED_1 to stop sending periodic synchronization data poll
    #     frames.
    print("Step 6: Router_1 (DUT): Automatically responds with MLE Child Update Response.")
    _step6_pkt = pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        must_next()

    # Step 7: Harness
    #   Description: Harness waits for 15 seconds.
    #   Pass Criteria: N/A.
    print("Step 7: Harness: Harness waits for 15 seconds.")

    # Step 8: Leader
    #   Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
    #     SSED_1 mesh-local address.
    #   Pass Criteria:
    #   - 8.1: SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
    #   - Verify that the Leader receives the ICMPv6 Echo Reply.
    print("Step 8: Leader: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request.")
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst16(SSED_1_RLOC16).\
        filter(lambda p: p.wpan.version == 2).\
        must_next()

    pkts.range((_step6_pkt.number + 5, 0), (_pkt.number, 0)).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        filter_wpan_cmd(consts.WPAN_DATA_REQUEST).\
        must_not_next()

    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        must_next()

    # Step 9: SSED_1
    #   Description: Harness instructs the device to modify its CSL Synchronized Timeout TLV to 10s and send a MLE
    #     Child Update Request.
    #   Pass Criteria: N/A.
    print("Step 9: SSED_1: Modify its CSL Synchronized Timeout TLV to 10s and send a MLE Child Update Request.")
    pkts.filter_wpan_src64(SSED_1).\
        filter_wpan_dst64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        filter(lambda p: {consts.CSL_SYNCHRONIZED_TIMEOUT} <= set(p.mle.tlv.type)).\
        must_next()

    # Step 10: Router_1 (DUT)
    #   Description: Automatically responds with MLE Child Update Response.
    #   Pass Criteria:
    #   - 10.1: The DUT MUST unicast MLE Child Update Response to SSED_1.
    print("Step 10: Router_1 (DUT): Automatically responds with MLE Child Update Response.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        must_next()

    # Step 11: Harness
    #   Description: Harness waits 15 seconds.
    #   Pass Criteria: N/A.
    print("Step 11: Harness: Harness waits 15 seconds.")

    # Step 12: Leader
    #   Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
    #     SSED_1 mesh-local address. The frame should be buffered by the DUT in the indirect (SED) queue.
    #   Pass Criteria:
    #   - 12.1: The DUT MUST NOT relay the frame to SSED_1.
    print("Step 12: Leader: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request.")
    _pkt12 = pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        must_next()

    # Step 13: SSED_1
    #   Description: Harness instructs the device to resume sending synchronization messages containing CSL IEs.
    #   Pass Criteria: N/A.
    print("Step 13: SSED_1: Resume sending synchronization messages containing CSL IEs.")
    _pkt13 = pkts.filter_wpan_src64(SSED_1).\
        filter_wpan_dst64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        must_next()

    # Verify Step 12.1: DUT MUST NOT relay the frame between Step 12 and Step 13.
    pkts.range((_pkt12.number, 0), (_pkt13.number, 0)).\
        filter_ping_request(identifier=_pkt12.icmpv6.echo.identifier).\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst16(SSED_1_RLOC16).\
        must_not_next()

    # Step 14: Router_1 (DUT)
    #   Description: Automatically sends MAC Acknowledgement and relays the message.
    #   Pass Criteria:
    #   - 14.1: The DUT MUST send MAC Acknowledgement with the Frame Pending bit = 1.
    #   - 14.2: The DUT MUST forward the ICMPv6 Echo Request to SSED_1.
    print("Step 14: Router_1 (DUT): Automatically sends MAC Acknowledgement and relays the message.")
    pkts.filter_wpan_ack().\
        filter(lambda p: p.wpan.pending == 1).\
        must_next()

    pkts.filter_ping_request().\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst16(SSED_1_RLOC16).\
        filter(lambda p: p.wpan.version == 2).\
        must_next()

    # Step 15: SSED_1
    #   Description: Automatically responds with ICMPv6 Echo Reply.
    #   Pass Criteria:
    #   - 15.1: The DUT MUST forward the ICMPv6 Echo Reply.
    #   - Note: Harness waits 10 seconds for CSL synchronization to resume.
    print("Step 15: SSED_1: Automatically responds with ICMPv6 Echo Reply.")
    pkts.filter_ping_reply().\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        must_next()

    pkts.filter_ping_reply().\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst16(LEADER_RLOC16).\
        must_next()

    # Step 16: Leader
    #   Description: Harness verifies CSL synchronization by instructing the device to send an Echo Request to
    #     the SSED_1 mesh-local address.
    #   Pass Criteria:
    #   - 16.1: The ICMPv6 Echo Request frame MUST be buffered by the DUT and sent to SSED_1 within a subsequent CSL
    #     slot.
    #   - 16.2: The DUT must forward the ICMPv6 Echo Reply.
    print("Step 16: Leader: Harness verifies CSL synchronization by instructing the device to send an Echo Request.")
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst16(SSED_1_RLOC16).\
        filter(lambda p: p.wpan.version == 2).\
        must_next()

    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
