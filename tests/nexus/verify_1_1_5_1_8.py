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
    # 5.1.8 Attaching to a Router with better connectivity
    #
    # 5.1.8.1 Topology
    # - Leader
    # - Router_1
    # - Router_2
    # - Router_3
    # - Router_4 (DUT)
    #
    # 5.1.8.2 Purpose & Description
    # The purpose of this test case is to verify that the DUT chooses to attach to a router with better connectivity.
    # - In order for this test case to be valid, the link quality between all nodes must be of the highest quality (3).
    #   If this condition cannot be met, the test case is invalid.
    #
    # Spec Reference                             | V1.1 Section    | V1.3.0 Section
    # -------------------------------------------|-----------------|-----------------
    # Parent Selection                           | 4.7.2           | 4.5.2

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_2 = pv.vars['ROUTER_2']
    ROUTER_3 = pv.vars['ROUTER_3']
    DUT = pv.vars['ROUTER_4']

    # Step 1: Leader, Router_1, Router_2, Router_3
    # - Description: Setup the topology without the DUT. Verify all routers and Leader are sending MLE advertisements.
    # - Pass Criteria: N/A
    print("Step 1: Leader, Router_1, Router_2, Router_3")
    for node in [LEADER, ROUTER_1, ROUTER_2, ROUTER_3]:
        pkts.filter_wpan_src64(node).\
            filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
            must_next()

    # Step 2: Test Harness
    # - Description: Harness configures the RSSI between Router_1, Router_2, and Router_3 and Router_4 (DUT) to enable
    #   a link quality of 3 (highest).
    # - Pass Criteria: N/A
    print("Step 2: Test Harness")

    # Step 3: Router_4 (DUT)
    # - Description: Automatically begins attach process by sending a multicast MLE Parent Request.
    # - Pass Criteria:
    #   - The DUT MUST send MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an IP Hop
    #     Limit of 255.
    #   - The following TLVs MUST be present in the MLE Parent Request:
    #     - Challenge TLV
    #     - Mode TLV
    #     - Scan Mask TLV = 0x80 (Active Routers)
    #     - Version TLV
    print("Step 3: Router_4 (DUT)")
    pkts.filter_wpan_src64(DUT).\
        filter_LLARMA().\
        filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        filter(lambda p: {consts.CHALLENGE_TLV, consts.MODE_TLV, consts.SCAN_MASK_TLV, consts.VERSION_TLV} <= set(p.mle.tlv.type)).\
        filter(lambda p: p.ipv6.hlim == 255).\
        filter(lambda p: p.mle.tlv.scan_mask.r == 1).\
        filter(lambda p: p.mle.tlv.scan_mask.e == 0).\
        must_next()

    # Step 4: Router_2, Router_3
    # - Description: Each device automatically responds to the DUT with MLE Parent Response.
    # - Pass Criteria: N/A
    print("Step 4: Router_2, Router_3")
    _start_idx = pkts.index
    pkts.filter_wpan_src64(ROUTER_2).\
        filter_wpan_dst64(DUT).\
        filter_mle_cmd(consts.MLE_PARENT_RESPONSE).\
        must_next()

    pkts.range(_start_idx).\
        filter_wpan_src64(ROUTER_3).\
        filter_wpan_dst64(DUT).\
        filter_mle_cmd(consts.MLE_PARENT_RESPONSE).\
        must_next()

    # Step 5: Router_4 (DUT)
    # - Description: Automatically sends a MLE Child ID Request to Router_3 due to better connectivity.
    # - Pass Criteria:
    #   - The DUT MUST unicast MLE Child ID Request to Router_3, including the following TLVs:
    #     - Link-layer Frame Counter TLV
    #     - Mode TLV
    #     - Response TLV
    #     - Timeout TLV
    #     - TLV Request
    #     - Version TLV
    #     - MLE Frame Counter TLV (optional)
    #   - The following TLV MUST NOT be present in the Child ID Request:
    #     - Address Registration TLV
    print("Step 5: Router_4 (DUT)")
    # We must reset the range to find Child ID Request because Step 4 might have advanced past it
    # depending on which Parent Response came last.
    child_id_req = pkts.range(_start_idx).\
        filter_wpan_src64(DUT).\
        filter_wpan_dst64(ROUTER_3).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter(lambda p: {
            consts.LINK_LAYER_FRAME_COUNTER_TLV,
            consts.MODE_TLV,
            consts.RESPONSE_TLV,
            consts.TIMEOUT_TLV,
            consts.TLV_REQUEST_TLV,
            consts.VERSION_TLV
        } <= set(p.mle.tlv.type)).\
        must_next()

    child_id_req.must_not_verify(lambda p: consts.ADDRESS_REGISTRATION_TLV in p.mle.tlv.type)


if __name__ == '__main__':
    verify_utils.run_main(verify)
