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
    # 5.3.1 SSED Attachment
    #
    # 5.3.1.1 Topology
    # - Leader (DUT)
    # - Router_1
    # - SSED_1
    #
    # 5.3.1.2 Purpose & Description
    # The purpose of this test is to validate that the DUT can successfully support a SSED that establishes CSL
    #   Synchronization after attach using Child Update Request.
    #
    # Spec Reference   | V1.2 Section
    # -----------------|--------------
    # SSED Attachment  | 4.6.5.2

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    LEADER_RLOC16 = pv.vars['LEADER_RLOC16']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_1_RLOC16 = pv.vars['ROUTER_1_RLOC16']
    SSED_1 = pv.vars['SSED_1']
    SSED_1_RLOC16 = pv.vars['SSED_1_RLOC16']

    # Step 1: All
    # - Description: Topology formation: DUT, Router_1, SSED_1.
    # - Pass Criteria: N/A.
    print("Step 1: Topology formation")

    # Step 2: SSED_1
    # - Description: Automatically attaches to the DUT. Automatically initiates a CSL connection via a MLE Child Update
    #   Request that contains the CSL IEs and the CSL Synchronized Timeout TLV.
    # - Pass Criteria: N/A.
    print("Step 2: SSED_1 attaches and initiates CSL connection")

    pkts.filter_wpan_src64(SSED_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        filter(lambda p: {
                          consts.CSL_SYNCHRONIZED_TIMEOUT
                          } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 3: Leader (DUT)
    # - Description: Automatically completes the CSL connection with SSED_1 by unicasting MLE Child Update Response.
    # - Pass Criteria: The DUT MUST unicast MLE Child Update Response to SSED_1.
    #   - The Frame Version of the packet MUST be: IEEE Std 802.15.4-2015 (value = 0b10).
    print("Step 3: Leader completes CSL connection")

    _response_pkt = pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(SSED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        filter(lambda p: p.wpan.version == consts.MAC_FRAME_VERSION_2015).\
        must_next()

    # Step 4: Router_1
    # - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
    #   SSED_1 mesh-local address (via the DUT).
    # - Pass Criteria:
    #   - The DUT MUST forward the ICMPv6 Echo Request to SSED_1.
    #   - The Frame Version of the packet MUST be: IEEE Std 802.15.4-2015 (value = 0b10).
    #   - SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the Leader.
    #   - Router_1 MUST receive an ICMPv6 Echo Response from SSED_1.
    print("Step 4: Router_1 sends Echo Request to SSED_1")

    # Echo Request from ROUTER_1 to LEADER
    pkts.filter_ping_request().\
        filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst16(LEADER_RLOC16).\
        must_next()

    # Forwarded Echo Request from LEADER to SSED_1
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst16(SSED_1_RLOC16).\
        filter(lambda p: p.wpan.version == consts.MAC_FRAME_VERSION_2015).\
        must_next()

    # SSED_1 MUST NOT send a MAC Data Request prior to receiving the ICMPv6 Echo Request from the Leader.
    pkts.range((_response_pkt.number, _response_pkt.number), (_pkt.number, _pkt.number)).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(LEADER_RLOC16).\
        filter_wpan_cmd(consts.WPAN_DATA_REQUEST).\
        must_not_next()

    # Echo Response from SSED_1 to LEADER
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(SSED_1).\
        filter_wpan_dst16(LEADER_RLOC16).\
        must_next()

    # Forwarded Echo Response from LEADER to ROUTER_1
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst16(ROUTER_1_RLOC16).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
