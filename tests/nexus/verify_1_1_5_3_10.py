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

# CoAP types
COAP_TYPE_CON = 0
COAP_TYPE_NON = 1


def verify(pv):
    # 5.3.10 Address Query - SLAAC GUA
    #
    # 5.3.10.1 Topology
    # - Leader
    # - Border Router
    # - Router 1
    # - Router 2 (DUT)
    # - MED 1
    #
    # 5.3.10.2 Purpose & Description
    # The purpose of this test case is to validate that the DUT is able to generate Address Query and Address
    #   Notification messages.
    #
    # Spec Reference                                  | V1.1 Section  | V1.3.0 Section
    # ------------------------------------------------|---------------|---------------
    # Address Query / Proactive Address Notifications | 5.4.2 / 5.4.3 | 5.4.2 / 5.4.3

    pkts = pv.pkts
    pv.summary.show()

    BR_RLOC = pv.vars['BR_RLOC']
    BR_RLOC16 = pv.vars['BR_RLOC16']
    ROUTER_1_RLOC16 = pv.vars['ROUTER_1_RLOC16']
    DUT = pv.vars['DUT']
    DUT_RLOC = pv.vars['DUT_RLOC']
    DUT_RLOC16 = pv.vars['DUT_RLOC16']
    MED_1 = pv.vars['MED_1']

    # Find GUA addresses with prefix 2003::
    ROUTER_1_GUA = [addr for addr in pv.vars['ROUTER_1_IPADDRS'] if str(addr).lower().startswith('2003:')][0]
    MED_1_GUA = [addr for addr in pv.vars['MED_1_IPADDRS'] if str(addr).lower().startswith('2003:')][0]

    def is_same_iid(addr1, addr2):
        # Compare only the last 64 bits (IID)
        return addr1[8:] == addr2[8:]

    # Step 1: Border Router
    # - Description: Harness configures the device with the two On-Mesh Prefixes below:
    #   - Prefix 1: P_Prefix=2003::/64 P_stable=1 P_default=1 P_slaac=1 P_on_mesh=1 P_preferred=1
    #   - Prefix 2: P_Prefix=2004::/64 P_stable=1 P_default=1 P_slaac=1 P_on_mesh=1 P_preferred=1
    # - Pass Criteria: N/A
    print("Step 1: Border Router")

    # Step 2: All
    # - Description: Build the topology as described and begin the wireless sniffer.
    # - Pass Criteria: N/A
    print("Step 2: All")

    # Step 3: MED_1
    # - Description: Harness instructs device to send an ICMPv6 Echo Request to Router_1 GUA 2003:: address.
    # - Pass Criteria:
    #   - The DUT MUST generate an Address Query Request on MED_1’s behalf to find Router_1 address.
    #   - The Address Query Request MUST be sent to the Realm-Local All-Routers multicast address (FF03::2).
    #   - CoAP URI-Path: NON POST coap://<FF03::2>
    #   - CoAP Payload:
    #     - Target EID TLV
    #   - The DUT MUST receive and process the incoming Address Query Response, and forward the ICMPv6 Echo Request
    #     packet to Router_1.
    print("Step 3: MED_1")

    # 1. Address Query Request from DUT to FF03::2
    pkts.filter_wpan_src16(DUT_RLOC16).\
        filter_ipv6_dst(consts.REALM_LOCAL_ALL_ROUTERS_ADDRESS).\
        filter_coap_request(consts.ADDR_QRY_URI).\
        filter(lambda p: p.coap.type == COAP_TYPE_NON).\
        filter(lambda p: is_same_iid(p.coap.tlv.target_eid, ROUTER_1_GUA)).\
        must_next()

    # 2. Address Notification to DUT
    pkts.filter_coap_request(consts.ADDR_NTF_URI).\
        filter_ipv6_dst(DUT_RLOC).\
        filter(lambda p: is_same_iid(p.coap.tlv.target_eid, ROUTER_1_GUA)).\
        must_next()

    # 3. DUT forwards Echo Request to Router_1
    pkts.filter_wpan_src16(DUT_RLOC16).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        filter_ping_request().\
        filter(lambda p: is_same_iid(p.ipv6.dst, ROUTER_1_GUA)).\
        must_next()

    # Step 4: Border Router
    # - Description: Harness instructs device to send an ICMPv6 Echo Request to MED_1 GUA 2003:: address.
    # - Pass Criteria:
    #   - The DUT MUST respond to the Address Query Request with a properly formatted Address Notification Message:
    #   - CoAP URI-PATH: CON POST coap://[<Address Query Source>]:MM/a/an
    #   - CoAP Payload:
    #     - Target EID TLV
    #     - RLOC16 TLV
    #     - ML-EID TLV
    #   - The IPv6 Source address MUST be the RLOC of the originator.
    #   - The IPv6 Destination address MUST be the RLOC of the destination.
    print("Step 4: Border Router")

    # Border Router sends Address Query for MED_1 GUA
    pkts.filter_wpan_src16(BR_RLOC16).\
        filter_coap_request(consts.ADDR_QRY_URI).\
        filter(lambda p: is_same_iid(p.coap.tlv.target_eid, MED_1_GUA)).\
        must_next()

    # DUT responds with Address Notification
    pkts.filter_wpan_src16(DUT_RLOC16).\
        filter_coap_request(consts.ADDR_NTF_URI).\
        filter(lambda p: p.coap.type == COAP_TYPE_CON).\
        filter_ipv6_src(DUT_RLOC).\
        filter_ipv6_dst(BR_RLOC).\
        filter(lambda p: {
                          consts.NL_TARGET_EID_TLV,
                          consts.NL_RLOC16_TLV,
                          consts.NL_ML_EID_TLV
                          } <= set(p.coap.tlv.type)).\
        filter(lambda p: is_same_iid(p.coap.tlv.target_eid, MED_1_GUA)).\
        must_next()

    # Step 5: MED_1
    # - Description: Harness instructs device to send an ICMPv6 Echo Request to Router_1 GUA 2003:: address.
    # - Pass Criteria:
    #   - The DUT MUST NOT send an Address Query, as the Router_1 address should be cached.
    #   - The DUT MUST forward the ICMPv6 Echo Reply to MED_1.
    print("Step 5: MED_1")

    # We want to check no Address Query happened *before* the Echo Reply of this step.
    start_of_step_5 = pkts.index
    # First, find the Echo Reply being forwarded to MED_1.
    pkts.filter_wpan_src16(DUT_RLOC16).\
        filter_ping_reply().\
        filter(lambda p: is_same_iid(p.ipv6.dst, MED_1_GUA)).\
        must_next()
    end_of_step_5 = pkts.index

    # Now check in the range up to this reply for Address Query.
    pkts.range(start_of_step_5, end_of_step_5).\
        filter_wpan_src16(DUT_RLOC16).\
        filter_coap_request(consts.ADDR_QRY_URI).\
        filter(lambda p: is_same_iid(p.coap.tlv.target_eid, ROUTER_1_GUA)).\
        must_not_next()

    # Step 6: Router_2 (DUT)
    # - Description: Harness silently powers off Router_1 and waits 580 seconds to allow the Leader to expire its
    #   Router ID. Send an ICMPv6 Echo Request from MED_1 to Router_1 GUA 2003:: address.
    # - Pass Criteria:
    #   - The DUT MUST update its address cache and removes all entries based on Router_1’s Router ID.
    #   - The DUT MUST send an Address Query Request to discover Router_1’s RLOC address.
    print("Step 6: Router_2 (DUT)")

    pkts.filter_wpan_src16(DUT_RLOC16).\
        filter_coap_request(consts.ADDR_QRY_URI).\
        filter(lambda p: is_same_iid(p.coap.tlv.target_eid, ROUTER_1_GUA)).\
        must_next()

    # Step 7: MED_1
    # - Description: Harness silently powers off MED_1 and waits to allow the DUT to timeout the child. Send two
    #   ICMPv6 Echo Requests from Border Router to MED_1 GUA 2003:: address (one to clear the EID-to-RLOC Map Cache of
    #   the sender and the other to produce Address Query).
    # - Pass Criteria:
    #   - The DUT MUST NOT respond with an Address Notification message.
    print("Step 7: MED_1")

    # Border Router sends Address Query for MED_1 GUA
    pkts.filter_wpan_src16(BR_RLOC16).\
        filter_coap_request(consts.ADDR_QRY_URI).\
        filter(lambda p: is_same_iid(p.coap.tlv.target_eid, MED_1_GUA)).\
        must_next()

    # DUT must NOT respond with Address Notification
    with pkts.save_index():
        pkts.filter_ipv6_src(DUT_RLOC).\
            filter_coap_request(consts.ADDR_NTF_URI).\
            filter(lambda p: is_same_iid(p.coap.tlv.target_eid, MED_1_GUA)).\
            must_not_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
