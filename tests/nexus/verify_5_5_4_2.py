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
    # 5.5.4 Split and Merge with Routers
    #
    # 5.5.4.2 Topology B (DUT as Router)
    #
    # Purpose & Description
    # The purpose of this test case is to show that:
    # - DUT device (R1) will join a new partition once the Leader is removed from the network for a time period
    #   longer than the leader timeout (120 seconds).
    # - If DUT device, before NETWORK_ID_TIMEOUT expires, hears MLE advertisements from a singleton Thread
    #   Partition (with higher partition id), it will consider its partition has a higher priority and will not
    #   try to join the singleton Thread partition.
    # - The network will merge once the Leader is reintroduced to the network.
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # Partitioning     | 4.8          | 4.6
    # Merging          | 4.9          | 4.7

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_2 = pv.vars['ROUTER_2']
    ROUTER_3 = pv.vars['ROUTER_3']
    ROUTER_4 = pv.vars['ROUTER_4']

    ROUTER_1_MLEID = pv.vars['ROUTER_1_MLEID']
    ROUTER_4_MLEID = pv.vars['ROUTER_4_MLEID']

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A.
    print("Step 1: All")

    # Step 2: Leader, Router_1
    # - Description: Automatically transmit MLE advertisements.
    # - Pass Criteria:
    #   - Devices are sending properly formatted MLE Advertisements.
    #   - Advertisements MUST be sent with an IP Hop Limit of 255 to the Link-Local All-Nodes multicast address
    #     (FF02::1).
    #   - The following TLVs MUST be present:
    #     - Source Address TLV
    #     - Leader Data TLV
    #     - Route64 TLV.
    print("Step 2: Leader, Router_1")
    pkts.copy().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter_LLANMA().\
        filter_wpan_src64(LEADER).\
        filter(lambda p: {
            consts.SOURCE_ADDRESS_TLV,
            consts.LEADER_DATA_TLV,
            consts.ROUTE64_TLV
        } <= set(p.mle.tlv.type) and
            p.ipv6.hlim == 255).\
        must_next()

    pkts.copy().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter_LLANMA().\
        filter_wpan_src64(ROUTER_1).\
        filter(lambda p: {
            consts.SOURCE_ADDRESS_TLV,
            consts.LEADER_DATA_TLV,
            consts.ROUTE64_TLV
        } <= set(p.mle.tlv.type) and
            p.ipv6.hlim == 255).\
        must_next()

    # Step 3: Router_3
    # - Description: Harness sets Partition ID on the device to maximum value. (This will take effect after
    #   partitioning and when Router_3 creates a new partition).
    # - Pass Criteria: N/A.
    print("Step 3: Router_3")

    # Step 4: Leader
    # - Description: Harness powers the device down for 200 seconds.
    # - Pass Criteria: The device stops sending MLE advertisements.
    print("Step 4: Leader")
    # No packets to verify here, but the following steps will fail if it's still sending.

    # Step 5: Router_3
    # - Description: After NETWORK_ID_TIMEOUT=55s expires, automatically forms new partition with maximum
    #   Partition ID, takes leader role and begins transmitting MLE Advertisements.
    # - Pass Criteria: N/A.
    print("Step 5: Router_3")
    r3_adv = pkts.filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter_wpan_src64(ROUTER_3).\
        filter(lambda p: p.mle.tlv.leader_data.partition_id == 0xffffffff).\
        must_next()

    # Step 6: Router_1 (DUT)
    # - Description: Does not try to join Router_3’s partition.
    # - Pass Criteria: During the ~10 seconds after the first MLE Advertisement is sent by Router_3 (with max
    #   Partition ID), the DUT MUST NOT send a Child ID Request frame – as an attempt to join Router_3’s partition.
    print("Step 6: Router_1 (DUT)")
    # We wait 10 seconds in the test. We'll check that no Child ID Request is sent by DUT in the next 10s.
    pkts.copy().\
        filter(lambda p: p.sniff_timestamp < r3_adv.sniff_timestamp + 10.0).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter_wpan_src64(ROUTER_1).\
        must_not_next()

    # Step 7: Router_1 (DUT)
    # - Description: After NETWORK_ID_TIMEOUT=120s expires, automatically attempts to reattach to previous
    #   partition.
    # - Pass Criteria:
    #   - The DUT MUST attempt to reattach to its original partition by sending MLE Parent Requests to the
    #     Link-Local All-Routers multicast address with an IP Hop Limit of 255.
    #   - The following TLVs MUST be present:
    #     - Mode TLV
    #     - Challenge TLV
    #     - Scan Mask TLV (MUST have E and R flags set)
    #     - Version TLV
    #   - Router_1 MUST make two separate attempts to reconnect to its current Partition in this manner,.
    print("Step 7: Router_1 (DUT)")
    # First attempt
    pkts.filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        filter_LLARMA().\
        filter_wpan_src64(ROUTER_1).\
        filter(lambda p: {
            consts.MODE_TLV,
            consts.CHALLENGE_TLV,
            consts.SCAN_MASK_TLV,
            consts.VERSION_TLV
        } <= set(p.mle.tlv.type) and
            p.ipv6.hlim == 255 and
            p.mle.tlv.scan_mask.r == 1 and
            p.mle.tlv.scan_mask.e == 1).\
        must_next()

    # Second attempt
    pkts.filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        filter_LLARMA().\
        filter_wpan_src64(ROUTER_1).\
        filter(lambda p: {
            consts.MODE_TLV,
            consts.CHALLENGE_TLV,
            consts.SCAN_MASK_TLV,
            consts.VERSION_TLV
        } <= set(p.mle.tlv.type) and
            p.ipv6.hlim == 255 and
            p.mle.tlv.scan_mask.r == 1 and
            p.mle.tlv.scan_mask.e == 1).\
        must_next()

    # Step 8: Router_1 (DUT)
    # - Description: Automatically attaches to Router_3 partition.
    # - Pass Criteria: DUT attaches to the new partition by sending Parent Request, Child ID Request, and Address
    #   Solicit Request messages (See 5.1.1 Attaching for formatting).
    print("Step 8: Router_1 (DUT)")
    pkts.filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        filter_LLARMA().\
        filter_wpan_src64(ROUTER_1).\
        must_next()

    pkts.filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter_wpan_src64(ROUTER_1).\
        must_next()

    pkts.filter_coap_request('/a/as').\
        filter_wpan_src64(ROUTER_1).\
        must_next()

    # Step 9: Leader
    # - Description: Harness powers the device back up; it reattaches to the network.
    # - Pass Criteria:
    #   - Leader sends a properly formatted MLE Parent Request to the Link-Local All-Routers multicast address
    #     with an IP Hop Limit of 255.
    #   - The following TLVs MUST be present in the MLE Parent Request:
    #     - Mode TLV
    #     - Challenge TLV
    #     - Scan Mask TLV = 0x80 (active Routers)
    #     - Version TLV.
    print("Step 9: Leader")
    pkts.filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        filter_LLARMA().\
        filter_wpan_src64(LEADER).\
        filter(lambda p: {
            consts.MODE_TLV,
            consts.CHALLENGE_TLV,
            consts.SCAN_MASK_TLV,
            consts.VERSION_TLV
        } <= set(p.mle.tlv.type) and
            p.ipv6.hlim == 255 and
            p.mle.tlv.scan_mask.r == 1 and
            p.mle.tlv.scan_mask.e == 0).\
        must_next()

    # Step 10: Harness
    # - Description: Waits for Network to merge.
    # - Pass Criteria: N/A.
    print("Step 10: Harness")

    # Step 11: Router_4
    # - Description: Harness instructs device to send an ICMPv6 ECHO Request to the DUT.
    # - Pass Criteria: Router_4 MUST get an ICMPv6 ECHO Reply from DUT.
    print("Step 11: Router_4")
    pkts.filter_ping_request().\
        filter(lambda p: p.ipv6.src == ROUTER_4_MLEID and
               p.ipv6.dst == ROUTER_1_MLEID).\
        must_next()
    pkts.filter_ping_reply().\
        filter(lambda p: p.ipv6.src == ROUTER_1_MLEID and
               p.ipv6.dst == ROUTER_4_MLEID).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
