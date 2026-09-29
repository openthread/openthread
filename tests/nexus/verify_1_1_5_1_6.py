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
    # 5.1.6 Leader removes Router ID
    #
    # 5.1.6.1 Topology
    # - Leader
    # - Router_1 (DUT)
    #
    # 5.1.6.2 Purpose & Description
    # The purpose of this test case is to verify that when the Leader de-allocates a Router ID, the DUT, as a
    # router, re-attaches.
    #
    # Spec Reference                               | V1.1 Section | V1.3.0 Section
    # ---------------------------------------------|--------------|---------------
    # Router ID Management / Router ID Assignment  | 5.16.1       | 5.16.1

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    LEADER_RLOC16 = pv.vars['LEADER_RLOC16']
    ROUTER = pv.vars['ROUTER']

    # Step 0: All
    # - Description: Verify topology is formed correctly
    # - Pass Criteria: N/A
    print("Step 0: Verify topology is formed correctly")
    pv.verify_attached('ROUTER')
    pkts.copy().\
        filter_wpan_src64(LEADER).\
        filter_coap_ack(consts.ADDR_SOL_URI).\
        filter(lambda p: {
                          consts.NL_STATUS_TLV,
                          consts.NL_RLOC16_TLV,
                          consts.NL_ROUTER_MASK_TLV
                          } <= set(p.coap.tlv.type) and\
               p.coap.tlv.status == 0
               ).\
        must_next()

    # Step 1: Leader
    # - Description: Harness instructs the Leader to send a ' helper ping' (ICMPv6 Echo Request) to the DUT
    # - Pass Criteria:
    #   - The DUT MUST respond with an ICMPv6 Echo Reply
    print("Step 1: Leader sends a ' helper ping' (ICMPv6 Echo Request) to the DUT")
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst64(ROUTER).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(ROUTER).\
        filter_wpan_dst64(LEADER).\
        must_next()

    # Step 2: Leader
    # - Description: Harness instructs the Leader to remove the Router ID of Router_1 (the DUT)
    # - Pass Criteria: N/A
    print("Step 2: Leader removes the Router ID of Router_1 (the DUT)")

    # Step 3: Router_1 (DUT)
    # - Description: Automatically re-attaches once it recognizes its Router ID has been removed.
    # - Pass Criteria:
    #   - The DUT MUST send a properly formatted MLE Parent Request, MLE Child ID Request, and Address Solicit
    #     Request messages to the Leader. (See 5.1.1 for formatting)
    print("Step 3: Router_1 (DUT) automatically re-attaches")

    # MLE Parent Request
    pkts.filter_wpan_src64(ROUTER).\
        filter_LLARMA().\
        filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        filter(lambda p: {
                          consts.CHALLENGE_TLV,
                          consts.MODE_TLV,
                          consts.SCAN_MASK_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type) and\
               p.ipv6.hlim == 255 and\
               p.mle.tlv.scan_mask.r == 1 and\
               p.mle.tlv.scan_mask.e == 0).\
        must_next()

    # MLE Child ID Request
    _pkt = pkts.filter_wpan_src64(ROUTER).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter(lambda p: {
                          consts.LINK_LAYER_FRAME_COUNTER_TLV,
                          consts.MODE_TLV,
                          consts.RESPONSE_TLV,
                          consts.TIMEOUT_TLV,
                          consts.TLV_REQUEST_TLV,
                          consts.ADDRESS16_TLV,
                          consts.NETWORK_DATA_TLV,
                          consts.VERSION_TLV
                } <= set(p.mle.tlv.type) and\
               p.mle.tlv.addr16 is nullField and\
               p.thread_nwd.tlv.type is nullField).\
               must_next()
    _pkt.must_not_verify(lambda p: (consts.ADDRESS_REGISTRATION_TLV) in p.mle.tlv.type)

    # Address Solicit Request
    pkts.filter_wpan_src64(ROUTER).\
        filter_wpan_dst16(LEADER_RLOC16).\
        filter_coap_request(consts.ADDR_SOL_URI).\
        filter(lambda p: {
                          consts.NL_MAC_EXTENDED_ADDRESS_TLV,
                          consts.NL_STATUS_TLV
                          } <= set(p.coap.tlv.type)
               ).\
       must_next()

    # Step 4: Leader
    # - Description: Harness verifies connectivity by instructing the Leader to send an ICMPv6 Echo Request to the DUT
    # - Pass Criteria:
    #   - The DUT MUST respond with an ICMPv6 Echo Reply
    print("Step 4: Leader sends an ICMPv6 Echo Request to the DUT")
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst64(ROUTER).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(ROUTER).\
        filter_wpan_dst64(LEADER).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
