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
from pktverify.addrs import Ipv6Addr

# CoAP types
COAP_TYPE_CON = 0
COAP_TYPE_NON = 1


def verify(pv):
    # 5.3.9 Address Query - DHCP GUA
    #
    # 5.3.9.1 Topology
    # - The Leader is configured as a Border Router with DHCPv6 server for prefixes 2001:: & 2002::
    #
    # 5.3.9.2 Purpose & Description
    # The purpose of this test case is to validate that the DUT is able to generate Address Query and Address
    #   Notification messages properly.
    #
    # Spec Reference                                  | V1.1 Section  | V1.3.0 Section
    # ------------------------------------------------|---------------|---------------
    # Address Query / Proactive Address Notifications | 5.4.2 / 5.4.3 | 5.4.2 / 5.4.3

    pkts = pv.pkts
    pv.summary.show()

    ROUTER_1 = pv.vars['ROUTER_1']
    DUT = pv.vars['DUT']
    ROUTER_3 = pv.vars['ROUTER_3']
    SED_1 = pv.vars['SED_1']

    def get_gua(ipaddrs, prefix):
        for addr in ipaddrs:
            if str(addr).lower().startswith(prefix.lower()):
                return Ipv6Addr(addr)
        return None

    ROUTER_3_GUA = get_gua(pv.vars['ROUTER_3_IPADDRS'], '2001:')
    SED_1_GUA = get_gua(pv.vars['SED_1_IPADDRS'], '2001:')

    # Step 1: Leader
    # - Description: Harness configures the device to be a DHCPv6 Border Router for prefixes 2001:: & 2002::
    # - Pass Criteria: N/A
    print("Step 1: Leader")

    # Step 2: All
    # - Description: Build the topology as described and begin the wireless sniffer.
    # - Pass Criteria: N/A
    print("Step 2: All")

    # Step 3: SED_1
    # - Description: Harness instructs device to send an ICMPv6 Echo Request to Router_3 using GUA 2001:: address.
    # - Pass Criteria:
    #   - The DUT MUST generate an Address Query Request on SED_1’s behalf to find Router_3 address.
    #   - The Address Query Request MUST be sent to the Realm-Local All-Routers multicast address (FF03::2).
    #   - CoAP URI-Path: NON POST coap://<FF03::2>
    #   - CoAP Payload:
    #     - Target EID TLV
    #   - The DUT MUST receive and process the incoming Address Query Response and forward the ICMPv6 Echo Request
    #     packet to SED_1.
    print("Step 3: SED_1")
    # 1. Address Query Request from DUT
    pkts.filter_coap_request(consts.ADDR_QRY_URI).\
        filter(lambda p: p.coap.tlv.target_eid == ROUTER_3_GUA).\
        must_next()

    # 2. Address Notification from Router_3 to DUT
    pkts.filter_coap_request(consts.ADDR_NTF_URI).\
        filter(lambda p: p.coap.tlv.target_eid == ROUTER_3_GUA).\
        must_next()

    # 3. Echo Reply forwarded by DUT to SED_1
    pkts.filter_ping_reply().\
        must_next()

    # Step 4: Router_1
    # - Description: Harness instructs device to send an ICMPv6 Echo Request to SED_1 using GUA 2001:: address.
    # - Pass Criteria:
    #   - The DUT MUST respond to the Address Query Request with a properly formatted Address Notification Message:
    #   - CoAP URI-PATH: CON POST coap://[<Address Query Source>]:MM/a/an
    #   - CoAP Payload:
    #     - Target EID TLV
    #     - RLOC16 TLV
    #     - ML-EID TLV
    #   - The IPv6 Source address MUST be the RLOC of the originator.
    #   - The IPv6 Destination address MUST be the RLOC of the destination.
    print("Step 4: Router_1")
    # 1. Address Query Request from Router_1
    pkts.filter_coap_request(consts.ADDR_QRY_URI).\
        filter(lambda p: p.coap.tlv.target_eid == SED_1_GUA).\
        must_next()

    # 2. Address Notification from DUT to Router_1
    pkts.filter_coap_request(consts.ADDR_NTF_URI).\
        filter(lambda p: p.coap.tlv.target_eid == SED_1_GUA).\
        must_next()

    # Step 5: SED_1
    # - Description: Harness instructs device to send an ICMPv6 Echo Request to Router_3 using GUA 2001:: address.
    # - Pass Criteria:
    #   - The DUT MUST NOT send an Address Query as Router_3 address should be cached.
    #   - The DUT MUST forward the ICMPv6 Echo Reply to SED_1.
    print("Step 5: SED_1")
    start_of_step_5 = pkts.index
    pkts.filter_ping_reply().\
        must_next()
    end_of_step_5 = pkts.index

    pkts.range(start_of_step_5, end_of_step_5).\
        filter_coap_request(consts.ADDR_QRY_URI).\
        filter(lambda p: p.coap.tlv.target_eid == ROUTER_3_GUA).\
        must_not_next()

    # Step 6: Router_2 (DUT)
    # - Description: Harness silently powers off Router_3 and waits 580 seconds to allow Leader to expire its Router
    #   ID. Send an ICMPv6 Echo Request from MED_1 to Router_3 GUA 2001:: address.
    # - Pass Criteria:
    #   - The DUT MUST update its address cache and remove all entries based on Router_3’s Router ID.
    #   - The DUT MUST send an Address Query to discover Router_3’s RLOC address.
    print("Step 6: Router_2 (DUT)")
    pkts.filter_coap_request(consts.ADDR_QRY_URI).\
        filter(lambda p: p.coap.tlv.target_eid == ROUTER_3_GUA).\
        must_next()

    # Step 7: SED_1
    # - Description: Harness silently powers off SED_1 and waits to allow the DUT to timeout the child. Send two
    #   ICMPv6 Echo Requests from Router_1 to SED_1 GUA 2001:: address (one to clear the EID-to-RLOC Map Cache of the
    #   sender and the other to produce Address Query).
    # - Pass Criteria:
    #   - The DUT MUST NOT respond with an Address Notification message.
    print("Step 7: SED_1")
    # 1. Address Query Request from Router_1
    pkts.filter_coap_request(consts.ADDR_QRY_URI).\
        filter(lambda p: p.coap.tlv.target_eid == SED_1_GUA).\
        must_next()

    # 2. DUT must NOT respond with Address Notification
    with pkts.save_index():
        pkts.filter_coap_request(consts.ADDR_NTF_URI).\
            filter(lambda p: p.coap.tlv.target_eid == SED_1_GUA).\
            must_not_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
