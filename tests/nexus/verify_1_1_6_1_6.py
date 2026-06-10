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
    # 6.1.6 Attaching to a REED with Better Link Quality
    #
    # 6.1.6.1 Topology
    # - Topology A: DUT as End Device (ED_1)
    # - Topology B: DUT as Sleepy End Device (SED_1)
    # - Leader
    # - Router_1: Link quality = 1
    # - REED_1: Link quality = 3
    #
    # 6.1.6.2 Purpose & Description
    # The purpose of this test is to verify that the DUT sends a second Parent Request to the all-routers and
    #   all-reeds multicast address if it gets a reply from the first Parent Request to the all-routers address
    #   with a bad link quality.
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # Parent Selection | 4.7.2        | 4.5.2

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    REED_1 = pv.vars['REED_1']

    if 'ED_1' in pv.vars:
        dut = pv.vars['ED_1']
        name = 'ED_1'
    elif 'SED_1' in pv.vars:
        dut = pv.vars['SED_1']
        name = 'SED_1'
    else:
        raise ValueError("Neither ED_1 nor SED_1 found in topology vars")

    # Step 1: All
    # - Description: Setup the topology without the DUT. Ensure all routers and leader are sending
    #   MLE advertisements.
    # - Pass Criteria: N/A
    print("Step 1: All")
    pkts.copy().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter_wpan_src64(LEADER).\
        must_next()
    pkts.copy().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter_wpan_src64(ROUTER_1).\
        must_next()

    # Step 2: Router_1
    # - Description: Harness configures the device to broadcast a link quality of 1 (bad).
    # - Pass Criteria: N/A
    print("Step 2: Router_1")

    # Step 3: ED_1 / SED_1 (DUT)
    # - Description: Automatically begins attach process by sending a multicast MLE Parent Request
    #   to the All-Routers multicast address with the Scan Mask TLV set for all Routers.
    # - Pass Criteria:
    #   - The DUT MUST send MLE Parent Request to the Link-Local All-Routers multicast address
    #     (FF02::2) with an IP Hop Limit of 255.
    #   - The following TLVs MUST be present in the Parent Request:
    #     - Challenge TLV
    #     - Mode TLV
    #     - Scan Mask TLV (Value = 0x80 (active Routers))
    #     - Version TLV
    print(f"Step 3: {name} (DUT)")
    pkts.filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        filter_LLARMA().\
        filter_wpan_src64(dut).\
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

    # Step 4: Router_1
    # - Description: Automatically responds with MLE Parent Response.
    # - Pass Criteria: N/A
    print("Step 4: Router_1")
    pkts.filter_mle_cmd(consts.MLE_PARENT_RESPONSE).\
        filter_wpan_src64(ROUTER_1).\
        must_next()

    # Step 5: ED_1 / SED_1 (DUT)
    # - Description: Automatically sends another multicast MLE Parent Request to the All-Routers
    #   multicast with the Scan Mask TLV set for all Routers and REEDs.
    # - Pass Criteria:
    #   - The DUT MUST send MLE Parent Request to the Link-Local All-Routers multicast address
    #     (FF02::2) with an IP Hop Limit of 255.
    #   - The following TLVs MUST be present in the Parent Request:
    #     - Challenge TLV
    #     - Mode TLV
    #     - Scan Mask TLV (Value = 0xC0 (Routers and REEDs))
    #     - Version TLV
    print(f"Step 5: {name} (DUT)")
    pkts.filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        filter_LLARMA().\
        filter_wpan_src64(dut).\
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

    # Step 6: REED_1
    # - Description: Automatically responds with MLE Parent Response (in addition to Router_1).
    # - Pass Criteria: N/A
    print("Step 6: REED_1")
    pkts.filter_mle_cmd(consts.MLE_PARENT_RESPONSE).\
        filter_wpan_src64(REED_1).\
        must_next()

    # Step 7: ED_1 / SED_1 (DUT)
    # - Description: Automatically sends MLE Child ID Request to REED_1 due to better link quality.
    # - Pass Criteria:
    #   - The DUT MUST unicast MLE Child ID Request to REED_1.
    #   - The following TLVs MUST be present in the Child ID Request:
    #     - Address Registration TLV
    #     - Link-layer Frame Counter TLV
    #     - Mode TLV
    #     - Response TLV
    #     - Timeout TLV
    #     - TLV Request TLV
    #     - Version TLV
    #     - MLE Frame Counter TLV (optional)
    print(f"Step 7: {name} (DUT)")
    pkts.filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter_wpan_src64(dut).\
        filter_wpan_dst64(REED_1).\
        filter(lambda p: {
                          consts.ADDRESS_REGISTRATION_TLV,
                          consts.LINK_LAYER_FRAME_COUNTER_TLV,
                          consts.MODE_TLV,
                          consts.RESPONSE_TLV,
                          consts.TIMEOUT_TLV,
                          consts.TLV_REQUEST_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
