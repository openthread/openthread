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
    # 6.1.4 Attaching to a REED with Better Connectivity
    #
    # 6.1.4.1 Topology
    #   - Ensure link quality between all nodes is set to 3.
    #   - Topology B: DUT as Sleepy End Device (SED_1)
    #   - Leader
    #   - Router_1
    #   - Router_2 (REED_2 in spec)
    #   - Router_3 (REED_1 in spec)
    #
    # 6.1.4.2 Purpose & Description
    # The purpose of this test case is to validate that the DUT will pick REED_1 as its parent because of its better
    #   connectivity.
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
    SED_1 = pv.vars['SED_1']

    # Step 1: All
    # - Description: Setup the topology without the DUT. Ensure all routers and leader are sending MLE advertisements.
    # - Pass Criteria: N/A
    print("Step 1: All")
    pkts.copy().\
        filter_wpan_src64(LEADER).\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        must_next()
    pkts.copy().\
        filter_wpan_src64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        must_next()

    # Step 2: ED_1 / SED_1 (DUT)
    # - Description: Automatically begins attach process by sending a multicast MLE Parent Request.
    # - Pass Criteria:
    #   - The DUT MUST send MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an IP
    #     Hop Limit of 255.
    #   - The following TLVs MUST be present in the Parent Request:
    #     - Challenge TLV
    #     - Mode TLV
    #     - Scan Mask TLV = 0x80 (active Routers)
    #     - Version TLV
    print("Step 2: ED_1 / SED_1 (DUT)")
    pkts.filter_wpan_src64(SED_1).\
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

    # Step 3: REED_1, REED_2
    # - Description: Do not respond to Parent Request.
    # - Pass Criteria: N/A
    print("Step 3: REED_1, REED_2")

    # Step 4: ED_1 / SED_1 (DUT)
    # - Description: Automatically sends MLE Parent Request with Scan Mask set to Routers AND REEDs.
    # - Pass Criteria:
    #   - The DUT MUST send MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an IP
    #     Hop Limit of 255.
    #   - The following TLVs MUST be present in the Parent Request:
    #     - Challenge TLV
    #     - Mode TLV
    #     - Scan Mask TLV (Value = 0xC0 [Routers and REEDs])
    #     - Version TLV
    print("Step 4: ED_1 / SED_1 (DUT)")
    pkts.filter_wpan_src64(SED_1).\
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
               p.mle.tlv.scan_mask.e == 1).\
        must_next()

    # Step 5: REED_1, REED_2
    # - Description: Automatically respond with MLE Parent Response. REED_1 has one more link quality connection than
    #   REED_2 in Connectivity TLV.
    # - Pass Criteria: N/A
    print("Step 5: REED_1, REED_2")
    # Verify parent response from REED_2 (ROUTER_2)
    pkts.copy().\
        filter_wpan_src64(ROUTER_2).\
        filter_wpan_dst64(SED_1).\
        filter_mle_cmd(consts.MLE_PARENT_RESPONSE).\
        must_next()
    # Verify parent response from REED_1 (ROUTER_3)
    pkts.copy().\
        filter_wpan_src64(ROUTER_3).\
        filter_wpan_dst64(SED_1).\
        filter_mle_cmd(consts.MLE_PARENT_RESPONSE).\
        must_next()

    # Step 6: ED_1 / SED_1 (DUT)
    # - Description: Automatically sends MLE Child ID Request to REED_1.
    # - Pass Criteria:
    #   - The DUT MUST unicast MLE Child ID Request to REED_1.
    #   - The following TLVs MUST be present in the Child ID Request:
    #     - Address Registration TLV
    #     - Link-layer Frame Counter TLV
    #     - Mode TLV
    #     - Response TLV
    #     - Timeout TLV
    #     - TLV Request TLV
    #     - Version TLV
    #     - MLE Frame Counter TLV (optional)
    print("Step 6: ED_1 / SED_1 (DUT)")
    pkts.filter_wpan_src64(SED_1).\
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

    # Step 7: REED_1
    # - Description: Automatically sends an Address Solicit Request to Leader with TOO_FEW_ROUTERS upgrade request.
    #   Leader automatically sends an Address Solicit Response and REED_1 becomes active router. REED_1 automatically
    #   sends MLE Child ID Response with DUT’s new 16-bit Address.
    # - Pass Criteria: N/A
    print("Step 7: REED_1")
    pkts.filter_wpan_src64(ROUTER_3).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter_wpan_dst64(SED_1).\
        must_next()

    # Step 8: REED_1
    # - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the DUT
    #   link local address.
    # - Pass Criteria:
    #   - The DUT MUST respond with ICMPv6 Echo Reply.
    print("Step 8: REED_1")
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(ROUTER_3).\
        filter_wpan_dst64(SED_1).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(SED_1).\
        filter_wpan_dst64(ROUTER_3).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
