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
    # 5.1.11 Attaching to a REED with better link quality
    #
    # 5.1.11.1 Topology
    # - Leader
    # - REED_1
    # - Router_2
    # - Router_1 (DUT)
    #
    # 5.1.11.2 Purpose & Description
    # The purpose of this test case is to validate that DUT will attach to a REED with the highest link quality,
    # when routers with the highest link quality are not available.
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # Parent Selection | 4.7.2        | 4.5.2

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    REED_1 = pv.vars['REED_1']
    ROUTER_2 = pv.vars['ROUTER_2']
    DUT = pv.vars['DUT']

    # Step 1: Leader, REED_1, Router_2
    # - Description: Setup the topology without the DUT. Verify Leader and Router_2 are sending MLE
    #   Advertisements.
    # - Pass Criteria: N/A
    print("Step 1: Leader, REED_1, Router_2")

    # Step 2: Test Harness
    # - Description: Harness configures the RSSI between Router_2 & Router_1 (DUT) to enable a link quality of 2
    #   (medium).
    # - Pass Criteria: N/A
    print("Step 2: Test Harness")

    # Step 3: Router_1 (DUT)
    # - Description: Automatically sends a MLE Parent Request.
    # - Pass Criteria:
    #   - The DUT MUST send MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an IP
    #     Hop Limit of 255.
    #   - The following TLVs MUST be present in the MLE Parent Request:
    #     - Challenge TLV
    #     - Mode TLV
    #     - Scan Mask TLV = 0x80 (active Routers)
    #     - Version TLV
    print("Step 3: Router_1 (DUT) sends MLE Parent Request to active Routers")
    pkts.filter_wpan_src64(DUT).\
        filter_LLARMA().\
        filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        filter(lambda p: {
                          consts.CHALLENGE_TLV,
                          consts.MODE_TLV,
                          consts.SCAN_MASK_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type) and\
               p.ipv6.hlim == 255 and\
               p.mle.tlv.scan_mask.r == 1 and\
               p.mle.tlv.scan_mask.e == 0).\
        must_next()

    # Step 4: Router_2
    # - Description: Automatically responds to DUT with MLE Parent Response.
    # - Pass Criteria: N/A
    print("Step 4: Router_2 responds with MLE Parent Response")
    pkts.filter_wpan_src64(ROUTER_2).\
        filter_wpan_dst64(DUT).\
        filter_mle_cmd(consts.MLE_PARENT_RESPONSE).\
        must_next()

    # Step 5: Router_1 (DUT)
    # - Description: Automatically sends another MLE Parent Request - to Routers and REEDs - when it doesn’t see the
    #   highest link quality in Router_2’s response.
    # - Pass Criteria:
    #   - The DUT MUST send MLE Parent Request with the Scan Mask set to All Routers and REEDs.
    #   - The following TLVs MUST be present in the MLE Parent Request:
    #     - Challenge TLV
    #     - Mode TLV
    #     - Scan Mask TLV = 0xC0 (Routers and REEDs)
    #     - Version TLV
    print("Step 5: Router_1 (DUT) sends MLE Parent Request to Routers and REEDs")
    pkts.filter_wpan_src64(DUT).\
        filter_LLARMA().\
        filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        filter(lambda p: {
                          consts.CHALLENGE_TLV,
                          consts.MODE_TLV,
                          consts.SCAN_MASK_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type) and\
               p.ipv6.hlim == 255 and\
               p.mle.tlv.scan_mask.r == 1 and\
               p.mle.tlv.scan_mask.e == 1).\
        must_next()

    # Step 6: Router_1 (DUT)
    # - Description: Automatically sends MLE Child ID Request to REED_1 due to its better link quality.
    # - Pass Criteria:
    #   - The DUT MUST unicast MLE Child ID Request to REED_1, including the following TLVs:
    #     - Link-layer Frame Counter TLV
    #     - Mode TLV
    #     - Response TLV
    #     - Timeout TLV
    #     - TLV Request TLV
    #     - Version TLV
    #     - MLE Frame Counter TLV (optional)
    #   - The following TLV MUST NOT be present in the Child ID Request:
    #     - Address Registration TLV
    print("Step 6: Router_1 (DUT) sends MLE Child ID Request to REED_1")
    pkt = pkts.filter_wpan_src64(DUT).\
        filter_wpan_dst64(REED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter(lambda p: {
                          consts.LINK_LAYER_FRAME_COUNTER_TLV,
                          consts.MODE_TLV,
                          consts.RESPONSE_TLV,
                          consts.TIMEOUT_TLV,
                          consts.TLV_REQUEST_TLV,
                          consts.VERSION_TLV,
                          consts.ADDRESS16_TLV,
                          consts.NETWORK_DATA_TLV
                          } <= set(p.mle.tlv.type) and\
               p.mle.tlv.addr16 is nullField and\
               p.thread_nwd.tlv.type is nullField).\
        must_next()
    pkt.must_not_verify(lambda p: (consts.ADDRESS_REGISTRATION_TLV) in p.mle.tlv.type)


if __name__ == '__main__':
    verify_utils.run_main(verify)
