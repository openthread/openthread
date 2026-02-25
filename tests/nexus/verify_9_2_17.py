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
    # 9.2.17 Orphaned End Devices
    #
    # 9.2.17.1 Topology
    # - Leader_1
    # - Leader_2
    # - ED_1 (DUT)
    #
    # 9.2.17.2 Purpose & Description
    # The purpose of this test case to validate end device functionality when its Parent is no longer available and
    #   searches for a new Parent using MLE Announce messages.
    #
    # Spec Reference       | V1.1 Section | V1.3.0 Section
    # ---------------------|--------------|---------------
    # Orphaned End Devices | 8.7.7        | 8.7.7

    pkts = pv.pkts
    pv.summary.show()

    LEADER_1 = pv.vars['LEADER_1']
    LEADER_2 = pv.vars['LEADER_2']
    DUT = pv.vars['DUT']

    PRIMARY_CHANNEL = 11
    SECONDARY_CHANNEL = 12
    LEADER_2_PAN_ID = 0x2222
    BROADCAST_PAN_ID = 0xffff

    # Step 1: All
    # - Description: Form the two topologies and ensure the DUT is attached to Leader_1
    # - Pass Criteria: Ensure topology is formed correctly. Verify that Leader_1 & Leader_2 are sending MLE
    #   Advertisements on separate channels.
    print("Step 1: Verify that Leader_1 & Leader_2 are sending MLE Advertisements on separate channels.")
    pkts.filter_wpan_src64(LEADER_1).\
      filter_LLANMA().\
      filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
      filter(lambda p: p.wpan_tap.ch_num == PRIMARY_CHANNEL).\
      must_next()
    pkts.filter_wpan_src64(LEADER_2).\
      filter_LLANMA().\
      filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
      filter(lambda p: p.wpan_tap.ch_num == SECONDARY_CHANNEL).\
      must_next()

    # Step 2: Leader_1
    # - Description: Harness silently powers-down Leader_1 and enables connectivity between the DUT and Leader_2
    # - Pass Criteria: N/A
    print("Step 2: Harness silently powers-down Leader_1 and enables connectivity between the DUT and Leader_2")

    # Step 3: DUT
    # - Description: Automatically recognizes that its Parent is gone when it doesn’t receive responses to MLE Child
    #   Update Requests
    # - Pass Criteria: N/A
    print("Step 3: DUT automatically recognizes that its Parent is gone")

    # Step 4: DUT
    # - Description: Automatically attempts to reattach to its current Thread Partition using the standard attaching
    #   process
    # - Pass Criteria: The DUT MUST send a MLE Parent Request
    print("Step 4: The DUT MUST send a MLE Parent Request")
    pkts.filter_wpan_src64(DUT).\
      filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
      filter(lambda p: p.wpan_tap.ch_num == PRIMARY_CHANNEL).\
      must_next()

    # Step 5: DUT
    # - Description: The DUT does not receive a MLE Parent Response to its request
    # - Pass Criteria: N/A
    print("Step 5: The DUT does not receive a MLE Parent Response to its request")

    # Step 6: DUT
    # - Description: After failing to receive a MLE Parent Response to its request, the DUT automatically sends a MLE
    #   Announce Message on the Secondary channel and waits on the Primary channel to hear any announcements.
    # - Pass Criteria: The DUT MUST send a MLE Announce Message, including the following TLVs:
    #   - Channel TLV: ‘Primary’
    #   - Active Timestamp TLV
    #   - PAN ID TLV
    #   - The Destination PAN ID in the IEEE 802.15.4 MAC header MUST be set to the Broadcast PAN ID (0xFFFF) and MUST
    #     be secured using Key ID Mode 2.
    print("Step 6: The DUT MUST send a MLE Announce Message")
    pkts.filter_wpan_src64(DUT).\
      filter_mle_cmd(consts.MLE_ANNOUNCE).\
      filter(lambda p: p.wpan_tap.ch_num == SECONDARY_CHANNEL).\
      filter(lambda p: {
        consts.CHANNEL_TLV,
        consts.ACTIVE_TIMESTAMP_TLV,
        consts.PAN_ID_TLV
      } <= set(p.mle.tlv.type)).\
      filter(lambda p: p.mle.tlv.channel == PRIMARY_CHANNEL).\
      filter(lambda p: p.wpan.dst_pan == BROADCAST_PAN_ID).\
      filter(lambda p: p.wpan.aux_sec.key_id_mode == 0x2).\
      must_next()

    # Step 7: Leader_2
    # - Description: Receives the MLE Announce from the DUT and automatically sends a MLE Announce on the Primary
    #   channel because Leader_2 has a new Active Timestamp
    # - Pass Criteria: N/A
    print("Step 7: Leader_2 sends a MLE Announce on the Primary channel")
    pkts.filter_wpan_src64(LEADER_2).\
      filter_mle_cmd(consts.MLE_ANNOUNCE).\
      filter(lambda p: p.wpan_tap.ch_num == PRIMARY_CHANNEL).\
      must_next()

    # Step 8: DUT
    # - Description: Receives the MLE Announce from Leader_2 and automatically attempts to attach on the Secondary
    #   channel
    # - Pass Criteria: The DUT MUST attempt to attach on the Secondary channel, with the new PAN ID it received in the
    #   MLE Announce message from Leader_2. The DUT MUST send a Parent Request on the Secondary channel
    print("Step 8: The DUT MUST send a Parent Request on the Secondary channel")
    pkts.filter_wpan_src64(DUT).\
      filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
      filter(lambda p: p.wpan_tap.ch_num == SECONDARY_CHANNEL).\
      filter(lambda p: p.wpan.dst_pan == LEADER_2_PAN_ID).\
      must_next()

    # Step 9: Leader_2
    # - Description: Harness verifies connectivity by instructing Leader_2 to send an ICMP Echo Request to the DUT
    # - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply
    print("Step 9: ICMPv6 Echo Request/Reply")
    _pkt = pkts.filter_ping_request().\
      filter_ipv6_src(pv.vars['LEADER_2_MLEID']).\
      filter_ipv6_dst(pv.vars['DUT_MLEID']).\
      must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
      filter_ipv6_src(pv.vars['DUT_MLEID']).\
      filter_ipv6_dst(pv.vars['LEADER_2_MLEID']).\
      must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
