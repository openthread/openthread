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
    # 5.2.5 Address Query - REED
    #
    # 5.2.5.1 Topology
    # - Build a topology that has a total of 16 active routers, including the Leader, with no communication
    #   constraints.
    # - The Leader is configured as a DHCPv6 server for prefix 2001::
    # - The Border Router is configured as a SLAAC server for prefix 2002::
    # - Each router numbered 1 through 14 have a link to leader
    # - MED_1 has a link to the leader
    # - Border Router has a link to the leader
    # - REED_1 has a link to router 1
    #
    # 5.2.5.2 Purpose & Description
    # The purpose of this test case is to validate that the DUT is able to generate Address Notification messages
    # in response to Address Query messages.
    #
    # Spec Reference | V1.1 Section | V1.3.0 Section
    # ---------------|--------------|---------------
    # Address Query  | 5.4.2        | 5.4.2

    pkts = pv.pkts
    pv.summary.show()

    REED_1 = pv.vars['REED_1']
    REED_1_RLOC = pv.vars['REED_1_RLOC']
    LEADER_RLOC = pv.vars['LEADER_RLOC']

    def verify_address_notification():
        pkts.filter_ipv6_src_dst(REED_1_RLOC, LEADER_RLOC).\
            filter_coap_request(consts.ADDR_NTF_URI).\
            filter(lambda p: {
                              consts.NL_TARGET_EID_TLV,
                              consts.NL_RLOC16_TLV,
                              consts.NL_ML_EID_TLV
                             } <= set(p.coap.tlv.type)).\
            must_next()
        pkts.filter_ping_reply().\
            must_next()

    # Step 1: Leader
    # - Description: Configure the Leader to be a DHCPv6 Border Router for prefix 2001::
    # - Pass Criteria: N/A
    print("Step 1: Configure the Leader to be a DHCPv6 Border Router for prefix 2001::")

    # Step 2: Border_Router
    # - Description: Attach the Border_Router to the network and configure the below On-Mesh Prefix:
    #   - Prefix 1: P_Prefix=2002::/64 P_stable=1 P_default=1 P_slaac=1 P_on_mesh=1 P_preferred=1
    # - Pass Criteria: N/A
    print("Step 2: Attach the Border_Router to the network and configure the below On-Mesh Prefix")

    # Step 3: All
    # - Description: Ensure topology is formed correctly without the DUT.
    # - Pass Criteria: N/A
    print("Step 3: Ensure topology is formed correctly without the DUT.")

    # Step 4: REED_1 (DUT)
    # - Description: Cause the DUT to attach to Router_1 (2-hops from the leader).
    # - Pass Criteria:
    #   - The DUT MUST NOT attempt to become an active router by sending an Address Solicit Request.
    #   - If the DUT sends Address Solicit Request, the test fails.
    print("Step 4: Cause the DUT to attach to Router_1 (2-hops from the leader).")
    pkts.filter_wpan_src64(REED_1).\
        filter_coap_request(consts.ADDR_SOL_URI).\
        must_not_next()

    # Step 5: REED_1 (DUT), Border Router
    # - Description: Harness enables a link between the DUT and BR to create a one-way link.
    # - Pass Criteria: N/A
    print("Step 5: Harness enables a link between the DUT and BR to create a one-way link.")

    # Step 6: MED_1
    # - Description: Harness instructs device to send an ICMPv6 Echo Request to the DUT (REED_1) using ML-EID.
    # - Pass Criteria:
    #   - The DUT MUST send a properly formatted Address Notification message:
    #     - CoAP Request URI-PATH: CON POST coap://[<Address Query Source>]:MM/a/an
    #     - CoAP Payload:
    #       - Target EID TLV
    #       - RLOC16 TLV
    #       - ML-EID TLV
    #   - The IPv6 Source address MUST be the RLOC of the originator (DUT).
    #   - The IPv6 Destination address MUST be the RLOC of the destination.
    #   - The DUT MUST send an ICMPv6 Echo Reply.
    print("Step 6: MED_1 sends ICMPv6 Echo Request to the DUT (REED_1) using ML-EID.")
    verify_address_notification()

    # Step 7: MED_1
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request to REED_1 (DUT) using 2001:: EID.
    # - Pass Criteria:
    #   - The DUT MUST send a properly formatted Address Notification message:
    #     - CoAP Request URI-PATH: CON POST coap://[<Address Query Source>]:MM/a/an
    #     - CoAP Payload:
    #       - Target EID TLV
    #       - RLOC16 TLV
    #       - ML-EID TLV
    #   - The IPv6 Source address MUST be the RLOC of the originator.
    #   - The IPv6 Destination address MUST be the RLOC of the destination.
    #   - The DUT MUST send an ICMPv6 Echo Reply.
    print("Step 7: MED_1 sends ICMPv6 Echo Request to REED_1 (DUT) using 2001:: EID.")
    verify_address_notification()

    # Step 8: MED_1
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request to REED_1 (DUT) using 2002:: EID.
    # - Pass Criteria:
    #   - The DUT MUST send a properly formatted Address Notification message:
    #     - CoAP Request URI-PATH: CON POST coap://[<Address Query Source>]:MM/a/an
    #     - CoAP Payload:
    #       - Target EID TLV
    #       - RLOC16 TLV
    #       - ML-EID TLV
    #   - The IPv6 Source address MUST be the RLOC of the originator.
    #   - The IPv6 Destination address MUST be the RLOC of the destination.
    #   - The DUT MUST send an ICMPv6 Echo Reply.
    print("Step 8: MED_1 sends ICMPv6 Echo Request to REED_1 (DUT) using 2002:: EID.")
    verify_address_notification()


if __name__ == '__main__':
    verify_utils.run_main(verify)
