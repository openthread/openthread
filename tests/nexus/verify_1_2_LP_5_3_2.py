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

import os
import sys

# Add the current directory to sys.path to find verify_utils
CUR_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(CUR_DIR)

import verify_utils
from pktverify import consts
from pktverify.null_field import nullField

CSL_PERIOD_MS = 500


def _has_csl_ie(p):
    csl_period_in_10_symbols_units = CSL_PERIOD_MS * 1000 // consts.US_PER_TEN_SYMBOLS
    return (p.wpan.version == 2 and p.wpan.ie_present == 1 and consts.CSL_IE_ID in p.wpan.header_ie.id and
            p.wpan.header_ie.csl.period == csl_period_in_10_symbols_units and
            p.wpan.header_ie.csl.phase is not nullField)


def _must_not_have_data_request_from_ssed1(pkts, start_pkt, end_pkt, ssed_1, router_1_rloc16):
    pkts.range((start_pkt.number if start_pkt else 1, 0), (end_pkt.number, 0)).\
        filter_wpan_src64(ssed_1).\
        filter_wpan_dst16(router_1_rloc16).\
        filter_wpan_cmd(consts.WPAN_DATA_REQUEST).\
        must_not_next()


def verify(pv):
    # 5.3.2 CSL Synchronized Communication
    #
    # 5.3.2.1 Topology
    # - Leader
    # - Router 1 (DUT)
    # - SSED 1
    #
    # 5.3.2.2 Purpose & Description
    # The purpose of this test is to validate that the DUT is successfully able to sustain a CSL connection with a
    #   SSED that uses different types of messages to maintain its CSL Synchronization.
    #
    # 5.3.2.3 Configuration
    # - Leader: Thread 1.2 Leader.
    # - Router 1 (DUT): Thread 1.2 Router.
    # - SSED_1: Thread 1.2 SSED.
    # - SSED_1 is configured to have a CSL Synchronized Timeout value of 20s.
    # - Set CSL Period = 500ms.
    #
    # Spec Reference                 | V1.2 Section
    # -------------------------------|--------------
    # CSL Synchronized Communication | 4.6.5.2.3

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    LEADER_ML_EID = pv.vars['LEADER_MLEID']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_1_RLOC16 = pv.vars['ROUTER_1_RLOC16']
    SSED_1 = pv.vars['SSED_1']
    SSED_1_ML_EID = pv.vars['SSED_1_MLEID']
    SSED_1_RLOC16 = pv.vars['SSED_1_RLOC16']

    # Step 0: SSED_1
    # - Description: Preconditions: Set CSL Synchronized Timeout = 20s, Set CSL Period = 500ms.
    # - Pass Criteria: N/A.
    print("Step 0: SSED_1 configuration")

    # Step 1: All
    # - Description: Topology formation: Leader, DUT, SSED_1.
    # - Pass Criteria: N/A.
    print("Step 1: Topology formation")

    # Step 2: SSED_1
    # - Description: Automatically attaches to the DUT.
    # - Pass Criteria: The DUT MUST unicast MLE Child Update Response to SSED_1.
    print("Step 2: SSED_1 attaches to DUT")

    child_update_resp = pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        must_next()

    # Step 2a: SSED_1
    # - Description: Harness instructs the device to deactivate autosynchronization.
    # - Pass Criteria: N/A.
    print("Step 2a: Deactivate autosynchronization")

    # Step 3: Harness
    # - Description: Harness waits for 18 seconds.
    # - Pass Criteria: N/A.
    print("Step 3: Harness waits for 18 seconds")

    # Step 4: Leader
    # - Description: Harness instructs the device to send a UDP message to SSED_1 mesh-local address to synchronize it.
    # - Pass Criteria: SSED_1 MUST NOT send a MAC Data Request prior to receiving the UDP message from the Leader.
    print("Step 4: Leader sends UDP to SSED_1")

    udp_pkt = pkts.range((child_update_resp.number, 0)).\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        filter(lambda p: p.udp).\
        filter_ipv6_dst(SSED_1_ML_EID).\
        must_next()

    # SSED_1 MUST NOT send a MAC Data Request prior to receiving the UDP message from the Leader.
    _must_not_have_data_request_from_ssed1(pkts, child_update_resp, udp_pkt, SSED_1, ROUTER_1_RLOC16)

    # Step 5: SSED_1
    # - Description: Automatically responds to the DUT with an enhanced acknowledgement that contains CSL IEs.
    # - Pass Criteria: The CSL unsynchronized timer on the Router (DUT) should be reset to 0.
    print("Step 5: SSED_1 responds with Enhanced ACK containing CSL IEs")

    # Enhanced ACK for the forwarded UDP message (Router 1 to SSED_1)
    udp_fwd = pkts.range((udp_pkt.number, 0)).\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst16(SSED_1_RLOC16).\
        filter(lambda p: p.udp).\
        filter_ipv6_dst(SSED_1_ML_EID).\
        filter(lambda p: p.wpan.version == 2).\
        must_next()

    # The DUT MUST receive an Enhanced ACK from SSED_1
    ack_step5 = pkts.range((udp_fwd.number, 0)).\
        filter_wpan_ack().\
        filter(_has_csl_ie).\
        must_next()

    # Step 6: Harness
    # - Description: Harness waits for 18 seconds.
    # - Pass Criteria: N/A.
    print("Step 6: Harness waits for 18 seconds")

    # Step 7: Leader
    # - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
    #   SSED_1 mesh-local address via the DUT.
    # - Pass Criteria:
    #   - SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the Leader.
    #   - The Leader MUST receive an ICMPv6 Echo Reply from SSED_1.
    print("Step 7: Leader sends Echo Request to SSED_1")

    echo_req_1 = pkts.range((ack_step5.number, 0)).\
        filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        filter_ipv6_dst(SSED_1_ML_EID).\
        must_next()

    # SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the Leader.
    _must_not_have_data_request_from_ssed1(pkts, udp_fwd, echo_req_1, SSED_1, ROUTER_1_RLOC16)

    echo_reply_1 = pkts.range((echo_req_1.number, 0)).\
        filter_ping_reply(identifier=echo_req_1.icmpv6.echo.identifier).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        must_next()

    # Step 8: Harness
    # - Description: Harness waits for 18 seconds.
    # - Pass Criteria: N/A.
    print("Step 8: Harness waits for 18 seconds")

    # Step 9: SSED_1
    # - Description: Harness instructs the device to send a MAC Data Request message containing CSL IEs to the DUT.
    # - Pass Criteria: The CSL unsynchronized timer on the Router (DUT) should be reset to 0.
    print("Step 9: SSED_1 sends MAC Data Request with CSL IEs")

    # We look for the Data Request starting from echo_req_1 because it could be delayed until Step 9.
    step9_data_req = pkts.range((echo_req_1.number, 0)).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        filter_wpan_cmd(consts.WPAN_DATA_REQUEST).\
        filter(_has_csl_ie).\
        must_next()

    # Step 10: Harness
    # - Description: Harness waits for 18 seconds.
    # - Pass Criteria: N/A.
    print("Step 10: Harness waits for 18 seconds")

    # Step 11: Leader
    # - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
    #   SSED_1 mesh-local address via the DUT.
    # - Pass Criteria:
    #   - SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the Leader.
    #   - The Leader MUST receive an ICMPv6 Echo Reply from SSED_1.
    print("Step 11: Leader sends Echo Request to SSED_1")

    # Start search from whichever packet was later: echo_reply_1 or step9_data_req.
    last_pkt_step9 = echo_reply_1 if echo_reply_1.number > step9_data_req.number else step9_data_req
    echo_req_2 = pkts.range((last_pkt_step9.number, 0)).\
        filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        filter_ipv6_dst(SSED_1_ML_EID).\
        must_next()

    # SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the Leader.
    _must_not_have_data_request_from_ssed1(pkts, step9_data_req, echo_req_2, SSED_1, ROUTER_1_RLOC16)

    echo_reply_2 = pkts.range((echo_req_2.number, 0)).\
        filter_ping_reply(identifier=echo_req_2.icmpv6.echo.identifier).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        must_next()

    # Step 12: Harness
    # - Description: Harness waits for 18 seconds.
    # - Pass Criteria: N/A.
    print("Step 12: Harness waits for 18 seconds")

    # Step 13: SSED_1
    # - Description: Harness instructs the device to send a UDP Message to the DUT.
    # - Pass Criteria:
    #   - The CSL unsynchronized timer on the Router (DUT) should be reset to 0.
    #   - Check that the 802.15.4 Frame Header includes following Information Elements:
    #     - CSL Period IE (Check value of this TLV)
    #     - CSL Phase IE
    #   - Compare the value of this TLV with the expected value.
    print("Step 13: SSED_1 sends UDP to DUT with CSL IEs")

    # UDP could also be delayed until Step 13 if Echo Request 2 was delayed.
    step13_udp = pkts.range((echo_req_2.number, 0)).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        filter(lambda p: p.udp).\
        filter(_has_csl_ie).\
        must_next()

    # Step 14: Router (DUT)
    # - Description: Automatically sends an Enhanced ACK to SSED_1.
    # - Pass Criteria: The DUT MUST send an enhanced ACK to SSED_1:
    #   - The Information Elements MUST NOT be included.
    #   - The Enhanced ACK must occur within the ACK timeout.
    print("Step 14: Router sends Enhanced ACK (no IEs)")

    ack_step14 = pkts.range((step13_udp.number, 0)).\
        filter_wpan_ack().\
        filter(lambda p: p.wpan.version == 2 and p.wpan.ie_present == 0).\
        must_next()

    # Step 15: Harness
    # - Description: Harness waits for 18 seconds.
    # - Pass Criteria: N/A.
    print("Step 15: Harness waits for 18 seconds")

    # Step 16: Leader
    # - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
    #   SSED_1 mesh-local address via the DUT.
    # - Pass Criteria:
    #   - SSED1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the Leader.
    #   - The Leader MUST receive an ICMPv6 Echo Reply from SSED_1.
    print("Step 16: Leader sends Echo Request to SSED_1")

    # Ensure we search after both echo_reply_2 and ack_step14.
    last_pkt_step14 = echo_reply_2 if echo_reply_2.number > ack_step14.number else ack_step14
    echo_req_3 = pkts.range((last_pkt_step14.number, 0)).\
        filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        filter_ipv6_dst(SSED_1_ML_EID).\
        must_next()

    # SSED1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the Leader.
    _must_not_have_data_request_from_ssed1(pkts, last_pkt_step14, echo_req_3, SSED_1, ROUTER_1_RLOC16)

    pkts.range((echo_req_3.number, 0)).\
        filter_ping_reply(identifier=echo_req_3.icmpv6.echo.identifier).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
