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
    # 5.1.9 Attaching to a REED with better connectivity
    #
    # 5.1.9.1 Topology
    # - Leader
    # - Router_1
    # - REED_1
    # - REED_2
    # - Router_2 (DUT)
    #
    # 5.1.9.2 Purpose & Description
    # The purpose of this test case is to validate that the DUT will pick REED_1 as its parent because of its better
    #   connectivity.
    # - In order for this test case to be valid, the link quality between all nodes must be of the highest quality (3).
    #   If this condition cannot be met the test case is invalid.
    #
    # Spec Reference                             | V1.1 Section    | V1.3.0 Section
    # -------------------------------------------|-----------------|-----------------
    # Parent Selection                           | 4.7.2           | 4.5.2

    pkts = pv.pkts
    pv.summary.show()

    REED_1 = pv.vars['REED_1']
    REED_2 = pv.vars['REED_2']
    DUT = pv.vars['DUT']
    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']

    # Step 1: Leader, Router_1, REED_2, REED_1
    # - Description: Setup the topology without the DUT. Verify Router_1 and the Leader are sending MLE
    #   advertisements.
    # - Pass Criteria: N/A
    print("Step 1: Setup the topology without the DUT")
    pkts.filter_wpan_src64(LEADER) \
        .filter_mle_cmd(consts.MLE_ADVERTISEMENT) \
        .must_next()
    pkts.filter_wpan_src64(ROUTER_1) \
        .filter_mle_cmd(consts.MLE_ADVERTISEMENT) \
        .must_next()

    # Step 2: Test Harness
    # - Description: Harness configures the RSSI between Leader, Router_1, Router_2 (DUT), REED_1, and REED_2 to enable
    #   a link quality of 3 (highest).
    # - Pass Criteria: N/A
    print("Step 2: Harness configures the RSSI")

    # Step 3: Router_2 (DUT)
    pkts.filter_wpan_src64(DUT) \
        .filter_LLARMA() \
        .filter_mle_cmd(consts.MLE_PARENT_REQUEST) \
        .filter(lambda p: {
                          consts.MODE_TLV,
                          consts.CHALLENGE_TLV,
                          consts.SCAN_MASK_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type) and \
               p.ipv6.hlim == 255 and \
               p.mle.tlv.scan_mask.r == 1 and \
               p.mle.tlv.scan_mask.e == 0) \
        .must_next()
    lstart = pkts.index

    # Step 4: REED_2, REED_1
    # - Description: Devices do not respond to the All-Routers Parent Request.
    # - Pass Criteria: N/A
    print("Step 4: REEDs do not respond to the All-Routers Parent Request")

    # Step 5: Router_2 (DUT)
    # - Description: Automatically sends MLE Parent Request with Scan Mask set to Routers and REEDs.
    # - Pass Criteria:
    #   - The DUT MUST send MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an IP Hop
    #     Limit of 255.
    #   - The following TLVs MUST be present in the MLE Parent Request:
    #     - Challenge TLV
    #     - Mode TLV
    #     - Scan Mask TLV = 0xC0 (Active Routers and REEDs)
    #     - Version TLV
    #   - The Key Identifier Mode of the Security Control field of the MAC frame Auxiliary Security Header MUST be set
    #     to '0x02'
    print("Step 5: Router_2 (DUT) sends MLE Parent Request with Scan Mask set to Routers and REEDs")
    pkts.filter_wpan_src64(DUT) \
        .filter_LLARMA() \
        .filter_mle_cmd(consts.MLE_PARENT_REQUEST) \
        .filter(lambda p: {
                          consts.CHALLENGE_TLV,
                          consts.MODE_TLV,
                          consts.SCAN_MASK_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type) and \
               p.ipv6.hlim == 255 and \
               p.mle.tlv.scan_mask.r == 1 and \
               p.mle.tlv.scan_mask.e == 1 and \
               p.wpan.aux_sec.key_id_mode == 0x02) \
        .must_next()
    lend = pkts.index

    # Verify Step 4: No Parent Response from REEDs between Step 3 and Step 5
    for reed in [REED_1, REED_2]:
        pkts.range(lstart, lend) \
            .filter_wpan_src64(reed) \
            .filter_wpan_dst64(DUT) \
            .filter_mle_cmd(consts.MLE_PARENT_RESPONSE) \
            .must_not_next()

    # Step 6: REED_1, REED_2
    # - Description: Each device automatically responds to DUT with MLE Parent Response. REED_1 reports more high
    #   quality connection than REED_2 in the Connectivity TLV.
    # - Pass Criteria: N/A
    print("Step 6: REEDs respond with MLE Parent Response")

    # Both REEDs respond, but the order is not guaranteed.
    _index = pkts.index

    def find_parent_response(src64):
        pkts.index = _index
        pkt = pkts.filter_wpan_src64(src64) \
            .filter_wpan_dst64(DUT) \
            .filter_mle_cmd(consts.MLE_PARENT_RESPONSE) \
            .filter(lambda p: {
                              consts.CHALLENGE_TLV,
                              consts.CONNECTIVITY_TLV,
                              consts.LEADER_DATA_TLV,
                              consts.LINK_LAYER_FRAME_COUNTER_TLV,
                              consts.LINK_MARGIN_TLV,
                              consts.RESPONSE_TLV,
                              consts.SOURCE_ADDRESS_TLV,
                              consts.VERSION_TLV
                              } <= set(p.mle.tlv.type) and \
                   p.wpan.aux_sec.key_id_mode == 0x02) \
            .must_next()
        return pkt, pkts.index

    p_reed1, index1 = find_parent_response(REED_1)
    p_reed2, index2 = find_parent_response(REED_2)

    # REED_1 has two router neighbors with link quality 3 (Leader and Router_1).
    # REED_2 only has one router neighbor with link quality 3 (Leader).
    # REED_1 should report a higher count in the link_quality_3 field of its Connectivity TLV.
    if not (p_reed1.mle.tlv.conn.lq3 > p_reed2.mle.tlv.conn.lq3):
        raise ValueError(f"REED_1 lq3 ({p_reed1.mle.tlv.conn.lq3}) should be greater than "
                         f"REED_2 lq3 ({p_reed2.mle.tlv.conn.lq3})")

    pkts.index = max(index1, index2)

    # Step 7: Router_2 (DUT)
    # - Description: Automatically sends a MLE Child ID Request to REED_1.
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
    print("Step 7: Router_2 (DUT) sends a MLE Child ID Request to REED_1")
    _pkt = pkts.filter_wpan_src64(DUT) \
        .filter_wpan_dst64(REED_1) \
        .filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST) \
        .filter(lambda p: {
                          consts.LINK_LAYER_FRAME_COUNTER_TLV,
                          consts.MODE_TLV,
                          consts.RESPONSE_TLV,
                          consts.TIMEOUT_TLV,
                          consts.TLV_REQUEST_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type)) \
        .must_next()
    _pkt.must_not_verify(lambda p: consts.ADDRESS_REGISTRATION_TLV in p.mle.tlv.type)


if __name__ == '__main__':
    verify_utils.run_main(verify)
