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
    # 7.1.2 Single Probe Link Metrics without Enhanced ACK
    #
    # 7.1.2.1 Topology
    # (Note: A specific topology diagram is not explicitly provided in the text, but the test involves the Leader
    #   (DUT), SED_1, and SSED_1.)
    #
    # 7.1.2.2 Purpose & Definition
    # The purpose of this test case is to ensure that the DUT can successfully respond to a Single Probe Link Metrics
    #   Request from SEDs without Enhanced ACKs.
    #
    # Spec Reference   | V1.2 Section
    # -----------------|-------------
    # Link Metrics     | 4.11.3

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    LEADER_RLOC16 = pv.vars['LEADER_RLOC16']
    SED_1 = pv.vars['SED_1']
    SED_1_RLOC16 = pv.vars['SED_1_RLOC16']
    SSED_1 = pv.vars['SSED_1']
    SSED_1_RLOC16 = pv.vars['SSED_1_RLOC16']

    # Step 1: All
    # - Description: Topology formation: DUT, SED_1, SSED_1.
    # - Pass Criteria: N/A
    print("Step 1: Topology formation: DUT, SED_1, SSED_1.")

    # Step 2: SED_1, SSED_1
    # - Description: Each device automatically attaches to the DUT.
    # - Pass Criteria: N/A
    print("Step 2: Each device automatically attaches to the DUT.")

    # Step 3: SED_1, SSED_1
    # - Description: Harness verifies connectivity by instructing each device to send an ICMPv6 Echo Request to the DUT.
    # - Pass Criteria:
    #   - 3.1 : The DUT MUST send an ICMPv6 Echo Reply to SED_1.
    #   - 3.2 : The DUT MUST send an ICMPv6 Echo Reply to SSED_1.
    print(
        "Step 3: Harness verifies connectivity by instructing each device to send an ICMPv6 Echo Request to the DUT.")
    # 3.1
    _pkt = pkts.copy().\
        filter_ping_request().\
        filter_wpan_src16(SED_1_RLOC16).\
        filter_wpan_dst16(LEADER_RLOC16).\
        must_next()
    pkts.copy().\
        filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src16(LEADER_RLOC16).\
        filter_wpan_dst16(SED_1_RLOC16).\
        must_next()
    # 3.2
    _pkt = pkts.copy().\
        filter_ping_request().\
        filter_wpan_src16(SSED_1_RLOC16).\
        filter_wpan_dst16(LEADER_RLOC16).\
        must_next()
    pkts.copy().\
        filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src16(LEADER_RLOC16).\
        filter_wpan_dst16(SSED_1_RLOC16).\
        must_next()

    # Step 4: SED_1
    # - Description: Harness configures the device with TxPower = ‘High’. Harness instructs the device to send a
    #   Single Probe Link Metric for RSSI using MLE Data Request with the following payload:
    #   - TLV Request TLV (Link Metrics Report TLV specified)
    #   - Link Metrics Query TLV
    #     - Link Metrics Query ID Sub-TLV
    #       - Query ID = 0x00 (Single Probe Query)
    #     - Link Metrics Query Options Sub-TLV
    #       - Concatenation of Link Metric Type ID Flags = 0x0b
    #         - E = 0x00; L = 0x00
    #         - Type / Average Enum = 1 (Exponential Moving Avg)
    #         - Metric Enum = 3 (RSSI)
    # - Pass Criteria: N/A
    print("Step 4: SED_1 sends Single Probe Link Metric for RSSI.")
    pkts.filter_wpan_src64(SED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_DATA_REQUEST).\
        filter(lambda p: {
                          consts.TLV_REQUEST_TLV,
                          consts.LINK_METRICS_QUERY_TLV
                          } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.mle.tlv.query_id == 0x00).\
        filter(lambda p: p.mle.tlv.link_query_options == '0b').\
        must_next()

    # Step 5: Leader (DUT)
    # - Description: Automatically responds to SED_1 with MLE Data Response.
    # - Pass Criteria: The DUT MUST reply to SED_1 with MLE Data Response with the following:
    #   - Link Metrics Report TLV
    #     - Link Metrics Report Sub-TLV
    #       - Metric Type ID Flags
    #         - Type / Average Enum = 0x01 (Exponential Moving Avg)
    #         - Metric Enum = 0x03 (RSSI)
    #       - RSSI Value (1-byte)
    print("Step 5: Leader (DUT) automatically responds to SED_1 with MLE Data Response.")
    _step5_pkt = pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SED_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: consts.LINK_METRICS_REPORT_TLV in p.mle.tlv.type).\
        filter(lambda p: p.mle.tlv.metric_type_id_flags.metric == [consts.LINK_METRICS_METRIC_TYPE_ENUM_RSSI]).\
        filter(lambda p: p.mle.tlv.metric_type_id_flags.type == [consts.LINK_METRICS_TYPE_AVERAGE_ENUM_EXPONENTIAL]).\
        must_next()

    # Step 6: SED_1
    # - Description: Harness configures the device with TxPower = ‘Low’. Harness instructs the device to send a
    #   Single Probe Link Metric for RSSI using MLE Data Request with the following payload:
    #   - TLV Request TLV (Link Metrics Report TLV specified)
    #   - Link Metrics Query TLV
    #     - Link Metrics Query ID Sub-TLV
    #       - Query ID = 0x00 (Single Probe Query)
    #     - Link Metrics Query Options Sub-TLV
    #       - Concatenation of Link Metric Type ID Flags = 0x0b
    #         - E = 0x00; L = 0x00
    #         - Type / Average Enum = 1 (Exponential Moving Avg)
    #         - Metric Enum = 3 (RSSI)
    # - Pass Criteria: N/A
    print("Step 6: SED_1 sends Single Probe Link Metric for RSSI.")
    pkts.filter_wpan_src64(SED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_DATA_REQUEST).\
        filter(lambda p: {
                          consts.TLV_REQUEST_TLV,
                          consts.LINK_METRICS_QUERY_TLV
                          } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.mle.tlv.query_id == 0x00).\
        filter(lambda p: p.mle.tlv.link_query_options == '0b').\
        must_next()

    # Step 7: Leader (DUT)
    # - Description: Automatically responds to SED_1 with MLE Data Response.
    # - Pass Criteria: The DUT MUST unicast MLE Data Response to SED_1 with a MLE Data Response including the
    #   following TLVs:
    #   - Link Metrics Report TLV
    #     - Link Metrics Report Sub-TLV
    #       - Metric Type ID Flags
    #         - E = 0x00
    #         - L = 0x00
    #         - Type / Average Enum = 0x01 (Exponential Moving Avg)
    #         - Metric Enum = 0x03 (RSSI)
    #       - RSSI value (1-byte)
    print("Step 7: Leader (DUT) automatically responds to SED_1 with MLE Data Response.")
    _step7_pkt = pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SED_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: consts.LINK_METRICS_REPORT_TLV in p.mle.tlv.type).\
        filter(lambda p: p.mle.tlv.metric_type_id_flags.metric == [consts.LINK_METRICS_METRIC_TYPE_ENUM_RSSI]).\
        filter(lambda p: p.mle.tlv.metric_type_id_flags.type == [consts.LINK_METRICS_TYPE_AVERAGE_ENUM_EXPONENTIAL]).\
        must_next()

    # Step 8: Leader (DUT)
    # - Description: There should be a difference in the RSSI reported in steps 5 & 7.
    # - Pass Criteria: The DUT MUST report different RSSI values in the Link Metrics Report Sub-TLV from steps 5 and 7.
    print("Step 8: There should be a difference in the RSSI reported in steps 5 & 7.")
    # Behavioral verification is performed in the C++ test code.

    # Step 9: SSED_1
    # - Description: Harness instructs the device to send a Single Probe Link Metrics for Layer 2 LQI using MLE Data
    #   Request with the following payload:
    #   - TLV Request TLV (Link Metrics Report TLV specified)
    #   - Link Metrics Query TLV
    #     - Link Metrics Query ID Sub-TLV
    #       - Query ID = 0x00 (Single Probe Query)
    #     - Link Metrics Query Options Sub-TLV
    #       - Concatenation of Link Metric Type ID Flags = 0x09
    #         - E = 0x00; L = 0x00
    #         - Type / Average Enum = 1 (Exponential Moving Avg)
    #         - Metric Enum = 1 (Layer 2 LQI)
    # - Pass Criteria: N/A
    print("Step 9: SSED_1 sends Single Probe Link Metrics for Layer 2 LQI.")
    pkts.filter_wpan_src64(SSED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_DATA_REQUEST).\
        filter(lambda p: {
                          consts.TLV_REQUEST_TLV,
                          consts.LINK_METRICS_QUERY_TLV
                          } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.mle.tlv.query_id == 0x00).\
        filter(lambda p: p.mle.tlv.link_query_options == '09').\
        must_next()

    # Step 10: Leader (DUT)
    # - Description: Automatically responds to SSED_1 with MLE Data Response.
    # - Pass Criteria: The DUT MUST unicast MLE Data Response to SSED_1 including the following TLVs:
    #   - Link Metrics Report TLV
    #     - Link Metrics Report Sub-TLV
    #       - Metric Type ID Flags
    #         - E = 0x00
    #         - L = 0x00
    #         - Type / Average Enum = 0x01 (Exponential Moving Avg)
    #         - Metric Enum = 0x01 (Layer 2 LQI)
    #       - Layer 2 LQI value (1-byte)
    print("Step 10: Leader (DUT) automatically responds to SSED_1 with MLE Data Response.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: consts.LINK_METRICS_REPORT_TLV in p.mle.tlv.type).\
        filter(lambda p: p.mle.tlv.metric_type_id_flags.metric == [consts.LINK_METRICS_METRIC_TYPE_ENUM_LQI]).\
        filter(lambda p: p.mle.tlv.metric_type_id_flags.type == [consts.LINK_METRICS_TYPE_AVERAGE_ENUM_EXPONENTIAL]).\
        must_next()

    # Step 11: SSED_1
    # - Description: Harness instructs the device to send a Single Probe Link Metrics for Link Margin using MLE Data
    #   Request with the following payload:
    #   - TLV Request TLV (Link Metrics Report TLV specified)
    #   - Link Metrics Query TLV
    #     - Link Metrics Query ID Sub-TLV
    #       - Query ID = 0 (Single Probe Query)
    #     - Link Metrics Query Options Sub-TLV
    #     - Concatenation of Link Metric Type ID Flags = 0x0a
    #       - E = 0x00; L = 0x00
    #       - Type / Average Enum = 1 (Exponential Moving Avg)
    #       - Metric Enum = 2 (Link Margin)
    # - Pass Criteria: N/A
    print("Step 11: SSED_1 sends Single Probe Link Metrics for Link Margin.")
    pkts.filter_wpan_src64(SSED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_DATA_REQUEST).\
        filter(lambda p: {
                          consts.TLV_REQUEST_TLV,
                          consts.LINK_METRICS_QUERY_TLV
                          } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.mle.tlv.query_id == 0x00).\
        filter(lambda p: p.mle.tlv.link_query_options == '0a').\
        must_next()

    # Step 12: Leader (DUT)
    # - Description: Automatically responds to SSED_1 with MLE Data Response.
    # - Pass Criteria: The DUT MUST unicast MLE Data Response to SSED_1, including the following TLVs:
    #   - Link Metrics Report TLV
    #     - Link Metrics Report Sub-TLV
    #       - Metric Type ID Flags
    #         - E = 0x00
    #         - L = 0x00
    #         - Type / Average Enum = 0x01 (Exponential Moving Avg)
    #         - Metric Enum = 0x02 (Link Margin)
    #       - Link Margin value (1-byte)
    print("Step 12: Leader (DUT) automatically responds to SSED_1 with MLE Data Response.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: consts.LINK_METRICS_REPORT_TLV in p.mle.tlv.type).\
        filter(lambda p: p.mle.tlv.metric_type_id_flags.metric == [consts.LINK_METRICS_METRIC_TYPE_ENUM_LINK_MARGIN]).\
        filter(lambda p: p.mle.tlv.metric_type_id_flags.type == [consts.LINK_METRICS_TYPE_AVERAGE_ENUM_EXPONENTIAL]).\
        must_next()

    # Step 13: SSED_1
    # - Description: Harness instructs the device to send a Single Probe Link Metrics using MLE Data Request with
    #   the following payload:
    #   - TLV Request TLV (Link Metrics Report TLV specified)
    #   - Link Metrics Query TLV
    #     - Link Metrics Query ID Sub-TLV
    #       - Query ID = 0 (Single Probe Query)
    #     - Link Metrics Query Options Sub-TLV
    #       - Concatenation of Link Metric Type ID Flags = 0x090a0b
    #         - E= 0x00; L = 0x00
    #         - Type / Average Enum = 1 (Exponential Moving Avg)
    #         - Metric Enum = 1 (Layer 2 LQI)
    #         - E= 0x00; L = 0x00
    #         - Type / Average Enum = 1 (Exponential Moving Avg)
    #         - Metric Enum = 2 (Link Margin)
    #         - E= 0x00; L = 0x00
    #         - Type / Average Enum = 1 (Exponential Moving Avg)
    #         - Metric Enum = 3 (RSSI)
    # - Pass Criteria: N/A
    print("Step 13: SSED_1 sends Single Probe Link Metrics for LQI, Link Margin, and RSSI.")
    pkts.filter_wpan_src64(SSED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_DATA_REQUEST).\
        filter(lambda p: {
                          consts.TLV_REQUEST_TLV,
                          consts.LINK_METRICS_QUERY_TLV
                          } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.mle.tlv.query_id == 0x00).\
        filter(lambda p: p.mle.tlv.link_query_options == '090a0b').\
        must_next()

    # Step 14: Leader (DUT)
    # - Description: Automatically responds to SSED_1 with MLE Data Response.
    # - Pass Criteria: The DUT MUST unicast MLE Data Response to SSED_1, including the following TLVs:
    #   - Link Metrics Report TLV
    #     - Link Metrics Report Sub-TLV
    #       - Metric Type ID Flags
    #         - E = 0x00
    #         - L = 0x00
    #         - Type / Average Enum = 0x01 (Exponential Moving Avg)
    #         - Metric Enum = 0x01 (Layer 2 LQI)
    #       - Layer 2 LQI value (1-byte)
    #       - E = 0x00
    #       - L = 0x00
    #       - Type / Average Enum = 0x01 (Exponential Moving Avg)
    #       - Metric Enum = 0x02 (Link Margin)
    #       - Link Margin value (1-byte)
    #       - E = 0x00
    #       - L = 0x00
    #       - Type / Average Enum = 0x01 (Exponential Moving Avg)
    #       - Metric Enum = 0x03 (RSSI)
    #       - RSSI value (1-byte)
    print("Step 14: Leader (DUT) automatically responds to SSED_1 with MLE Data Response.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: consts.LINK_METRICS_REPORT_TLV in p.mle.tlv.type).\
        filter(lambda p: p.mle.tlv.metric_type_id_flags.metric == [
            consts.LINK_METRICS_METRIC_TYPE_ENUM_LQI, consts.LINK_METRICS_METRIC_TYPE_ENUM_LINK_MARGIN,
            consts.LINK_METRICS_METRIC_TYPE_ENUM_RSSI
        ]).\
        filter(lambda p: p.mle.tlv.metric_type_id_flags.type == [
            consts.LINK_METRICS_TYPE_AVERAGE_ENUM_EXPONENTIAL, consts.LINK_METRICS_TYPE_AVERAGE_ENUM_EXPONENTIAL,
            consts.LINK_METRICS_TYPE_AVERAGE_ENUM_EXPONENTIAL
        ]).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
