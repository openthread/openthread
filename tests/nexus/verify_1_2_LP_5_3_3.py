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
    # 5.3.3 SSED Becomes Unsynchronized
    #
    # 5.3.3.1 Topology
    # - Leader (DUT)
    # - Router 1
    # - SSED 1
    #
    # 5.3.3.2 Purpose & Description
    # The purpose of this test is to validate that when the DUT considers the SSED unsynchronized, it does not use CSL
    #   transmission to transmit any messages, but rather falls back to indirect transmission until the SSED is CSL
    #   synchronized again.
    #
    # Spec Reference                | V1.2 Section
    # ------------------------------|-------------
    # SSED Becomes Unsynchronized   | 4.6.5.1.1

    pkts = pv.pkts
    pv.summary.show()

    DUT = pv.vars['DUT']
    DUT_RLOC16 = pv.vars['DUT_RLOC16']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_1_RLOC16 = pv.vars['ROUTER_1_RLOC16']
    SSED_1 = pv.vars['SSED_1']
    SSED_1_RLOC16 = pv.vars['SSED_1_RLOC16']

    # Step 0: SSED_1
    # - Description: Preconditions: Set CSL Synchronized Timeout = 20s. Set CSL Period = 500ms.
    # - Pass Criteria: N/A.
    print("Step 0: SSED_1 configuration")

    # Step 1: All
    # - Description: Topology formation: DUT, Router_1, SSED_1.
    # - Pass Criteria: N/A.
    print("Step 1: Topology formation")

    # Step 2: SSED_1
    # - Description: Automatically attaches to the DUT and establishes CSL synchronization.
    # - Pass Criteria:
    #   - The DUT MUST sent MLE Child ID Response to SSED_1.
    #   - The DUT MUST unicast MLE Child Update Response to SSED_1.
    print("Step 2: SSED_1 attaches to DUT and establishes CSL synchronization")

    pkts.filter_wpan_src64(DUT).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        must_next()

    pkts.filter_wpan_src64(SSED_1).\
        filter_wpan_dst64(DUT).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        filter(lambda p: p.wpan.version == 2).\
        filter(lambda p: p.wpan.header_ie.csl.period == 3125).\
        must_next()

    pkts.filter_wpan_src64(DUT).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        filter(lambda p: p.wpan.version == 2).\
        must_next()

    # Step 3: Router_1
    # - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to SSED_1
    #   mesh-local address.
    # - Pass Criteria: The DUT MUST buffer the ICMPv6 Echo Request frame and relay it to SSED_1 within a subsequent CSL
    #   slot.
    print("Step 3: Router_1 sends ICMPv6 Echo Request to SSED_1")

    # ICMPv6 Echo Request from Router_1 to SSED_1
    echo_req_1 = pkts.filter_ping_request().\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst16(DUT_RLOC16).\
        filter_ipv6_dst(pv.vars['SSED_1_MLEID']).\
        must_next()

    # Relayed ICMPv6 Echo Request from DUT to SSED_1 using CSL (wpan.version == 2)
    pkts.filter_ping_request().\
        filter_wpan_src64(DUT).\
        filter_wpan_dst16(SSED_1_RLOC16).\
        filter(lambda p: p.wpan.version == 2).\
        must_next()

    # Step 4: SSED_1
    # - Description: Automatically replies with ICMPv6 Echo Reply.
    # - Pass Criteria: The DUT MUST forward the ICMPv6 Echo Reply from SSED_1.
    print("Step 4: SSED_1 replies with ICMPv6 Echo Reply")

    echo_rep_1 = pkts.filter_ping_reply(identifier=echo_req_1.icmpv6.echo.identifier).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(DUT_RLOC16).\
        must_next()

    pkts.filter_ping_reply(identifier=echo_req_1.icmpv6.echo.identifier).\
        filter_wpan_src64(DUT).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        must_next()

    # Step 5: SSED_1
    # - Description: Harness instructs the device to stop sending periodic synchronization data poll frames.
    # - Pass Criteria: N/A.
    print("Step 5: SSED_1 stops periodic synchronization data poll frames")

    # Step 6: Harness
    # - Description: Harness waits 25 seconds to time out the SSED / CSL connection.
    # - Pass Criteria: N/A (Implicit).
    print("Step 6: Harness waits 25 seconds")

    # Step 7: Router_1
    # - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
    #   SSED_1 mesh-local address.
    # - Pass Criteria: N/A.
    print("Step 7: Router_1 sends ICMPv6 Echo Request to SSED_1")

    echo_req_2 = pkts.filter_ping_request().\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst16(DUT_RLOC16).\
        filter_ipv6_dst(pv.vars['SSED_1_MLEID']).\
        must_next()

    # Step 8: Leader (DUT)
    # - Description: Automatically buffers the frame for SSED_1 in the indirect (SED) queue.
    # - Pass Criteria: The DUT MUST NOT relay the ICMPv6 Echo Request frame to SSED_1.
    print("Step 8: DUT buffers the frame and does NOT relay it via CSL")

    # We will verify this by checking that no such packet exists before Step 11's Data Request.

    # Step 9: SSED_1
    # - Description: Harness instructs the device to resume sending synchronization messages containing CSL IEs.
    # - Pass Criteria: N/A.
    print("Step 9: SSED_1 resumes synchronization messages")

    # Step 10: Harness
    # - Description: Harness waits 18 seconds for the SSED / CSL connection to resume.
    # - Pass Criteria: N/A (Implicit).
    print("Step 10: Harness waits 18 seconds")

    # Step 11: Leader (DUT)
    # - Description: Automatically sends MAC Acknowledgement then relays the message.
    # - Pass Criteria: The DUT MUST verify that the Frame Pending bit is set and successfully transmits the ICMPv6 Echo
    #   Request.
    print("Step 11: DUT sends MAC Ack with Frame Pending and relays the message")

    # SSED_1 sends a Data Request or MLE Child Update Request (re-sync) to DUT
    data_req = pkts.filter(lambda p: (p.wpan.src64 == SSED_1 or p.wpan.src16 == SSED_1_RLOC16)).\
        filter_wpan_dst16(DUT_RLOC16).\
        filter(lambda p: (p.wpan.frame_type == consts.WPAN_CMD and p.wpan.cmd == consts.WPAN_DATA_REQUEST) or\
                         (p.wpan.frame_type == consts.WPAN_DATA and hasattr(p, 'mle') and p.mle.cmd == consts.MLE_CHILD_UPDATE_REQUEST)).\
        must_next()

    # Verify that the frame was NOT relayed before the Data Request/re-sync (Step 8 Pass Criteria)
    pkts.range((echo_req_2.number, 0), (data_req.number, 0)).\
        filter_ping_request().\
        filter_wpan_src64(DUT).\
        filter_wpan_dst16(SSED_1_RLOC16).\
        must_not_next()

    # DUT responds with an ACK that has Frame Pending set (1)
    pkts.filter_wpan_ack().\
        filter(lambda p: p.wpan.seq_no == data_req.wpan.seq_no).\
        filter(lambda p: p.wpan.pending == 1).\
        must_next()

    # Relayed ICMPv6 Echo Request from DUT to SSED_1 (indirect transmission)
    pkts.filter_ping_request().\
        filter_wpan_src64(DUT).\
        filter_wpan_dst16(SSED_1_RLOC16).\
        filter(lambda p: p.wpan.version == consts.MAC_FRAME_VERSION_2015).\
        must_next()

    # Step 12: SSED_1
    # - Description: Automatically responds with ICMPv6 Echo Reply.
    # - Pass Criteria:
    #   - SSED_1 MUST respond with ICMPv6 Echo Reply.
    #   - The DUT MUST relay the ICMPv6 Echo Reply to Router_1.
    print("Step 12: SSED_1 responds with ICMPv6 Echo Reply")

    echo_rep_2 = pkts.filter_ping_reply(identifier=echo_req_2.icmpv6.echo.identifier).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(DUT_RLOC16).\
        must_next()

    pkts.filter_ping_reply(identifier=echo_req_2.icmpv6.echo.identifier).\
        filter_wpan_src64(DUT).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        must_next()

    # Step 12b: SSED_1
    # - Description: Harness instructs the device to stop sending periodic synchronization data poll frames.
    # - Pass Criteria: N/A.
    print("Step 12b: SSED_1 stops periodic synchronization data poll frames")

    # Step 13: Router_1
    # - Description: Harness verifies CSL synchronization by instructing the device to send an ICMPv6 Echo Request to
    #   the SSED_1 mesh-local address.
    # - Pass Criteria:
    #   - The DUT MUST buffer the ICMPv6 Echo Request frame and relay it to SSED_1 within a subsequent CSL slot.
    #   - SSED_1 MUST respond with ICMPv6 Echo Reply.
    #   - The DUT MUST relay the ICMPv6 Echo Reply to Router_1.
    print("Step 13: Router_1 sends ICMPv6 Echo Request to SSED_1 (CSL relay)")

    echo_req_3 = pkts.filter_ping_request().\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst16(DUT_RLOC16).\
        filter_ipv6_dst(pv.vars['SSED_1_MLEID']).\
        must_next()

    # Relayed ICMPv6 Echo Request from DUT to SSED_1 using CSL (wpan.version == 2)
    # We verify it is CSL by checking that no Data Request was sent by SSED_1 before this frame.
    relay_req_3 = pkts.filter_ping_request().\
        filter_wpan_src64(DUT).\
        filter_wpan_dst16(SSED_1_RLOC16).\
        filter(lambda p: p.wpan.version == 2).\
        must_next()

    pkts.range((echo_req_3.number, 0), (relay_req_3.number, 0)).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_cmd(consts.WPAN_DATA_REQUEST).\
        must_not_next()

    echo_rep_3 = pkts.filter_ping_reply(identifier=echo_req_3.icmpv6.echo.identifier).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(DUT_RLOC16).\
        must_next()

    pkts.filter_ping_reply(identifier=echo_req_3.icmpv6.echo.identifier).\
        filter_wpan_src64(DUT).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
