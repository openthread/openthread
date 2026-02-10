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
    # 5.1.3 Router Address Reallocation – DUT attaches to new partition
    #
    # 5.1.3.1 Topology
    # - Set Partition ID on Leader to max value
    # - Set Router_2 NETWORK_ID_TIMEOUT to 110 seconds (10 seconds faster than default). If the DUT uses a timeout
    #   faster than default, timing may need to be adjusted.
    #
    # 5.1.3.2 Purpose & Description
    # The purpose of this test case is to verify that after the removal of the Leader from the network, the DUT will
    # first attempt to reattach to the original partition (P1), and then attach to a new partition (P2).
    #
    # Spec Reference                             | V1.1 Section    | V1.3.0 Section
    # -------------------------------------------|-----------------|-----------------
    # Router ID Management / Router ID Assignment | 5.9.9 / 5.9.10  | 5.9.9 / 5.9.10

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_2 = pv.vars['ROUTER_2']

    # Original Partition ID (P1)
    P1_ID = 0xffffffff

    # Step 1: Router_2
    # - Description: Harness configures the NETWORK ID TIMEOUT to be 110 seconds.
    # - Pass Criteria: N/A
    print("Step 1: Router_2 configured with NETWORK ID TIMEOUT 110s.")

    # Step 2: All
    # - Description: Verify topology is formed correctly
    # - Pass Criteria: N/A
    print("Step 2: Verifying topology formation.")
    pkts.filter_wpan_src64(LEADER).filter_mle_cmd(
        consts.MLE_ADVERTISEMENT).filter(lambda p: p.mle.tlv.leader_data.partition_id == P1_ID).must_next()

    # Step 3: Leader
    # - Description: Harness silently powers-off the Leader
    # - Pass Criteria: N/A
    print("Step 3: Leader is powered off.")

    # Step 4: Router_2
    # - Description: Times out after 110 seconds and automatically creates a new partition (P2) with itself as the
    #   Leader of P2
    # - Pass Criteria: N/A
    print("Step 4: Router_2 becomes leader of a new partition (P2).")
    pkts.filter_wpan_src64(ROUTER_2) \
        .filter_mle_cmd(consts.MLE_ADVERTISEMENT) \
        .filter(lambda p: p.mle.tlv.leader_data.partition_id != P1_ID) \
        .must_next()

    # Step 5: Router_1 (DUT)
    # - Description: Times out after 120 seconds and automatically attempts to reattach to original partition (P1)
    # - Pass Criteria:
    #   - The DUT MUST attempt to reattach to its original partition (P1) by sending a MLE Parent Request with an IP
    #     Hop Limit of 255 to the Link-Local All-Routers multicast address (FF02::2).
    #   - The following TLVs MUST be present in the MLE Parent Request:
    #     - Challenge TLV
    #     - Mode TLV
    #     - Scan Mask TLV (MUST have E and R flags set)
    #     - Version TLV
    #   - The DUT MUST make two separate attempts to reconnect to its original partition (P1) in this manner.
    print("Step 5: Router_1 attempts to reattach to original partition (P1).")
    for _ in range(2):
        pkts.filter_wpan_src64(ROUTER_1) \
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
                   p.mle.tlv.scan_mask.e == 1) \
            .must_next()

    # Step 6: Router_1 (DUT)
    # - Description: Automatically attempts to attach to any other partition
    # - Pass Criteria:
    #   - The DUT MUST attempt to attach to any other partition within range by sending a MLE Parent Request.
    #   - The following TLVs MUST be present in the MLE Parent Request:
    #     - Challenge TLV
    #     - Mode TLV
    #     - Scan Mask TLV
    #     - Version TLV
    print("Step 6: Router_1 attempts to attach to any other partition.")
    pkts.filter_wpan_src64(ROUTER_1) \
        .filter_LLARMA() \
        .filter_mle_cmd(consts.MLE_PARENT_REQUEST) \
        .filter(lambda p: {
                          consts.CHALLENGE_TLV,
                          consts.MODE_TLV,
                          consts.SCAN_MASK_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type)) \
        .must_next()

    # Step 7: Router_1 (DUT)
    # - Description: Automatically attaches to Router_2’s partition (P2)
    # - Pass Criteria:
    #   - The DUT MUST send a properly formatted Child ID Request to Router_2.
    #   - The following TLVs MUST be present in the Child ID Request:
    #     - Link-layer Frame Counter TLV
    #     - Mode TLV
    #     - Response TLV
    #     - Timeout TLV
    #     - TLV Request TLV
    #     - Version TLV
    #     - MLE Frame Counter TLV (optional)
    #   - The following TLV MUST NOT be present in the Child ID Request:
    #     - Address Registration TLV
    print("Step 7: Router_1 sends a Child ID Request to Router_2.")
    pkts.filter_wpan_src64(ROUTER_1) \
        .filter_wpan_dst64(ROUTER_2) \
        .filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST) \
        .filter(lambda p: {
                          consts.LINK_LAYER_FRAME_COUNTER_TLV,
                          consts.MODE_TLV,
                          consts.RESPONSE_TLV,
                          consts.TIMEOUT_TLV,
                          consts.TLV_REQUEST_TLV,
                          consts.VERSION_TLV
                } <= set(p.mle.tlv.type)) \
        .filter(lambda p: consts.ADDRESS_REGISTRATION_TLV not in p.mle.tlv.type) \
        .must_next()

    # Step 8: Router_1 (DUT)
    # - Description: Automatically sends Address Solicit Request
    # - Pass Criteria:
    #   - The DUT MUST send an Address Solicit Request.
    #   - Ensure the Address Solicit Request is properly formatted:
    #     - CoAP Request URI: coap://[<leader address>]:MM/a/as
    #     - CoAP Payload:
    #       - MAC Extended Address TLV
    #       - Status TLV
    #       - RLOC16 TLV (optional)
    print("Step 8: Router_1 sends an Address Solicit Request to Router_2 (new leader).")
    pkts.filter_wpan_src64(ROUTER_1) \
        .filter_coap_request(consts.ADDR_SOL_URI) \
        .filter(lambda p: {
                          consts.NL_MAC_EXTENDED_ADDRESS_TLV,
                          consts.NL_STATUS_TLV
                          } <= set(p.coap.tlv.type) \
               ) \
        .must_next()

    # Step 9: Router_2
    # - Description: Automatically responds with Address Solicit Response Message
    # - Pass Criteria: N/A
    print("Step 9: Router_2 responds with Address Solicit Response.")
    pkts.filter_wpan_src64(ROUTER_2) \
        .filter_coap_ack(consts.ADDR_SOL_URI) \
        .must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
