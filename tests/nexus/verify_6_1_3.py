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
    # 6.1.3 Attaching to a Router with better connectivity
    #
    # 6.1.3.1 Topology
    #   - Ensure link quality between all nodes is set to 3.
    #   - Leader
    #   - Router_1
    #   - Router_2
    #   - Router_3
    #   - DUT as ED_1 (Topology A) or SED_1 (Topology B)
    #
    # 6.1.3.2 Purpose & Description
    #   The purpose of this test case is to verify that the DUT chooses to attach to a Router with better
    #     connectivity.
    #
    # Spec Reference        | V1.1 Section | V1.3.0 Section
    # ----------------------|--------------|---------------
    # Attaching to a Parent | 4.7.1        | 4.5.1

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_2 = pv.vars['ROUTER_2']
    ROUTER_3 = pv.vars['ROUTER_3']
    DUT = pv.vars.get('ED_1') or pv.vars.get('SED_1')

    # Step 1: Leader, Router_1, _2, _3
    #   - Description: Setup the topology without the DUT. Ensure all routers and leader are sending MLE
    #     advertisements.
    #   - Pass Criteria: N/A
    print("Step 1: Leader, Router_1, _2, _3")
    for node in [LEADER, ROUTER_1, ROUTER_2, ROUTER_3]:
        pkts.copy().\
            filter_wpan_src64(node).\
            filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
            must_next()

    # Step 2: ED_1 / SED_1 (DUT)
    #   - Description: Automatically begins attach process by sending a multicast MLE Parent Request.
    #   - Pass Criteria:
    #     - The DUT MUST send MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an
    #       IP Hop Limit of 255.
    #     - The following TLVs MUST be present in the Parent Request:
    #       - Challenge TLV
    #       - Mode TLV
    #       - Scan Mask TLV = 0x80 (active Routers)
    #       - Version TLV
    print("Step 2: ED_1 / SED_1 (DUT)")
    pkts.filter_wpan_src64(DUT).\
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

    # Step 3: Router_2, Router_3
    #   - Description: Automatically responds with MLE Parent Response.
    #   - Pass Criteria: N/A
    print("Step 3: Router_2, Router_3")
    for router in [ROUTER_2, ROUTER_3]:
        pkts.copy().\
            filter_wpan_src64(router).\
            filter_mle_cmd(consts.MLE_PARENT_RESPONSE).\
            must_next()

    # Step 4: ED_1 / SED_1 (DUT)
    #   - Description: Automatically sends MLE Child ID Request to Router_3 due to better connectivity.
    #   - Pass Criteria:
    #     - The DUT MUST unicast MLE Child ID Request to Router_3.
    #     - The following TLVs MUST be present in the Child ID Request:
    #       - Address Registration TLV
    #       - Link-layer Frame Counter TLV
    #       - Mode TLV
    #       - Response TLV
    #       - Timeout TLV
    #       - TLV Request TLV
    #       - Version TLV
    #       - MLE Frame Counter TLV (optional)
    print("Step 4: ED_1 / SED_1 (DUT)")
    pkts.filter_wpan_src64(DUT).\
        filter_wpan_dst64(ROUTER_3).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter(lambda p: {
                          consts.ADDRESS_REGISTRATION_TLV,
                          consts.LINK_LAYER_FRAME_COUNTER_TLV,
                          consts.MODE_TLV,
                          consts.RESPONSE_TLV,
                          consts.TIMEOUT_TLV,
                          consts.TLV_REQUEST_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 5: Router_3
    #   - Description: Harness verifies connectivity by instructing device to send an ICMPv6 Echo Request to the
    #     DUT link local address.
    #   - Pass Criteria:
    #     - The DUT MUST respond with ICMPv6 Echo Reply.
    print("Step 5: Router_3")
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(ROUTER_3).\
        filter_wpan_dst64(DUT).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(DUT).\
        filter_wpan_dst64(ROUTER_3).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
