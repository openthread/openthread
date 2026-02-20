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
    # 6.4.1 Link-Local Addressing
    #
    # 6.4.1.1 Topology
    # - Topology A: DUT as End Device (ED_1)
    # - Topology B: DUT as Sleepy End Device (SED_1)
    # - Leader
    #
    # 6.4.1.2 Purpose & Description
    # The purpose of this test case is to validate the Link-Local addresses that the DUT configures.
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # Link-Local Scope | 5.11.1       | 5.11.1

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    DUT = pv.vars.get('ED_1') or pv.vars.get('SED_1') or pv.vars.get('DUT')
    DUT_LLA = pv.vars.get('ED_1_LLA') or pv.vars.get('SED_1_LLA') or pv.vars.get('DUT_LLA')

    assert DUT is not None, "DUT not found"
    assert DUT_LLA is not None, "DUT_LLA not found"

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A
    print("Step 1: All - Ensure topology is formed correctly.")

    # Step 2: Leader
    # - Description: Harness instructs the device to send a fragmented ICMPv6 Echo Request to the DUT MAC Extended
    #   Address-based Link-Local address.
    # - Pass Criteria:
    #   - The DUT MUST respond with an ICMPv6 Echo Reply.
    print("Step 2: Leader sends a fragmented ICMPv6 Echo Request to the DUT Link-Local address.")
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_ipv6_dst(DUT_LLA).\
        filter(lambda p: p.ipv6.plen > 1000).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(DUT).\
        filter_ipv6_src(DUT_LLA).\
        must_next()

    # Step 3: Leader
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request to the DUT MAC Extended
    #   Address-based Link-Local address.
    # - Pass Criteria:
    #   - The DUT MUST respond with an ICMPv6 Echo Reply.
    print("Step 3: Leader sends an ICMPv6 Echo Request to the DUT Link-Local address.")
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_ipv6_dst(DUT_LLA).\
        filter(lambda p: p.ipv6.plen < 100).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(DUT).\
        filter_ipv6_src(DUT_LLA).\
        must_next()

    # Step 4: Leader
    # - Description: Harness instructs the device to send a fragmented ICMPv6 Echo Request to the Link-Local All
    #   Thread Nodes multicast address.
    # - Pass Criteria:
    #   - The DUT MUST respond with an ICMPv6 Echo Reply.
    print(
        "Step 4: Leader sends a fragmented ICMPv6 Echo Request to the Link-Local All Thread Nodes multicast address.")
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_ipv6_dst(consts.LINK_LOCAL_ALL_THREAD_NODES_MULTICAST_ADDRESS).\
        filter(lambda p: p.ipv6.plen > 1000).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(DUT).\
        filter_ipv6_src(DUT_LLA).\
        must_next()

    # Step 5: Leader
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request to the Link-Local All Thread Nodes
    #   multicast address.
    # - Pass Criteria:
    #   - The DUT MUST respond with an ICMPv6 Echo Reply.
    print("Step 5: Leader sends an ICMPv6 Echo Request to the Link-Local All Thread Nodes multicast address.")
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_ipv6_dst(consts.LINK_LOCAL_ALL_THREAD_NODES_MULTICAST_ADDRESS).\
        filter(lambda p: p.ipv6.plen < 100).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(DUT).\
        filter_ipv6_src(DUT_LLA).\
        must_next()

    # Step 6: [Topology A only] Leader
    # - Description: Harness instructs the device to send a fragmented ICMPv6 Echo Request to the Link-Local All Nodes
    #   multicast address (FF02::1).
    # - Pass Criteria:
    #   - The DUT MUST respond with an ICMPv6 Echo Reply.
    is_topology_a = 'ED_1' in pv.vars

    if is_topology_a:
        print("Step 6: Leader sends a fragmented ICMPv6 Echo Request to the Link-Local All Nodes multicast address.")
        _pkt = pkts.filter_ping_request().\
            filter_wpan_src64(LEADER).\
            filter_ipv6_dst(consts.LINK_LOCAL_ALL_NODES_MULTICAST_ADDRESS).\
            filter(lambda p: p.ipv6.plen > 1000).\
            next()
        if _pkt:
            pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
                filter_wpan_src64(DUT).\
                filter_ipv6_src(DUT_LLA).\
                must_next()

        # Step 7: [Topology A only] Leader
        # - Description: Harness instructs the device to send an ICMPv6 Echo Request to the Link-Local All Nodes
        #   multicast address (FF02::1).
        # - Pass Criteria:
        #   - The DUT MUST respond with an ICMPv6 Echo Reply.
        print("Step 7: Leader sends an ICMPv6 Echo Request to the Link-Local All Nodes multicast address.")
        _pkt = pkts.filter_ping_request().\
            filter_wpan_src64(LEADER).\
            filter_ipv6_dst(consts.LINK_LOCAL_ALL_NODES_MULTICAST_ADDRESS).\
            filter(lambda p: p.ipv6.plen < 100).\
            next()
        if _pkt:
            pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
                filter_wpan_src64(DUT).\
                filter_ipv6_src(DUT_LLA).\
                must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
