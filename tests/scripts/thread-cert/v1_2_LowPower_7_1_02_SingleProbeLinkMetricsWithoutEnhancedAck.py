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
from mle import LinkMetricsSubTlvType
from pktverify import consts
from pktverify.null_field import nullField
from pktverify.packet_verifier import PacketVerifier

import config
import thread_cert

LEADER = 1
SED_1 = 2
SSED_1 = 3

SERIES_ID = 1
SERIES_ID_2 = 2
POLL_PERIOD = 2000  # 2s


class LowPower_7_1_02_SingleProbeLinkMetricsWithoutEnhancedAck(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False
    TOPOLOGY = {
        LEADER: {
            'version': '1.2',
            'name': 'LEADER',
            'mode': 'rdn',
            'allowlist': [SED_1, SSED_1],
        },
        SED_1: {
            'version': '1.2',
            'name': 'SED_1',
            'mode': '-',
            'allowlist': [LEADER],
        },
        SSED_1: {
            'version': '1.2',
            'name': 'SSED_1',
            'mode': '-',
            'allowlist': [LEADER],
        }
    }
    """All nodes are created with default configurations"""

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[SED_1].set_pollperiod(POLL_PERIOD)
        self.nodes[SED_1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[SED_1].get_state(), 'child')

        self.nodes[SSED_1].set_csl_period(consts.CSL_DEFAULT_PERIOD)
        self.nodes[SSED_1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[SSED_1].get_state(), 'child')

        leader_addr = self.nodes[LEADER].get_ip6_address(ADDRESS_TYPE.LINK_LOCAL)
        sed_extaddr = self.nodes[SED_1].get_addr64()

        # Step 3 - Verify connectivity by instructing each device to send an ICMPv6 Echo Request to the DUT
        self.assertTrue(self.nodes[SED_1].ping(leader_addr, timeout=POLL_PERIOD / 1000))
        self.assertTrue(self.nodes[SSED_1].ping(leader_addr, timeout=(2 * consts.CSL_DEFAULT_PERIOD_IN_SECOND)))
        self.simulator.go(5)

        # Step 4 - SED_1 sends a Single Probe Link Metric for RSSI using MLE Data Request
        # MLE Data Request Payload:
        # - TLV Request TLV (Link Metrics Report TLV specified)
        # - Link Metrics Query TLV
        # -- Link Metrics Query ID Sub-TLV
        # --- Query ID = 0 (Single Probe Query)
        # -- Link Metrics Query Options Sub-TLV
        # --- Metric Type ID Flags
        # ---- Type / Average Enum = 1  (Exponential Moving Avg)
        # ---- Metric Enum = 3  (RSSI)
        #
        # In this step, SED_1 should set its TxPower to 'High'. In simulation, this will be implemented by
        # setting Macfilter on the Rx side (Leader).
        self.nodes[LEADER].add_allowlist(sed_extaddr, -30)
        res = self.nodes[SED_1].link_metrics_request_single_probe(leader_addr, 'r')
        rss_1 = int(res['RSSI'])
        self.simulator.go(5)

        # Step 6 - SED_1 sends a Single Probe Link Metric for RSSI using MLE Data Request
        # MLE Data Request Payload:
        # - TLV Request TLV (Link Metrics Report TLV specified)
        # - Link Metrics Query TLV
        # -- Link Metrics Query ID Sub-TLV
        # --- Query ID = 0 (Single Probe Query)
        # -- Link Metrics Query Options Sub-TLV
        # --- Metric Type ID Flags
        # ---- Type / Average Enum = 1  (Exponential Moving Avg)
        # ---- Metric Enum = 3  (RSSI)
        #
        # In this step, SED_1 should set its TxPower to 'Low'.
        self.nodes[LEADER].add_allowlist(sed_extaddr, -95)
        res = self.nodes[SED_1].link_metrics_request_single_probe(leader_addr, 'r')
        rss_2 = int(res['RSSI'])
        self.simulator.go(5)

        # Step 8 - Compare the rssi value in step 5 & 7, RSSI in 5 should be larger than RSSI in 7
        self.assertTrue(rss_1 > rss_2)

        # Step 9 - SSED_1 sends a Single Probe Link Metric for Layer 2 LQI using MLE Data Request
        # MLE Data Request Payload:
        # - TLV Request TLV (Link Metrics Report TLV specified)
        # - Link Metrics Query TLV
        # -- Link Metrics Query ID Sub-TLV
        # --- Query ID = 0 (Single Probe Query)
        # -- Link Metrics Query Options Sub-TLV
        # --- Metric Type ID Flags
        # ---- Type / Average Enum = 1  (Exponential Moving Avg)
        # ---- Metric Enum = 1  (Layer 2 LQI)
        self.nodes[SSED_1].link_metrics_request_single_probe(leader_addr, 'q')
        self.simulator.go(5)

        # Step 11 - SSED_1 sends a Single Probe Link Metric for Link Margin using MLE Data Request
        # MLE Data Request Payload:
        # - TLV Request TLV (Link Metrics Report TLV specified)
        # - Link Metrics Query TLV
        # -- Link Metrics Query ID Sub-TLV
        # --- Query ID = 0 (Single Probe Query)
        # -- Link Metrics Query Options Sub-TLV
        # --- Metric Type ID Flags
        # ---- Type / Average Enum = 1  (Exponential Moving Avg)
        # ---- Metric Enum = 2  (Link Margin)
        self.nodes[SSED_1].link_metrics_request_single_probe(leader_addr, 'm')
        self.simulator.go(5)

        # Step 13 - SSED_1 sends a Single Probe Link Metric using MLE Data Request
        # MLE Data Request Payload:
        # - TLV Request TLV (Link Metrics Report TLV specified)
        # - Link Metrics Query TLV
        # -- Link Metrics Query ID Sub-TLV
        # --- Query ID = 0 (Single Probe Query)
        # -- Link Metrics Query Options Sub-TLV
        # --- Metric Type ID Flags
        # ---- Type / Average Enum = 1  (Exponential Moving Avg)
        # ---- Metric Enum = 1  (Layer 2 LQI)
        # ---- Metric Enum = 2  (Link Margin)
        # ---- Metric Enum = 3  (RSSI)
        self.nodes[SSED_1].link_metrics_request_single_probe(leader_addr, 'qmr')
        self.simulator.go(5)

    def verify(self, pv):
        pkts = pv.pkts
        pv.summary.show()
        LEADER = pv.vars['LEADER']
        SED_1 = pv.vars['SED_1']
        SSED_1 = pv.vars['SSED_1']

        # Step 3 - The DUT MUST send ICMPv6 Echo Responses to both SED1 & SSED1
        pkts.filter_wpan_src64(LEADER) \
            .filter_wpan_dst64(SED_1) \
            .filter_ping_reply() \
            .must_next()
        pkts.filter_wpan_src64(LEADER) \
            .filter_wpan_dst64(SSED_1) \
            .filter_ping_reply() \
            .must_next()

        # Step 4 - SED_1 sends a Single Probe Link Metric for RSSI using MLE Data Request
        # MLE Data Request Payload:
        # - TLV Request TLV (Link Metrics Report TLV specified)
        # - Link Metrics Query TLV
        # -- Link Metrics Query ID Sub-TLV
        # --- Query ID = 0 (Single Probe Query)
        # -- Link Metrics Query Options Sub-TLV
        # --- Metric Type ID Flags
        # ---- Type / Average Enum = 1  (Exponential Moving Avg)
        # ---- Metric Enum = 3  (RSSI)
        # TODO: Currently the ot-pktverify version of wireshark cannot parse Link Metrics Query Options Sub-TLV correctly. Will add check for the fields after fixing this.
        pkts.filter_wpan_src64(SED_1) \
           .filter_wpan_dst64(LEADER) \
           .filter_mle_cmd(consts.MLE_DATA_REQUEST) \
           .filter_mle_has_tlv(consts.TLV_REQUEST_TLV) \
           .filter_mle_has_tlv(consts.LINK_METRICS_QUERY_TLV) \
           .filter(lambda p: LinkMetricsSubTlvType.LINK_METRICS_QUERY_ID in p.mle.tlv.link_sub_tlv) \
           .filter(lambda p: p.mle.tlv.query_id == 0) \
           .filter(lambda p: LinkMetricsSubTlvType.LINK_METRICS_QUERY_OPTIONS in p.mle.tlv.link_sub_tlv) \
           .must_next()

        # Step 5 The DUT MUST reply to SED_1 with MLE Data Response with the following:
        # - Link Metrics Report TLV
        # -- Link Metrics Report Sub-TLV
        # --- Metric Type ID Flags
        # ---- Type / Average Enum = 1  (Exponential Moving Avg)
        # ---- Metric Enum = 3  (RSSI)
        # --- RSSI Value (1-byte)
        pkts.filter_wpan_src64(LEADER) \
           .filter_wpan_dst64(SED_1) \
           .filter_mle_cmd(consts.MLE_DATA_RESPONSE) \
           .filter_mle_has_tlv(consts.LINK_METRICS_REPORT_TLV) \
           .filter(lambda p: LinkMetricsSubTlvType.LINK_METRICS_REPORT in p.mle.tlv.link_sub_tlv) \
           .filter(lambda p: consts.LINK_METRICS_TYPE_AVERAGE_ENUM_EXPONENTIAL in p.mle.tlv.metric_type_id_flags.type) \
           .filter(lambda p: consts.LINK_METRICS_METRIC_TYPE_ENUM_RSSI in p.mle.tlv.metric_type_id_flags.metric) \
           .must_next()

        # Step 6 - SED_1 sends a Single Probe Link Metric for RSSI using MLE Data Request
        # MLE Data Request Payload:
        # - TLV Request TLV (Link Metrics Report TLV specified)
        # - Link Metrics Query TLV
        # -- Link Metrics Query ID Sub-TLV
        # --- Query ID = 0 (Single Probe Query)
        # -- Link Metrics Query Options Sub-TLV
        # --- Metric Type ID Flags
        # ---- Type / Average Enum = 1  (Exponential Moving Avg)
        # ---- Metric Enum = 3  (RSSI)
        pkts.filter_wpan_src64(SED_1) \
           .filter_wpan_dst64(LEADER) \
           .filter_mle_cmd(consts.MLE_DATA_REQUEST) \
           .filter_mle_has_tlv(consts.TLV_REQUEST_TLV) \
           .filter_mle_has_tlv(consts.LINK_METRICS_QUERY_TLV) \
           .filter(lambda p: LinkMetricsSubTlvType.LINK_METRICS_QUERY_ID in p.mle.tlv.link_sub_tlv) \
           .filter(lambda p: p.mle.tlv.query_id == 0) \
           .filter(lambda p: LinkMetricsSubTlvType.LINK_METRICS_QUERY_OPTIONS in p.mle.tlv.link_sub_tlv) \
           .must_next()

        # Step 7 The DUT MUST reply to SED_1 with MLE Data Response with the following:
        # - Link Metrics Report TLV
        # -- Link Metrics Report Sub-TLV
        # --- Metric Type ID Flags
        # ---- Type / Average Enum = 1  (Exponential Moving Avg)
        # ---- Metric Enum = 3  (RSSI)
        # --- RSSI Value (1-byte)
        pkts.filter_wpan_src64(LEADER) \
           .filter_wpan_dst64(SED_1) \
           .filter_mle_cmd(consts.MLE_DATA_RESPONSE) \
           .filter_mle_has_tlv(consts.LINK_METRICS_REPORT_TLV) \
           .filter(lambda p: LinkMetricsSubTlvType.LINK_METRICS_REPORT in p.mle.tlv.link_sub_tlv) \
           .filter(lambda p: consts.LINK_METRICS_TYPE_AVERAGE_ENUM_EXPONENTIAL in p.mle.tlv.metric_type_id_flags.type) \
           .filter(lambda p: consts.LINK_METRICS_METRIC_TYPE_ENUM_RSSI in p.mle.tlv.metric_type_id_flags.metric) \
           .must_next()

        # Step 9 - SSED_1 sends a Single Probe Link Metric for Layer 2 LQI using MLE Data Request
        # MLE Data Request Payload:
        # - TLV Request TLV (Link Metrics Report TLV specified)
        # - Link Metrics Query TLV
        # -- Link Metrics Query ID Sub-TLV
        # --- Query ID = 0 (Single Probe Query)
        # -- Link Metrics Query Options Sub-TLV
        # --- Metric Type ID Flags
        # ---- Type / Average Enum = 1  (Exponential Moving Avg)
        # ---- Metric Enum = 1  (Layer 2 LQI)
        pkts.filter_wpan_src64(SSED_1) \
           .filter_wpan_dst64(LEADER) \
           .filter_mle_cmd(consts.MLE_DATA_REQUEST) \
           .filter_mle_has_tlv(consts.TLV_REQUEST_TLV) \
           .filter_mle_has_tlv(consts.LINK_METRICS_QUERY_TLV) \
           .filter(lambda p: LinkMetricsSubTlvType.LINK_METRICS_QUERY_ID in p.mle.tlv.link_sub_tlv) \
           .filter(lambda p: p.mle.tlv.query_id == 0) \
           .filter(lambda p: LinkMetricsSubTlvType.LINK_METRICS_QUERY_OPTIONS in p.mle.tlv.link_sub_tlv) \
           .must_next()

        # Step 10 The DUT MUST reply to SSED_1 with MLE Data Response with the following:
        # - Link Metrics Report TLV
        # -- Link Metrics Report Sub-TLV
        # --- Metric Type ID Flags
        # ---- Type / Average Enum = 1  (Exponential Moving Avg)
        # ---- Metric Enum = 1  (Layer 2 LQI)
        # --- Layer 2 LQI value (1-byte)
        pkts.filter_wpan_src64(LEADER) \
           .filter_wpan_dst64(SSED_1) \
           .filter_mle_cmd(consts.MLE_DATA_RESPONSE) \
           .filter_mle_has_tlv(consts.LINK_METRICS_REPORT_TLV) \
           .filter(lambda p: LinkMetricsSubTlvType.LINK_METRICS_REPORT in p.mle.tlv.link_sub_tlv) \
           .filter(lambda p: consts.LINK_METRICS_TYPE_AVERAGE_ENUM_EXPONENTIAL in p.mle.tlv.metric_type_id_flags.type) \
           .filter(lambda p: consts.LINK_METRICS_METRIC_TYPE_ENUM_LQI in p.mle.tlv.metric_type_id_flags.metric) \
           .must_next()

        # Step 11 - SSED_1 sends a Single Probe Link Metric for Link Margin using MLE Data Request
        # MLE Data Request Payload:
        # - TLV Request TLV (Link Metrics Report TLV specified)
        # - Link Metrics Query TLV
        # -- Link Metrics Query ID Sub-TLV
        # --- Query ID = 0 (Single Probe Query)
        # -- Link Metrics Query Options Sub-TLV
        # --- Metric Type ID Flags
        # ---- Type / Average Enum = 1  (Exponential Moving Avg)
        # ---- Metric Enum = 2  (Link Margin)
        pkts.filter_wpan_src64(SSED_1) \
           .filter_wpan_dst64(LEADER) \
           .filter_mle_cmd(consts.MLE_DATA_REQUEST) \
           .filter_mle_has_tlv(consts.TLV_REQUEST_TLV) \
           .filter_mle_has_tlv(consts.LINK_METRICS_QUERY_TLV) \
           .filter(lambda p: LinkMetricsSubTlvType.LINK_METRICS_QUERY_ID in p.mle.tlv.link_sub_tlv) \
           .filter(lambda p: p.mle.tlv.query_id == 0) \
           .filter(lambda p: LinkMetricsSubTlvType.LINK_METRICS_QUERY_OPTIONS in p.mle.tlv.link_sub_tlv) \
           .must_next()

        # Step 12 The DUT MUST reply to SSED_1 with MLE Data Response with the following:
        # - Link Metrics Report TLV
        # -- Link Metrics Report Sub-TLV
        # --- Metric Type ID Flags
        # ---- Type / Average Enum = 1  (Exponential Moving Avg)
        # ---- Metric Enum = 2  (Link Margin)
        # --- Link Margin value (1-byte)
        pkts.filter_wpan_src64(LEADER) \
           .filter_wpan_dst64(SSED_1) \
           .filter_mle_cmd(consts.MLE_DATA_RESPONSE) \
           .filter_mle_has_tlv(consts.LINK_METRICS_REPORT_TLV) \
           .filter(lambda p: LinkMetricsSubTlvType.LINK_METRICS_REPORT in p.mle.tlv.link_sub_tlv) \
           .filter(lambda p: consts.LINK_METRICS_TYPE_AVERAGE_ENUM_EXPONENTIAL in p.mle.tlv.metric_type_id_flags.type) \
           .filter(lambda p: consts.LINK_METRICS_METRIC_TYPE_ENUM_LINK_MARGIN in p.mle.tlv.metric_type_id_flags.metric) \
           .must_next()

        # Step 13 - SSED_1 sends a Single Probe Link Metric using MLE Data Request
        # MLE Data Request Payload:
        # - TLV Request TLV (Link Metrics Report TLV specified)
        # - Link Metrics Query TLV
        # -- Link Metrics Query ID Sub-TLV
        # --- Query ID = 0 (Single Probe Query)
        # -- Link Metrics Query Options Sub-TLV
        # --- Metric Type ID Flags
        # ---- Type / Average Enum = 1  (Exponential Moving Avg)
        # ---- Metric Enum = 1  (Layer 2 LQI)
        # ---- Type / Average Enum = 1  (Exponential Moving Avg)
        # ---- Metric Enum = 2  (Link Margin)
        # ---- Type / Average Enum = 1  (Exponential Moving Avg)
        # ---- Metric Enum = 3  (RSSI)
        pkts.filter_wpan_src64(SSED_1) \
           .filter_wpan_dst64(LEADER) \
           .filter_mle_cmd(consts.MLE_DATA_REQUEST) \
           .filter_mle_has_tlv(consts.TLV_REQUEST_TLV) \
           .filter_mle_has_tlv(consts.LINK_METRICS_QUERY_TLV) \
           .filter(lambda p: LinkMetricsSubTlvType.LINK_METRICS_QUERY_ID in p.mle.tlv.link_sub_tlv) \
           .filter(lambda p: p.mle.tlv.query_id == 0) \
           .filter(lambda p: LinkMetricsSubTlvType.LINK_METRICS_QUERY_OPTIONS in p.mle.tlv.link_sub_tlv) \
           .must_next()

        # Step 14 The DUT MUST reply to SSED_1 with MLE Data Response with the following:
        # - Link Metrics Report TLV
        # -- Link Metrics Report Sub-TLV
        # --- Metric Type ID Flags
        # ---- Type / Average Enum = 1  (Exponential Moving Avg)
        # ---- Metric Enum = 1  (Layer 2 LQI)
        # ---- Layer 2 LQI value (1-byte)
        # ---- Type / Average Enum = 1  (Exponential Moving Avg)
        # ---- Metric Enum = 2  (Link Margin)
        # ---- Link Margin value (1-byte)
        # ---- Type / Average Enum = 1  (Exponential Moving Avg)
        # ---- Metric Enum = 3  (RSSI)
        # ---- RSSI value (1-byte)
        pkts.filter_wpan_src64(LEADER) \
           .filter_wpan_dst64(SSED_1) \
           .filter_mle_cmd(consts.MLE_DATA_RESPONSE) \
           .filter_mle_has_tlv(consts.LINK_METRICS_REPORT_TLV) \
           .filter(lambda p: LinkMetricsSubTlvType.LINK_METRICS_REPORT in p.mle.tlv.link_sub_tlv) \
           .filter(lambda p: consts.LINK_METRICS_TYPE_AVERAGE_ENUM_EXPONENTIAL in p.mle.tlv.metric_type_id_flags.type) \
           .filter(lambda p: consts.LINK_METRICS_METRIC_TYPE_ENUM_LQI in p.mle.tlv.metric_type_id_flags.metric) \
           .filter(lambda p: consts.LINK_METRICS_METRIC_TYPE_ENUM_LINK_MARGIN in p.mle.tlv.metric_type_id_flags.metric) \
           .filter(lambda p: consts.LINK_METRICS_METRIC_TYPE_ENUM_RSSI in p.mle.tlv.metric_type_id_flags.metric) \
           .must_next()


if __name__ == '__main__':
    unittest.main()
