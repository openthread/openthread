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
    # 5.1.4 Router Address Reallocation – DUT creates new partition
    #
    # 5.1.4.1 Topology
    # - Set Router_2 NETWORK_ID_TIMEOUT to 200 seconds
    # - Set Partition ID on Leader to 1.
    #
    # 5.1.4.2 Purpose & Description
    # The purpose of this test case is to verify that when the original Leader is removed from the network, the DUT
    # will create a new partition as Leader and will assign a router ID if a specific ID is requested.
    #
    # Spec Reference                             | V1.1 Section    | V1.3.0 Section
    # -------------------------------------------|-----------------|-----------------
    # Router ID Management / Router ID Assignment | 5.9.9 / 5.9.10  | 5.9.9 / 5.9.10

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_2 = pv.vars['ROUTER_2']

    # Original Partition ID
    P1_ID = 1

    # Step 1: Router_2
    # - Description: Harness configures NETWORK ID TIMEOUT to be 200 seconds
    # - Pass Criteria: N/A
    print("Step 1: Router_2 configured with NETWORK ID TIMEOUT 200s.")

    # Step 2: Leader
    # - Description: Harness configures Partition ID to 1
    # - Pass Criteria: N/A
    print("Step 2: Leader configured with Partition ID 1.")

    # Step 3: All
    # - Description: Verify topology is formed correctly
    # - Pass Criteria: N/A
    print("Step 3: Verifying topology formation.")
    pkt = pkts.filter_wpan_src64(LEADER).filter_mle_cmd(
        consts.MLE_ADVERTISEMENT).filter(lambda p: p.mle.tlv.leader_data.partition_id == P1_ID).must_next()
    original_leader_data = pkt.mle.tlv.leader_data

    # Step 4: Leader
    # - Description: Harness silently powers-off the Leader
    # - Pass Criteria: N/A
    print("Step 4: Leader is powered off.")

    # Step 5: Router_1 (DUT)
    # - Description: Times out after 120 seconds and automatically attempts to reattach to partition
    # - Pass Criteria:
    #   - The DUT MUST attempt to reattach to its original partition by sending a MLE Parent Request to the Link-Local
    #     All-Routers multicast address (FF02::2) with an IP Hop Limit of 255.
    #   - The following TLVs MUST be present in the MLE Parent Request:
    #     - Challenge TLV
    #     - Mode TLV
    #     - Scan Mask TLV (MUST have E and R flags set)
    #     - Version TLV
    #   - The DUT MUST make two separate attempts to reconnect to its current Partition in this manner.
    print("Step 5: Router_1 attempts to reattach to original partition.")
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
    # - Description: Automatically creates a new partition with different Partition ID, initial VN_version, initial
    #   VN_stable_version, and initial ID sequence number
    # - Pass Criteria: N/A
    print("Step 7: Router_1 creates a new partition.")

    # Step 8: Router_2
    # - Description: Automatically starts attaching to the DUT-led partition by sending MLE Parent Request
    # - Pass Criteria:
    #   - The DUT MUST send a properly formatted MLE Parent Response to Router_2, including the following:
    #   - Leader Data TLV:
    #     - Partition ID different from original
    #     - Initial VN_version & VN_stable_version different from the original
    #     - Initial ID sequence number different from the original
    print("Step 8: Router_2 attaches to Router_1 (DUT) and receives MLE Parent Response.")
    pkts.filter_wpan_src64(ROUTER_1) \
        .filter_wpan_dst64(ROUTER_2) \
        .filter_mle_cmd(consts.MLE_PARENT_RESPONSE) \
        .filter(lambda p: p.mle.tlv.leader_data.partition_id != P1_ID) \
        .filter(lambda p: (p.mle.tlv.leader_data.data_version != original_leader_data.data_version) or
               (p.mle.tlv.leader_data.stable_data_version != original_leader_data.stable_data_version)) \
        .must_next()

    # Step 9: Router_2
    # - Description: Automatically sends MLE Child ID Request
    # - Pass Criteria:
    #   - The DUT MUST send a properly formatted Child ID Response to Router_2 (See 5.1.1 Attaching for pass criteria)
    print("Step 9: Router_1 sends Child ID Response to Router_2.")
    pkts.filter_wpan_src64(ROUTER_1) \
        .filter_wpan_dst64(ROUTER_2) \
        .filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE) \
        .must_next()

    # Step 10: Router_1 (DUT)
    # - Description: Automatically sends Address Solicit Response Message
    # - Pass Criteria:
    #   - The DUT MUST send a properly-formatted Address Solicit Response Message to Router_2.
    #   - If a specific router ID is requested, the DUT MUST provide this router ID:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload:
    #     - Status TLV (value = 0 [Success])
    #     - RLOC16 TLV
    #     - Router Mask TLV
    print("Step 10: Router_1 sends Address Solicit Response to Router_2.")
    pkts.filter_wpan_src64(ROUTER_1) \
        .filter_coap_ack(consts.ADDR_SOL_URI) \
        .filter(lambda p: {consts.NL_STATUS_TLV, consts.NL_RLOC16_TLV, consts.NL_ROUTER_MASK_TLV} <=
               set(p.coap.tlv.type) and
               p.coap.tlv.status == 0) \
        .must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
