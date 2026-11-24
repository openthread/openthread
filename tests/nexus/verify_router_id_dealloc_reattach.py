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

# Number of Parent Requests in the first attach cycle (Thread 1.3 and later), and how many of them are sent to
# routers only in a regular attach.
FIRST_CYCLE_PARENT_REQUESTS = 6
FIRST_CYCLE_PARENT_REQUESTS_TO_ROUTERS = 2

# Upper bound for the delay between the Child ID Response and the Address Solicit when the DUT requests its
# previous Router ID immediately (much shorter than the large router selection jitter used by the DUT).
MAX_IMMEDIATE_UPGRADE_DELAY = 1.0


def is_r_and_e(p):
    return p.mle.tlv.scan_mask.r == 1 and p.mle.tlv.scan_mask.e == 1


def is_r_only(p):
    return p.mle.tlv.scan_mask.r == 1 and p.mle.tlv.scan_mask.e == 0


def next_parent_request(pkts, src):
    return pkts.filter_wpan_src64(src).\
        filter_LLARMA().\
        filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        filter(lambda p: {
                          consts.CHALLENGE_TLV,
                          consts.MODE_TLV,
                          consts.SCAN_MASK_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type) and\
               p.ipv6.hlim == 255).\
        must_next()


def collect_parent_requests(pkts, start, stop, src):
    f = pkts.range(start, stop, cascade=False).\
        filter_wpan_src64(src).\
        filter_LLARMA().\
        filter_mle_cmd(consts.MLE_PARENT_REQUEST)
    result = []
    while True:
        p = f.next()
        if p is None:
            break
        result.append(p)
    return result


def verify_reconnect_attempt(pkts, dut, parent):
    # The reconnect attempt sends only Parent Requests with R and E set until the DUT sends a Child ID Request to
    # `parent`. Returns the Child ID Response.
    first = next_parent_request(pkts, dut)
    assert is_r_and_e(first), "First Parent Request after Router ID release must have R and E set"
    start = pkts.index

    pkts.filter_wpan_src64(dut).\
        filter_wpan_dst64(parent).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter(lambda p: consts.ADDRESS16_TLV in p.mle.tlv.type).\
        must_next()

    for p in collect_parent_requests(pkts, start, pkts.index, dut):
        assert is_r_and_e(p), "All Parent Requests during the reconnect attempt must have R and E set"

    return pkts.filter_wpan_src64(parent).\
        filter_wpan_dst64(dut).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        must_next()


def next_addr_sol_success(pkts, solicit):
    # Match by CoAP token: once the DUT is a child, its parent addresses it by short address.
    return pkts.filter_coap_ack(consts.ADDR_SOL_URI).\
        filter(lambda p: p.coap.token == solicit.coap.token and\
               p.coap.tlv.status == consts.ADDR_SOL_SUCCESS).\
        must_next()


def verify_router_id_request(pkts, dut, prev_rloc16, child_id_rsp):
    # The DUT immediately requests its previous Router ID. Returns the newly assigned RLOC16.
    solicit = pkts.filter_wpan_src64(dut).\
        filter_coap_request(consts.ADDR_SOL_URI).\
        filter(lambda p: {
                          consts.NL_MAC_EXTENDED_ADDRESS_TLV,
                          consts.NL_RLOC16_TLV,
                          consts.NL_STATUS_TLV
                          } <= set(p.coap.tlv.type) and\
               p.coap.tlv.rloc16 == prev_rloc16 and\
               p.coap.tlv.status == consts.NL_TOO_FEW_ROUTERS
               ).\
        must_next()
    assert solicit.sniff_timestamp - child_id_rsp.sniff_timestamp < MAX_IMMEDIATE_UPGRADE_DELAY, \
        "Previous Router ID must be requested immediately after attaching"

    return next_addr_sol_success(pkts, solicit).coap.tlv.rloc16


def verify(pv):
    # Router whose Router ID is deallocated by the Leader (Thread 1.4.1 5.9.9 and 5.16.1).
    #
    # Topology: LEADER ---- REED ---- DUT ---- OTHER, plus a LEADER ---- DUT link in some phases.

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    LEADER_RLOC16 = pv.vars['LEADER_RLOC16']
    REED = pv.vars['REED']
    DUT = pv.vars['DUT']
    OTHER = pv.vars['OTHER']

    # Step 0: DUT becomes router (Address Solicit success).
    print("Step 0: DUT becomes router")
    pkts.filter_wpan_src64(DUT).\
        filter_wpan_dst16(LEADER_RLOC16).\
        filter_coap_request(consts.ADDR_SOL_URI).\
        must_next()
    ack = pkts.filter_wpan_src64(LEADER).\
        filter_coap_ack(consts.ADDR_SOL_URI).\
        filter(lambda p: p.coap.tlv.status == consts.ADDR_SOL_SUCCESS).\
        must_next()
    dut_rloc16 = ack.coap.tlv.rloc16

    # Step 1-2: Reconnect through the REED with R and E set, then immediately request the previous Router ID.
    print("Step 1-2: DUT reconnects through the REED and immediately requests its previous Router ID")
    rsp = verify_reconnect_attempt(pkts, DUT, REED)
    dut_rloc16 = verify_router_id_request(pkts, DUT, dut_rloc16, rsp)

    # Step 3-4: After a multi-step ID sequence jump, the DUT accepts the REED again.
    print("Step 3-4: DUT reconnects after an ID sequence jump and requests its previous Router ID")
    rsp = verify_reconnect_attempt(pkts, DUT, REED)
    verify_router_id_request(pkts, DUT, dut_rloc16, rsp)

    # Step 5-6: The reconnect attempt uses only R and E, then the fallback runs a full regular first attach cycle.
    print("Step 5-6: Reconnect attempt uses R and E, then falls back to a full regular attach")
    requests = [next_parent_request(pkts, DUT) for _ in range(2 * FIRST_CYCLE_PARENT_REQUESTS)]
    reconnect = requests[:FIRST_CYCLE_PARENT_REQUESTS]
    fallback = requests[FIRST_CYCLE_PARENT_REQUESTS:]

    for p in reconnect:
        assert is_r_and_e(p), "All Parent Requests in the reconnect attempt must have R and E set"

    for p in fallback[:FIRST_CYCLE_PARENT_REQUESTS_TO_ROUTERS]:
        assert is_r_only(p), "Fallback attach must start with Parent Requests to routers only"

    for p in fallback[FIRST_CYCLE_PARENT_REQUESTS_TO_ROUTERS:]:
        assert is_r_and_e(p), "Fallback attach must continue with Parent Requests to routers and REEDs"

    # Step 7: DUT merges back into the Leader's partition and becomes router.
    print("Step 7: DUT merges back and becomes router")
    solicit = pkts.filter_wpan_src64(DUT).\
        filter_coap_request(consts.ADDR_SOL_URI).\
        must_next()
    next_addr_sol_success(pkts, solicit)

    # Step 8-9: The DUT attaches to OTHER's higher-priority partition during the reconnect attempt.
    print("Step 8-9: DUT attaches to the higher-priority partition during the reconnect attempt")
    verify_reconnect_attempt(pkts, DUT, OTHER)


if __name__ == '__main__':
    verify_utils.run_main(verify)
