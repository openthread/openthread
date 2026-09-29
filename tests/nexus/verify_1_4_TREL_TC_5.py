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
    # 8.5. [1.4] [CERT] Discover Scan over multi-radio
    #
    # 8.5.1. Purpose
    # This test covers MLE discovery scan when nodes support different radio links.
    #
    # 8.5.2. Topology
    # - 1. Node_1 BR (DUT) - Support multi-radio (15.4 and TREL).
    # - 2. Node_2 - Reference device that supports multi-radio (15.4 and TREL).
    # - 3. Node_3 - Reference device that supports 15.4 radio only.
    # Devices using TREL MUST be connected to the same infrastructure link.
    # Note: the infrastructure link is not shown in the figure below.
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # Discovery Scan   | 4.7.2.1      | 4.5.2.1

    pkts = pv.pkts
    pv.summary.show()

    NODE_1 = pv.vars['NODE_1_DUT']
    NODE_2 = pv.vars['NODE_2']
    NODE_3 = pv.vars['NODE_3']

    # Step 1
    # - Device: Node_1 (DUT), Node_2, Node_3
    # - Description (TREL-8.5): Form a network on Node_1. Form a different network on Node_2 and different one on
    #   Node_3 (each node has its own network).
    # - Pass Criteria:
    #   - N/A
    print("Step 1: Form separate networks on Node_1, Node_2, and Node_3")
    # Verified by C++ code internally

    # Step 2
    # - Device: Node_2
    # - Description (TREL-8.5): Harness instructs device to perform MLE Discovery Scan.
    # - Pass Criteria:
    #   - Node_2 MUST see both the DUT and Node_3 in its scan result.
    print("Step 2: Node_2 performs MLE Discovery Scan")
    # Node_2 MUST send MLE Discovery Request on both 15.4 and TREL
    # We verify at least one of them, but specifically check for TREL response from DUT
    # and 15.4 response from Node_3.

    # 1. Node_2 MUST send MLE Discovery Request on 15.4
    pkts.copy().\
        filter_wpan_src64(NODE_2).\
        filter_mle_cmd(consts.MLE_DISCOVERY_REQUEST).\
        filter(lambda p: p.wpan).\
        must_next()

    # 2. Node_2 MUST send MLE Discovery Request on TREL
    pkts.copy().\
        filter_eth_src(pv.vars['NODE_2_ETH']).\
        filter(lambda p: p.udp.dstport == pv.vars['NODE_1_DUT_TREL_PORT'] or p.udp.dstport == pv.vars['NODE_2_TREL_PORT']).\
        filter(lambda p: p.trel).\
        must_next()

    # Node_2 MUST see Node_1 (DUT) in its scan result (verify TREL response)
    pkts.copy().\
        filter_eth_src(pv.vars['NODE_1_DUT_ETH']).\
        filter(lambda p: p.eth.dst == pv.vars['NODE_2_ETH']).\
        filter(lambda p: p.udp.dstport == pv.vars['NODE_2_TREL_PORT']).\
        filter(lambda p: p.trel).\
        must_next()

    # Node_2 MUST see Node_3 in its scan result (verify 15.4 response)
    pkts.copy().\
        filter_wpan_src64(NODE_3).\
        filter_wpan_dst64(NODE_2).\
        filter_mle_cmd(consts.MLE_DISCOVERY_RESPONSE).\
        must_next()

    # Step 3
    # - Device: Node_3
    # - Description (TREL-8.5): Harness instructs device to perform MLE Discovery Scan.
    # - Pass Criteria:
    #   - Node_3 MUST see both the DUT and Node_2 in its scan result.
    print("Step 3: Node_3 performs MLE Discovery Scan")
    # Node_3 MUST send MLE Discovery Request (15.4 only)
    pkts.filter_wpan_src64(NODE_3).\
        filter_mle_cmd(consts.MLE_DISCOVERY_REQUEST).\
        must_next()

    # Node_3 MUST see Node_1 (DUT) and Node_2 in its scan result (both on 15.4)
    pkts.copy().\
        filter_wpan_src64(NODE_1).\
        filter_wpan_dst64(NODE_3).\
        filter_mle_cmd(consts.MLE_DISCOVERY_RESPONSE).\
        must_next()

    pkts.copy().\
        filter_wpan_src64(NODE_2).\
        filter_wpan_dst64(NODE_3).\
        filter_mle_cmd(consts.MLE_DISCOVERY_RESPONSE).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
