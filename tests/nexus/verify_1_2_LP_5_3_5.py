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
import verify_utils
from pktverify import consts

# Constants from C++ test
ECHO_IDENTIFIER_STEP3 = 0x1234
ECHO_IDENTIFIER_STEP6 = 0x5678
PRIMARY_CHANNEL = 11
SECONDARY_CHANNEL = 26


def verify(pv):
    # 5.3.5 Minimum number of SSED Support
    #
    # 5.3.5.1 Topology
    # - Leader
    # - Router_1 (DUT)
    # - SSED_1
    # - SSED_2
    # - SSED_3
    # - SSED_4
    # - SSED_5
    # - SSED_6
    #
    # 5.3.5.2 Purpose and Description
    # The purpose of this test is to verify that a Router can reliably support
    #   a minimum of 6 SSED children simultaneously that are each using a
    #   different CSL channel.
    #
    # - SSED_1 and SSED_2 are each configured with a CSL Synchronized
    #   Timeout of 10 seconds.
    # - SSED_3 and SSED_4 are each configured with a CSL Synchronized
    #   Timeout of 20 seconds.
    # - SSED_5 and SSED_6 are each configured with a CSL Synchronized
    #   Timeout of 30 seconds.
    #
    # SSED 1 and 6 are configured to use the Primary and Secondary harness
    #   channels, respectively. The other four SSEDs are configured to run
    #   on another four available random channels. No over-the-air
    #   captures are generated for these four SSEDs.
    #
    # SPEC Section: 3.2.6.3.2

    pkts = pv.pkts
    pv.summary.show()

    DUT = pv.vars['DUT']
    LEADER = pv.vars['LEADER']

    NUM_SSEDS = 6
    SSEDS = [pv.vars[f'SSED_{i}'] for i in range(1, NUM_SSEDS + 1)]
    SSED_1, SSED_2, SSED_3, SSED_4, SSED_5, SSED_6 = SSEDS

    LEADER_MLEID = pv.vars['LEADER_MLEID']
    SSED_MLEIDS = [pv.vars[f'SSED_{i}_MLEID'] for i in range(1, NUM_SSEDS + 1)]
    SSED_1_MLEID, SSED_2_MLEID, SSED_3_MLEID, SSED_4_MLEID, SSED_5_MLEID, SSED_6_MLEID = SSED_MLEIDS

    # Use RLOC16s from JSON as they are reliable in Nexus
    def _to_int(val):
        return int(val, 16) if isinstance(val, str) else val

    DUT_RLOC16 = _to_int(pv.vars['DUT_RLOC16'])
    LEADER_RLOC16 = _to_int(pv.vars['LEADER_RLOC16'])

    # Step 0: SSED_1-6
    # - Description: Preconditions:
    #   - Set CSL Period = 500ms
    #   - SSED_1, _2: Set CSL Synchronized Timeout = 10s
    #   - SSED_3, _4: Set CSL Synchronized Timeout = 20s
    #   - SSED_5, _6: Set CSL Synchronized Timeout = 30s
    # - Pass Criteria: N/A
    print("Step 0: SSED_1-6")

    # Step 1: All
    # - Description: Topology formation: DUT, SSED_1, SSED_2, SSED_3,
    #   SSED_4, SSED_5, SSED_6.
    # - Pass Criteria: N/A
    print("Step 1: All")

    # Just advance past some initial advertisements to establish a baseline
    pkts.filter_wpan_src64(DUT).filter_mle_cmd(consts.MLE_ADVERTISEMENT).must_next()

    # Step 2: SSED_1, SSED_2, SSED_3, SSED_4, SSED_5, SSED_6
    # - Description: Each device automatically attaches to the DUT and
    #   establishes CSL synchronization.
    # - Pass Criteria:
    #   - 2.1: The DUT MUST unicast MLE Child ID Response to SSED_1.
    #   - 2.2: The DUT MUST unicast MLE Child Update Response to SSED_1.
    #   - 2.3: The DUT MUST unicast MLE Child ID Response to SSED_2.
    #   - 2.4: The DUT MUST unicast MLE Child ID Response to SSED_3.
    #   - 2.5: The DUT MUST unicast MLE Child ID Response to SSED_4.
    #   - 2.6: The DUT MUST unicast MLE Child ID Response to SSED_5.
    #   - 2.7: The DUT MUST unicast MLE Child ID Response to SSED_6.
    print("Step 2: SSED_1, SSED_2, SSED_3, SSED_4, SSED_5, SSED_6")

    # 2.1, 2.3-2.7: The DUT MUST unicast MLE Child ID Response to SSED_1-6.
    for ssed in SSEDS:
        with pkts.save_index():
            pkts.filter_wpan_src64(DUT).\
                filter_wpan_dst64(ssed).\
                filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
                must_next()

    # 2.2: The DUT MUST unicast MLE Child Update Response to SSED_1.
    with pkts.save_index():
        pkts.filter_wpan_src64(DUT).\
            filter_wpan_dst64(SSED_1).\
            filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
            must_next()

    # Advance index past Step 2 by finding the last packet of this phase,
    # which is the Child Update Response to SSED_1.
    pkts.filter_wpan_src64(DUT).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        must_next()

    def _verify_echo_cycle(step_str, echo_identifier):
        print(f"Step {step_str}: Leader")
        start_index = pkts.index

        # Echo Request from LEADER to DUT (for SSED_1)
        # We allow any source address from the leader as long as it's to SSED_1 and uses correct RLOC16s
        with pkts.save_index():
            _echo_req_leader_to_dut = pkts.filter_ping_request(identifier=echo_identifier).\
                filter_ipv6_dst(SSED_1_MLEID).\
                filter_wpan_src16(LEADER_RLOC16).\
                filter_wpan_dst16(DUT_RLOC16).\
                must_next()
            _idx1 = pkts.index

        # Forwarded Echo Request to SSED_1
        with pkts.save_index():
            _echo_req_ssed1 = pkts.filter_ping_request(identifier=echo_identifier).\
                filter_wpan_src16(DUT_RLOC16).\
                filter_ipv6_dst(SSED_1_MLEID).\
                filter(lambda p: p.wpan.channel == PRIMARY_CHANNEL).\
                must_next()
            _idx2 = pkts.index

        # Forwarded Echo Request to SSED_6
        with pkts.save_index():
            pkts.filter_ping_request(identifier=echo_identifier).\
                filter_wpan_src16(DUT_RLOC16).\
                filter_ipv6_dst(SSED_6_MLEID).\
                filter(lambda p: p.wpan.channel == SECONDARY_CHANNEL).\
                must_next()

        # SSED_1 MUST NOT send a MAC Data Request prior to receiving the
        # ICMPv6 Echo Request
        pkts.range(_idx1, _idx2).\
            filter_wpan_src64(SSED_1).\
            filter_wpan_cmd(consts.WPAN_DATA_REQUEST).\
            must_not_next()

        print(f"Step {int(step_str)+1}: SSED_1, SSED_2, SSED_3, SSED_4, SSED_5, SSED_6")

        # Check for CSL IEs in Echo Replies from SSED_1 and SSED_6
        with pkts.save_index():
            pkts.filter_ping_reply(identifier=echo_identifier).\
                filter_wpan_src64(SSED_1).\
                filter_wpan_dst16(DUT_RLOC16).\
                filter(lambda p: consts.CSL_IE_ID in p.wpan.header_ie.id).\
                must_next()

        with pkts.save_index():
            pkts.filter_ping_reply(identifier=echo_identifier).\
                filter_wpan_src64(SSED_6).\
                filter_wpan_dst16(DUT_RLOC16).\
                filter(lambda p: consts.CSL_IE_ID in p.wpan.header_ie.id).\
                must_next()

        # DUT MUST forward an ICMPv6 Echo Reply from all SSEDs
        for ssed_mleid in SSED_MLEIDS:
            with pkts.save_index():
                pkts.filter_ping_reply(identifier=echo_identifier).\
                    filter_wpan_src16(DUT_RLOC16).\
                    filter_wpan_dst16(LEADER_RLOC16).\
                    filter_ipv6_src(ssed_mleid).\
                    must_next()

        # Advance index past replies by finding the last one, which is from SSED_6.
        pkts.filter_ping_reply(identifier=echo_identifier).\
            filter_wpan_src16(DUT_RLOC16).\
            filter_wpan_dst16(LEADER_RLOC16).\
            filter_ipv6_src(SSED_6_MLEID).\
            must_next()

    # Step 3: Leader
    # - Description: Harness verifies connectivity by instructing the
    #   device to send an ICMPv6 Echo Request to each SSED mesh-local
    #   address.
    # - Pass Criteria:
    #   - 3.1: The DUT MUST forward the ICMPv6 Echo Requests to SSED_1
    #     and SSED_6 on the correct channel.
    #   - 3.2: SSED_1 MUST NOT send a MAC Data Request prior to
    #     receiving the ICMPv6 Echo Request from the Leader.
    #
    # Step 4: SSED_1, SSED_2, SSED_3, SSED_4, SSED_5, SSED_6
    # - Description: Each device automatically replies with ICMPv6 Echo
    #   Reply. The CSL unsynchronized timer on the DUT should be reset
    #   to 0.
    # - Pass Criteria:
    #   - 4.1: The 802.15.4 Frame Headers for the SSED_1 and SSED_6
    #     ICMPv6 Echo Replies MUST include the CSL Period IE and CSL
    #     Phase IE.
    #   - 4.2: The DUT MUST forward an ICMPv6 Echo Reply from SSED_1.
    #   - 4.3: The DUT MUST forward an ICMPv6 Echo Reply from SSED_2.
    #   - 4.4: The DUT MUST forward an ICMPv6 Echo Reply from SSED_3.
    #   - 4.5: The DUT MUST forward an ICMPv6 Echo Reply from SSED_4.
    #   - 4.6: The DUT MUST forward an ICMPv6 Echo Reply from SSED_5.
    #   - 4.7: The DUT MUST forward an ICMPv6 Echo Reply from SSED_6.
    _verify_echo_cycle('3', ECHO_IDENTIFIER_STEP3)

    # Step 5: Harness
    # - Description: Harness waits for 35 seconds.
    # - Pass Criteria: N/A
    print("Step 5: Harness")

    # Step 6: Leader
    # - Description: Harness verifies connectivity by instructing the
    #   device to send an ICMPv6 Echo Request to each SSED mesh-local
    #   address.
    # - Pass Criteria:
    #   - 6.1: The DUT MUST forward the ICMPv6 Echo Requests to SSED_1
    #     and SSED_6 on the correct channel.
    #   - 6.2: SSED_1 MUST NOT send a MAC Data Request prior to
    #     receiving the ICMPv6 Echo Request from the Leader.
    #
    # Step 7: SSED_1, SSED_2, SSED_3, SSED_4, SSED_5, SSED_6
    # - Description: Each device automatically replies with ICMPv6 Echo
    #   Reply. The CSL unsynchronized timer on the DUT should be reset
    #   to 0.
    # - Pass Criteria:
    #   - 7.1: The 802.15.4 Frame Headers for the SSED_1 and SSED_6
    #     ICMPv6 Echo Replies MUST include the CSL Period IE and CSL
    #     Phase IE.
    #   - 7.2: The DUT MUST forward an ICMPv6 Echo Reply from SSED_1.
    #   - 7.3: The DUT MUST forward an ICMPv6 Echo Reply from SSED_2.
    #   - 7.4: The DUT MUST forward an ICMPv6 Echo Reply from SSED_3.
    #   - 7.5: The DUT MUST forward an ICMPv6 Echo Reply from SSED_4.
    #   - 7.6: The DUT MUST forward an ICMPv6 Echo Reply from SSED_5.
    #   - 7.7: The DUT MUST forward an ICMPv6 Echo Reply from SSED_6.
    _verify_echo_cycle('6', ECHO_IDENTIFIER_STEP6)


if __name__ == '__main__':
    verify_utils.run_main(verify)
