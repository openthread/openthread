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

FORWARD_SERIES_FLAGS_MAC_DATA_REQUEST = 4
FORWARD_SERIES_FLAGS_ALL_DATA_FRAMES = 2
FORWARD_SERIES_FLAGS_CLEAR = 0

LM_QUERY_ID_SUB_TLV = 1
LM_QUERY_OPTIONS_SUB_TLV = 2


def _verify_ping(pkts, src_node, dst_node):
    """Helper to verify ICMPv6 Echo Request and Reply."""
    _pkt = pkts.filter_ping_request(). \
        filter_wpan_src64(src_node). \
        filter_wpan_dst64(dst_node). \
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier). \
        filter_wpan_src64(dst_node). \
        filter_wpan_dst64(src_node). \
        must_next()


def _verify_forward_series_mgmt_req_res(pkts, src_node, dst_node, series_id, series_flags, metric_type_enum,
                                        metric_enum, metric_l_flag):
    """Helper to verify a forward series management request and its response."""
    pkts.filter_wpan_src64(src_node). \
        filter_wpan_dst64(dst_node). \
        filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_REQUEST). \
        filter(lambda p: consts.LM_FORWARD_PROBING_REGISTRATION_SUB_TLV in p.mle.tlv.link_sub_tlv). \
        filter(lambda p: series_id in p.mle.tlv.link_forward_series). \
        filter(lambda p: p.mle.tlv.link_forward_series_flags == series_flags). \
        filter(lambda p: metric_type_enum in p.mle.tlv.metric_type_id_flags.type). \
        filter(lambda p: metric_enum in p.mle.tlv.metric_type_id_flags.metric). \
        filter(lambda p: metric_l_flag in p.mle.tlv.metric_type_id_flags.l). \
        must_next()

    pkts.filter_wpan_src64(dst_node). \
        filter_wpan_dst64(src_node). \
        filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_RESPONSE). \
        filter(lambda p: p.mle.tlv.link_status_sub_tlv == consts.LINK_METRICS_STATUS_SUCCESS). \
        must_next()


def _verify_enhanced_ack_config_req_res(pkts, src_node, dst_node, enh_ack_flags, requested_type_ids):
    """Helper to verify Enhanced ACK configuration request and response."""
    pkts.filter_wpan_src64(src_node). \
        filter_wpan_dst64(dst_node). \
        filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_REQUEST). \
        filter(lambda p: consts.LM_ENHANCED_ACK_CONFIGURATION_SUB_TLV in p.mle.tlv.link_sub_tlv and \
               p.mle.tlv.link_enh_ack_flags == enh_ack_flags and \
               set(p.mle.tlv.link_requested_type_id_flags) == set(requested_type_ids)). \
        must_next()

    pkts.filter_wpan_src64(dst_node). \
        filter_wpan_dst64(src_node). \
        filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_RESPONSE). \
        filter(lambda p: p.mle.tlv.link_status_sub_tlv == consts.LINK_METRICS_STATUS_SUCCESS). \
        must_next()


def _verify_data_req_res_with_report(pkts, src_node, dst_node, series_id, metric_type_enum, metric_enum,
                                     metric_l_flag):
    """Helper to verify data request for query and data response with report."""
    pkts.filter_wpan_src64(src_node). \
        filter_wpan_dst64(dst_node). \
        filter_mle_cmd(consts.MLE_DATA_REQUEST). \
        filter(lambda p: p.mle.tlv.query_id == series_id and \
               LM_QUERY_OPTIONS_SUB_TLV not in p.mle.tlv.link_sub_tlv). \
        must_next()

    pkts.filter_wpan_src64(dst_node). \
        filter_wpan_dst64(src_node). \
        filter_mle_cmd(consts.MLE_DATA_RESPONSE). \
        filter(lambda p: consts.LINK_METRICS_REPORT_TLV in p.mle.tlv.type and \
               metric_type_enum in p.mle.tlv.metric_type_id_flags.type and \
               metric_enum in p.mle.tlv.metric_type_id_flags.metric and \
               metric_l_flag in p.mle.tlv.metric_type_id_flags.l). \
        must_next()


def _verify_single_probe_req_res(pkts, src_node, dst_node, variable_len):
    """Helper to verify single probe request and Enhanced ACK response."""
    pkts.filter_wpan_src64(src_node). \
        filter_wpan_dst64(dst_node). \
        filter(lambda p: p.wpan.version == 2). \
        must_next()

    pkts.filter_wpan_dst64(src_node). \
        filter(lambda p: p.wpan.frame_type == consts.MAC_FRAME_TYPE_ACK and \
               p.wpan.version == 2 and \
               p.wpan.payload_ie.vendor.oui == consts.THREAD_IEEE_802154_COMPANY_ID and \
               p.wpan.payload_ie.vendor.variable[0] == 0 and \
               len(p.wpan.payload_ie.vendor.variable) == variable_len). \
        must_next()


def verify(pv):
    # 7.2.2 Forward Series Probing With Single Shot
    #
    # 7.2.2.1 Topology
    # - Leader (DUT)
    # - SED_1
    # - SED_2
    # - SED_3
    # - SSED_1
    # - SSED_2
    # - SSED_3
    #
    # 7.2.2.2 Purpose & Definition
    # The purpose of this test case is to ensure that the DUT can successfully support a minimum of 6 simultaneous
    #   children performing link metrics.
    #
    # Spec Reference                          | V1.2 Section
    # ----------------------------------------|-------------
    # Forward Series Probing With Single Shot | 4.11.3.2

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    SED_1 = pv.vars['SED_1']
    SED_2 = pv.vars['SED_2']
    SED_3 = pv.vars['SED_3']
    SSED_1 = pv.vars['SSED_1']
    SSED_2 = pv.vars['SSED_2']
    SSED_3 = pv.vars['SSED_3']

    SERIES_ID_4 = 4
    SERIES_ID_6 = 6
    SERIES_ID_8 = 8
    SERIES_ID_10 = 10

    # Step 0: SSED_1,2,3 / SED_1,2,3
    # - Description: Preconditions: SSED_1,2,3 - set CSLTimeout = 30s; SED_1 - set poll rate = 1s; SED_2 = set poll rate
    #   = 3s; SED_3 = set poll rate = 5s.
    # - Pass Criteria: N/A.
    print("Step 0: Preconditions: Set poll rates and CSL timeout.")

    # Step 1: All
    # - Description: Topology formation: DUT, SED_1, SED_2, SED_3, SSED_1, SSED_2, SSED_3. SSED_1,2,3 Preconditions: Set
    #   polling rate = 5s,.
    # - Pass Criteria: N/A.
    print("Step 1: Topology formation: DUT, SED_1, SED_2, SED_3, SSED_1, SSED_2, SSED_3.")

    # Step 2: SED_1,2,3 / SSED_1,2,3
    # - Description: Each device automatically attaches to the Leader (DUT).
    # - Pass Criteria: N/A.
    print("Step 2: Each device automatically attaches to the Leader (DUT).")

    # Step 3: SED_1,2,3 / SSED_1,2,3
    # - Description: Harness verifies connectivity by sending an ICMPv6 Echo Request from each device to the Leader
    #   (DUT).
    # - Pass Criteria:
    #   - 3.1: The DUT MUST send an ICMPv6 Echo Reply to SED_1.
    #   - 3.2: The DUT MUST send an ICMPv6 Echo Reply to SED_2.
    #   - 3.3: The DUT MUST send an ICMPv6 Echo Reply to SED_3.
    #   - 3.4: The DUT MUST send an ICMPv6 Echo Reply to SSED_1.
    #   - 3.5: The DUT MUST send an ICMPv6 Echo Reply to SSED_2.
    #   - 3.6: The DUT MUST send an ICMPv6 Echo Reply to SSED_3.
    print("Step 3: Harness verifies connectivity.")
    for node in [SED_1, SED_2, SED_3, SSED_1, SSED_2, SSED_3]:
        _verify_ping(pkts, node, LEADER)

    # Step 4: SED_1
    # - Description: Harness instructs the device to request a Forward Series Link Metric by sending a Link Metrics
    #   Management Request with the following payload:
    #   - MLE Link Metrics Management TLV
    #   - Forward Probing Registration Sub-TLV
    #     - Type = 3
    #     - Forward Series ID = 4
    #     - Forward Series Flags = 0x04
    #       - Bits 0,1 = 0
    #       - Bit 2 = 1 (Track MAC Data Request command frames)
    #       - Bits 3-7 = 0
    #     - Concatenation of Link Metric Type ID Flags=0x09
    #       - E = 0x00; L = 0x00
    #       - Type/Average Enum = 1 (Exponential Moving Avg)
    #       - Metric Enum = 1 (Layer 2 LQI)
    # - Pass Criteria: N/A.
    print("Step 4: SED_1 requests a Forward Series Link Metric ID 4.")
    # Step 5: Leader (DUT)
    # - Description: Automatically responds with a Link Metrics Management Response.
    # - Pass Criteria: The DUT MUST respond to SED_1 with a Link Metrics Management Response containing the following:
    #   - MLE Link Metrics Management TLV
    #   - Link Metrics Status Sub-TLV = 0 (Success).
    print("Step 5: Leader responds to SED_1 with a Link Metrics Management Response.")
    _verify_forward_series_mgmt_req_res(pkts, SED_1, LEADER, SERIES_ID_4, FORWARD_SERIES_FLAGS_MAC_DATA_REQUEST,
                                        consts.LINK_METRICS_TYPE_AVERAGE_ENUM_EXPONENTIAL,
                                        consts.LINK_METRICS_METRIC_TYPE_ENUM_LQI, 0)

    # Step 6: SED_2
    # - Description: Harness instructs the device to request a Forward Series Link Metric by sending a Link Metrics
    #   Management Request with the following payload:
    #   - MLE Link Metrics Management TLV
    #   - Forward Probing Registration Sub-TLV
    #     - Type = 3
    #     - Forward Series ID = 6
    #     - Forward Series Flags = 0x02
    #       - Bit 0 = 0
    #       - Bit 1 = 1 (Track Link Layer data frames)
    #       - Bits 2-7 = 0
    #     - Concatenation of Link Metric Type ID Flags=0x40
    #       - E = 0x00; L = 0x01
    #       - Type/Average Enum = 0 (count)
    #       - Metric Enum = 0 (# of PDUs received)
    # - Pass Criteria: N/A.
    print("Step 6: SED_2 requests a Forward Series Link Metric ID 6.")
    # Step 7: Leader (DUT)
    # - Description: Automatically responds with a Link Metrics Management Response.
    # - Pass Criteria: The DUT MUST respond to SED_2 with a Link Metrics Management Response containing the following:
    #   - MLE Link Metrics Management TLV
    #   - Link Metrics Status Sub-TLV = 0 (Success).
    print("Step 7: Leader responds to SED_2 with a Link Metrics Management Response.")
    _verify_forward_series_mgmt_req_res(pkts, SED_2, LEADER, SERIES_ID_6, FORWARD_SERIES_FLAGS_ALL_DATA_FRAMES,
                                        consts.LINK_METRICS_TYPE_AVERAGE_ENUM_COUNT,
                                        consts.LINK_METRICS_METRIC_TYPE_ENUM_PDU_COUNT, 1)

    # Step 8: SSED_1
    # - Description: Harness instructs device to request a Forward Series Link Metric by sending a Link Metrics
    #   Management Request with the following payload:
    #   - MLE Link Metrics Management TLV
    #   - Forward Probing Registration Sub-TLV
    #     - Type = 3
    #     - Forward Series ID = 8
    #     - Forward Series Flags = 0x02
    #       - Bit 0 = 0
    #       - Bit 1 = 1 (Track Link Layer data frames)
    #       - Bits 2-7 = 0
    #     - Concatenation of Link Metric Type ID Flags=0x0a
    #       - E = 0x00; L = 0x00
    #       - Type/Average Enum = 1 (Exponential Moving Avg)
    #       - Metric Enum = 2 (Link Margin)
    # - Pass Criteria: N/A.
    print("Step 8: SSED_1 requests a Forward Series Link Metric ID 8.")
    # Step 9: Leader (DUT)
    # - Description: Automatically responds with a Link Metrics Management Response.
    # - Pass Criteria: The DUT MUST respond to SSED_1 with a Link Metrics Management Response containing the following:
    #   - MLE Link Metrics Management TLV
    #   - Link Metrics Status Sub-TLV = 0 (Success).
    print("Step 9: Leader responds to SSED_1 with a Link Metrics Management Response.")
    _verify_forward_series_mgmt_req_res(pkts, SSED_1, LEADER, SERIES_ID_8, FORWARD_SERIES_FLAGS_ALL_DATA_FRAMES,
                                        consts.LINK_METRICS_TYPE_AVERAGE_ENUM_EXPONENTIAL,
                                        consts.LINK_METRICS_METRIC_TYPE_ENUM_LINK_MARGIN, 0)

    # Step 10: SSED_2
    # - Description: Harness instructs device to request a Forward Series Link Metric by sending a Link Metrics
    #   Management Request with the following payload:
    #   - MLE Link Metrics Management TLV
    #   - Forward Probing Registration Sub-TLV
    #     - Type = 3
    #     - Forward Series ID = 10
    #     - Forward Series Flags = 0x04
    #       - Bits 0,1 = 0
    #       - Bit 2 = 1 (Track MAC Data Request command frames)
    #       - Bits 3-7 = 0
    #     - Concatenation of Link Metric Type ID Flags=0x0b
    #       - E = 0x00; L = 0x00
    #       - Type/Average Enum = 1 (Exponential Moving Avg)
    #       - Metric Enum = 3 (RSSI)
    # - Pass Criteria: N/A.
    print("Step 10: SSED_2 requests a Forward Series Link Metric ID 10.")
    # Step 11: Leader (DUT)
    # - Description: Automatically responds with a Link Metrics Management Response.
    # - Pass Criteria: The DUT MUST respond to SSED_2 with a Link Metrics Management Response containing the following:
    #   - MLE Link Metrics Management TLV
    #   - Link Metrics Status Sub-TLV = 0 (Success).
    print("Step 11: Leader responds to SSED_2 with a Link Metrics Management Response.")
    _verify_forward_series_mgmt_req_res(pkts, SSED_2, LEADER, SERIES_ID_10, FORWARD_SERIES_FLAGS_MAC_DATA_REQUEST,
                                        consts.LINK_METRICS_TYPE_AVERAGE_ENUM_EXPONENTIAL,
                                        consts.LINK_METRICS_METRIC_TYPE_ENUM_RSSI, 0)

    # Step 12: SED_3
    # - Description: Harness instructs device to enable IEEE 802.15.4-2015 Enhanced ACK based Probing by sending a Link
    #   Metrics Management Request with the following payload:
    #   - MLE Link Metrics Management TLV
    #   - Enhanced ACK Link Metrics Configuration Sub-TLV
    #     - Type = 7
    #     - Enh-ACK Flags = 1
    #     - Concatenation of Link Metrics Type ID Flags=0x0a
    #       - E = 0x00; L = 0x00
    #       - Type / Average Enum = 1 (Exponential Moving Avg)
    #       - Metric Enum = 2 (Link Margin)
    # - Pass Criteria: N/A.
    print("Step 12: SED_3 enables Enhanced ACK based Probing.")
    # Step 13: Leader (DUT)
    # - Description: Automatically responds with a Link Metrics Management Response.
    # - Pass Criteria: The DUT MUST respond to SED_3 with a Link Metrics Management Response containing the following:
    #   - MLE Link Metrics Management TLV
    #   - Link Metrics Status Sub-TLV = 0 (Success).
    print("Step 13: Leader responds to SED_3 with a Link Metrics Management Response.")
    _verify_enhanced_ack_config_req_res(pkts, SED_3, LEADER, 1, [0x0a])

    # Step 14: SSED_3
    # - Description: Harness instructs device to enable IEEE 802.15.4-2015 Enhanced ACK based Probing by sending a Link
    #   Metrics Management Request with the following payload:
    #   - MLE Link Metrics Management TLV
    #   - Enhanced ACK Link Metrics Configuration Sub-TLV
    #     - Type = 7
    #     - Enh-ACK Flags = 1
    #     - Concatenation of Link Metrics Type ID Flags=0x09, 0x0b
    #       - E = 0x00; L = 0x00
    #       - Type / Average Enum = 1 (Exponential Moving Avg)
    #       - Metric Enum = 1 (Layer 2 LQI)
    #       - Metric Enum = 3 (RSSI)
    # - Pass Criteria: N/A.
    print("Step 14: SSED_3 enables Enhanced ACK based Probing.")
    # Step 15: Leader (DUT)
    # - Description: Automatically responds with a Link Metrics Management Response.
    # - Pass Criteria: The DUT MUST respond to SSED_3 with a Link Metrics Management Response containing the following:
    #   - MLE Link Metrics Management TLV
    #   - Link Metrics Status Sub-TLV = 0 (Success).
    print("Step 15: Leader responds to SSED_3 with a Link Metrics Management Response.")
    _verify_enhanced_ack_config_req_res(pkts, SSED_3, LEADER, 1, [0x09, 0x0b])

    # Step 16: SED_1, SSED_2
    # - Description: Harness configures devices to send MAC Data Requests periodically.
    # - Pass Criteria: N/A.
    print("Step 16: Configuration already done in Step 0.")

    # Step 17: SED_2, SSED_1
    # - Description: Harness instructs devices to send MAC Data packets every 1 second for 30 seconds.
    # - Pass Criteria: N/A.
    print("Step 17: Devices send data packets.")

    # Step 18: SED_1
    # - Description: Harness instructs device to retrieve aggregated Forward Series results by sending a MLE Data
    #   Request message to the Leader (DUT) with the following payload:
    #   - TLV Request TLV (Link Metrics Report TLV specified)
    #   - Link Metrics Query TLV
    #     - Link Metrics Query ID Sub-TLV
    #       - Query ID = 4 [series configured in step 4]
    # - Pass Criteria: N/A.
    print("Step 18: SED_1 retrieves aggregated Forward Series ID 4 results.")
    # Step 19: Leader (DUT)
    # - Description: Automatically responds with a MLE Data Response.
    # - Pass Criteria: The DUT MUST reply to SED_1 with a MLE Data Response with the following:
    #   - Link Metrics Report TLV
    #     - Link Metrics Report Sub-TLV
    #       - Metric Type ID Flags
    #         - E = 0x00
    #         - L = 0x00
    #         - Type / Average Enum = 1
    #         - Metric Enum = 1
    #       - Value (1 bytes)
    print("Step 19: Leader responds to SED_1 with a MLE Data Response.")
    _verify_data_req_res_with_report(pkts, SED_1, LEADER, SERIES_ID_4,
                                     consts.LINK_METRICS_TYPE_AVERAGE_ENUM_EXPONENTIAL,
                                     consts.LINK_METRICS_METRIC_TYPE_ENUM_LQI, 0)

    # Step 20: SED_2
    # - Description: Harness instructs device to retrieve aggregated Forward Series results by sending a MLE Data
    #   Request message to the Leader (DUT) with the following payload:
    #   - TLV Request TLV (Link Metrics Report TLV specified)
    #   - Link Metrics Query TLV
    #     - Link Metrics Query ID Sub-TLV
    #       - Query ID = 6 [series configured in step 6]
    # - Pass Criteria: N/A.
    print("Step 20: SED_2 retrieves aggregated Forward Series ID 6 results.")
    # Step 21: Leader (DUT)
    # - Description: Automatically responds with a MLE Data Response.
    # - Pass Criteria: The DUT MUST reply to SED_2 with a MLE Data Response with the following:
    #   - Link Metrics Report TLV
    #     - Link Metrics Report Sub-TLV
    #       - Metric Type ID Flags
    #         - E = 0x00
    #         - L = 0x01
    #         - Type / Average Enum = 0 (Count)
    #         - Metric Enum = 0 (Number of PDUs received)
    #       - Count Value (4 bytes)
    print("Step 21: Leader responds to SED_2 with a MLE Data Response.")
    _verify_data_req_res_with_report(pkts, SED_2, LEADER, SERIES_ID_6, consts.LINK_METRICS_TYPE_AVERAGE_ENUM_COUNT,
                                     consts.LINK_METRICS_METRIC_TYPE_ENUM_PDU_COUNT, 1)

    # Step 22: SED_3
    # - Description: Harness makes a Single Probe Link Metric query for Link Margin by instructing the device to send a
    #   MAC Data message with the Frame Version subfield within the MAC header of the message to set 0b10.
    # - Pass Criteria: N/A.
    print("Step 22: SED_3 makes a Single Probe Link Metric query.")
    # Step 23: Leader (DUT)
    # - Description: Automatically responds with an Enhanced Acknowledgement containing link metrics.
    # - Pass Criteria: The DUT MUST reply to SED_3 with Enhanced ACK containing the following:
    #   - Header IE
    #     - Element ID = 0x00
    #     - Vendor CID = 0xeab89b (Thread Group)
    #     - Vendor Specific Information
    #       - 1st byte = 0 (Enhanced ACK Link Metrics)
    #       - 2nd byte … Link Margin data
    print("Step 23: Leader responds with an Enhanced Acknowledgement.")
    _verify_single_probe_req_res(pkts, SED_3, LEADER, 2)

    # Step 24: SSED_1
    # - Description: Harness instructs device to retrieve aggregated Forward Series Results by sending a MLE Data
    #   Request message to the Leader (DUT) with the following payload:
    #   - TLV Request TLV (Link Metrics Report TLV specified)
    #   - Link Metrics Query TLV
    #     - Link Metrics Query ID Sub-TLV
    #       - Query ID = 8 [series configured in step 8]
    #   - The Link Metrics Query Options Sub-TLV is NOT sent
    # - Pass Criteria: N/A.
    print("Step 24: SSED_1 retrieves aggregated Forward Series ID 8 results.")
    # Step 25: Leader (DUT)
    # - Description: Automatically responds with MLE Data Response.
    # - Pass Criteria: The DUT MUST unicast MLE Data Response to SSED_1, including the following TLVs:
    #   - Link Metrics Report TLV
    #     - Link Metrics Report Sub-TLV
    #       - Metric Type ID Flags
    #         - E = 0x00
    #         - L = 0x00
    #         - Type / Average Enum = 1
    #         - Metric Enum = 2
    #       - Value (1 byte)
    print("Step 25: Leader responds to SSED_1 with a MLE Data Response.")
    _verify_data_req_res_with_report(pkts, SSED_1, LEADER, SERIES_ID_8,
                                     consts.LINK_METRICS_TYPE_AVERAGE_ENUM_EXPONENTIAL,
                                     consts.LINK_METRICS_METRIC_TYPE_ENUM_LINK_MARGIN, 0)

    # Step 26: SSED_2
    # - Description: Harness instructs the device to retrieve aggregated Forward Series results by sending a MLE Data
    #   Request message to the Leader (DUT) with the following payload:
    #   - TLV Request TLV (Link Metrics Report TLV specified)
    #   - Link Metrics Query TLV
    #     - Link Metrics Query ID Sub-TLV
    #       - Query ID = 10 [series configured in step 10]
    # - Pass Criteria: N/A.
    print("Step 26: SSED_2 retrieves aggregated Forward Series ID 10 results.")
    # Step 27: Leader (DUT)
    # - Description: Automatically responds with a MLE Data Response.
    # - Pass Criteria: The DUT MUST unicast MLE Data Response to SSED_2, including the following TLVs:
    #   - Link Metrics Report TLV
    #     - Link Metrics Report Sub-TLV
    #       - Metric Type ID Flags
    #         - E = 0x00
    #         - L = 0x00
    #         - Type / Average Enum = 1
    #         - Metric Enum = 3
    #       - Value (1 byte)
    print("Step 27: Leader responds to SSED_2 with a MLE Data Response.")
    _verify_data_req_res_with_report(pkts, SSED_2, LEADER, SERIES_ID_10,
                                     consts.LINK_METRICS_TYPE_AVERAGE_ENUM_EXPONENTIAL,
                                     consts.LINK_METRICS_METRIC_TYPE_ENUM_RSSI, 0)

    # Step 28: SSED_3
    # - Description: Harness makes a Single Probe Link Metric query for Link Margin by instructing the device to send a
    #   MAC Data message with the Frame Version subfield within the MAC header of the message to 0b10.
    # - Pass Criteria: N/A.
    print("Step 28: SSED_3 makes a Single Probe Link Metric query.")
    # Step 29: Leader (DUT)
    # - Description: Automatically responds with an Enhanced Acknowledgement containing link metrics.
    # - Pass Criteria: The DUT MUST reply to SSED_3 with Enhanced ACK containing:
    #   - Header IE
    #     - Element ID = 0x00
    #     - Vendor CID = 0xeab89b (Thread Group)
    #     - Vendor Specific Information
    #       - 1st byte = 0 (Enhanced ACK Link Metrics)
    #       - 2nd byte …LQI
    #       - 3rd byte … RSSI
    print("Step 29: Leader responds with an Enhanced Acknowledgement.")
    _verify_single_probe_req_res(pkts, SSED_3, LEADER, 3)


if __name__ == "__main__":
    verify_utils.run_main(verify)
