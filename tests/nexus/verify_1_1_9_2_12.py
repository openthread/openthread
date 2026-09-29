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

# Primary and Secondary channels.
kPrimaryChannel = 12
kSecondaryChannel = 11

# Primary and Secondary PAN IDs.
kPrimaryPanId = 0x1111
kSecondaryPanId = 0x2222

# Primary and Secondary Active Timestamps.
kPrimaryActiveTimestamp = 10
kSecondaryActiveTimestamp = 20


def verify(pv):
    # 9.2.12 Commissioning - Merging networks on different channels and different PANs using MLE Announce
    #
    # 9.2.12.1 Purpose & Description
    # The purpose of this test case is to verify that networks on different channels - and having different PAN IDs -
    #   can merge using the MLE Announce command. The primary channel is always used to host the DUT network.
    #
    # Spec Reference                          | V1.1 Section | V1.3.0 Section
    # ----------------------------------------|--------------|---------------
    # Merging Channel and PAN ID Partitions   | 8.7.8        | 8.7.8

    pkts = pv.pkts
    pv.summary.show()

    LEADER_1 = pv.vars['LEADER_1']
    ROUTER_1 = pv.vars['ROUTER_1']
    LEADER_2 = pv.vars['LEADER_2']
    MED_1 = pv.vars['MED_1']

    # Step 1: All
    # - Description: Topology Ensure topology is formed correctly.
    # - Pass Criteria: N/A.
    print("Step 1: All")

    # Step 2: Leader_1 (Commissioner)
    # - Description: Harness instructs Leader_1 to unicast MGMT_ANNOUNCE_BEGIN.ntf to Router_1:
    #   - CoAP Request URI: coap://[R1]:MM/c/ab
    #   - CoAP Payload: Commissioner Session ID TLV, Channel Mask TLV: ‘Primary’, Count TLV: 3, Period TLV: 3000ms
    # - Pass Criteria: N/A.
    print("Step 2: Leader_1 (Commissioner)")
    pkts.filter_wpan_src64(LEADER_1). \
        filter_coap_request("/c/ab"). \
        must_next()

    # Step 3: Router_1
    # - Description: Automatically multicasts 3 MLE Announce messages on channel ‘Primary’, including the following
    #   TLVs:
    #   - Channel TLV: ‘Secondary’
    #   - Active Timestamp TLV: 20s
    #   - PAN ID TLV: ‘Secondary’
    # - Pass Criteria:
    #   - The MLE Announce messages have the Destination PAN ID in the IEEE 802.15.4 MAC header set to the Broadcast
    #     PAN ID (0xFFFF) and are secured using: MAC Key ID Mode 2, a Key Source set to 0xffffffff, the Key Index set
    #     to 0xff,.
    #   - The MLE Announce messages are secured at the MLE layer using MLE Key Identifier Mode 2.
    print("Step 3: Router_1")
    pkts.filter_mle_cmd(consts.MLE_ANNOUNCE). \
        filter(lambda p: p.wpan.src64 == ROUTER_1). \
        filter(lambda p: p.wpan.dst_pan == 0xffff and \
                         p.wpan.aux_sec.key_id_mode == 2 and \
                         p.wpan.aux_sec.key_source == 0xffffffff and \
                         p.wpan.aux_sec.key_index == 0xff and \
                         p.mle.sec_suite == 0 and \
                         p.mle.tlv.channel == kSecondaryChannel and \
                         p.mle.tlv.active_tstamp == kSecondaryActiveTimestamp). \
        must_next()

    # Step 4: Leader_2
    # - Description: Automatically attaches to the network on the Secondary channel.
    # - Pass Criteria:
    #   - For DUT = Leader: The DUT MUST send a MLE Child ID Request on its new channel – the Secondary channel,
    #     including the following TLV: Active Timestamp TLV: 10s.
    #   - After receiving the MLE Child ID Response from Router_1, the DUT MUST send an MLE Announce on its previous
    #     channel – the Primary channel, including the following TLVs: Active Timestamp TLV: 20s, Channel TLV:
    #     ‘Secondary’, PAN ID TLV: ‘Secondary’.
    #   - The MLE Announce MUST have Destination PAN ID in the IEEE 802.15.4 MAC header set to the Broadcast PAN ID
    #     (0xFFFF) and MUST be secured using: MAC Key ID Mode 2, a Key Source set to 0xffffffff, the Key Index set to
    #     0xff.
    #   - The MLE Announce MUST be secured at the MLE layer. MLE Key Identifier Mode MUST be set to 2.
    print("Step 4: Leader_2")
    pkts.copy(). \
        filter_wpan_src64(LEADER_2). \
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST). \
        filter(lambda p: p.mle.tlv.active_tstamp == kPrimaryActiveTimestamp). \
        must_next()

    pkts.copy(). \
        filter_wpan_src64(LEADER_2). \
        filter_mle_cmd(consts.MLE_ANNOUNCE). \
        filter(lambda p: p.wpan.dst_pan == 0xffff and \
                         p.wpan.aux_sec.key_id_mode == 2 and \
                         p.wpan.aux_sec.key_source == 0xffffffff and \
                         p.wpan.aux_sec.key_index == 0xff and \
                         p.mle.sec_suite == 0 and \
                         p.mle.tlv.channel == kSecondaryChannel and \
                         p.mle.tlv.active_tstamp == kSecondaryActiveTimestamp). \
        must_next()

    # Step 5: MED_1
    # - Description: Automatically attaches to the network on the Secondary channel.
    # - Pass Criteria:
    #   - For DUT = MED: The DUT MUST send an MLE Child ID Request on its new channel - the Secondary channel,
    #     including the following TLV: Active Timestamp TLV: 10s.
    #   - After receiving a Child ID Response from Router_1 or Leader_2, the DUT MUST send an MLE Announce on its
    #     previous channel - the Primary channel, including the following TLVs: Active Timestamp TLV: 20s, Channel
    #     TLV: ‘Secondary’, PAN ID TLV: ‘Secondary’.
    #   - The MLE Announce MUST have Destination PAN ID in the IEEE 802.15.4 MAC header set to the Broadcast PAN ID
    #     (0xFFFF) and MUST be secured using: MAC Key ID Mode 2, a Key Source set to 0xffffffff, Key Index set to 0xff.
    #   - The MLE Announce MUST be secured at the MLE layer. MLE Key Identifier Mode MUST be set to 2.
    print("Step 5: MED_1")
    pkts.copy(). \
        filter_wpan_src64(MED_1). \
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST). \
        filter(lambda p: p.mle.tlv.active_tstamp == kPrimaryActiveTimestamp). \
        must_next()

    pkts.copy(). \
        filter_wpan_src64(MED_1). \
        filter_mle_cmd(consts.MLE_ANNOUNCE). \
        filter(lambda p: p.wpan.dst_pan == 0xffff and \
                         p.wpan.aux_sec.key_id_mode == 2 and \
                         p.wpan.aux_sec.key_source == 0xffffffff and \
                         p.wpan.aux_sec.key_index == 0xff and \
                         p.mle.sec_suite == 0 and \
                         p.mle.tlv.channel == kSecondaryChannel and \
                         p.mle.tlv.active_tstamp == kSecondaryActiveTimestamp). \
        must_next()

    # Step 6: All
    # - Description: Verify connectivity by sending an ICMPv6 Echo Request to the DUT mesh local address.
    # - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
    print("Step 6: All")
    pkts.copy(). \
        filter_ping_request(). \
        must_next()
    pkts.copy(). \
        filter_ping_reply(). \
        must_next()

    pkts.copy(). \
        filter_ping_request(). \
        must_next()
    pkts.copy(). \
        filter_ping_reply(). \
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
