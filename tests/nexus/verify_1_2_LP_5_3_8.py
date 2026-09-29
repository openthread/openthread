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

# Constants to avoid magic numbers
PRIMARY_CHANNEL = 11
SECONDARY_CHANNEL = 12
TERNARY_CHANNEL = 13

CSL_CHANNEL_TLV = 80


def verify(pv):
    # 5.3.8 Router supports SSED modifying CSL Channel TLV
    #
    # 5.3.8.1 Topology
    # - Leader
    # - Router_1 (DUT)
    # - SSED_1
    #
    # 5.3.8.2 Purpose & Description
    # The purpose of this test case is to validate that a Router can support a SSED that modifies the CSL Channel TLV.
    #
    # Spec Reference                             | V1.2 Section
    # -------------------------------------------|--------------
    # Router supports SSED modifying CSL Channel | 4.6.5.2

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_1_RLOC16 = pv.vars['ROUTER_1_RLOC16']
    SSED_1 = pv.vars['SSED_1']
    SSED_1_RLOC16 = pv.vars['SSED_1_RLOC16']

    # Step 0: SSED_1
    # - Description: Preconditions: Set CSL Synchronized Timeout = 20s. Set CSL Period = 500ms.
    # - Pass Criteria: N/A.
    print("Step 0: SSED_1 preconditions")

    # Step 1: All
    # - Description: Topology formation: Leader, DUT, SSED_1.
    # - Pass Criteria: N/A.
    print("Step 1: Topology formation")

    # Step 2: SSED_1
    # - Description: Automatically attaches to the DUT on the Primary Channel and establishes CSL synchronization.
    # - Pass Criteria:
    #   - 2.1: The DUT MUST send MLE Child ID Response to SSED_1.
    #   - 2.2: The DUT MUST unicast MLE Child Update Response to SSED_1.
    print("Step 2: SSED_1 attaches and establishes CSL synchronization")

    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        must_next()

    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        must_next()

    checkpoint = pkts.index

    # Step 3: Leader
    # - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
    #   SSED_1 mesh-local address.
    # - Pass Criteria:
    #   - 3.1: SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
    #   - 3.2: The Leader MUST receive the ICMPv6 Echo Reply.
    print("Step 3: Leader verifies connectivity")

    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst16(SSED_1_RLOC16).\
        filter(lambda p: p.wpan.channel == PRIMARY_CHANNEL).\
        must_next()

    pkts.range(checkpoint, pkts.index).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        filter_wpan_cmd(consts.WPAN_DATA_REQUEST).\
        filter(lambda p: p.wpan.channel == PRIMARY_CHANNEL).\
        must_not_next()

    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        filter(lambda p: p.wpan.channel == PRIMARY_CHANNEL).\
        must_next()

    checkpoint = pkts.index

    # Step 4: SSED_1
    # - Description: Harness instructs the device to modify its CSL Channel TLV to the Secondary Channel and send a
    #   MLE Child Update Request.
    # - Pass Criteria: N/A.
    print("Step 4: SSED_1 modifies CSL Channel TLV to Secondary Channel")

    pkts.filter_wpan_src64(SSED_1).\
        filter_wpan_dst64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        filter(lambda p: CSL_CHANNEL_TLV in p.mle.tlv.type and p.wpan.channel == PRIMARY_CHANNEL).\
        must_next()

    # Step 5: Router_1 (DUT)
    # - Description: Automatically responds with MLE Child Update Response.
    # - Pass Criteria:
    #   - 5.1: The DUT MUST send MLE Child Update Response on the Secondary Channel.
    print("Step 5: Router_1 (DUT) responds with MLE Child Update Response on Secondary Channel")

    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        filter(lambda p: p.wpan.channel == SECONDARY_CHANNEL).\
        must_next()

    # Step 6: Harness
    # - Description: Harness waits 18 seconds for the CSL change to complete.
    # - Pass Criteria: N/A.
    print("Step 6: Wait for CSL change to complete")

    # Step 7: Leader
    # - Description: Harness verifies connectivity by sending an ICMPv6 Echo Request to the SSED_1 mesh-local
    #   address.
    # - Pass Criteria:
    #   - 7.1: The DUT MUST transmit the ICMPv6 Echo Request on the Secondary Channel.
    #   - 7.2: SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
    #   - 7.3: The Leader MUST receive the ICMPv6 Echo Reply.
    print("Step 7: Leader verifies connectivity on Secondary Channel")

    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst16(SSED_1_RLOC16).\
        filter(lambda p: p.wpan.channel == SECONDARY_CHANNEL).\
        must_next()

    pkts.range(checkpoint, pkts.index).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        filter_wpan_cmd(consts.WPAN_DATA_REQUEST).\
        filter(lambda p: p.wpan.channel == SECONDARY_CHANNEL).\
        must_not_next()

    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        filter(lambda p: p.wpan.channel == PRIMARY_CHANNEL).\
        must_next()

    checkpoint = pkts.index

    # Step 8: SSED_1
    # - Description: Harness instructs the device to modify its CSL Channel TLV back to the original (Primary)
    #   Channel and send a MLE Child Update Request. To do this, the Harness sends the channel value ‘0’ which
    #   denotes ‘same channel as used for Thread Network’. Note: in OT CLI, this is set with `csl channel 0`.
    # - Pass Criteria: N/A.
    print("Step 8: SSED_1 modifies CSL Channel TLV back to Primary Channel")

    pkts.filter_wpan_src64(SSED_1).\
        filter_wpan_dst64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        filter(lambda p: CSL_CHANNEL_TLV in p.mle.tlv.type and p.wpan.channel == PRIMARY_CHANNEL).\
        must_next()

    # Step 9: Router_1 (DUT)
    # - Description: Automatically responds with a MLE Child Update Response.
    # - Pass Criteria:
    #   - 9.1: The DUT MUST send a MLE Child Update Response.
    print("Step 9: Router_1 (DUT) responds with MLE Child Update Response on Primary Channel")

    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        filter(lambda p: p.wpan.channel == PRIMARY_CHANNEL).\
        must_next()

    # Step 10: Harness
    # - Description: Harness waits 18 seconds for the network activities to complete.
    # - Pass Criteria: N/A.
    print("Step 10: Wait for network activities to complete")

    # Step 11: Leader
    # - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
    #   SSED_1 mesh-local address.
    # - Pass Criteria:
    #   - 11.1: The DUT MUST transmit the ICMPv6 Echo Request on the Primary Channel.
    #   - 11.2: SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
    #   - 11.3: The Leader MUST receive the ICMPv6 Echo Reply.
    print("Step 11: Leader verifies connectivity on Primary Channel")

    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst16(SSED_1_RLOC16).\
        filter(lambda p: p.wpan.channel == PRIMARY_CHANNEL).\
        must_next()

    pkts.range(checkpoint, pkts.index).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        filter_wpan_cmd(consts.WPAN_DATA_REQUEST).\
        filter(lambda p: p.wpan.channel == PRIMARY_CHANNEL).\
        must_not_next()

    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        filter(lambda p: p.wpan.channel == PRIMARY_CHANNEL).\
        must_next()

    checkpoint = pkts.index

    # Step 12: Leader
    # - Description: Harness configures a new Terniary channel for the Thread Network, using the Pending Dataset.
    #   The new dataset automatically propagates to all devices in the network.
    # - Pass Criteria: N/A.
    print("Step 12: Leader configures Ternary channel via Pending Dataset")

    # Step 13: Leader
    # - Description: Harness verifies connectivity by sending an ICMPv6 Echo Request to the SSED_1 mesh-local
    #   address.
    # - Pass Criteria:
    #   - 13.1: The DUT MUST transmit the ICMPv6 Echo Request on the Terniary Channel.
    #   - 13.2: SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the DUT.
    print("Step 13: Leader verifies connectivity on Ternary Channel")

    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst16(SSED_1_RLOC16).\
        filter(lambda p: p.wpan.channel == TERNARY_CHANNEL).\
        must_next()

    # Pass Criteria 13.2: SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request.
    # We only disallow Data Requests that happen close to the Echo Request (within 1 second)
    # to allow for re-attachment polls that may occur early in the channel switch transition.
    pkts.range(checkpoint, pkts.index).\
        filter(lambda p: p.sniff_timestamp > _pkt.sniff_timestamp - 1.0).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        filter_wpan_cmd(consts.WPAN_DATA_REQUEST).\
        filter(lambda p: p.wpan.channel == TERNARY_CHANNEL).\
        must_not_next()

    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        filter(lambda p: p.wpan.channel == TERNARY_CHANNEL).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
