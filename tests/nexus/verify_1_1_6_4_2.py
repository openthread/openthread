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
from pktverify.addrs import Ipv6Addr


def verify(pv):
    # 6.4.2 Realm-Local Addressing
    #
    # 6.4.2.1 Topology
    # - Topology A: DUT as End Device (ED_1)
    # - Topology B: DUT as Sleepy End Device (SED_1)
    #
    # 6.4.2.2 Purpose & Description
    # The purpose of this test case is to validate the Realm-Local addresses that the DUT configures.
    #
    # Spec Reference    | V1.1 Section | V1.3.0 Section
    # ------------------|--------------|---------------
    # Realm-Local Scope | 5.2.3.2      | 5.2.1.2

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    DUT = pv.vars.get('ED_1') or\
        pv.vars.get('SED_1') or\
        pv.vars.get('DUT')
    DUT_MLEID = pv.vars.get('ED_1_MLEID') or\
        pv.vars.get('SED_1_MLEID') or\
        pv.vars.get('DUT_MLEID')

    assert DUT is not None, "DUT not found"
    assert DUT_MLEID is not None, "DUT_MLEID not found"

    topology = 'A' if 'ED_1' in pv.vars else 'B'

    # The Realm-Local All Thread Nodes multicast address (FF33:40:<MeshLocalPrefix>::1)
    # Derived from Link-Local All Thread Nodes (FF32:40:<MeshLocalPrefix>::1)
    ll_all_thread = consts.LINK_LOCAL_ALL_THREAD_NODES_MULTICAST_ADDRESS
    rl_all_thread = bytearray(ll_all_thread)
    rl_all_thread[1] = 0x33
    REALM_LOCAL_ALL_THREAD_NODES = Ipv6Addr(rl_all_thread)

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A
    print("Step 1: All - Ensure topology is formed correctly.")

    # Step 2: Leader
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request to the DUT ML-EID.
    # - Pass Criteria:
    #   - The DUT MUST respond with an ICMPv6 Echo Reply.
    print(f"Step 2: Leader sends an ICMPv6 Echo Request to the DUT ML-EID.")
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_ipv6_dst(DUT_MLEID).\
        filter(lambda p: p.ipv6.plen < 100).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(DUT).\
        filter_ipv6_dst(_pkt.ipv6.src).\
        must_next()

    # Step 3: Leader
    # - Description: Harness instructs the device to send a fragmented ICMPv6 Echo Request to the DUT ML-EID.
    # - Pass Criteria:
    #   - The DUT MUST respond with an ICMPv6 Echo Reply.
    print(f"Step 3: Leader sends a fragmented ICMPv6 Echo Request to the DUT ML-EID.")
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_ipv6_dst(DUT_MLEID).\
        filter(lambda p: p.ipv6.plen > 1000).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(DUT).\
        filter_ipv6_dst(_pkt.ipv6.src).\
        must_next()

    if topology == 'A':
        # Step 4: Leader [Topology A only]
        # - Description: Harness instructs the device to send a ICMPv6 Echo Request to the Realm-Local All-Nodes
        #   multicast address (FF03::1).
        # - Pass Criteria:
        #   - The DUT (ED_1) MUST respond with an ICMPv6 Echo Reply.
        print("Step 4: Leader sends a ICMPv6 Echo Request to the Realm-Local All-Nodes multicast address (FF03::1).")
        _pkt = pkts.filter_ping_request().\
            filter_wpan_src64(LEADER).\
            filter_ipv6_dst("FF03::1").\
            filter(lambda p: p.ipv6.plen < 100).\
            must_next()
        pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
            filter_wpan_src64(DUT).\
            filter_ipv6_dst(_pkt.ipv6.src).\
            must_next()

        # Step 5: Leader [Topology A only]
        # - Description: Harness instructs the device to send a fragmented ICMPv6 Echo Request to the Realm-Local
        #   All-Nodes multicast address (FF03::1).
        # - Pass Criteria:
        #   - The DUT (ED_1) MUST respond with an ICMPv6 Echo Reply.
        print("Step 5: Leader sends a fragmented ICMPv6 Echo Request to the Realm-Local All-Nodes multicast "
              "address (FF03::1).")
        _pkt = pkts.filter_ping_request().\
            filter_wpan_src64(LEADER).\
            filter_ipv6_dst("FF03::1").\
            filter(lambda p: p.ipv6.plen > 1000).\
            must_next()
        pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
            filter_wpan_src64(DUT).\
            filter_ipv6_dst(_pkt.ipv6.src).\
            must_next()

    # Step 6: Leader
    # - Description: Harness instructs the device to send an ICMPv6 Echo Request to the Realm-Local All Thread Nodes
    #   multicast address.
    # - Pass Criteria:
    #   - The DUT MUST respond with an ICMPv6 Echo Reply.
    print(f"Step 6: Leader sends an ICMPv6 Echo Request to the Realm-Local All Thread Nodes multicast address.")
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_ipv6_dst(REALM_LOCAL_ALL_THREAD_NODES).\
        filter(lambda p: p.ipv6.plen < 100).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(DUT).\
        filter_ipv6_dst(_pkt.ipv6.src).\
        must_next()

    # Step 7: Leader
    # - Description: Harness instructs the device to send a fragmented ICMPv6 Echo Request to the Realm-Local All
    #   Thread Nodes multicast address.
    # - Pass Criteria:
    #   - The DUT MUST respond with an ICMPv6 Echo Reply.
    print(f"Step 7: Leader sends a fragmented ICMPv6 Echo Request to the Realm-Local All Thread Nodes multicast "
          "address.")
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_ipv6_dst(REALM_LOCAL_ALL_THREAD_NODES).\
        filter(lambda p: p.ipv6.plen > 1000).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(DUT).\
        filter_ipv6_dst(_pkt.ipv6.src).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
