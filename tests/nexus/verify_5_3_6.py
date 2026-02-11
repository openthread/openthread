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
    # 5.3.6 Router ID Mask
    #
    # 5.3.6.1 Topology
    # - Router_1
    # - Router_2
    # - Leader (DUT)
    #
    # 5.3.6.2 Purpose & Description
    # The purpose of this test case is to verify that the router ID mask is managed correctly, as the connectivity to a
    #   router or group of routers is lost and / or a new router is added to the network.
    #
    # Spec Reference        | V1.1 Section | V1.3.0 Section
    # ----------------------|--------------|---------------
    # Router ID Management  | 5.9.9        | 5.9.9

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_2 = pv.vars['ROUTER_2']

    # Step 1: All
    #
    #   - Description: Ensure topology is formed correctly.
    #   - Pass Criteria: N/A
    print("Step 1: All")
    pkts.filter_mle_cmd(consts.MLE_ADVERTISEMENT).filter_wpan_src64(LEADER).must_next()
    router1_pkt = pkts.filter_mle_cmd(consts.MLE_ADVERTISEMENT).filter_wpan_src64(ROUTER_1).must_next()
    router1_id = (router1_pkt.mle.tlv.source_addr >> 10)
    router2_pkt = pkts.filter_mle_cmd(consts.MLE_ADVERTISEMENT).filter_wpan_src64(ROUTER_2).must_next()
    router2_id = (router2_pkt.mle.tlv.source_addr >> 10)

    # Step 2: Router_2
    #
    #   - Description: Harness silently disables the device.
    #   - Pass Criteria: N/A
    print("Step 2: Router_2")

    # Step 3: Delay
    #
    #   - Description: Pause for 12 minutes.
    #   - Pass Criteria: N/A
    print("Step 3: Delay")

    # Step 4: Leader (DUT)
    #
    #   - Description: The DUT updates its routing cost and ID set.
    #   - Pass Criteria:
    #     - The DUT’s routing cost to Router_2 MUST count to infinity.
    #     - The DUT MUST remove Router_2 ID from its ID set.
    #     - Verify route data has settled.
    print("Step 4: Leader (DUT)")

    # Check for MLE Advertisement from Leader showing Router 2 ID removed.
    # We wait for the advertisement that happens after the 12 minutes delay.
    pkts.filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter_wpan_src64(LEADER).\
        filter(lambda p: router2_id not in p.mle.tlv.route64.id_mask).\
        must_next()

    # Step 5: Router_2
    #
    #   - Description: Harness re-enables the device and waits for it to reattach and upgrade to a
    #     router.
    #   - Pass Criteria:
    #     - The DUT MUST reset the MLE Advertisement trickle timer and send an Advertisement.
    print("Step 5: Router_2")
    # Verify Router 2 becomes router again (sends advertisement) and get its new ID
    router2_rejoin_pkt = pkts.filter_mle_cmd(consts.MLE_ADVERTISEMENT).filter_wpan_src64(ROUTER_2).must_next()
    router2_id_reattached = (router2_rejoin_pkt.mle.tlv.source_addr >> 10)

    # Verify Leader sends advertisement after Router 2 re-joins
    pkts.filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter_wpan_src64(LEADER).\
        filter(lambda p: router2_id_reattached in p.mle.tlv.route64.id_mask).\
        must_next()

    # Step 6: Router_1, Router_2
    #
    #   - Description: Harness silently disables both devices.
    #   - Pass Criteria:
    #     - The DUT’s routing cost to Router_1 MUST go directly to infinity as there is no
    #       multi-hop cost for Router_1.
    #     - The DUT MUST remove Router_1 & Router_2 IDs from its ID set.
    print("Step 6: Router_1, Router_2")

    # Verify Leader advertisement shows both IDs removed
    pkts.filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter_wpan_src64(LEADER).\
        filter(lambda p: router1_id not in p.mle.tlv.route64.id_mask and
               router2_id_reattached not in p.mle.tlv.route64.id_mask).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
