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
    # 5.2.3 Leader rejects CoAP Address Solicit (2-hops from Leader)
    #
    # 5.2.3.1 Topology
    #   - Build a topology with the DUT as the Leader and have a total of 32 routers, including the Leader.
    #   - Attempt to attach a 33rd router, two hops from the leader.
    #
    # 5.2.3.2 Purpose & Description
    #   The purpose of this test case is to show that the DUT will only allow 32 active routers on the network and
    #   reject the Address Solicit Request from a 33rd router - that is 2-hops away - with a No Address Available status.
    #
    #   Spec Reference                               | V1.1 Section    | V1.3.0 Section
    #   ---------------------------------------------|-----------------|-----------------
    #   Attaching to a Parent / Router ID Assignment | 4.7.1 / 5.9.10  | 4.5.1 / 5.9.10

    pkts = pv.pkts
    pv.summary.show()

    MAX_ROUTERS = 32

    LEADER = pv.vars['Leader']
    ROUTER_31 = pv.vars['Router 31']
    ROUTER_32 = pv.vars['Router 32']

    # Step 1: All
    # - Description: Begin wireless sniffer and ensure topology is created and connectivity between nodes.
    # - Pass Criteria: Topology is created, the DUT is the Leader of the network and there is a total of 32
    #   active routers, including the Leader.
    print("Step 1: All")
    pkts.filter_wpan_src64(LEADER) \
        .filter_mle_cmd(consts.MLE_ADVERTISEMENT) \
        .filter(lambda p: consts.LEADER_DATA_TLV in p.mle.tlv.type) \
        .must_next()

    # Step 2: Router_31
    # - Description: The harness causes Router_31 to attach to the network and send an Address Solicit Request to
    #   become an active router.
    # - Pass Criteria: N/A
    print("Step 2: Router_31")
    pkts.filter_wpan_src64(ROUTER_31) \
        .filter_coap_request(consts.ADDR_SOL_URI) \
        .must_next()

    # Step 3: Leader (DUT)
    # - Description: The DUT receives the Address Solicit Request and automatically replies with an Address
    #   Solicit Response.
    # - Pass Criteria:
    #   - The DUT MUST reply to the Address Solicit Request with an Address Solicit Response containing:
    #     - CoAP Response Code: 2.04 Changed
    #     - CoAP Payload:
    #       - Status TLV (value = Success)
    #       - RLOC16 TLV
    #       - Router Mask TLV
    print("Step 3: Leader (DUT)")
    pkts.filter_wpan_src64(LEADER) \
        .filter_coap_ack(consts.ADDR_SOL_URI) \
        .filter(lambda p: {
            consts.NL_STATUS_TLV,
            consts.NL_RLOC16_TLV,
            consts.NL_ROUTER_MASK_TLV
        } <= set(p.coap.tlv.type) and \
            p.coap.code == consts.COAP_CODE_ACK and \
            p.coap.tlv.status == consts.ADDR_SOL_SUCCESS) \
        .must_next()

    # Step 4: Leader (DUT)
    # - Description: Automatically sends MLE Advertisements.
    # - Pass Criteria: The DUT’s MLE Advertisements MUST contain the Route64 TLV with 32 assigned Router IDs.
    print("Step 4: Leader (DUT)")
    pkts.filter_wpan_src64(LEADER) \
        .filter_LLANMA() \
        .filter_mle_cmd(consts.MLE_ADVERTISEMENT) \
        .filter(lambda p: consts.ROUTE64_TLV in p.mle.tlv.type and \
               len(p.mle.tlv.route64.id_mask) == MAX_ROUTERS) \
        .must_next()

    # Step 5: Router_32
    # - Description: The harness causes Router_32 to attach to any of the active routers, 2-hops from the leader,
    #   and to send an Address Solicit Request to become an active router.
    # - Pass Criteria: N/A
    print("Step 5: Router_32")
    pkts.filter_wpan_src64(ROUTER_32) \
        .filter_coap_request(consts.ADDR_SOL_URI) \
        .must_next()

    # Step 6: Leader (DUT)
    # - Description: The DUT receives the Address Solicit Request and automatically replies with an Address
    #   Solicit Response.
    # - Pass Criteria:
    #   - The DUT MUST reply to the Address Solicit Request with an Address Solicit Response containing:
    #     - CoAP Response Code: 2.04 Changed
    #     - CoAP Payload:
    #       - Status TLV (value = No Address Available)
    print("Step 6: Leader (DUT)")
    pkts.filter_wpan_src64(LEADER) \
        .filter_coap_ack(consts.ADDR_SOL_URI) \
        .filter(lambda p: {
            consts.NL_STATUS_TLV
        } <= set(p.coap.tlv.type) and \
            p.coap.code == consts.COAP_CODE_ACK and \
            p.coap.tlv.status == consts.NL_NO_ADDRESS_AVAILABLE) \
        .must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
