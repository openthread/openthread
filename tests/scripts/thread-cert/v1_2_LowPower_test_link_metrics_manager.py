#!/usr/bin/env python3
#
#  Copyright (c) 2023, The OpenThread Authors.
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

import unittest

from config import ADDRESS_TYPE
from mle import LinkMetricsSubTlvType, TlvType
from pktverify import consts
from pktverify.null_field import nullField
from pktverify.packet_verifier import PacketVerifier

import config
import thread_cert

LEADER = 1
ROUTER = 2
CHILD = 3
"""
Acronyms:
- EAP - Enhanced-ACK Based Probing

Test Process:
1. Initiate a leader and a router at the beginning.
2. Enable Link Metrics Manager on leader.
3. Wait a few seconds.
  - At this moment, leader should have configured EAP successfully at router.
4. Instruct the leader ping the router.
  - Enhanced ACK sent by router should have Thread IE containing Link Metrics data.
5. Add a child into the network. The child should attach to the leader.
6. Wait 150 seconds.
  - At this moment, leader should have configured EAP successfully at the child.
7. Instruct both the router and the child ping the leader.
  - Enhanced ACK sent by router and child should have Thread IE containing Link Metrics data.
8. Shutdown the child.
9. Wait 300 seconds.
  - The leader should not send a Link Management Request to the child.
  - The leader should have sent a Link Management Request to the router and get a response.
10. Disable Link Metrics Manager on leader.
  - The leader shonld send a Link Management Request to the router to unregister EAP.
"""


class LowPower_test_LinkMetricsManager(thread_cert.TestCase):
    TOPOLOGY = {
        LEADER: {
            'version': '1.2',
            'name': 'LEADER',
            'mode': 'rdn',
            'allowlist': [ROUTER, CHILD],
        },
        ROUTER: {
            'version': '1.2',
            'name': 'ROUTER',
            'mode': 'rdn',
            'allowlist': [LEADER],
        },
        CHILD: {
            'version': '1.2',
            'name': 'CHILD',
            'mode': 'r',
            'allowlist': [LEADER],
        }
    }
    """All nodes are created with default configurations"""

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[ROUTER].start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual(self.nodes[ROUTER].get_state(), 'router')

        leader_addr = self.nodes[LEADER].get_ip6_address(ADDRESS_TYPE.LINK_LOCAL)
        router_addr = self.nodes[ROUTER].get_ip6_address(ADDRESS_TYPE.LINK_LOCAL)

        # Step 2 - Enable Link Metrics Manager on leader.
        self.nodes[LEADER].link_metrics_mgr_set_enabled(True)

        self.simulator.go(10)

        # Step 4 - Instruct the leader ping the router.
        self.nodes[LEADER].ping(router_addr)

        # Step 5 - Add a child into the network.
        self.nodes[CHILD].start()
        self.simulator.go(10)
        self.assertEqual(self.nodes[CHILD].get_state(), 'child')
        child_addr = self.nodes[CHILD].get_ip6_address(ADDRESS_TYPE.LINK_LOCAL)

        # Step 6 - Wait the leader to configure EAP at the child.
        self.simulator.go(150)

        # Step 7 - Instruct both the router and the child ping the leader.
        self.nodes[LEADER].ping(router_addr)
        self.nodes[LEADER].ping(child_addr)

        # Step 8 - Shutdown the child.
        self.nodes[CHILD].stop()

        # Step 9 - Wait the leader to refresh EAP.
        self.simulator.go(300)

        # Step 10 - Disable Link Metrics Manager on leader.
        self.nodes[LEADER].link_metrics_mgr_set_enabled(False)

    def verify(self, pv):
        pkts = pv.pkts
        pv.summary.show()
        LEADER = pv.vars['LEADER']
        ROUTER = pv.vars['ROUTER']
        CHILD = pv.vars['CHILD']

        # Step 3 - Leader enables IEEE 802.15.4-2015 Enhanced ACK based Probing by sending a Link Metrics Management
        # Request to the Router
        # MLE Link Metrics Management TLV Payload:
        # - Enhanced ACK Configuration Sub-TLV
        # -- Enh-ACK Flags = 1 (register a configuration)
        # -- Concatenation of Link Metric Type ID Flags = 0x00:
        # --- Item1: (0)(0)(001)(010) = 0x0a
        # ---- E = 0
        # ---- L = 0
        # ---- Type/Average Enum = 1 (Exponential Moving Avg)
        # ---- Metrics Enum = 2 (Link Margin)
        pkts.filter_wpan_src64(LEADER) \
            .filter_wpan_dst64(ROUTER) \
            .filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_REQUEST) \
            .filter(lambda p: consts.LM_ENHANCED_ACK_CONFIGURATION_SUB_TLV in p.mle.tlv.link_sub_tlv) \
            .filter(lambda p: p.mle.tlv.link_enh_ack_flags == consts.LINK_METRICS_ENH_ACK_PROBING_REGISTER) \
            .filter(lambda p: p.mle.tlv.link_requested_type_id_flags == '0a0b') \
            .must_next()

        # Step 3 - The Router MUST send a Link Metrics Management Response to Leader containing the following TLVs:
        # - MLE LInk Metrics Management TLV
        # -- Link Metrics Status Sub-TLV = 0 (Success)
        pkts.filter_wpan_src64(ROUTER) \
            .filter_wpan_dst64(LEADER) \
            .filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_RESPONSE) \
            .filter(lambda p: p.mle.tlv.link_status_sub_tlv == consts.LINK_METRICS_STATUS_SUCCESS) \
            .must_next()

        # Step 4 - The Enhanced Ack sent by both Router should contain Thread specific IE
        pkt = pkts.filter_wpan_src64(LEADER) \
            .filter_wpan_dst64(ROUTER) \
            .filter_ping_request() \
            .must_next()
        ack_seq_no = pkt.wpan.seq_no
        pkts.filter_wpan_ack() \
            .filter_wpan_seq(ack_seq_no) \
            .filter(lambda p: p.wpan.payload_ie.vendor.oui == consts.THREAD_IEEE_802154_COMPANY_ID) \
            .must_next()

        # Step 6 - Leader enables IEEE 802.15.4-2015 Enhanced ACK based Probing by sending a Link Metrics Management
        # Request to the Child
        # MLE Link Metrics Management TLV Payload:
        # - Enhanced ACK Configuration Sub-TLV
        # -- Enh-ACK Flags = 1 (register a configuration)
        # -- Concatenation of Link Metric Type ID Flags = 0x00:
        # --- Item1: (0)(0)(001)(010) = 0x0a
        # ---- E = 0
        # ---- L = 0
        # ---- Type/Average Enum = 1 (Exponential Moving Avg)
        # ---- Metrics Enum = 2 (Link Margin)
        pkts.filter_wpan_src64(LEADER) \
            .filter_wpan_dst64(CHILD) \
            .filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_REQUEST) \
            .filter(lambda p: consts.LM_ENHANCED_ACK_CONFIGURATION_SUB_TLV in p.mle.tlv.link_sub_tlv) \
            .filter(lambda p: p.mle.tlv.link_enh_ack_flags == consts.LINK_METRICS_ENH_ACK_PROBING_REGISTER) \
            .filter(lambda p: p.mle.tlv.link_requested_type_id_flags == '0a0b') \
            .must_next()

        # Step 6 - The Child MUST send a Link Metrics Management Response to Leader containing the following TLVs:
        # - MLE LInk Metrics Management TLV
        # -- Link Metrics Status Sub-TLV = 0 (Success)
        pkts.filter_wpan_src64(CHILD) \
            .filter_wpan_dst64(LEADER) \
            .filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_RESPONSE) \
            .filter(lambda p: p.mle.tlv.link_status_sub_tlv == consts.LINK_METRICS_STATUS_SUCCESS) \
            .must_next()

        # Step 7 - The Enhanced Ack sent by both Router and Child should contain Thread specific IE
        pkt1 = pkts.filter_wpan_src64(LEADER) \
            .filter_wpan_dst64(ROUTER) \
            .filter_ping_request() \
            .must_next()
        ack_seq_no = pkt1.wpan.seq_no
        pkts.filter_wpan_ack() \
            .filter_wpan_seq(ack_seq_no) \
            .filter(lambda p: p.wpan.payload_ie.vendor.oui == consts.THREAD_IEEE_802154_COMPANY_ID) \
            .must_next()

        pkt2 = pkts.filter_wpan_src64(LEADER) \
            .filter_wpan_dst64(CHILD) \
            .filter_ping_request() \
            .must_next()
        ack_seq_no = pkt2.wpan.seq_no
        pkts.filter_wpan_ack() \
            .filter_wpan_seq(ack_seq_no) \
            .filter(lambda p: p.wpan.payload_ie.vendor.oui == consts.THREAD_IEEE_802154_COMPANY_ID) \
            .must_next()

        # Step 9 - Leader refreshes IEEE 802.15.4-2015 Enhanced ACK based Probing by sending a Link Metrics Management
        # Request to the Router
        # MLE Link Metrics Management TLV Payload:
        # - Enhanced ACK Configuration Sub-TLV
        # -- Enh-ACK Flags = 1 (register a configuration)
        # -- Concatenation of Link Metric Type ID Flags = 0x00:
        # --- Item1: (0)(0)(001)(010) = 0x0a
        # ---- E = 0
        # ---- L = 0
        # ---- Type/Average Enum = 1 (Exponential Moving Avg)
        # ---- Metrics Enum = 2 (Link Margin)
        pkts.filter_wpan_src64(LEADER) \
            .filter_wpan_dst64(ROUTER) \
            .filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_REQUEST) \
            .filter(lambda p: consts.LM_ENHANCED_ACK_CONFIGURATION_SUB_TLV in p.mle.tlv.link_sub_tlv) \
            .filter(lambda p: p.mle.tlv.link_enh_ack_flags == consts.LINK_METRICS_ENH_ACK_PROBING_REGISTER) \
            .filter(lambda p: p.mle.tlv.link_requested_type_id_flags == '0a0b') \
            .must_next()

        # Step 9 - The Router MUST send a Link Metrics Management Response to Leader containing the following TLVs:
        # - MLE LInk Metrics Management TLV
        # -- Link Metrics Status Sub-TLV = 0 (Success)
        pkts.filter_wpan_src64(ROUTER) \
            .filter_wpan_dst64(LEADER) \
            .filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_RESPONSE) \
            .filter(lambda p: p.mle.tlv.link_status_sub_tlv == consts.LINK_METRICS_STATUS_SUCCESS) \
            .must_next()

        # Step 10 - Leader unregisters IEEE 802.15.4-2015 Enhanced ACK based Probing by sending a Link Metrics Management
        # Request to the Router
        # MLE Link Metrics Management TLV Payload:
        # - Enhanced ACK Configuration Sub-TLV
        # -- Enh-ACK Flags = 0 (unregister a configuration)
        pkts.filter_wpan_src64(LEADER) \
            .filter_wpan_dst64(ROUTER) \
            .filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_REQUEST) \
            .filter(lambda p: consts.LM_ENHANCED_ACK_CONFIGURATION_SUB_TLV in p.mle.tlv.link_sub_tlv) \
            .filter(lambda p: p.mle.tlv.link_enh_ack_flags == consts.LINK_METRICS_ENH_ACK_PROBING_CLEAR) \
            .must_next()


if __name__ == '__main__':
    unittest.main()
