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
    # 5.5.4 Split and Merge with Routers
    #
    # 5.5.4.1 Topology A (DUT as Leader)
    # - The topology consists of a Leader (DUT) connected to Router_1 and Router_2. Router_1 is connected to Router_3.
    #   Router_2 is connected to Router_4.
    #
    # Purpose & Description
    # The purpose of this test case is to show that the Leader will merge two separate network partitions and allow
    #   communication across a single unified network.
    #
    # Spec Reference            | V1.1 Section | V1.3.0 Section
    # --------------------------|--------------|---------------
    # Thread Network Partitions | 5.16         | 5.16

    pkts = pv.pkts
    pv.summary.show()

    DUT = pv.vars['DUT']
    ROUTER_3_MLEID = pv.vars['ROUTER_3_MLEID']
    ROUTER_4_MLEID = pv.vars['ROUTER_4_MLEID']

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A.
    print("Step 1: All")

    # Step 2: Leader (DUT)
    # - Description: Automatically transmits MLE advertisements.
    # - Pass Criteria:
    #   - The DUT MUST send formatted MLE Advertisements with an IP Hop Limit of 255 to the Link-Local All Nodes
    #     multicast address (FF02::1).
    #   - The following TLVs MUST be present in the Advertisements:
    #     - Leader Data TLV
    #     - Route64 TLV
    #     - Source Address TLV.
    print("Step 2: Leader (DUT)")
    pkts.filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter_LLANMA().\
        filter_wpan_src64(DUT).\
        filter(lambda p: {
                          consts.LEADER_DATA_TLV,
                          consts.ROUTE64_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type) and\
               p.ipv6.hlim == 255).\
        must_next()

    # Step 3: Leader (DUT)
    # - Description: Reset the DUT for 300 seconds (longer than NETWORK_ID_TIMEOUT default value of 120 seconds).
    # - Pass Criteria: The DUT MUST stop sending MLE advertisements,.
    print("Step 3: Leader (DUT)")
    # Find the markers for the reset period.
    markers = pv.pkts.copy()
    markers.filter_ping_request().\
        filter(lambda p: p.icmpv6.echo.identifier == 0x5303).\
        must_next()
    start_index = markers.index

    markers.filter_ping_request().\
        filter(lambda p: p.icmpv6.echo.identifier == 0x5304).\
        must_next()
    stop_index = markers.index

    # We verify that the DUT stops sending advertisements by checking the range between markers.
    # Using pv.pkts.copy() to ensure we have the full trace range available for the range() call.
    pv.pkts.copy().range(start_index, stop_index).\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter_wpan_src64(DUT).\
        must_not_next()

    # Step 4: Routers
    # - Description: Automatically create two partitions after DUT is removed and NETWORK_ID_TIMEOUT expires.
    # - Pass Criteria: N/A.
    print("Step 4: Routers")

    # Step 5: Harness
    # - Description: Wait for 200 seconds (After 200 seconds the DUT will be done resetting, and the network will
    #   have merged into a single partition).
    # - Pass Criteria: N/A.
    print("Step 5: Harness")

    # Step 6: Router_3
    # - Description: Harness instructs device to send an ICMPv6 Echo Request to Router_4.
    # - Pass Criteria: Router_4 MUST send an ICMPv6 Echo Reply to Router_3.
    print("Step 6: Router_3")
    pkts.filter_ping_request().\
        filter_ipv6_src(ROUTER_3_MLEID).\
        filter_ipv6_dst(ROUTER_4_MLEID).\
        must_next()
    pkts.filter_ping_reply().\
        filter_ipv6_src(ROUTER_4_MLEID).\
        filter_ipv6_dst(ROUTER_3_MLEID).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
