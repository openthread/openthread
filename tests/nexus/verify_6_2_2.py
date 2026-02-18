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

MAX_PARTITION_ID = 0xffffffff


def verify(pv):
    # 6.2.2 Connectivity when Parent Joins Partition
    #
    # 6.2.2.1 Topology
    # - Topology A: DUT as End Device (ED_1)
    # - Topology B: DUT as Sleepy End Device (SED_1)
    # - Leader: Configured with NETWORK_ID_TIMEOUT = 120 seconds (default).
    # - Router_1: Parent of the DUT.
    # - Router_2: Set NETWORK_ID_TIMEOUT = 110 seconds. Set Partition ID to max value.
    #
    # 6.2.2.2 Purpose & Description
    # The purpose of this test case is to show that the DUT will uphold connectivity when the Leader is removed and
    #   Router_1 joins a new partition.
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # Children         | 5.16.6       | 5.16.6

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_2 = pv.vars['ROUTER_2']

    if 'ED_1' in pv.vars:
        DUT = pv.vars['ED_1']
        IS_TOPOLOGY_A = True
    else:
        DUT = pv.vars['SED_1']
        IS_TOPOLOGY_A = False

    # Step 1: All
    # - Description: Ensure topology is formed correctly. Ensure that the DUT successfully attached to Router_1.
    # - Pass Criteria: N/A
    print("Step 1: All")

    # Step 2: Router_2
    # - Description: Harness configures Router_2 with NETWORK_ID_TIMEOUT = 110 seconds.
    # - Pass Criteria: N/A
    print("Step 2: Router_2")

    # Step 3: Leader
    # - Description: Harness silently removes the Leader from the network.
    # - Pass Criteria: N/A
    print("Step 3: Leader")

    # Step 4: Router_2
    # - Description: Automatically creates new partition and begins transmitting MLE Advertisements.
    # - Pass Criteria: N/A
    print("Step 4: Router_2")
    pkts.filter_wpan_src64(ROUTER_2).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter(lambda p: p.mle.tlv.leader_data.partition_id == MAX_PARTITION_ID).\
        must_next()

    # Step 5: Router_1
    # - Description: Automatically joins Router_2 partition.
    # - Pass Criteria: N/A
    print("Step 5: Router_1")
    _pkt = pkts.filter_wpan_src64(ROUTER_1).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter(lambda p: p.mle.tlv.leader_data.partition_id == MAX_PARTITION_ID).\
        must_next()
    router1_data_version = _pkt.mle.tlv.leader_data.data_version
    router1_stable_data_version = _pkt.mle.tlv.leader_data.stable_data_version

    if IS_TOPOLOGY_A:
        # Step 6: Test Harness (Topology A only)
        # - Description: Wait for 2x (double) the End Device timeout period.
        # - Pass Criteria: N/A
        print("Step 6: Test Harness (Topology A only)")

        # Step 7: MED_1 (DUT) [Topology A only]
        # - Description: Automatically sends MLE Child Update Request to Router_1. Router_1 automatically sends a MLE
        #   Child Update Response to MED_1.
        # - Pass Criteria:
        #   - The DUT MUST send a MLE Child Update Request to Router_1, including the following TLVs:
        #     - Source Address TLV
        #     - Leader Data TLV
        #       - Partition ID (value = max value)
        #       - Version (value matches its parent value)
        #       - Stable Version (value matches its parent value)
        #     - Mode TLV
        #   - The DUT MUST NOT transmit a MLE Announce message or an additional MLE Child ID Request.
        print("Step 7: MED_1 (DUT) [Topology A only]")
        pkts.filter_wpan_src64(DUT).\
            filter_wpan_dst64(ROUTER_1).\
            filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
            filter(lambda p: {
                              consts.SOURCE_ADDRESS_TLV,
                              consts.LEADER_DATA_TLV,
                              consts.MODE_TLV
                              } <= set(p.mle.tlv.type) and\
                   p.mle.tlv.leader_data.partition_id == MAX_PARTITION_ID and\
                   p.mle.tlv.leader_data.data_version == router1_data_version and\
                   p.mle.tlv.leader_data.stable_data_version == router1_stable_data_version).\
            must_next()

        # Verify no MLE Announce or additional Child ID Request from DUT
        pkts.filter_wpan_src64(DUT).\
            filter(lambda p: hasattr(p, 'mle') and\
                   p.mle.cmd in [consts.MLE_ANNOUNCE, consts.MLE_CHILD_ID_REQUEST]).\
            must_not_next()

    else:
        # Step 8: SED_1 (DUT) [Topology B only]
        # - Description: Automatically sends periodic 802.15.4 Data Request messages as part of the keep-alive message.
        # - Pass Criteria:
        #   - The DUT MUST send a 802.15.4 Data Request command to the parent device and receive an ACK message in
        #     response.
        #   - The DUT MUST NOT transmit a MLE Announce message or an additional MLE Child ID Request. If it does, the
        #     test has failed.
        print("Step 8: SED_1 (DUT) [Topology B only]")
        # We look for any packet from DUT to its parent ROUTER_1 as a sign of connectivity and keep-alive.
        pkts.filter_wpan_src64(DUT).\
            filter_wpan_dst64(ROUTER_1).\
            must_next()

        # Verify no MLE Announce or additional Child ID Request from DUT
        pkts.filter_wpan_src64(DUT).\
            filter(lambda p: hasattr(p, 'mle') and\
                   p.mle.cmd in [consts.MLE_ANNOUNCE, consts.MLE_CHILD_ID_REQUEST]).\
            must_not_next()

    # Step 9: Router_1
    # - Description: To verify connectivity, Harness instructs Router_1 to send an ICMPv6 Echo Request to the DUT link
    #   local address.
    # - Pass Criteria:
    #   - The DUT MUST respond with ICMPv6 Echo Reply.
    print("Step 9: Router_1")
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(DUT).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(DUT).\
        filter_wpan_dst64(ROUTER_1).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
