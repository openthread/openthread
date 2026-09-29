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
from pktverify.null_field import nullField


def verify(pv):
    # 5.6.7 Request Network Data Updates – REED device
    #
    # 5.6.7.1 Topology
    # - RF isolation is required for this test case.
    # - An additional, live stand-alone sniffer is recommended to monitor the DUT’s Child Update Request cycle
    #   timing.
    # - Leader is configured as Border Router.
    # - Build a topology that has a total of 16 active routers on the network, including the Leader, with no
    #   communication constraints.
    #
    # 5.6.7.2 Purpose & Description
    # The purpose of this test case is to verify that the DUT identifies that it has an old version of the Network
    #   Data and requests an update from its parent.
    #
    # Spec Reference               | V1.1 Section | V1.3.0 Section
    # -----------------------------|--------------|---------------
    # Network Data and Propagation | 5.15         | 5.15

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    REED_1 = pv.vars['REED_1']

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A.
    print("Step 1: All")

    # Step 2: All
    # - Description: Wait for 630 seconds to elapse (570s REED_ADVERTISEMENT_INTERVAL + 60s
    #   REED_ADVERTISEMENT_MAX_JITTER).
    # - Pass Criteria: N/A.
    print("Step 2: All")

    # Step 3: REED_1 (DUT)
    # - Description: User places the DUT in RF isolation for time < REED timeout value (30 seconds). It is useful to
    #   monitor the DUT’s Child Update Request cycle timing and, if prudent, wait to execute this step until just
    #   after the cycle has completed. If the Child Update cycle occurs while the DUT is in RF isolation, the test
    #   will fail because the DUT will go through (re) attachment when it emerges.
    # - Pass Criteria: N/A.
    print("Step 3: REED_1 (DUT)")

    # Step 4: Leader
    # - Description: Harness updates the Network Data by configuring the Leader with the following Prefix Set:
    #   - Prefix 1: P_Prefix=2003::/64 P_stable=1 P_default=1 P_slaac=1 P_on_mesh=1 P_preferred=1.
    #   - The Leader multicasts an MLE Data Response containing the new information. The Network Data TLV includes
    #     the following fields:
    #     - Prefix TLV, including:
    #       - Border Router sub-TLV
    #       - 6LoWPAN ID sub-TLV.
    # - Pass Criteria: N/A.
    print("Step 4: Leader")
    pkts.filter_wpan_src64(LEADER).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
                          consts.NETWORK_DATA_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 5: All Routers
    # - Description: Automatically multicast the MLE Data Response sent by the Leader device.
    # - Pass Criteria: N/A.
    print("Step 5: All Routers")

    # Step 6: REED_1 (DUT)
    # - Description: User removes the RF isolation after time < REED timeout value (30 seconds).
    # - Pass Criteria: N/A.
    print("Step 6: REED_1 (DUT)")

    # Step 7: All
    # - Description: Wait for 630 seconds to elapse (570s REED_ADVERTISEMENT_INTERVAL + 60s
    #   REED_ADVERTISEMENT_MAX_JITTER).
    # - Pass Criteria: N/A.
    print("Step 7: All")

    # Step 8: REED_1 (DUT)
    # - Description: Hears an incremented Data Version in the MLE Advertisement messages sent by its Parent and
    #   automatically requests the updated network data.
    # - Pass Criteria:
    #   - The DUT MUST send an MLE Data Request to its parent to get the new Network Dataset.
    #   - The MLE Data Request MUST include a TLV Request TLV for the Network Data TLV.
    print("Step 8: REED_1 (DUT)")
    pkts.filter_wpan_src64(REED_1).\
        filter_wpan_dst64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_DATA_REQUEST).\
        filter(lambda p: {
                          consts.TLV_REQUEST_TLV,
                          consts.NETWORK_DATA_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 9: REED_1 (DUT)
    # - Description: Receives an MLE Data Response from its Parent.
    # - Pass Criteria: N/A.
    print("Step 9: REED_1 (DUT)")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(REED_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
                          consts.NETWORK_DATA_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 10: REED_1 (DUT)
    # - Description: Automatically broadcasts an MLE Advertisement.
    # - Pass Criteria: The VN_version in the Leader Data TLV of the advertisement MUST be incremented for new network
    #   data.
    print("Step 10: REED_1 (DUT)")
    pkts.filter_wpan_src64(REED_1).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter(lambda p: {
                          consts.LEADER_DATA_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
