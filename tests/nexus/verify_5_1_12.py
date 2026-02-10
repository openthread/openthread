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
    # 5.1.12 New Router Neighbor Synchronization
    #
    # 5.1.12.1 Topology
    # Topology information is not explicitly detailed in the text, but the procedure involves Router_1 (DUT) and
    # Router_2.
    #
    # 5.1.12.2 Purpose & Description
    # The purpose of this test case is to validate that when the DUT sees a new router for the first time, it will
    # synchronize using the New Router Neighbor Synchronization procedure.
    #
    # Spec Reference                     | V1.1 Section | V1.3.0 Section
    # -----------------------------------|--------------|---------------
    # New Router Neighbor Synchronization| 4.7.7.2      | 4.7.1.2

    pkts = pv.pkts
    pv.summary.show()

    DUT = pv.vars['DUT']
    ROUTER_2 = pv.vars['ROUTER_2']

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A
    print("Step 1: Ensure topology is formed correctly.")

    # Step 2: Router_1 (DUT)
    # - Description: Automatically transmits MLE advertisements.
    # - Pass Criteria:
    #   - The DUT MUST send properly formatted MLE Advertisements with an IP Hop Limit of 255 to the Link-Local All
    #     Nodes multicast address (FF02::1).
    #   - The following TLVs MUST be present in the Advertisements:
    #     - Leader Data TLV
    #     - Route64 TLV
    #     - Source Address TLV
    print("Step 2: Router_1 (DUT) transmits MLE advertisements.")
    pkts.filter_wpan_src64(DUT) \
        .filter_LLANMA() \
        .filter_mle_cmd(consts.MLE_ADVERTISEMENT) \
        .filter(lambda p: {
                          consts.LEADER_DATA_TLV,
                          consts.ROUTE64_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type) and \
               p.ipv6.hlim == 255) \
        .must_next()

    # Step 3: Test Harness
    # - Description: Harness enables communication between Router_1 (DUT) and Router_2.
    # - Pass Criteria: N/A
    print("Step 3: Harness enables communication between Router_1 (DUT) and Router_2.")

    # Step 4: Router_1 (DUT) OR Router_2
    # - Description: The DUT and Router_2 automatically exchange unicast Link Request and unicast Link Accept messages
    #   OR Link Accept and Request messages.
    # - Pass Criteria:
    #   - Link Request messages MUST be Unicast.
    #   - The following TLVs MUST be present in the Link Request messages:
    #     - Challenge TLV
    #     - Leader Data TLV
    #     - TLV Request TLV: Link Margin TLV
    #     - Source Address TLV
    #     - Version TLV
    #   - Link Accept or Link Accept and Request Messages MUST be Unicast.
    #   - The following TLVs MUST be present in the Link Accept or Link Accept And Request Messages:
    #     - Leader Data TLV
    #     - Link-layer Frame Counter TLV
    #     - Link Margin TLV
    #     - Response TLV
    #     - Source Address TLV
    #     - Version TLV
    #     - TLV Request TLV: Link Margin TLV (situational)*
    #     - Challenge TLV (situational)*
    #     - MLE Frame Counter TLV (optional)
    print("Step 4: The DUT and Router_2 exchange Link Request and Link Accept messages.")

    # In this test, DUT should send Link Request to Router_2 when it hears Router_2's advertisement
    # It might also send Link Accept and Request if it heard a Link Request from Router_2 first.
    pkts.filter_wpan_src64(DUT) \
        .filter_wpan_dst64(ROUTER_2) \
        .filter(lambda p: p.mle.cmd in [consts.MLE_LINK_REQUEST, consts.MLE_LINK_ACCEPT_AND_REQUEST] and \
               {
                consts.CHALLENGE_TLV,
                consts.LEADER_DATA_TLV,
                consts.TLV_REQUEST_TLV,
                consts.SOURCE_ADDRESS_TLV,
                consts.VERSION_TLV
                } <= set(p.mle.tlv.type)) \
        .must_next()

    pkts.filter_wpan_src64(ROUTER_2) \
        .filter_wpan_dst64(DUT) \
        .filter(lambda p: p.mle.cmd in [consts.MLE_LINK_ACCEPT, consts.MLE_LINK_ACCEPT_AND_REQUEST] and \
               {
                consts.LEADER_DATA_TLV,
                consts.LINK_LAYER_FRAME_COUNTER_TLV,
                consts.LINK_MARGIN_TLV,
                consts.RESPONSE_TLV,
                consts.SOURCE_ADDRESS_TLV,
                consts.VERSION_TLV
                } <= set(p.mle.tlv.type)) \
        .must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
