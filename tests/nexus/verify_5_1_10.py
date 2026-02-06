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
    # 5.1.10 Parent Selection - Superior Link Quality
    #
    # 5.1.10.1 Topology
    # - Leader
    # - Router_1
    # - Router_2
    # - Router_3 (DUT)
    #
    # 5.1.10.2 Purpose & Description
    # The purpose of this test case is to verify that the DUT selects a parent with superior link quality.
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # Parent Selection | 4.7.2        | 4.5.2

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_2 = pv.vars['ROUTER_2']
    DUT = pv.vars['DUT']

    # Step 1: Leader, Router_1, Router_2
    # - Description: Setup the topology without the DUT. Verify all are sending MLE Advertisements.
    # - Pass Criteria: N/A
    print("Step 1: Leader, Router_1, Router_2")
    pkts.copy().filter_mle_cmd(consts.MLE_ADVERTISEMENT).filter_wpan_src64(LEADER).must_next()
    pkts.copy().filter_mle_cmd(consts.MLE_ADVERTISEMENT).filter_wpan_src64(ROUTER_1).must_next()
    pkts.copy().filter_mle_cmd(consts.MLE_ADVERTISEMENT).filter_wpan_src64(ROUTER_2).must_next()

    # Step 2: Test Harness
    # - Description: Set RSSI of Router_2 at the DUT to -85dBm (Link Quality 2).
    # - Pass Criteria: N/A
    print("Step 2: Test Harness")

    # Step 3: Router_3 (DUT)
    # - Description: DUT begins attaching to the network.
    # - Pass Criteria:
    #   - MLE Parent Request:
    #     - IP Destination: Link-Local All Routers Multicast Address (FF02::2)
    #     - Hop Limit: 255
    #     - TLVs:
    #       - Mode TLV
    #       - Challenge TLV
    #       - Scan Mask TLV (Router bit = 1, REED bit = 0)
    #       - Version TLV
    print("Step 3: Router_3 (DUT)")
    pkts.filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        filter_LLARMA().\
        filter_wpan_src64(DUT).\
        filter(lambda p: {
                          consts.MODE_TLV,
                          consts.CHALLENGE_TLV,
                          consts.SCAN_MASK_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type) and\
               p.ipv6.hlim == 255 and\
               p.mle.tlv.scan_mask.r == 1 and\
               p.mle.tlv.scan_mask.e == 0).\
        must_next()

    # Step 4: Router_1, Router_2
    # - Description: Leader, Router_1, and Router_2 each send MLE Parent Response.
    # - Pass Criteria: N/A
    print("Step 4: Router_1, Router_2")
    # Verify that we see parent responses from expected nodes.
    pkts.copy().filter_mle_cmd(consts.MLE_PARENT_RESPONSE).filter_wpan_src64(ROUTER_1).must_next()
    pkts.copy().filter_mle_cmd(consts.MLE_PARENT_RESPONSE).filter_wpan_src64(ROUTER_2).must_next()

    # Step 5: Router_3 (DUT)
    # - Description: DUT sends Child ID Request to Router_1.
    # - Pass Criteria:
    #   - MLE Child ID Request:
    #     - IP Destination: Link-Local address of Router_1
    #     - Hop Limit: 255
    #     - TLVs:
    #       - Mode TLV
    #       - Response TLV
    #       - Timeout TLV
    #       - TLV Request TLV
    #       - Version TLV
    #       - Link-layer Frame Counter TLV
    #       - Exclude Address Registration TLV
    print("Step 5: Router_3 (DUT)")
    pkt = pkts.filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter_wpan_src64(DUT).\
        filter_wpan_dst64(ROUTER_1).\
        filter(lambda p: {
                          consts.MODE_TLV,
                          consts.RESPONSE_TLV,
                          consts.TIMEOUT_TLV,
                          consts.TLV_REQUEST_TLV,
                          consts.VERSION_TLV,
                          consts.LINK_LAYER_FRAME_COUNTER_TLV,
                          consts.ADDRESS16_TLV,
                          consts.NETWORK_DATA_TLV
                          } <= set(p.mle.tlv.type) and\
               p.ipv6.hlim == 255 and\
               p.mle.tlv.addr16 is nullField and\
               p.thread_nwd.tlv.type is nullField).\
        must_next()
    pkt.must_not_verify(lambda p: (consts.ADDRESS_REGISTRATION_TLV) in p.mle.tlv.type)


if __name__ == '__main__':
    verify_utils.run_main(verify)
