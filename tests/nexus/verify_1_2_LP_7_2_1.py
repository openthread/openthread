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
    # 7.2.1 Forward Tracking Series
    #
    # 7.2.1.1 Topology
    # - Leader (DUT)
    # - SED_1
    # - SSED_1
    #
    # 7.2.1.2 Purpose & Definition
    # The purpose of this test case is to ensure that the DUT can successfully respond to a Forward Series Link Metrics
    #   Request from both a SED & SSED.
    #
    # Spec Reference           | V1.2 Section
    # -------------------------|-------------
    # Forward Tracking Series  | 4.11.3.2

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    LEADER_RLOC16 = pv.vars['LEADER_RLOC16']
    SED_1 = pv.vars['SED_1']
    SED_1_RLOC16 = pv.vars['SED_1_RLOC16']
    SSED_1 = pv.vars['SSED_1']
    SSED_1_RLOC16 = pv.vars['SSED_1_RLOC16']

    SERIES_ID_4 = 4
    SERIES_ID_11 = 11
    SERIES_ID_16 = 16

    FORWARD_SERIES_FLAGS_MAC_DATA_REQUEST = 0x04
    FORWARD_SERIES_FLAGS_ALL_DATA_FRAMES = 0x02
    FORWARD_SERIES_FLAGS_CLEAR = 0x00

    # Step 0: SED_1
    # - Description: Preconditions: SED_1 data poll rate set to 2 seconds.
    # - Pass Criteria: N/A
    print("Step 0: SED_1 data poll rate set to 2 seconds.")

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
    #   - 3.1: The DUT MUST send an ICMPv6 Echo Reply to SED_1.
    #   - 3.2: The DUT MUST send an ICMPv6 Echo Reply to SSED_1.
    print("Step 3: Harness verifies connectivity.")
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(SED_1).\
        filter_wpan_dst64(LEADER).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SED_1).\
        must_next()

    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst64(LEADER).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SSED_1).\
        must_next()

    # Step 4: SED_1
    # - Description: Harness instructs the device to request a Forward Series Link Metric by sending a Link Metrics
    #   Management Request to the DUT with the following payload:
    #   - MLE Link Metrics Management TLV
    #   - Forward Probing Registration Sub-TLV
    #     - Type = 3
    #     - Forward Series ID = 4
    #     - Forward Series Flags = 0x04
    #       - Bits 0-1 = 0
    #       - Bit 2 = 1 (MAC Data Request command frames)
    #       - Bits 3-7 = 0
    #     - Concatenation of Link Metric Type ID Flags=0x40
    #       - E = 0x00; L = 0x01
    #       - Type/Average Enum = 0x00 (count)
    #       - Metric Enum = 0x00 (count)
    # - Pass Criteria: N/A
    print("Step 4: SED_1 requests Forward Series Link Metric ID 4.")
    pkts.filter_wpan_src64(SED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_REQUEST).\
        filter(lambda p: consts.LINK_METRICS_MANAGEMENT_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.LM_FORWARD_PROBING_REGISTRATION_SUB_TLV in p.mle.tlv.link_sub_tlv).\
        filter(lambda p: SERIES_ID_4 in p.mle.tlv.link_forward_series).\
        filter(lambda p: p.mle.tlv.link_forward_series_flags == FORWARD_SERIES_FLAGS_MAC_DATA_REQUEST).\
        filter(lambda p: consts.LINK_METRICS_TYPE_AVERAGE_ENUM_COUNT in p.mle.tlv.metric_type_id_flags.type).\
        filter(lambda p: consts.LINK_METRICS_METRIC_TYPE_ENUM_PDU_COUNT in p.mle.tlv.metric_type_id_flags.metric).\
        filter(lambda p: 1 in p.mle.tlv.metric_type_id_flags.l).\
        must_next()

    # Step 5: Leader (DUT)
    # - Description: Automatically responds to SED_1 with a Link Metrics Management Response.
    # - Pass Criteria: The DUT MUST unicast Link Metrics Management Response to SED_1, including the following TLVs:
    #   - MLE Link Metrics Management TLV
    #   - Link Metrics Status Sub-TLV = 0 (Success)
    print("Step 5: Leader responds to SED_1 with a Link Metrics Management Response.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SED_1).\
        filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_RESPONSE).\
        filter(lambda p: p.mle.tlv.link_status_sub_tlv == consts.LINK_METRICS_STATUS_SUCCESS).\
        must_next()

    # Step 6: SED_1
    # - Description: Harness waits for 30 seconds (MAC Data Requests sent every 2 seconds).
    # - Pass Criteria: N/A
    print("Step 6: Wait for 30 seconds.")
    # Verify that SED_1 is actually polling
    pkts.copy().\
        filter(lambda p: p.wpan.src64 == SED_1 or p.wpan.src16 == SED_1_RLOC16).\
        filter(lambda p: p.wpan.dst64 == LEADER or p.wpan.dst16 == LEADER_RLOC16).\
        filter(lambda p: p.wpan.frame_type == consts.WPAN_CMD).\
        must_next()

    # Step 7: SED_1
    # - Description: Harness instructs the device to retrieve aggregated Forward Series results by sending a MLE Data
    #   Request message to the DUT with the following payload:
    #   - TLV Request TLV (Link Metrics Report TLV specified)
    #   - Link Metrics Query TLV
    #     - Link Metrics Query ID Sub-TLV
    #       - Query ID = 4 [series configured in step 4]
    # - Pass Criteria: N/A
    print("Step 7: SED_1 retrieves aggregated Forward Series ID 4 results.")
    pkts.filter_wpan_src64(SED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_DATA_REQUEST).\
        filter(lambda p: consts.TLV_REQUEST_TLV in p.mle.tlv.type).\
        filter(lambda p: p.mle.tlv.query_id == SERIES_ID_4).\
        must_next()

    # Step 8: Leader (DUT)
    # - Description: Automatically responds to SED_1 with a MLE Data Response.
    # - Pass Criteria: The DUT MUST unicast MLE Data Response to SED_1, including the following TLVs:
    #   - Link Metrics Report TLV
    #     - Link Metrics Report Sub-TLV
    #       - Metric Type ID Flags
    #         - E = 0x00
    #         - L = 0x01
    #         - Type / Average Enum = 0x00 (count/sum)
    #         - Metric Enum = 0x00 (number of PDUs received)
    #       - Count Value (4 bytes)
    print("Step 8: Leader responds to SED_1 with a MLE Data Response.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SED_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: consts.LINK_METRICS_REPORT_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.LINK_METRICS_TYPE_AVERAGE_ENUM_COUNT in p.mle.tlv.metric_type_id_flags.type).\
        filter(lambda p: consts.LINK_METRICS_METRIC_TYPE_ENUM_PDU_COUNT in p.mle.tlv.metric_type_id_flags.metric).\
        filter(lambda p: 1 in p.mle.tlv.metric_type_id_flags.l).\
        must_next()

    # Step 9: SED_1
    # - Description: Harness instructs the device to clear the Forward Series Link Metric by sending a Link Metrics
    #   Management Request with the following payload:
    #   - Link Metrics Management Request:
    #   - Link Metrics Management TLV
    #   - Forward Probing Registration Sub-TLV
    #     - Type = 3
    #     - Forward Series ID = 4 [series configured in step 4]
    #     - Forward Series Flags = 0x00
    #       - Bits 0 -7 = 0
    # - Pass Criteria: N/A
    print("Step 9: SED_1 clears Forward Series Link Metric ID 4.")
    pkts.filter_wpan_src64(SED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_REQUEST).\
        filter(lambda p: consts.LINK_METRICS_MANAGEMENT_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.LM_FORWARD_PROBING_REGISTRATION_SUB_TLV in p.mle.tlv.link_sub_tlv).\
        filter(lambda p: SERIES_ID_4 in p.mle.tlv.link_forward_series).\
        filter(lambda p: p.mle.tlv.link_forward_series_flags == FORWARD_SERIES_FLAGS_CLEAR).\
        must_next()

    # Step 10: Leader (DUT)
    # - Description: Automatically responds to SED_1 with a Link Metrics Management Response.
    # - Pass Criteria: The DUT MUST unicast Link Metrics Management Response to SED_1, including the following TLVs:
    #   - MLE Link Metrics Management TLV
    #   - Link Metrics Status Sub-TLV = 0 (Success)
    print("Step 10: Leader responds to SED_1 with a Link Metrics Management Response.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SED_1).\
        filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_RESPONSE).\
        filter(lambda p: p.mle.tlv.link_status_sub_tlv == consts.LINK_METRICS_STATUS_SUCCESS).\
        must_next()

    # Step 11: SED_1
    # - Description: Harness instructs the device to request a Forward Series Link Metric by sending a Link Metrics
    #   Management Request with the following payload:
    #   - MLE Link Metrics Management TLV
    #   - Forward Probing Registration Sub-TLV
    #     - Type = 3
    #     - Forward Series ID: 11
    #     - Forward Series Flags = 0x04
    #       - Bits 0,1 = 0
    #       - Bit 2 = 1 (MAC Data Request command frames)
    #       - Bits 3-7 = 0
    #     - Concatenation of Link Metric Type ID Flags=0x40
    #       - E= 0x00; L = 0x01
    #       - Metric Enum = 0x00 (count)
    #       - Type/Average Enum = 0x00 (count)
    # - Pass Criteria: N/A
    print("Step 11: SED_1 requests Forward Series Link Metric ID 11.")
    pkts.filter_wpan_src64(SED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_REQUEST).\
        filter(lambda p: consts.LINK_METRICS_MANAGEMENT_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.LM_FORWARD_PROBING_REGISTRATION_SUB_TLV in p.mle.tlv.link_sub_tlv).\
        filter(lambda p: SERIES_ID_11 in p.mle.tlv.link_forward_series).\
        filter(lambda p: p.mle.tlv.link_forward_series_flags == FORWARD_SERIES_FLAGS_MAC_DATA_REQUEST).\
        filter(lambda p: consts.LINK_METRICS_TYPE_AVERAGE_ENUM_COUNT in p.mle.tlv.metric_type_id_flags.type).\
        filter(lambda p: consts.LINK_METRICS_METRIC_TYPE_ENUM_PDU_COUNT in p.mle.tlv.metric_type_id_flags.metric).\
        filter(lambda p: 1 in p.mle.tlv.metric_type_id_flags.l).\
        must_next()

    # Step 12: Leader (DUT)
    # - Description: Automatically responds to SED_1 with a Link Metrics Management Response.
    # - Pass Criteria: The DUT MUST unicast Link Metrics Management Response to SED_1, including the following TLVs:
    #   - MLE Link Metrics Management TLV
    #   - Link Metrics Status Sub-TLV = 0 (Success)
    print("Step 12: Leader responds to SED_1 with a Link Metrics Management Response.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SED_1).\
        filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_RESPONSE).\
        filter(lambda p: p.mle.tlv.link_status_sub_tlv == consts.LINK_METRICS_STATUS_SUCCESS).\
        must_next()

    # Step 13: SED_1
    # - Description: Harness waits for 30 seconds (MAC Data Requests sent every two seconds).
    # - Pass Criteria: N/A
    print("Step 13: Wait for 30 seconds.")
    pkts.copy().\
        filter(lambda p: p.wpan.src64 == SED_1 or p.wpan.src16 == SED_1_RLOC16).\
        filter(lambda p: p.wpan.dst64 == LEADER or p.wpan.dst16 == LEADER_RLOC16).\
        filter(lambda p: p.wpan.frame_type == consts.WPAN_CMD).\
        must_next()

    # Step 14: SED_1
    # - Description: Harness instructs the device to retrieve aggregated Forward Series results by sending a MLE Data
    #   Request message to the DUT with the following payload:
    #   - TLV Request TLV (Link Metrics Report TLV specified)
    #   - Link Metrics Query TLV
    #     - Link Metrics Query ID Sub-TLV
    #       - Query ID = 11 [series configured in step 11]
    # - Pass Criteria: N/A
    print("Step 14: SED_1 retrieves aggregated Forward Series ID 11 results.")
    pkts.filter_wpan_src64(SED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_DATA_REQUEST).\
        filter(lambda p: consts.TLV_REQUEST_TLV in p.mle.tlv.type).\
        filter(lambda p: p.mle.tlv.query_id == SERIES_ID_11).\
        must_next()

    # Step 15: Leader (DUT)
    # - Description: Automatically responds to SED_1 with a MLE Data Response.
    # - Pass Criteria: The DUT MUST unicast MLE Data Response to SED_1, including the following TLVs:
    #   - Link Metrics Report TLV
    #     - Link Metrics Report Sub-TLV
    #       - Metric Type ID Flags
    #         - E = 0x00
    #         - L = 0x01
    #         - Type / Average Enum = 0x00 (count / sum)
    #         - Metric Enum = 0x00 (number of PDUs received)
    #       - Count Value (4 bytes)
    print("Step 15: Leader responds to SED_1 with a MLE Data Response.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SED_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: consts.LINK_METRICS_REPORT_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.LINK_METRICS_TYPE_AVERAGE_ENUM_COUNT in p.mle.tlv.metric_type_id_flags.type).\
        filter(lambda p: consts.LINK_METRICS_METRIC_TYPE_ENUM_PDU_COUNT in p.mle.tlv.metric_type_id_flags.metric).\
        filter(lambda p: 1 in p.mle.tlv.metric_type_id_flags.l).\
        must_next()

    # Step 16: SSED_1
    # - Description: Harness instructs the device to request a Forward Series Link Metric by sending a Link Metrics
    #   Management Request with the following payload:
    #   - MLE Link Metrics Management TLV
    #   - Forward Probing Registration Sub-TLV
    #     - Type = 3
    #     - Forward Series ID: 16
    #     - Forward Series Flags = 0x02
    #       - Bit 0 =0
    #       - Bit 1 = 1 (all Link Layer Data Frames)
    #       - Bits 2-7 = 0
    #     - Concatenation of Link Metric Type ID Flags = 0x0a
    #       - E = 0x00; L = 0x00
    #       - Type/Average Enum = 0x01 - Exponential Moving Avg
    #       - Metric Enum = 0x02 (Link Margin)
    # - Pass Criteria: N/A
    print("Step 16: SSED_1 requests Forward Series Link Metric ID 16.")
    pkts.filter_wpan_src64(SSED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_REQUEST).\
        filter(lambda p: consts.LINK_METRICS_MANAGEMENT_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.LM_FORWARD_PROBING_REGISTRATION_SUB_TLV in p.mle.tlv.link_sub_tlv).\
        filter(lambda p: SERIES_ID_16 in p.mle.tlv.link_forward_series).\
        filter(lambda p: p.mle.tlv.link_forward_series_flags == FORWARD_SERIES_FLAGS_ALL_DATA_FRAMES).\
        filter(lambda p: consts.LINK_METRICS_TYPE_AVERAGE_ENUM_EXPONENTIAL in p.mle.tlv.metric_type_id_flags.type).\
        filter(lambda p: consts.LINK_METRICS_METRIC_TYPE_ENUM_LINK_MARGIN in p.mle.tlv.metric_type_id_flags.metric).\
        filter(lambda p: 0 in p.mle.tlv.metric_type_id_flags.l).\
        must_next()

    # Step 17: Leader (DUT)
    # - Description: Automatically responds with a Link Metrics Management Response.
    # - Pass Criteria: The DUT MUST unicast Link Metrics Management Response to SSED_1, including the following TLVs:
    #   - MLE Link Metrics Management TLV
    #   - Link Metrics Status Sub-TLV = 0 (Success)
    print("Step 17: Leader responds to SSED_1 with a Link Metrics Management Response.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_RESPONSE).\
        filter(lambda p: p.mle.tlv.link_status_sub_tlv == consts.LINK_METRICS_STATUS_SUCCESS).\
        must_next()

    # Step 18: SSED_1
    # - Description: Harness instructs the device to send MAC Data packets every 1 second for 30 seconds.
    # - Pass Criteria: N/A
    print("Step 18: SSED_1 sends MAC Data packets every 1 second for 30 seconds.")
    pkts.copy().\
        filter(lambda p: p.wpan.src64 == SSED_1 or p.wpan.src16 == SSED_1_RLOC16).\
        filter(lambda p: p.wpan.dst64 == LEADER or p.wpan.dst16 == LEADER_RLOC16).\
        filter_ping_request().\
        must_next()

    # Step 19: SSED_1
    # - Description: Harness instructs the device to retrieve aggregated Forward Series results by sending a MLE Data
    #   Request message to the DUT with the following payload:
    #   - TLV Request TLV (Link Metrics Report TLV specified)
    #   - Link Metrics Query TLV
    #     - Link Metrics Query ID Sub-TLV
    #       - Query ID = 16 [same value as set in step 16]
    # - Pass Criteria: N/A
    print("Step 19: SSED_1 retrieves aggregated Forward Series ID 16 results.")
    pkts.filter_wpan_src64(SSED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_DATA_REQUEST).\
        filter(lambda p: consts.TLV_REQUEST_TLV in p.mle.tlv.type).\
        filter(lambda p: p.mle.tlv.query_id == SERIES_ID_16).\
        must_next()

    # Step 20: Leader (DUT)
    # - Description: Automatically responds with a MLE Data Response.
    # - Pass Criteria: The DUT MUST reply to SSED_1 with a MLE Data Response with the following:
    #   - Link Metrics Report TLV
    #     - Link Metrics Report Sub-TLV
    #     - Metric Type ID Flags
    #       - E = 0x00
    #       - L = 0x00
    #       - Type / Average Enum = 0x01 (Exponential Moving Avg)
    #       - Metric Enum = 0x02 (Link Margin)
    #     - Value (1 byte)
    print("Step 20: Leader responds to SSED_1 with a MLE Data Response.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: consts.LINK_METRICS_REPORT_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.LINK_METRICS_TYPE_AVERAGE_ENUM_EXPONENTIAL in p.mle.tlv.metric_type_id_flags.type).\
        filter(lambda p: consts.LINK_METRICS_METRIC_TYPE_ENUM_LINK_MARGIN in p.mle.tlv.metric_type_id_flags.metric).\
        filter(lambda p: 0 in p.mle.tlv.metric_type_id_flags.l).\
        must_next()

    # Step 21: SSED_1
    # - Description: Harness instructs device to clear an ongoing Forward Series by sending a Link Metrics Management
    #   Request with the following payload:
    #   - MLE Link Metrics Management TLV
    #   - Forward Probing Registration Sub-TLV
    #     - Type = 3
    #     - Forward Series ID: = 16 [same value as set in step 16]
    #     - Forward Series Flags = 0x00
    # - Pass Criteria: N/A
    print("Step 21: SSED_1 clears Forward Series ID 16.")
    pkts.filter_wpan_src64(SSED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_REQUEST).\
        filter(lambda p: consts.LINK_METRICS_MANAGEMENT_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.LM_FORWARD_PROBING_REGISTRATION_SUB_TLV in p.mle.tlv.link_sub_tlv).\
        filter(lambda p: SERIES_ID_16 in p.mle.tlv.link_forward_series).\
        filter(lambda p: p.mle.tlv.link_forward_series_flags == FORWARD_SERIES_FLAGS_CLEAR).\
        must_next()

    # Step 22: Leader (DUT)
    # - Description: Automatically responds with a Link Metrics Management Response.
    # - Pass Criteria: The DUT MUST reply to SSED_1 with a Link Metrics Management Response containing the following:
    #   - MLE Link Metrics Management TLV
    #   - Link Metrics Status Sub-TLV = 0 (Success)
    print("Step 22: Leader responds to SSED_1 with a Link Metrics Management Response.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_RESPONSE).\
        filter(lambda p: p.mle.tlv.link_status_sub_tlv == consts.LINK_METRICS_STATUS_SUCCESS).\
        must_next()

    # Step 23: SSED_1
    # - Description: Harness instructs the device to retrieve aggregated Forward Series results by sending a MLE Data
    #   Request message to the DUT with the following payload:
    #   - TLV Request TLV (Link Metrics Report TLV specified)
    #   - Link Metrics Query TLV
    #     - Link Metrics Query ID Sub-TLV
    #       - Query ID = 16 [same value as set in step 16]
    #   - This is a negative step
    # - Pass Criteria: N/A
    print("Step 23: SSED_1 retrieves aggregated Forward Series ID 16 results (negative step).")
    pkts.filter_wpan_src64(SSED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_DATA_REQUEST).\
        filter(lambda p: consts.TLV_REQUEST_TLV in p.mle.tlv.type).\
        filter(lambda p: p.mle.tlv.query_id == SERIES_ID_16).\
        must_next()

    # Step 24: Leader (DUT)
    # - Description: Automatically responds with a MLE Data Response.
    # - Pass Criteria: The DUT MUST reply to SSED_1 with a MLE Data Response with the following:
    #   - Link Metrics Report TLV
    #     - Link Metrics Status Sub-TLV
    #       - Status = 3 (Failure - Series ID not recognized)
    print("Step 24: Leader responds to SSED_1 with a MLE Data Response (Failure).")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: p.mle.tlv.link_status_sub_tlv == consts.LINK_METRICS_STATUS_SERIES_ID_NOT_RECOGNIZED).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
