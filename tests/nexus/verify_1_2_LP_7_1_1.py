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
    # 7.1.1 Single Probe Link Metrics with Enhanced ACKs
    #
    # 7.1.1.1 Topology
    # (No specific topology text description is provided other than an implicit
    #   reference to a diagram with DUT as Leader, SED_1, and SSED_1).
    #
    # 7.1.1.2 Purpose & Definition
    # The purpose of this test case is to ensure that the DUT can successfully
    #   respond to a Single Probe Link Metrics Request from a SED & a SSED
    #   with Enhanced ACKs.
    # - 4 - SED, Enhanced ACK, type/avg enum=1, metric enum=2
    # - 6 - SSED, Enhanced ACK, type/avg enum=1, metric enum =2 and 3
    # - 8 - SSED requests link metrics report via Enhanced ACK
    # - 10 - SED requests link metrics report via Enhanced ACK
    # - 12 - Clear SSED Enhanced ACK config
    # - 14 - SSED requests link metrics report via Enhanced ACK (negative test)
    # - 16 - SSED, Enhanced ACK, type/avg enum=1, metric enum= 1, 2, and 3
    #   (negative test)
    # - 18 - SSED, Enhanced ACK, type/avg enum=2, metric enum=2 (negative test)
    #
    # Specification Reference: 4.11.3

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
    # - Pass Criteria: N/A.
    print("Step 1: Topology formation: DUT, SED_1, SSED_1.")

    # Step 2: SED_1, SSED_1
    # - Description: Each device automatically attaches to the Leader (DUT).
    # - Pass Criteria: Successfully attaches to DUT.
    print("Step 2: Each device automatically attaches to the Leader (DUT).")
    pkts.copy().\
        filter_wpan_src64(SED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        must_next()
    pkts.copy().\
        filter_wpan_src64(SSED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        must_next()

    # Step 3: SED_1, SSED_1
    # - Description: Harness verifies connectivity by instructing each device to
    #   send an ICMPv6 Echo Request to the DUT.
    # - Pass Criteria:
    #   - 3.1 : The DUT MUST send an ICMPv6 Echo Reply to SED_1.
    #   - 3.2 : The DUT MUST send an ICMPv6 Echo Reply to SSED_1.
    print(
        "Step 3: Harness verifies connectivity by instructing each device to send an ICMPv6 Echo Request to the DUT.")
    _pkt_sed = pkts.copy().\
        filter_ping_request().\
        filter_wpan_src16(SED_1_RLOC16).\
        filter_wpan_dst16(LEADER_RLOC16).\
        must_next()
    pkts.copy().\
        filter_ping_reply(identifier=_pkt_sed.icmpv6.echo.identifier).\
        filter_wpan_src16(LEADER_RLOC16).\
        filter_wpan_dst16(SED_1_RLOC16).\
        must_next()

    _pkt_ssed = pkts.copy().\
        filter_ping_request().\
        filter_wpan_src16(SSED_1_RLOC16).\
        filter_wpan_dst16(LEADER_RLOC16).\
        must_next()
    pkts.copy().\
        filter_ping_reply(identifier=_pkt_ssed.icmpv6.echo.identifier).\
        filter_wpan_src16(LEADER_RLOC16).\
        filter_wpan_dst16(SSED_1_RLOC16).\
        must_next()

    # Step 4: SED_1
    # - Description: Harness instructs the device to enable
    #   IEEE 802.15.4-2015 Enhanced ACK-based Probing by sending a
    #   Link Metrics Management Request to the DUT.
    #   - Link Metrics Management TLV payload:
    #     - Enhanced ACK Link Metrics Configuration Sub-TLV
    #     - Enh-ACK Flags = 0x01 (register a configuration)
    #     - Concatenation of Link Metric Type ID Flags = 0x0a
    #       - E = 0x00
    #       - L = 0x00
    #       - Type / Average Enum = 1 (Exponential Moving Avg)
    #       - Metric Enum = 2 (Link Margin)
    # - Pass Criteria: N/A.
    print("Step 4: SED_1 enable IEEE 802.15.4-2015 Enhanced ACK-based Probing.")
    pkts.filter_wpan_src64(SED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_REQUEST).\
        filter(lambda p: consts.LM_ENHANCED_ACK_CONFIGURATION_SUB_TLV in p.mle.tlv.link_sub_tlv).\
        filter(lambda p: p.mle.tlv.link_enh_ack_flags == consts.LINK_METRICS_ENH_ACK_PROBING_REGISTER).\
        filter(lambda p: p.mle.tlv.link_requested_type_id_flags == '0a').\
        must_next()

    # Step 5: Leader (DUT)
    # - Description: Automatically responds to SED_1 with a Link Metrics
    #   Management Response.
    # - Pass Criteria: The DUT MUST send a Link Metrics Management Response to
    #   SED_1 containing the following TLVs:
    #   - MLE Link Metrics Management TLV
    #   - Link Metrics Status Sub-TLV = 0 (Success)
    print("Step 5: Leader (DUT) automatically responds to SED_1 with a Link Metrics Management Response.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SED_1).\
        filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_RESPONSE).\
        filter(lambda p: p.mle.tlv.link_status_sub_tlv == consts.LINK_METRICS_STATUS_SUCCESS).\
        must_next()

    # Step 6: SSED_1
    # - Description: Harness instructs the device to enable
    #   IEEE 802.15.4-2015 Enhanced ACK-based Probing by sending a
    #   Link Metrics Management Request to the DUT.
    #   - MLE Link Metrics Management TLV Payload:
    #     - Enhanced ACK Link Metrics Configuration Sub-TLV
    #     - Enh-ACK Flags = 1 (register a configuration)
    #     - Concatenation of Link Metric Type ID Flags = 0x0a0b
    #       - E = 0x00
    #       - L = 0x00
    #       - Type / Average Enum = 1 (Exponential Moving Avg)
    #       - Metric Enum = 2 (Link Margin)
    #       - E = 0x00
    #       - L = 0x00
    #       - Type / Average Enum = 1 (Exponential Moving Avg)
    #       - Metric Enum = 3 (RSSI)
    # - Pass Criteria: N/A.
    print("Step 6: SSED_1 enable IEEE 802.15.4-2015 Enhanced ACK-based Probing.")
    pkts.filter_wpan_src64(SSED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_REQUEST).\
        filter(lambda p: consts.LM_ENHANCED_ACK_CONFIGURATION_SUB_TLV in p.mle.tlv.link_sub_tlv).\
        filter(lambda p: p.mle.tlv.link_enh_ack_flags == consts.LINK_METRICS_ENH_ACK_PROBING_REGISTER).\
        filter(lambda p: p.mle.tlv.link_requested_type_id_flags == '0a0b').\
        must_next()

    # Step 7: Leader (DUT)
    # - Description: Automatically responds to SSED_1 with a Link Metrics
    #   Management Response.
    # - Pass Criteria: The DUT MUST send Link Metrics Management Response to
    #   SSED_1 containing the following:
    #   - MLE Link Metrics Management TLV
    #   - Link Metrics Status Sub-TLV = 0 (Success)
    print("Step 7: Leader (DUT) automatically responds to SSED_1 with a Link Metrics Management Response.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_RESPONSE).\
        filter(lambda p: p.mle.tlv.link_status_sub_tlv == consts.LINK_METRICS_STATUS_SUCCESS).\
        must_next()

    # Step 8: SSED_1
    # - Description: Harness instructs the device to solicit an Enhanced ACK from
    #   the DUT by sending a MAC Data Message with the Frame Version subfield
    #   within the MAC header of the message set to 0b10.
    # - Pass Criteria: N/A.
    print("Step 8: SSED_1 solicit an Enhanced ACK from the DUT.")
    _pkt = pkts.filter_wpan_src16(SSED_1_RLOC16).\
        filter_wpan_data().\
        filter_wpan_version(consts.MAC_FRAME_VERSION_2015).\
        must_next()
    _ack_seq_no = _pkt.wpan.seq_no

    # Step 9: Leader (DUT)
    # - Description: Automatically responds to SSED_1 with an Enhanced
    #   Acknowledgement containing link metrics.
    # - Pass Criteria: The DUT MUST reply to SSED_1 with an Enhanced ACK,
    #   containing the following:
    #   - Frame Control Field
    #   - Security Enabled = True
    #   - Header IE
    #     - Element ID = 0x00
    #     - Vendor CID = 0xeab89b (Thread Group)
    #     - Vendor Specific Information
    #       - 1st byte = 0 (Enhanced ACK Link Metrics)
    #       - 2nd byte ... Link Margin data
    #       - 3rd byte ... RSSI data
    print("Step 9: Leader (DUT) automatically responds to SSED_1 with an Enhanced ACK containing link metrics.")
    pkts.filter_wpan_ack().\
        filter_wpan_seq(_ack_seq_no).\
        filter_wpan_dst16(SSED_1_RLOC16).\
        filter(lambda p: p.wpan.payload_ie.vendor.oui == consts.THREAD_IEEE_802154_COMPANY_ID).\
        must_next()

    # Step 10: SED_1
    # - Description: Harness instructs the device to solicit an Enhanced ACK from
    #   the DUT by sending a MAC Data Message with the Frame Version subfield
    #   within the MAC header of the message set to 0b10.
    # - Pass Criteria: N/A.
    print("Step 10: SED_1 solicit an Enhanced ACK from the DUT.")
    _pkt = pkts.filter_wpan_src16(SED_1_RLOC16).\
        filter_wpan_data().\
        filter_wpan_version(consts.MAC_FRAME_VERSION_2015).\
        must_next()
    _ack_seq_no = _pkt.wpan.seq_no

    # Step 11: Leader (DUT)
    # - Description: Automatically responds to SED_1 with an Enhanced
    #   Acknowledgement containing link metrics.
    # - Pass Criteria: The DUT MUST reply to SED_1 with an Enhanced ACK,
    #   containing the following:
    #   - Header IE
    #   - Element ID = 0x00
    #   - Vendor CID = 0xeab89b (Thread Group)
    #   - Vendor Specific Information
    #     - 1st byte = 0 (Enhanced ACK Link Metrics)
    #     - 2nd byte ... Link Margin data
    print("Step 11: Leader (DUT) automatically responds to SED_1 with an Enhanced ACK containing link metrics.")
    pkts.filter_wpan_ack().\
        filter_wpan_seq(_ack_seq_no).\
        filter_wpan_dst16(SED_1_RLOC16).\
        filter(lambda p: p.wpan.payload_ie.vendor.oui == consts.THREAD_IEEE_802154_COMPANY_ID).\
        must_next()

    # Step 12: SSED_1
    # - Description: Harness instructs the device to clear its Enhanced ACK link
    #   metrics configuration by instructing it to send a Link Metrics Management
    #   Request to the DUT.
    #   - MLE Link Metrics Management TLV Payload:
    #     - Enhanced ACK Configuration Sub-TLV
    #     - Enh-ACK Flags = 0 (clear enhanced ACK link metric config)
    # - Pass Criteria: N/A.
    print("Step 12: SSED_1 clear its Enhanced ACK link metrics configuration.")
    pkts.filter_wpan_src64(SSED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_REQUEST).\
        filter(lambda p: consts.LM_ENHANCED_ACK_CONFIGURATION_SUB_TLV in p.mle.tlv.link_sub_tlv).\
        filter(lambda p: p.mle.tlv.link_enh_ack_flags == consts.LINK_METRICS_ENH_ACK_PROBING_CLEAR).\
        must_next()

    # Step 13: Leader (DUT)
    # - Description: Automatically responds to SSED_1 with a Link Metrics
    #   Management Response.
    # - Pass Criteria: The DUT MUST send Link Metrics Management Response to
    #   SSED_1 containing the following:
    #   - MLE Link Metrics Management TLV
    #   - Link Metrics Status Sub-TLV = 0 (Success)
    print("Step 13: Leader (DUT) automatically responds to SSED_1 with a Link Metrics Management Response.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_RESPONSE).\
        filter(lambda p: p.mle.tlv.link_status_sub_tlv == consts.LINK_METRICS_STATUS_SUCCESS).\
        must_next()

    # Step 14: SSED_1
    # - Description: Harness instructs the device to solicit an Enhanced ACK from
    #   the DUT by sending a MAC Data Message with the Frame Version subfield
    #   within the MAC header of the message set to 0b10.
    # - Pass Criteria: N/A.
    print("Step 14: SSED_1 solicit an Enhanced ACK from the DUT.")
    _pkt = pkts.filter_wpan_src16(SSED_1_RLOC16).\
        filter_wpan_data().\
        must_next()
    _ack_seq_no = _pkt.wpan.seq_no

    # Step 15: Leader (DUT)
    # - Description: Automatically responds to SSED_1 with an immediate ACK.
    # - Pass Criteria: The DUT MUST NOT include a Link Metrics Report in the ACK.
    print("Step 15: Leader (DUT) automatically responds to SSED_1 with an immediate ACK.")
    pkts.filter_wpan_ack().\
        filter_wpan_seq(_ack_seq_no).\
        filter_wpan_ie_not_present().\
        must_next()

    # Step 16: SSED_1
    # - Description: Harness verifies that Enhanced ACKs cannot be enabled while
    #   requesting 3 metric types by instructing the device to send the following
    #   Link Metrics Management Request to the DUT.
    #   - MLE Link Metrics Management TLV Payload:
    #     - Enhanced ACK Link Metrics Configuration Sub-TLV
    #     - Enh-ACK Flags = 1 (register a configuration)
    #     - Concatenation of Link Metric Type ID Flags = 0x090a0b
    #       - E = 0x00; L = 0x00
    #       - Type / Average Enum = 1 (Exponential Moving Avg)
    #       - Metric Enum = 1 (Layer 2 LQI)
    #       - E = 0x00; L = 0x00
    #       - Type / Average Enum = 1 (Exponential Moving Avg)
    #       - Metric Enum = 2 (Link Margin)
    #       - E = 0x00; L = 0x00
    #       - Type / Average Enum = 1 (Exponential Moving Avg)
    #       - Metric Enum = 3 (RSSI)
    # - Pass Criteria: N/A.
    print("Step 16: SSED_1 request 3 metric types for Enhanced ACKs.")
    pkts.filter_wpan_src64(SSED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_REQUEST).\
        filter(lambda p: consts.LM_ENHANCED_ACK_CONFIGURATION_SUB_TLV in p.mle.tlv.link_sub_tlv).\
        filter(lambda p: p.mle.tlv.link_enh_ack_flags == consts.LINK_METRICS_ENH_ACK_PROBING_REGISTER).\
        filter(lambda p: p.mle.tlv.link_requested_type_id_flags == '090a0b').\
        must_next()

    # Step 17: Leader (DUT)
    # - Description: Automatically responds to the invalid query from SSED_1
    #   with a failure.
    # - Pass Criteria: The DUT MUST send Link Metrics Management Response to
    #   SSED_1 containing the following:
    #   - MLE Link Metrics Management TLV
    #   - Link Metrics Status Sub-TLV = 254 (Failure)
    print("Step 17: Leader (DUT) automatically responds to SSED_1 with a failure.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_RESPONSE).\
        filter(lambda p: p.mle.tlv.link_status_sub_tlv == consts.LINK_METRICS_STATUS_OTHER_ERROR).\
        must_next()

    # Step 18: SSED_1
    # - Description: Harness verifies that Enhanced ACKs cannot be enabled while
    #   requesting a reserved Type/Average Enum of value 2 by instructing the
    #   device to send the following Link Metrics Management Request to the DUT.
    #   - MLE Link Metrics Management TLV Payload:
    #     - Enhanced ACK Link Metrics Configuration Sub-TLV
    #     - Enh-ACK Flags = 1 (register a configuration)
    #     - Link Metric Type ID Flags = 0x12
    #       - E = 0x00; L = 0x00
    #       - Type / Average Enum = 2 (Reserved)
    #       - Metric Enum = 2 (Link Margin)
    # - Pass Criteria: N/A.
    print("Step 18: SSED_1 tries to register reserved Type/Average Enum.")
    pkts.filter_wpan_src64(SSED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_REQUEST).\
        filter(lambda p: consts.LM_ENHANCED_ACK_CONFIGURATION_SUB_TLV in p.mle.tlv.link_sub_tlv).\
        filter(lambda p: p.mle.tlv.link_enh_ack_flags == consts.LINK_METRICS_ENH_ACK_PROBING_REGISTER).\
        filter(lambda p: p.mle.tlv.link_requested_type_id_flags == '12').\
        must_next()

    # Step 19: Leader (DUT)
    # - Description: Automatically responds to the invalid SSED_1 query
    #   with a failure.
    # - Pass Criteria: The DUT MUST respond to SSED_1 with a Link Metrics
    #   Management Response containing the following:
    #   - MLE Link Metrics Management TLV
    #   - Link Metrics Status Sub-TLV = 254 (Failure)
    print("Step 19: Leader (DUT) automatically responds to the invalid SSED_1 query with a failure.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_LINK_METRICS_MANAGEMENT_RESPONSE).\
        filter(lambda p: p.mle.tlv.link_status_sub_tlv == consts.LINK_METRICS_STATUS_OTHER_ERROR).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
