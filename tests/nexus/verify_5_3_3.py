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
    # 5.3.3 Address Query - ML-EID
    #
    # 5.3.3.1 Topology
    # - Leader
    # - Router_1
    # - Router_2 (DUT)
    # - Router_3
    # - MED_1 (Attached to DUT)
    #
    # 5.3.3.2 Purpose & Description
    # The purpose of this test case is to validate that the DUT is able to generate Address Query messages and properly
    #   respond with Address Notification messages.
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # Address Query    | 5.4.2        | 5.4.2

    pkts = pv.pkts
    pv.summary.show()

    ROUTER_1 = pv.vars['ROUTER_1']
    DUT = pv.vars['DUT']  # Router_2
    ROUTER_3 = pv.vars['ROUTER_3']
    MED_1 = pv.vars['MED_1']

    # Step 1: All
    # - Description: Build the topology as described and begin the wireless sniffer.
    # - Pass Criteria: N/A
    print("Step 1: All")

    # Step 2: MED_1
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request to the Router_3 ML-EID address.
    # - Pass Criteria:
    #   - The DUT MUST generate an Address Query Request on MED_1’s behalf to find Router_3 address.
    #   - The Address Query Request MUST be sent to the Realm-Local All-Routers address (FF03::2).
    #   - CoAP URI-Path: NON POST coap://<FF03::2>
    #   - CoAP Payload:
    #     - Target EID TLV
    #   - The DUT MUST receive and process the incoming Address Notification.
    #   - The DUT MUST then forward the ICMPv6 Echo Request from MED_1 and forward the ICMPv6 Echo Reply to MED_1.
    print("Step 2: MED_1 sends ICMPv6 Echo Request to Router_3 ML-EID address")

    # 1. Address Query Request from DUT
    pkts.filter_wpan_src64(DUT).\
        filter_ipv6_dst(consts.REALM_LOCAL_ALL_ROUTERS_ADDRESS).\
        filter_coap_request(consts.ADDR_QRY_URI).\
        filter(lambda p: p.coap.type == COAP_TYPE_NON).\
        filter(lambda p: p.coap.tlv.target_eid == pv.vars['ROUTER_3_MLEID']).\
        must_next()

    # 2. Address Notification to DUT
    pkts.filter_coap_request(consts.ADDR_NTF_URI).\
        filter_ipv6_dst(pv.vars['DUT_RLOC']).\
        must_next()

    # 3. DUT forwards Echo Request to Router_3
    pkts.filter_wpan_src64(DUT).\
        filter(lambda p: p.icmpv6.type == consts.ICMPV6_TYPE_ECHO_REQUEST).\
        filter_ipv6_dst(pv.vars['ROUTER_3_MLEID']).\
        must_next()

    # 4. DUT forwards Echo Reply to MED_1
    pkts.filter_wpan_src64(DUT).\
        filter(lambda p: p.icmpv6.type == consts.ICMPV6_TYPE_ECHO_REPLY).\
        filter_ipv6_dst(pv.vars['MED_1_MLEID']).\
        must_next()

    # Step 3: Router_1
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request to the MED_1 ML-EID address.
    # - Pass Criteria:
    #   - The DUT MUST respond to the Address Query Request with a properly formatted Address Notification Message:
    #   - CoAP URI-PATH: CON POST coap://[<Address Query Source>]:MM/a/an
    #   - CoAP Payload:
    #     - ML-EID TLV
    #     - RLOC16 TLV
    #     - Target EID TLV
    #   - The IPv6 Source address MUST be the RLOC of the originator.
    #   - The IPv6 Destination address MUST be the RLOC of the destination.
    print("Step 3: Router_1 sends ICMPv6 Echo Request to MED_1 ML-EID address")

    # Router_1 sends Address Query for MED_1
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_coap_request(consts.ADDR_QRY_URI).\
        filter(lambda p: p.coap.tlv.target_eid == pv.vars['MED_1_MLEID']).\
        must_next()

    # DUT responds with Address Notification
    pkts.filter_coap_request(consts.ADDR_NTF_URI).\
        filter(lambda p: p.coap.type == COAP_TYPE_CON).\
        filter(lambda p: p.ipv6.src == pv.vars['DUT_RLOC']).\
        filter_ipv6_dst(pv.vars['ROUTER_1_RLOC']).\
        filter(lambda p: {
            consts.NL_ML_EID_TLV,
            consts.NL_RLOC16_TLV,
            consts.NL_TARGET_EID_TLV
        } <= set(p.coap.tlv.type)).\
        must_next()

    # Step 4: MED_1
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request from MED_1 to the Router_3 ML-EID
    #   address.
    # - Pass Criteria:
    #   - The DUT MUST NOT send an Address Query, as the Router_3 address should be cached.
    #   - The DUT MUST forward the ICMPv6 Echo Reply to MED_1.
    print("Step 4: MED_1 sends ICMPv6 Echo Request to Router_3 ML-EID address")

    # We want to check no Address Query happened *before* the Echo Reply of this step.
    start_of_step_4 = pkts.index
    # First, find the Echo Reply.
    pkts.filter_wpan_src64(DUT).\
        filter(lambda p: p.icmpv6.type == consts.ICMPV6_TYPE_ECHO_REPLY).\
        filter_ipv6_dst(pv.vars['MED_1_MLEID']).\
        must_next()
    end_of_step_4 = pkts.index

    # Now check in the range up to this reply.
    pkts.range(start_of_step_4, end_of_step_4).filter_wpan_src64(DUT).\
        filter(lambda p: p.ipv6.src == pv.vars['DUT_RLOC']).\
        filter_coap_request(consts.ADDR_QRY_URI).\
        filter(lambda p: p.coap.tlv.target_eid == pv.vars['ROUTER_3_MLEID']).\
        must_not_next()

    # Step 5: Router_2 (DUT)
    # - Description: Power off Router_3 and wait for the Leader to expire its Router ID (Timeout = MAX_NEIGHBOR_AGE +
    #   INFINITE_COST_TIMEOUT + ID_REUSE_DELAY + propagation time = 320 s). Harness instructs the device to send an
    #   ICMPv6 Echo Request from MED_1 to the Router_3 ML-EID address.
    # - Pass Criteria:
    #   - The DUT MUST update its address cache and remove all entries based on Router_3’s Router ID.
    #   - The DUT MUST be sent an Address Query to discover Router_3’s RLOC address.
    print("Step 5: Power off Router_3 and MED_1 sends ICMPv6 Echo Request to Router_3 ML-EID address")

    pkts.filter_wpan_src64(DUT).\
        filter_coap_request(consts.ADDR_QRY_URI).\
        filter(lambda p: p.coap.tlv.target_eid == pv.vars['ROUTER_3_MLEID']).\
        must_next()

    # Step 6: MED_1
    # - Description: Power off MED_1 and wait for the DUT to timeout the child. Harness instructs the device to send
    #   two ICMPv6 Echo Requests from Router_1 to MED_1 GUA 2001:: address (one to clear the EID-to-RLOC Map Cache of
    #   the sender and the other to produce Address Query).
    # - Pass Criteria:
    #   - The DUT MUST NOT respond with an Address Notification message.
    print("Step 6: Power off MED_1 and Router_1 sends ICMPv6 Echo Request to MED_1 GUA address")

    # Note: Because 2001:: is not configured as an on-mesh prefix, Step 6 as specified will not work.
    # We ping the ML-EID instead.

    # Router_1 sends Address Query for MED_1 ML-EID
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_ipv6_dst(consts.REALM_LOCAL_ALL_ROUTERS_ADDRESS).\
        filter_coap_request(consts.ADDR_QRY_URI).\
        filter(lambda p: p.coap.type == COAP_TYPE_NON).\
        filter(lambda p: p.coap.tlv.target_eid == pv.vars['MED_1_MLEID']).\
        must_next()

    # DUT must NOT respond with Address Notification for MED_1 ML-EID
    with pkts.save_index():
        pkts.filter(lambda p: p.ipv6.src == pv.vars['DUT_RLOC']).\
            filter_coap_request(consts.ADDR_NTF_URI).\
            filter(lambda p: p.coap.tlv.target_eid == pv.vars['MED_1_MLEID']).\
            must_not_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
