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
    # 6.1.7 End Device Synchronization
    #
    # 6.1.7.1 Topology
    #   DUT as Full End Device (FED)
    #   Leader
    #   Router_1
    #   Router_2
    #   Router_3
    #
    # 6.1.7.2 Purpose & Description
    #   The purpose of this test case is to validate End Device Synchronization on the DUT.
    #
    #   Spec Reference                | V1.1 Section | V1.3.0 Section
    #   ------------------------------|--------------|---------------
    #   REED and FED Synchronization  | 4.7.7.4      | 4.7.1.4

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_2 = pv.vars['ROUTER_2']
    ROUTER_3 = pv.vars['ROUTER_3']
    DUT = pv.vars['DUT']

    # Step 1: All
    # - Description: Setup the topology without the DUT. Ensure all routers and the Leader are sending MLE
    #   advertisements.
    # - Pass Criteria: N/A
    print("Step 1: All")

    # Step 2: FED (DUT)
    # - Description: Automatically attaches to the Leader.
    # - Pass Criteria:
    #   - The DUT MUST unicast MLE Child ID Request to the Leader.
    print("Step 2: FED (DUT)")
    pkts.filter_wpan_src64(DUT).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        must_next()

    # Step 3: FED (DUT)
    # - Description: Automatically sends Link Requests to Router_1, Router_2 & Router_3.
    # - Pass Criteria:
    #   - The DUT MUST unicast Link Requests to each Router which contains the following TLVs:
    #     - Challenge TLV
    #     - Leader Data TLV
    #     - Source Address TLV
    #     - Version TLV
    print("Step 3: FED (DUT)")
    step3_start_pkts = pkts.copy()
    max_idx = pkts.index
    for router in [ROUTER_1, ROUTER_2, ROUTER_3]:
        f = step3_start_pkts.copy().\
            filter_wpan_src64(DUT).\
            filter_wpan_dst64(router).\
            filter_mle_cmd(consts.MLE_LINK_REQUEST).\
            filter(lambda p: {
                              consts.CHALLENGE_TLV,
                              consts.LEADER_DATA_TLV,
                              consts.SOURCE_ADDRESS_TLV,
                              consts.VERSION_TLV
                             } <= set(p.mle.tlv.type))
        f.must_next()
        max_idx = max(max_idx, f.index)

    # Step 4: Router_1, Router_2, Router_3
    # - Description: Automatically send Link Accept to the DUT.
    # - Pass Criteria: N/A
    print("Step 4: Router_1, Router_2, Router_3")
    for router in [ROUTER_1, ROUTER_2, ROUTER_3]:
        f = step3_start_pkts.copy().\
            filter_wpan_src64(router).\
            filter_wpan_dst64(DUT).\
            filter(lambda p: p.mle.cmd in [consts.MLE_LINK_ACCEPT, consts.MLE_LINK_ACCEPT_AND_REQUEST])
        f.must_next()
        max_idx = max(max_idx, f.index)

    pkts.index = max_idx


if __name__ == '__main__':
    verify_utils.run_main(verify)
