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

# Bit offset of router ID in RLOC16
ROUTER_ID_OFFSET = 10


def verify(pv):
    # 5.1.5 Router Address Timeout
    #
    # 5.1.5.1 Topology
    # - Leader (DUT)
    # - Router_1
    #
    # 5.1.5.2 Purpose & Description
    # The purpose of this test case is to verify that after deallocating a Router ID, the Leader (DUT) does not reassign
    # the Router ID for at least ID_REUSE_DELAY seconds.
    #
    # Spec Reference                              | V1.1 Section   | V1.3.0 Section
    # --------------------------------------------|----------------|---------------
    # Router ID Management / Router ID Assignment | 5.9.9 / 5.9.10 | 5.9.9 / 5.9.10

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    LEADER_RLOC16 = pv.vars['LEADER_RLOC16']
    ROUTER_1 = pv.vars['ROUTER_1']

    # Step 1: All
    # - Description: Verify topology is formed correctly
    # - Pass Criteria: N/A
    print("Step 1: Verify topology is formed correctly")

    # First attach of Router_1
    _pkt = pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst16(LEADER_RLOC16).\
        filter_coap_request(consts.ADDR_SOL_URI).\
        must_next()

    _pkt_res = pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst16(_pkt.wpan.src16).\
        filter_coap_ack(consts.ADDR_SOL_URI).\
        must_next()

    first_router_id = _pkt_res.coap.tlv.rloc16 >> ROUTER_ID_OFFSET
    print(f"Router_1 first Router ID: {first_router_id}")

    # Step 2: Router_1
    # - Description: Harness silently powers-off Router_1 for 160 seconds.
    #   - (160 = MAX_NEIGHBOR_AGE + INFINITE_COST_TIMEOUT + extra time)
    #   - Extra time is added so Router_1 is brought back within ID_REUSE_DELAY interval
    # - Pass Criteria: N/A
    print("Step 2: Router_1 powered off for 160 seconds")

    # Step 3: Router_1
    # - Description: Harness silently powers-on Router_1 after 160 seconds.
    #   - Router_1 automatically sends a link request, re-attaches and requests its original Router ID.
    # - Pass Criteria: N/A
    print("Step 3: Router_1 powered on after 160 seconds")

    # Step 4: Leader (DUT)
    # - Description: Automatically attaches Router_1 (Parent Response, Child ID Response, Address Solicit Response)
    # - Pass Criteria:
    #   - The RLOC16 TLV in the Address Solicit Response message MUST contain a different Router ID than the one
    #     allocated in the original attach because ID_REUSE_DELAY interval has not timed out.
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload:
    #     - Status TLV (value = Success)
    #     - RLOC16 TLV
    #     - Router Mask TLV
    print("Step 4: Leader (DUT) automatically attaches Router_1 and reassigns a different Router ID")

    _pkt = pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst16(LEADER_RLOC16).\
        filter_coap_request(consts.ADDR_SOL_URI).\
        filter(lambda p: p.coap.tlv.rloc16 >> ROUTER_ID_OFFSET == first_router_id).\
        must_next()

    _pkt_res = pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst16(_pkt.wpan.src16).\
        filter_coap_ack(consts.ADDR_SOL_URI).\
        filter(lambda p: {
            consts.NL_STATUS_TLV,
            consts.NL_RLOC16_TLV,
            consts.NL_ROUTER_MASK_TLV
        } <= set(p.coap.tlv.type) and
            p.coap.tlv.status == 0).\
        must_next()

    second_router_id = _pkt_res.coap.tlv.rloc16 >> ROUTER_ID_OFFSET
    print(f"Router_1 second Router ID: {second_router_id}")
    assert second_router_id != first_router_id, "Router ID was reused too early"

    # Step 5: Router_1
    # - Description: Harness silently powers-off Router_1 for 300 seconds.
    #   - (300 = MAX_NEIGHBOR_AGE + INFINITE_COST_TIMEOUT + ID_REUSE_DELAY + extra time)
    #   - Extra time is added to bring Router_1 back after ID_REUSE_DELAY interval
    # - Pass Criteria: N/A
    print("Step 5: Router_1 powered off for 300 seconds")

    # Step 6: Router_1
    # - Description: Harness silently powers-on Router_1 after 300 seconds.
    #   - Router_1 reattaches and requests its most recent Router ID.
    # - Pass Criteria: N/A
    print("Step 6: Router_1 powered on after 300 seconds")

    # Step 7: Leader (DUT)
    # - Description: Automatically attaches Router_1 (Parent Response, Child ID Response, Address Solicit Response)
    # - Pass Criteria:
    #   - The RLOC16 TLV in the Address Solicit Response message MUST contain the requested Router ID
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload:
    #     - Status TLV (value = Success)
    #     - RLOC16 TLV
    #     - Router Mask TLV
    print("Step 7: Leader (DUT) automatically attaches Router_1 and reassigns the requested Router ID")

    _pkt = pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst16(LEADER_RLOC16).\
        filter_coap_request(consts.ADDR_SOL_URI).\
        filter(lambda p: p.coap.tlv.rloc16 >> ROUTER_ID_OFFSET == second_router_id).\
        must_next()

    _pkt_res = pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst16(_pkt.wpan.src16).\
        filter_coap_ack(consts.ADDR_SOL_URI).\
        filter(lambda p: {
            consts.NL_STATUS_TLV,
            consts.NL_RLOC16_TLV,
            consts.NL_ROUTER_MASK_TLV
        } <= set(p.coap.tlv.type) and
            p.coap.tlv.status == 0).\
        must_next()

    third_router_id = _pkt_res.coap.tlv.rloc16 >> ROUTER_ID_OFFSET
    print(f"Router_1 third Router ID: {third_router_id}")
    assert third_router_id == second_router_id, "Router ID was not reused after ID_REUSE_DELAY"


if __name__ == '__main__':
    verify_utils.run_main(verify)
