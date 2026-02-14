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
    # 5.7.1 CoAP Diagnostic Get Request, Response and Reset Commands
    #
    # 5.7.1.1 Topology
    # - Topology A
    # - Topology B
    #
    # 5.7.1.2 Purpose & Description
    # These cases test the Diagnostic Get and Reset Commands as a part of the Network Management.
    #
    # Spec Reference      | V1.1 Section | V1.3.0 Section
    # --------------------|--------------|---------------
    # Diagnostic Commands | 10.11.2      | 10.11.2

    pkts = pv.pkts
    pv.summary.show()

    DUT = pv.vars['DUT']
    DUT_RLOC = pv.vars['DUT_RLOC']
    LEADER = pv.vars['LEADER']
    LEADER_RLOC = pv.vars['LEADER_RLOC']

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A
    print("Step 1: Ensure topology is formed correctly.")

    # Step 2: Leader
    # - Description: Harness instructs the device to send DIAG_GET.req to the DUT’s Routing Locator (RLOC) for the
    #   following diagnostic TLV types:
    #   - TLV Type 0 – MAC Extended Address (64-bit)
    #   - TLV Type 1 - MAC Address (16-bit)
    #   - TLV Type 2 - Mode (Capability information)
    #   - TLV Type 4 – Connectivity
    #   - TLV Type 5 – Route64
    #   - TLV Type 6 – Leader Data
    #   - TLV Type 7 – Network Data
    #   - TLV Type 8 – IPv6 address list
    #   - TLV Type 17 – Channel Pages
    # - Pass Criteria:
    #   - The DUT MUST respond with a DIAG_GET.rsp response containing the requested diagnostic TLVs:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload:
    #     - TLV Type 0 - MAC Extended Address (64-bit)
    #     - TLV Type 1 - MAC Address (16-bit)
    #     - TLV Type 2 - Mode (Capability information)
    #     - TLV Type 4 – Connectivity
    #     - TLV Type 5 – Route64 (required ONLY for Topology A)
    #     - TLV Type 6 – Leader Data
    #     - TLV Type 7 – Network Data
    #     - TLV Type 8 – IPv6 address list
    #     - TLV Type 17 – Channel Pages
    #   - The presence of each TLV MUST be validated. Where possible, the value of the TLV’s MUST be validated.
    #   - Route64 TLV MUST be omitted in Topology B.
    print("Step 2: Leader sends DIAG_GET.req and DUT responds with all requested TLVs.")
    pkts.filter_wpan_src64(LEADER).\
        filter_ipv6_dst(DUT_RLOC).\
        filter_coap_request(consts.DIAG_GET_URI).\
        filter(lambda p: p.coap.tlv.type is not nullField and\
               consts.DG_TYPE_LIST_TLV in p.coap.tlv.type).\
        must_next()

    pkts.filter_wpan_src64(DUT).\
        filter_ipv6_dst(LEADER_RLOC).\
        filter_coap_ack(consts.DIAG_GET_URI).\
        filter(lambda p: p.coap.tlv.type is not nullField and\
               {\
                consts.DG_MAC_EXTENDED_ADDRESS_TLV,\
                consts.DG_MAC_ADDRESS_TLV,\
                consts.DG_MODE_TLV,\
                consts.DG_CONNECTIVITY_TLV,\
                consts.DG_ROUTE64_TLV,\
                consts.DG_LEADER_DATA_TLV,\
                consts.DG_NETWORK_DATA_TLV,\
                consts.DG_IPV6_ADDRESS_LIST_TLV,\
                consts.DG_CHANNEL_PAGES_TLV\
                } <= set(p.coap.tlv.type)).\
        filter(lambda p: p.coap.tlv.mac_addr == DUT).\
        filter(lambda p: p.coap.tlv.mode is not nullField).\
        filter(lambda p: p.coap.tlv.leader_router_id is not nullField).\
        must_next()

    # Step 3: Leader
    # - Description: Harness instructs the device to send DIAG_GET.req to the DUT’s Routing Locator (RLOC) for the
    #   following diagnostic TLV type:
    #   - TLV Type 9 - MAC Counters
    # - Pass Criteria:
    #   - The DUT MUST respond with a DIAG_GET.rsp response containing the requested diagnostic TLV:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload:
    #     - TLV Type 9 - MAC Counters
    #   - TLV Type 9 - MAC Counters MUST contain a list of MAC Counters.
    print("Step 3: Leader sends DIAG_GET.req for MAC Counters.")
    pkts.filter_wpan_src64(LEADER).\
        filter_ipv6_dst(DUT_RLOC).\
        filter_coap_request(consts.DIAG_GET_URI).\
        filter(lambda p: p.coap.tlv.type is not nullField and\
               consts.DG_TYPE_LIST_TLV in p.coap.tlv.type).\
        must_next()

    p3 = pkts.filter_wpan_src64(DUT).\
        filter_ipv6_dst(LEADER_RLOC).\
        filter_coap_ack(consts.DIAG_GET_URI).\
        filter(lambda p: p.coap.tlv.type is not nullField and\
               consts.DG_MAC_COUNTERS_TLV in p.coap.tlv.type).\
        filter(lambda p: p.coap.tlv.mac_counter is not nullField and\
               len(p.coap.tlv.mac_counter) >= 9).\
        must_next()

    # Capture MAC counters from step 3 for comparison in step 8
    mac_counters_step_3 = [int(val) for val in p3.coap.tlv.mac_counter]

    # Step 4: Leader
    # - Description: Harness instructs the device to send DIAG_GET.req to the DUT’s Routing Locator (RLOC) for the
    #   following diagnostic TLV type:
    #   - TLV Type 3 – Timeout
    # - Pass Criteria:
    #   - The DUT MUST respond with a DIAG_GET.rsp response containing the required diagnostic TLV payload:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload:
    #     - TLV Value 3 - Timeout MUST be omitted from the response.
    print("Step 4: Leader sends DIAG_GET.req for Timeout (expected to be omitted).")
    pkts.filter_wpan_src64(LEADER).\
        filter_ipv6_dst(DUT_RLOC).\
        filter_coap_request(consts.DIAG_GET_URI).\
        filter(lambda p: p.coap.tlv.type is not nullField and\
               consts.DG_TYPE_LIST_TLV in p.coap.tlv.type).\
        must_next()

    pkts.filter_wpan_src64(DUT).\
        filter_ipv6_dst(LEADER_RLOC).\
        filter_coap_ack(consts.DIAG_GET_URI).\
        filter(lambda p: p.coap.tlv.type is nullField or\
               consts.DG_TIMEOUT_TLV not in p.coap.tlv.type).\
        must_next()

    # Step 5: Leader
    # - Description: Harness instructs the device to send DIAG_GET.req to the DUT’s Routing Locator (RLOC) for the
    #   following diagnostic TLV types:
    #   - TLV Type 14 – Battery Level
    #   - TLV Type 15 – Supply Voltage
    # - Pass Criteria:
    #   - The DUT MUST respond with a DIAG_GET.rsp response optionally containing the requested diagnostic TLVs:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload:
    #     - TLV Type 14 – Battery Level (optional)
    #     - TLV Type 15 – Supply Voltage (optional)
    print("Step 5: Leader sends DIAG_GET.req for Battery Level and Supply Voltage.")
    pkts.filter_wpan_src64(LEADER).\
        filter_ipv6_dst(DUT_RLOC).\
        filter_coap_request(consts.DIAG_GET_URI).\
        filter(lambda p: p.coap.tlv.type is not nullField and\
               consts.DG_TYPE_LIST_TLV in p.coap.tlv.type).\
        must_next()

    pkts.filter_wpan_src64(DUT).\
        filter_ipv6_dst(LEADER_RLOC).\
        filter_coap_ack(consts.DIAG_GET_URI).\
        must_next()

    # Step 6: Leader
    # - Description: Harness instructs the device to send DIAG_GET.req to the DUT’s Routing Locator (RLOC) for the
    #   following diagnostic TLV type:
    #   - TLV Type 16 – Child Table
    # - Pass Criteria:
    #   - For Topology A:
    #     - CoAP Response Code: 2.04 Changed
    #     - CoAP Payload: TLV Type 16 – Child Table. The content of the TLV MUST be correct.
    #   - For Topology B:
    #     - CoAP Response Code: 2.04 Changed
    #     - CoAP Payload: Empty
    print("Step 6: Leader sends DIAG_GET.req for Child Table.")
    pkts.filter_wpan_src64(LEADER).\
        filter_ipv6_dst(DUT_RLOC).\
        filter_coap_request(consts.DIAG_GET_URI).\
        filter(lambda p: p.coap.tlv.type is not nullField and\
               consts.DG_TYPE_LIST_TLV in p.coap.tlv.type).\
        must_next()

    pkts.filter_wpan_src64(DUT).\
        filter_ipv6_dst(LEADER_RLOC).\
        filter_coap_ack(consts.DIAG_GET_URI).\
        filter(lambda p: p.coap.tlv.type is not nullField and\
               consts.DG_CHILD_TABLE_TLV in p.coap.tlv.type).\
        must_next()

    # Step 7: Leader
    # - Description: Harness instructs the device to send DIAG_RST.ntf to DUT’s Routing Locator (RLOC) for the
    #   following diagnostic TLV type:
    #   - TLV Type 9 - MAC Counters
    # - Pass Criteria:
    #   - The DUT MUST respond with a CoAP response:
    #   - CoAP Response Code: 2.04 Changed
    print("Step 7: Leader sends DIAG_RST.ntf for MAC Counters.")
    pkts.filter_wpan_src64(LEADER).\
        filter_ipv6_dst(DUT_RLOC).\
        filter_coap_request(consts.DIAG_RST_URI).\
        filter(lambda p: p.coap.tlv.type is not nullField and\
               consts.DG_TYPE_LIST_TLV in p.coap.tlv.type).\
        must_next()

    pkts.filter_wpan_src64(DUT).\
        filter_ipv6_dst(LEADER_RLOC).\
        filter_coap_ack(consts.DIAG_RST_URI).\
        must_next()

    # Step 8: Leader
    # - Description: Harness instructs the device to send DIAG_GET.req to the DUT’s Routing Locator (RLOC) for the
    #   following diagnostic TLV type:
    #   - TLV Type 9 - MAC Counters
    # - Pass Criteria:
    #   - The DUT MUST respond with a DIAG_GET.rsp response containing the requested diagnostic TLV:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload:
    #     - TLV Type 9 - MAC Counters
    #   - TLV Type 9 - MAC Counters MUST contain a list of MAC Counters with 0 value or less than value returned in
    #     step 3.
    print("Step 8: Leader sends DIAG_GET.req for MAC Counters (verify reset).")
    pkts.filter_wpan_src64(LEADER).\
        filter_ipv6_dst(DUT_RLOC).\
        filter_coap_request(consts.DIAG_GET_URI).\
        filter(lambda p: p.coap.tlv.type is not nullField and\
               consts.DG_TYPE_LIST_TLV in p.coap.tlv.type).\
        must_next()

    p8 = pkts.filter_wpan_src64(DUT).\
        filter_ipv6_dst(LEADER_RLOC).\
        filter_coap_ack(consts.DIAG_GET_URI).\
        filter(lambda p: p.coap.tlv.type is not nullField and\
               consts.DG_MAC_COUNTERS_TLV in p.coap.tlv.type).\
        filter(lambda p: p.coap.tlv.mac_counter is not nullField and\
               len(p.coap.tlv.mac_counter) >= 9).\
        must_next()

    # Verify MAC counters were reset (sum of counters should be less than before, or most should be 0)
    mac_counters_step_8 = [int(val) for val in p8.coap.tlv.mac_counter]
    for i in range(len(mac_counters_step_3)):
        assert mac_counters_step_8[i] <= mac_counters_step_3[i], \
            f"MAC counter {i} increased after reset: {mac_counters_step_3[i]} -> {mac_counters_step_8[i]}"

    assert sum(mac_counters_step_8) <= sum(mac_counters_step_3), "Total MAC counters increased after reset"


if __name__ == '__main__':
    verify_utils.run_main(verify)
