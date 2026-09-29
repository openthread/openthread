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
    # 5.7.3 CoAP Diagnostic Query and Answer Commands - Router, FED
    #
    # 5.7.3.1 Topology
    # - Topology A
    # - Topology B
    #
    # 5.7.3.2 Purpose & Description
    # The purpose of this test case is to verify functionality of commands Diagnostic_Get.query and Diagnostic_Get.ans.
    #   Thread Diagnostic commands MUST be supported by FTDs.
    #
    # Spec Reference                               | V1.1 Section          | V1.3.0 Section
    # ---------------------------------------------|-----------------------|-----------------------
    # Get Diagnostic Query / Get Diagnostic Answer | 10.11.2.3 / 10.11.2.4 | 10.11.2.3 / 10.11.2.4

    pkts = pv.pkts
    pv.summary.show()

    LEADER_RLOC16 = pv.vars['LEADER_RLOC16']
    ROUTER_1_RLOC16 = pv.vars['ROUTER_1_RLOC16']
    FED_1_RLOC16 = pv.vars['FED_1_RLOC16']
    MED_1_RLOC16 = pv.vars['MED_1_RLOC16']
    SED_1_RLOC16 = pv.vars['SED_1_RLOC16']

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A.
    print("Step 1: All")

    # Step 2: Leader
    # - Description: Harness instructs the device to send DIAG_GET.query to the Realm-Local All-Thread-Nodes multicast
    #   address for the following diagnostic TLV types:
    #   - Topology A (Router DUT):
    #     - TLV Type 0 - MAC Extended Address (64-bit)
    #     - TLV Type 1 - MAC Address (16-bit)
    #     - TLV Type 2 - Mode (Capability information)
    #     - TLV Type 4 - Connectivity
    #     - TLV Type 5 - Route64
    #     - TLV Type 6 - Leader Data
    #     - TLV Type 7 - Network Data
    #     - TLV Type 8 - IPv6 address list
    #     - TLV Type 16 - Child Table
    #     - TLV Type 17 - Channel Pages
    #   - Topology B (FED DUT):
    #     - TLV Type 0 - MAC Extended Address (64-bit)
    #     - TLV Type 1 - MAC Address (16-bit)
    #     - TLV Type 2 - Mode (Capability information)
    #     - TLV Type 6 - Leader Data
    #     - TLV Type 7 - Network Data
    #     - TLV Type 8 - IPv6 address list
    #     - TLV Type 17 - Channel Pages
    # - Pass Criteria: N/A.
    print("Step 2: Leader")
    pkts.filter_wpan_src16(LEADER_RLOC16). \
        filter_coap_request(consts.DIAG_GET_QRY_URI). \
        filter(lambda p: {
            consts.DG_TYPE_LIST_TLV
        } <= set(p.coap.tlv.type)). \
        must_next()

    # Step 3: Topology A (Router DUT)
    # - Description: Automatically responds with a DIAG_GET.ans response.
    # - Pass Criteria:
    #   - The DIAG_GET.ans response MUST contain the requested diagnostic TLVs:
    #   - CoAP Payload:
    #     - TLV Type 0 - MAC Extended Address (64-bit)
    #     - TLV Type 1 - MAC Address (16-bit)
    #     - TLV Type 2 - Mode (Capability information)
    #     - TLV Type 4 - Connectivity
    #     - TLV Type 5 - Route64
    #     - TLV Type 6 - Leader Data
    #     - TLV Type 7 - Network Data
    #     - TLV Type 8 - IPv6 address list
    #     - TLV Type 16 - Child Table
    #     - TLV Type 17 - Channel Pages
    #   - The presence of each TLV MUST be validated. Where possible, the value of the TLVs MUST be validated.
    print("Step 3: Topology A (Router DUT)")
    with pkts.save_index():
        pkts.filter_wpan_src16(ROUTER_1_RLOC16). \
            filter_coap_request(consts.DIAG_GET_ANS_URI). \
            filter(lambda p: {
                consts.DG_MAC_EXTENDED_ADDRESS_TLV,
                consts.DG_MAC_ADDRESS_TLV,
                consts.DG_MODE_TLV,
                consts.DG_CONNECTIVITY_TLV,
                consts.DG_ROUTE64_TLV,
                consts.DG_LEADER_DATA_TLV,
                consts.DG_NETWORK_DATA_TLV,
                consts.DG_IPV6_ADDRESS_LIST_TLV,
                consts.DG_CHILD_TABLE_TLV,
                consts.DG_CHANNEL_PAGES_TLV
            } <= set(p.coap.tlv.type)). \
            filter(lambda p:
                   p.coap.tlv.mac_addr == pv.vars['ROUTER_1'] and
                   p.coap.tlv.rloc16 == ROUTER_1_RLOC16 and
                   p.coap.tlv.mode == 0x0f and
                   p.coap.tlv.leader_router_id == (LEADER_RLOC16 >> 10) and
                   set(p.coap.tlv.ipv6_address) >= {pv.vars['ROUTER_1_RLOC'], pv.vars['ROUTER_1_MLEID']} and
                   set(p.coap.tlv.child_id) >= {
                       FED_1_RLOC16 & 0x1ff,
                       MED_1_RLOC16 & 0x1ff,
                       SED_1_RLOC16 & 0x1ff
                   }
            ). \
            must_next()

    # Step 4: Topology A (Router DUT)
    # - Description: The DUT automatically multicasts the DIAG_GET.query frame.
    # - Pass Criteria:
    #   - The DUT MUST use IEEE 802.15.4 indirect transmissions to forward the DIAG_GET.query to SED_1.
    print("Step 4: Topology A (Router DUT)")
    with pkts.save_index():
        pkts.filter_wpan_src16(ROUTER_1_RLOC16). \
            filter_wpan_dst16(SED_1_RLOC16). \
            filter_coap_request(consts.DIAG_GET_QRY_URI). \
            must_next()

    # Step 5: Topology B (FED DUT)
    # - Description: The DUT automatically responds with DIAG_GET.ans.
    # - Pass Criteria:
    #   - The DIAG_GET.ans response MUST contain the requested diagnostic TLVs:
    #   - CoAP Payload:
    #     - TLV Type 0 - MAC Extended Address (64-bit)
    #     - TLV Type 1 - MAC Address (16-bit)
    #     - TLV Type 2 - Mode (Capability information)
    #     - TLV Type 6 - Leader Data
    #     - TLV Type 7 - Network Data
    #     - TLV Type 8 - IPv6 address list
    #     - TLV Type 17 - Channel Pages
    #   - The presence of each TLV MUST be validated. Where possible, the value of the TLVs MUST be validated.
    print("Step 5: Topology B (FED DUT)")
    with pkts.save_index():
        pkts.filter_wpan_src16(FED_1_RLOC16). \
            filter_coap_request(consts.DIAG_GET_ANS_URI). \
            filter(lambda p: {
                consts.DG_MAC_EXTENDED_ADDRESS_TLV,
                consts.DG_MAC_ADDRESS_TLV,
                consts.DG_MODE_TLV,
                consts.DG_LEADER_DATA_TLV,
                consts.DG_NETWORK_DATA_TLV,
                consts.DG_IPV6_ADDRESS_LIST_TLV,
                consts.DG_CHANNEL_PAGES_TLV
            } <= set(p.coap.tlv.type)). \
            filter(lambda p:
                   p.coap.tlv.mac_addr == pv.vars['FED_1'] and
                   p.coap.tlv.rloc16 == FED_1_RLOC16 and
                   p.coap.tlv.mode == 0x0f and
                   p.coap.tlv.leader_router_id == (LEADER_RLOC16 >> 10) and
                   set(p.coap.tlv.ipv6_address) >= {pv.vars['FED_1_RLOC'], pv.vars['FED_1_MLEID']}
            ). \
            must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
