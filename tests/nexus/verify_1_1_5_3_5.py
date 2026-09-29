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

# ICMPv6 Echo Request identifier used in the C++ test.
ECHO_IDENTIFIER = 0x1234


def get_hops(p):
    return p.lowpan.mesh.hops8 if hasattr(p.lowpan.mesh, 'hops8') else p.lowpan.mesh.hops


def verify(pv):
    # 5.3.5 Routing - Link Quality
    #
    # 5.3.5.1 Topology
    # - Leader
    # - Router_1 (DUT)
    # - Router_2
    # - Router_3
    #
    # 5.3.5.2 Purpose & Description
    # The purpose of this test case is to ensure that the DUT routes traffic properly when link qualities between the
    #   nodes are adjusted.
    #
    # Spec Reference                                   | V1.1 Section   | V1.3.0 Section
    # -------------------------------------------------|----------------|---------------
    # Routing Protocol / Full Thread Device Forwarding | 5.9 / 5.10.1.1 | 5.9 / 5.10.1.1

    pkts = pv.pkts
    pv.summary.show()

    LEADER_RLOC16 = pv.vars['LEADER_RLOC16']
    DUT_RLOC16 = pv.vars['DUT_RLOC16']
    ROUTER_2_RLOC16 = pv.vars['ROUTER_2_RLOC16']
    ROUTER_3_RLOC16 = pv.vars['ROUTER_3_RLOC16']

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A
    print("Step 1: All")

    # Step 2: Harness
    # - Description: Modifies the link quality between the DUT and the Leader to be 3.
    # - Pass Criteria: N/A
    print("Step 2: Harness")

    # Step 3: Router_3
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request to the Leader.
    # - Pass Criteria:
    #   - The ICMPv6 Echo Request MUST take the shortest path: Router_3 -> DUT -> Leader.
    #   - The hopsLft field of the 6LoWPAN Mesh Header MUST be greater than the route cost to the destination.
    print("Step 3: Router_3")
    # Router_3 -> DUT
    p1 = pkts.filter_wpan_src16(ROUTER_3_RLOC16).\
        filter_wpan_dst16(DUT_RLOC16).\
        filter_ping_request(identifier=ECHO_IDENTIFIER).\
        must_next()
    # DUT -> Leader
    p2 = pkts.filter_wpan_src16(DUT_RLOC16).\
        filter_wpan_dst16(LEADER_RLOC16).\
        filter_ping_request(identifier=ECHO_IDENTIFIER).\
        filter(lambda p: p.lowpan.mesh.hops > 1).\
        must_next()
    assert get_hops(p2) == get_hops(p1) - 1, f'Hops left not decremented correctly: {get_hops(p1)} -> {get_hops(p2)}'

    # Step 4: Harness
    # - Description: Sets the link quality between the Leader and the DUT to 1.
    # - Pass Criteria: N/A
    print("Step 4: Harness")

    # Step 5: Router_3
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request to the Leader.
    # - Pass Criteria:
    #   - The ICMPv6 Echo Request MUST take the longer path: Router_3 -> DUT -> Router_2 -> Leader.
    #   - The hopsLft field of the 6LoWPAN Mesh Header MUST be greater than the route cost to the destination.
    print("Step 5: Router_3")
    # Router_3 -> DUT
    p1 = pkts.filter_wpan_src16(ROUTER_3_RLOC16).\
        filter_wpan_dst16(DUT_RLOC16).\
        filter_ping_request(identifier=ECHO_IDENTIFIER).\
        must_next()
    # DUT -> Router_2
    p2 = pkts.filter_wpan_src16(DUT_RLOC16).\
        filter_wpan_dst16(ROUTER_2_RLOC16).\
        filter_ping_request(identifier=ECHO_IDENTIFIER).\
        filter(lambda p: p.lowpan.mesh.hops > 2).\
        must_next()
    assert get_hops(p2) == get_hops(p1) - 1, f'Hops left not decremented correctly: {get_hops(p1)} -> {get_hops(p2)}'
    # Router_2 -> Leader
    p3 = pkts.filter_wpan_src16(ROUTER_2_RLOC16).\
        filter_wpan_dst16(LEADER_RLOC16).\
        filter_ping_request(identifier=ECHO_IDENTIFIER).\
        filter(lambda p: p.lowpan.mesh.hops > 1).\
        must_next()
    assert get_hops(p3) == get_hops(p2) - 1, f'Hops left not decremented correctly: {get_hops(p2)} -> {get_hops(p3)}'

    # Step 6: Harness
    # - Description: Sets the link quality between the Leader and the DUT to 2.
    # - Pass Criteria: N/A
    print("Step 6: Harness")

    # Step 7: Router_3
    # - Description: Harness instructs device to send an ICMPv6 Echo Request to the Leader.
    # - Pass Criteria:
    #   - The DUT MUST have two paths with the same cost, and MUST prioritize sending to a direct neighbor:
    #     Router_3 -> DUT -> Leader.
    #   - The hopsLft field of the 6LoWPAN Mesh Header MUST be greater than the route cost to the destination.
    print("Step 7: Router_3")
    # Router_3 -> DUT
    p1 = pkts.filter_wpan_src16(ROUTER_3_RLOC16).\
        filter_wpan_dst16(DUT_RLOC16).\
        filter_ping_request(identifier=ECHO_IDENTIFIER).\
        must_next()
    # DUT -> Leader
    p2 = pkts.filter_wpan_src16(DUT_RLOC16).\
        filter_wpan_dst16(LEADER_RLOC16).\
        filter_ping_request(identifier=ECHO_IDENTIFIER).\
        filter(lambda p: p.lowpan.mesh.hops > 2).\
        must_next()
    assert get_hops(p2) == get_hops(p1) - 1, f'Hops left not decremented correctly: {get_hops(p1)} -> {get_hops(p2)}'

    # Step 8: Harness
    # - Description: Sets the link quality between the Leader and the DUT to 0 (infinite).
    # - Pass Criteria: N/A
    print("Step 8: Harness")

    # Step 9: Router_3
    # - Description: Harness instructs device to send an ICMPv6 Echo Request from to the Leader.
    # - Pass Criteria:
    #   - The ICMPv6 Echo Request MUST follow the longer path: Router_3 -> DUT -> Router_2 -> Leader.
    #   - The hopsLft field of the 6LoWPAN Mesh Header MUST be greater than the route cost to the destination.
    print("Step 9: Router_3")
    # Router_3 -> DUT
    p1 = pkts.filter_wpan_src16(ROUTER_3_RLOC16).\
        filter_wpan_dst16(DUT_RLOC16).\
        filter_ping_request(identifier=ECHO_IDENTIFIER).\
        must_next()
    # DUT -> Router_2
    p2 = pkts.filter_wpan_src16(DUT_RLOC16).\
        filter_wpan_dst16(ROUTER_2_RLOC16).\
        filter_ping_request(identifier=ECHO_IDENTIFIER).\
        filter(lambda p: p.lowpan.mesh.hops > 2).\
        must_next()
    assert get_hops(p2) == get_hops(p1) - 1, f'Hops left not decremented correctly: {get_hops(p1)} -> {get_hops(p2)}'
    # Router_2 -> Leader
    p3 = pkts.filter_wpan_src16(ROUTER_2_RLOC16).\
        filter_wpan_dst16(LEADER_RLOC16).\
        filter_ping_request(identifier=ECHO_IDENTIFIER).\
        filter(lambda p: p.lowpan.mesh.hops > 1).\
        must_next()
    assert get_hops(p3) == get_hops(p2) - 1, f'Hops left not decremented correctly: {get_hops(p2)} -> {get_hops(p3)}'


if __name__ == '__main__':
    verify_utils.run_main(verify)
