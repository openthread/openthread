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
    # 5.7.2 CoAP Diagnostic Get Query and Answer Commands – REED
    #
    # 5.7.2.1 Topology
    # - Leader
    # - Router_1
    # - REED_1 (DUT)
    # - (Additional routers as needed to satisfy REED conditions, typically a total of 16 active routers)
    #
    # 5.7.2.2 Purpose & Description
    # This test case exercises the Diagnostic Get Query and Answer commands as part of the Network Management. This
    #   test case topology is specific to REED DUTs.
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # Diag Commands    | 10.11.2      | 10.11.2

    EXPECTED_DIAG_GET_TLVS = {
        consts.DG_MAC_EXTENDED_ADDRESS_TLV, consts.DG_MAC_ADDRESS_TLV, consts.DG_MODE_TLV, consts.DG_CONNECTIVITY_TLV,
        consts.DG_LEADER_DATA_TLV, consts.DG_NETWORK_DATA_TLV, consts.DG_IPV6_ADDRESS_LIST_TLV,
        consts.DG_CHANNEL_PAGES_TLV
    }

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    LEADER_RLOC = pv.vars['LEADER_RLOC']
    REED_1 = pv.vars['REED_1']
    REED_1_RLOC = pv.vars['REED_1_RLOC']

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
    #     - TLV Type 5 – Route64 (optional)
    #     - TLV Type 6 – Leader Data
    #     - TLV Type 7 – Network Data
    #     - TLV Type 8 – IPv6 address list
    #     - TLV Type 17 – Channel Pages
    #   - The presence of each TLV MUST be validated. Where possible, the value of the TLVs MUST be validated.
    print("Step 2: Leader sends DIAG_GET.req to REED_1.")
    pkts.filter_ipv6_src(LEADER_RLOC).\
        filter_ipv6_dst(REED_1_RLOC).\
        filter_coap_request(consts.DIAG_GET_URI).\
        filter(lambda p: consts.DG_TYPE_LIST_TLV in p.coap.tlv.type).\
        must_next()

    pkts.filter_ipv6_src(REED_1_RLOC).\
        filter_ipv6_dst(LEADER_RLOC).\
        filter_coap_ack(consts.DIAG_GET_URI).\
        filter(lambda p: EXPECTED_DIAG_GET_TLVS <= set(p.coap.tlv.type)).\
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
    pkts.filter_ipv6_src(LEADER_RLOC).\
        filter_ipv6_dst(REED_1_RLOC).\
        filter_coap_request(consts.DIAG_GET_URI).\
        filter(lambda p: consts.DG_TYPE_LIST_TLV in p.coap.tlv.type).\
        must_next()

    p3 = pkts.filter_ipv6_src(REED_1_RLOC).\
        filter_ipv6_dst(LEADER_RLOC).\
        filter_coap_ack(consts.DIAG_GET_URI).\
        filter(lambda p: consts.DG_MAC_COUNTERS_TLV in p.coap.tlv.type).\
        must_next()

    mac_counters_step_3 = [int(val) for val in p3.coap.tlv.mac_counter]

    # Step 4: Leader
    # - Description: Harness instructs the device to send DIAG_GET.req to the DUT’s Routing Locator (RLOC) for the
    #   following diagnostic TLV types:
    #   - TLV Type 3 – Timeout
    #   - TLV Type 16 – Child Table TLV
    # - Pass Criteria:
    #   - The DUT MUST respond with a DIAG_GET.rsp response containing the required diagnostic TLV payload:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload:
    #     - The Timeout TLV MUST NOT be present.
    print("Step 4: Leader sends DIAG_GET.req for Timeout and Child Table.")
    pkts.filter_ipv6_src(LEADER_RLOC).\
        filter_ipv6_dst(REED_1_RLOC).\
        filter_coap_request(consts.DIAG_GET_URI).\
        filter(lambda p: consts.DG_TYPE_LIST_TLV in p.coap.tlv.type).\
        must_next()

    pkts.filter_ipv6_src(REED_1_RLOC).\
        filter_ipv6_dst(LEADER_RLOC).\
        filter_coap_ack(consts.DIAG_GET_URI).\
        filter(lambda p: p.coap.tlv.type is nullField or \
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
    pkts.filter_ipv6_src(LEADER_RLOC).\
        filter_ipv6_dst(REED_1_RLOC).\
        filter_coap_request(consts.DIAG_GET_URI).\
        filter(lambda p: consts.DG_TYPE_LIST_TLV in p.coap.tlv.type).\
        must_next()

    pkts.filter_ipv6_src(REED_1_RLOC).\
        filter_ipv6_dst(LEADER_RLOC).\
        filter_coap_ack(consts.DIAG_GET_URI).\
        must_next()

    # Step 5a: Test Harness
    # - Description: Harness waits 20 seconds to allow DIAG_GET to complete.
    # - Pass Criteria: N/A
    print("Step 5a: Harness waits 20 seconds.")

    # Step 6: Leader
    # - Description: Harness instructs the device to send DIAG_RST.ntf to DUT’s Routing Locator (RLOC) for the
    #   following diagnostic TLV type:
    #   - TLV Type 9 - MAC Counters
    # - Pass Criteria:
    #   - The DUT MUST respond with a CoAP response:
    #   - CoAP Response Code: 2.04 Changed
    print("Step 6: Leader sends DIAG_RST.ntf.")
    pkts.filter_ipv6_src(LEADER_RLOC).\
        filter_ipv6_dst(REED_1_RLOC).\
        filter_coap_request(consts.DIAG_RST_URI).\
        filter(lambda p: consts.DG_TYPE_LIST_TLV in p.coap.tlv.type).\
        must_next()

    pkts.filter_ipv6_src(REED_1_RLOC).\
        filter_ipv6_dst(LEADER_RLOC).\
        filter_coap_ack(consts.DIAG_RST_URI).\
        must_next()

    # Step 6a: Test Harness
    # - Description: Harness waits ONLY 2 seconds before executing next step.
    # - Pass Criteria: N/A
    print("Step 6a: Harness waits 2 seconds.")

    # Step 7: Leader
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
    print("Step 7: Leader sends DIAG_GET.req for MAC Counters after reset.")
    pkts.filter_ipv6_src(LEADER_RLOC).\
        filter_ipv6_dst(REED_1_RLOC).\
        filter_coap_request(consts.DIAG_GET_URI).\
        filter(lambda p: consts.DG_TYPE_LIST_TLV in p.coap.tlv.type).\
        must_next()

    p7 = pkts.filter_ipv6_src(REED_1_RLOC).\
        filter_ipv6_dst(LEADER_RLOC).\
        filter_coap_ack(consts.DIAG_GET_URI).\
        filter(lambda p: consts.DG_MAC_COUNTERS_TLV in p.coap.tlv.type).\
        must_next()

    mac_counters_step_7 = [int(val) for val in p7.coap.tlv.mac_counter]
    for i in range(len(mac_counters_step_3)):
        assert mac_counters_step_7[i] <= mac_counters_step_3[i], \
            f"MAC counter {i} increased after reset: {mac_counters_step_3[i]} -> {mac_counters_step_7[i]}"

    # Step 8: Leader
    # - Description: Harness instructs the device to send DIAG_GET.query to Realm-Local All-Nodes multicast address
    #   (FF03::1) for the following diagnostic TLV types:
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
    #   - The DUT MUST respond with a DIAG_GET.ans response containing the requested diagnostic TLVs:
    #   - CoAP Payload:
    #     - TLV Type 0 - MAC Extended Address (64-bit)
    #     - TLV Type 1 - MAC Address (16-bit)
    #     - TLV Type 2 - Mode (Capability information)
    #     - TLV Type 4 – Connectivity
    #     - TLV Type 5 – Route64 (optional)
    #     - TLV Type 6 – Leader Data
    #     - TLV Type 7 – Network Data
    #     - TLV Type 8 – IPv6 address list
    #     - TLV Type 17 – Channel Pages
    #   - The presence of each TLV MUST be validated. Where possible, the value of the TLVs MUST be validated.
    print("Step 8: Leader sends DIAG_GET.query to FF03::1.")
    pkts.filter_ipv6_src(LEADER_RLOC).\
        filter_ipv6_dst("ff03::1").\
        filter_coap_request(consts.DIAG_GET_QRY_URI).\
        filter(lambda p: consts.DG_TYPE_LIST_TLV in p.coap.tlv.type).\
        must_next()

    pkts.filter_ipv6_src(REED_1_RLOC).\
        filter_ipv6_dst(LEADER_RLOC).\
        filter_coap_request(consts.DIAG_GET_ANS_URI).\
        filter(lambda p: EXPECTED_DIAG_GET_TLVS <= set(p.coap.tlv.type)).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
