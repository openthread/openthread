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
    # 5.3.1 Link-Local Addressing
    #
    # 5.3.1.1 Topology
    # - Leader
    # - Router_1 (DUT)
    #
    # 5.3.1.2 Purpose & Description
    # The purpose of this test case is to validate the Link-Local addresses that the DUT auto-configures.
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # Link-Local Scope | 5.2.3.1      | 5.2.1.1

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    DUT = pv.vars['DUT']

    # Step 1: Router_1 and Leader
    # - Description: Build the topology as described and begin the wireless sniffer
    # - Pass Criteria: N/A
    print("Step 1: Router_1 and Leader")

    # Step 2: Leader
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request to the DUT’s MAC extended
    #   address-based Link-Local address
    # - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply
    print("Step 2: Leader sends Echo Request to DUT LL64 address")
    pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst64(DUT).\
        must_next()
    pkts.filter_ping_reply().\
        filter_wpan_src64(DUT).\
        filter_wpan_dst64(LEADER).\
        must_next()

    # Step 3: Leader
    # - Description: Harness instructs the device to send a fragmented ICMPv6 Echo Request to DUT’s MAC extended
    #   address-based Link-Local address
    # - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply
    print("Step 3: Leader sends fragmented Echo Request to DUT LL64 address")
    pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst64(DUT).\
        must_next()
    pkts.filter_ping_reply().\
        filter_wpan_src64(DUT).\
        filter_wpan_dst64(LEADER).\
        must_next()

    # Step 4: Leader
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request to the Link-Local All Nodes
    #   multicast address (FF02::1)
    # - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply
    print("Step 4: Leader sends Echo Request to All Nodes multicast address")
    pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_LLANMA().\
        must_next()
    pkts.filter_ping_reply().\
        filter_wpan_src64(DUT).\
        filter_wpan_dst64(LEADER).\
        must_next()

    # Step 5: Leader
    # - Description: Harness instructs the device to send a fragmented ICMPv6 Echo Request to the Link-Local All Nodes
    #   multicast address (FF02::1)
    # - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply
    print("Step 5: Leader sends fragmented Echo Request to All Nodes multicast address")
    pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_LLANMA().\
        must_next()
    pkts.filter_ping_reply().\
        filter_wpan_src64(DUT).\
        filter_wpan_dst64(LEADER).\
        must_next()

    # Step 6: Leader
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request to the Link-Local All-Routers
    #   multicast address (FF02::2)
    # - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply
    print("Step 6: Leader sends Echo Request to All Routers multicast address")
    pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_LLARMA().\
        must_next()
    pkts.filter_ping_reply().\
        filter_wpan_src64(DUT).\
        filter_wpan_dst64(LEADER).\
        must_next()

    # Step 7: Leader
    # - Description: Harness instructs the device to send a fragmented ICMPv6 Echo Request to the Link-Local
    #   All-Routers multicast address (FF02::2)
    # - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply
    print("Step 7: Leader sends fragmented Echo Request to All Routers multicast address")
    pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_LLARMA().\
        must_next()
    pkts.filter_ping_reply().\
        filter_wpan_src64(DUT).\
        filter_wpan_dst64(LEADER).\
        must_next()

    # Step 8: Leader
    # - Description: Harness instructs the device to send a ICMPv6 Echo Request to the Link-Local All Thread Nodes
    #   multicast address
    # - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply
    print("Step 8: Leader sends Echo Request to All Thread Nodes multicast address")
    pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_LLATNMA().\
        must_next()
    pkts.filter_ping_reply().\
        filter_wpan_src64(DUT).\
        filter_wpan_dst64(LEADER).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
