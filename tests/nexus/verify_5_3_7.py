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
    # 5.3.7 Duplicate Address Detection
    #
    # 5.3.7.1 Topology
    # - Leader (DUT)
    # - Router_1
    # - Router_2
    # - MED_1 (Attached to Router_1)
    # - MED_2 (Attached to Leader)
    # - SED_1 (Attached to Router_2)
    #
    # 5.3.7.2 Purpose & Description
    # The purpose of this test case is to validate the DUT’s ability to perform duplicate address detection.
    #
    # Spec Reference                   | V1.1 Section | V1.3.0 Section
    # ---------------------------------|--------------|---------------
    # Duplicate IPv6 Address Detection | 5.6          | 5.6

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_2 = pv.vars['ROUTER_2']

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A
    print("Step 1: All")

    # Step 2: Leader (DUT)
    # - Description: Transmit MLE advertisements.
    # - Pass Criteria:
    #   - The DUT MUST send properly formatted MLE Advertisements.
    #   - MLE Advertisements MUST be sent with an IP Hop Limit of 255 to the Link-Local All Nodes multicast address
    #     (FF02::1).
    #   - The following TLVs MUST be present in the MLE Advertisements:
    #     - Leader Data TLV
    #     - Route64 TLV
    #     - Source Address TLV
    print("Step 2: Leader (DUT)")
    pkts.copy().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter_wpan_src64(LEADER).\
        filter_LLANMA().\
        filter(lambda p: {
                          consts.LEADER_DATA_TLV,
                          consts.ROUTE64_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type) and\
               p.ipv6.hlim == 255).\
        must_next()

    # Step 3: Router_2
    # - Description: Harness configures the following On-Mesh Prefix on the device:
    #   - Prefix 1: P_Prefix=2001::/64 P_stable=1 P_default=1 P_slaac=1 P_on_mesh=1 P_preferred=1
    # - Pass Criteria: N/A
    print("Step 3: Router_2")

    # Step 4: MED_1, SED_1
    # - Description: Harness configures both devices with the same 2001:: address.
    # - Pass Criteria: N/A
    print("Step 4: MED_1, SED_1")

    # Step 5: MED_2
    # - Description: Harness instructs device to send an ICMPv6 Echo Request to the address configured on MED_1 and
    #   SED_1 with Prefix 2001::
    # - Pass Criteria:
    #   - The DUT MUST multicast an Address Query message to the Realm-Local All-Routers address (FF03::2):
    #     - ADDR_QRY.req (/aq) - Address Query Request
    #     - CoAP URI-Path: NON POST coap://[<FF03::2>]:MM/a/aq
    #     - CoAP Payload:
    #       - Target EID TLV
    print("Step 5: MED_2")
    pkts.copy().\
        filter_wpan_src64(LEADER).\
        filter_RLARMA().\
        filter_coap_request(consts.ADDR_QRY_URI).\
        filter(lambda p: p.coap.tlv.target_eid == '2001::1').\
        must_next()

    # Step 6: Router_1, Router_2
    # - Description: Automatically respond with Address Notification message with matching Target TLVs.
    # - Pass Criteria: N/A
    print("Step 6: Router_1, Router_2")
    pkts.copy().\
        filter_wpan_src64(ROUTER_1).\
        filter_coap_request(consts.ADDR_NTF_URI).\
        must_next()
    pkts.copy().\
        filter_wpan_src64(ROUTER_2).\
        filter_coap_request(consts.ADDR_NTF_URI).\
        must_next()

    # Step 7: Leader (DUT)
    # - Description: Automatically sends a Multicast Address Error Notification.
    # - Pass Criteria:
    #   - The DUT MUST issue an Address Error Notification message to the Realm-Local All-Routers multicast address
    #     (FF03::2):
    #     - ADDR_ERR.ntf(/ae) - Address Error Notification
    #     - CoAP URI-Path: NON POST coap://[<peer address>]:MM/a/ae
    #     - CoAP Payload:
    #       - Target EID TLV
    #       - ML-EID TLV
    #   - The IPv6 Source address MUST be the RLOC of the originator,,,
    print("Step 7: Leader (DUT)")
    pkts.copy().\
        filter_wpan_src64(LEADER).\
        filter_RLARMA().\
        filter_coap_request(consts.ADDR_ERR_URI).\
        filter(lambda p: p.coap.tlv.target_eid == '2001::1' and p.ipv6.src == pv.vars['LEADER_RLOC']).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
