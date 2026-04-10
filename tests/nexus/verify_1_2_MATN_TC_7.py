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
from pktverify.addrs import Ipv6Addr


def verify(pv):
    # 5.10.6 MATN-TC-07: Default BR multicast forwarding
    #
    # 5.10.6.1 Topology
    # - BR_1
    # - BR_2
    # - ROUTER
    #
    # 5.10.6.2 Purpose & Description
    # Verify if the default forwarding of multicast on a Primary BBR is correct. Note: this can be changed by
    #   forwarding flags of the BBR; however, this is application specific and change is not tested. This test case
    #   also verifies that a Secondary BBR does not forward multicast packets to the backbone link.
    #
    # Spec Reference   | V1.2 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # Multicast        | 5.10.6       | N/A

    pkts = pv.pkts
    vars = pv.vars

    # Step 0
    # - Device: N/A
    # - Description: Topology formation - BR_1, BR_2, ROUTER
    # - Pass Criteria:
    #   - N/A
    print("Step 0: Topology formation - BR_1, BR_2, ROUTER")

    # Step 1
    # - Device: ROUTER
    # - Description: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet with realm-local scope
    #   MA5.
    # - Pass Criteria:
    #   - N/A
    print("Step 1: ROUTER sends an ICMPv6 Echo (ping) Request packet with realm-local scope MA5.")
    pkts.filter_ping_request().\
        filter(lambda p: p.wpan).\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['MA5']) or\
                         p.ipv6.dst == 'ff03::fc').\
        must_next()

    # Step 2 & 3
    # - Device: BR_1, BR_2
    # - Description: Does not forward the multicast ping request packet to the LAN.
    # - Pass Criteria:
    #   - The DUT MUST NOT forward the multicast ICMPv6 Echo (ping) Request packet with realm-local scope address
    #     MA5 to the LAN.
    print("Step 2 & 3: BR_1 and BR_2 do not forward the multicast ping request packet to the LAN.")
    pkts.copy().\
        filter_ping_request().\
        filter(lambda p: not p.wpan).\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['MA5']) or\
                         p.ipv6.dst == 'ff03::fc').\
        must_not_next()

    # Step 4
    # - Device: ROUTER
    # - Description: Harness instructs the device to sends an ICMPv6 Echo (ping) Request packet with admin-local scope
    #   (address ff04::…), encapsulated in an MPL packet.
    # - Pass Criteria:
    #   - N/A
    print("Step 4: ROUTER sends an ICMPv6 Echo Request with admin-local scope, encapsulated in an MPL packet.")
    pkts_copy = pkts.copy()
    pkts_copy.filter_ping_request().\
        filter(lambda p: p.wpan).\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['ADMIN_LOCAL_MCAST']) or\
                         p.ipv6.dst == 'ff03::fc').\
        must_next()

    # Step 5 & 6
    # - Device: BR_1, BR_2
    # - Description: BR_1 automatically forwards the ping request packet to the LAN. BR_2 does not.
    # - Pass Criteria:
    #   - BR_1 MUST forward the multicast ICMPv6 Echo (ping) Request packet with admin-local scope to the LAN.
    #   - BR_2 MUST NOT forward it.
    print("Step 5 & 6: BR_1 forwards the ping request to the LAN; BR_2 does not.")
    # Check that it is forwarded to the LAN by BR_1.
    # Note: MPL retransmissions may cause multiple copies on the LAN from the same BR.
    pkts.filter_ping_request().\
        filter(lambda p: not p.wpan).\
        filter(lambda p: p.eth.src == vars['BR_1_ETH']).\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['ADMIN_LOCAL_MCAST']) or\
                         p.ipv6.dst == 'ff03::fc').\
        must_next()

    # Check that BR_2 does NOT forward it.
    pkts.copy().\
        filter_ping_request().\
        filter(lambda p: not p.wpan).\
        filter(lambda p: p.eth.src == vars['BR_2_ETH']).\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['ADMIN_LOCAL_MCAST']) or\
                         p.ipv6.dst == 'ff03::fc').\
        must_not_next()

    if pkts.index < pkts_copy.index:
        pkts.index = pkts_copy.index

    # Step 7
    # - Device: ROUTER
    # - Description: Harness instructs the device to send a ICMPv6 Echo (ping) Request packet with site-local scope
    #   (address ff05::…), encapsulated in an MPL packet.
    # - Pass Criteria:
    #   - N/A
    print("Step 7: ROUTER sends an ICMPv6 Echo Request with site-local scope, encapsulated in an MPL packet.")
    pkts_copy = pkts.copy()
    pkts_copy.filter_ping_request().\
        filter(lambda p: p.wpan).\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['SITE_LOCAL_MCAST']) or\
                         p.ipv6.dst == 'ff03::fc').\
        must_next()

    # Step 8 & 9
    # - Device: BR_1, BR_2
    # - Description: BR_1 automatically forwards the ping request packet to the LAN. BR_2 does not.
    print("Step 8 & 9: BR_1 forwards the ping request to the LAN; BR_2 does not.")
    # Check that it is forwarded to the LAN by BR_1.
    pkts.filter_ping_request().\
        filter(lambda p: not p.wpan).\
        filter(lambda p: p.eth.src == vars['BR_1_ETH']).\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['SITE_LOCAL_MCAST']) or\
                         p.ipv6.dst == 'ff03::fc').\
        must_next()

    # Check that BR_2 does NOT forward it.
    pkts.copy().\
        filter_ping_request().\
        filter(lambda p: not p.wpan).\
        filter(lambda p: p.eth.src == vars['BR_2_ETH']).\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['SITE_LOCAL_MCAST']) or\
                         p.ipv6.dst == 'ff03::fc').\
        must_not_next()

    if pkts.index < pkts_copy.index:
        pkts.index = pkts_copy.index

    # Step 10
    # - Device: ROUTER
    # - Description: Harness instructs the device to send a ICMPv6 Echo (ping) Request packet with global scope
    #   (address ff0e::…) , encapsulated in an MPL packet.
    # - Pass Criteria:
    #   - N/A
    print("Step 10: ROUTER sends an ICMPv6 Echo Request with global scope, encapsulated in an MPL packet.")
    pkts_copy = pkts.copy()
    pkts_copy.filter_ping_request().\
        filter(lambda p: p.wpan).\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['GLOBAL_MCAST']) or\
                         p.ipv6.dst == 'ff03::fc').\
        must_next()

    # Step 11 & 12
    # - Device: BR_1, BR_2
    # - Description: BR_1 automatically forwards the ping request packet to the LAN. BR_2 does not.
    print("Step 11 & 12: BR_1 forwards the ping request to the LAN; BR_2 does not.")
    # Check that it is forwarded to the LAN by BR_1.
    pkts.filter_ping_request().\
        filter(lambda p: not p.wpan).\
        filter(lambda p: p.eth.src == vars['BR_1_ETH']).\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['GLOBAL_MCAST']) or\
                         p.ipv6.dst == 'ff03::fc').\
        must_next()

    # Check that BR_2 does NOT forward it.
    pkts.copy().\
        filter_ping_request().\
        filter(lambda p: not p.wpan).\
        filter(lambda p: p.eth.src == vars['BR_2_ETH']).\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['GLOBAL_MCAST']) or\
                         p.ipv6.dst == 'ff03::fc').\
        must_not_next()

    if pkts.index < pkts_copy.index:
        pkts.index = pkts_copy.index

    # Step 13
    # - Device: ROUTER
    # - Description: Harness instructs the device to send a ICMPv6 Echo (ping) Request packet with link-local scope
    #   (address ff02::…).
    # - Pass Criteria:
    #   - N/A
    print("Step 13: ROUTER sends an ICMPv6 Echo (ping) Request packet with link-local scope.")
    pkts.filter_ping_request().\
        filter(lambda p: p.wpan).\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['LINK_LOCAL_MCAST']) or\
                         p.ipv6.dst == 'ff03::fc').\
        must_next()

    # Step 14 & 15
    # - Device: BR_1, BR_2
    # - Description: Does not forward the multicast ping packet to the LAN.
    print("Step 14 & 15: BR_1 and BR_2 do not forward the multicast ping packet to the LAN.")
    pkts.copy().\
        filter_ping_request().\
        filter(lambda p: not p.wpan).\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['LINK_LOCAL_MCAST']) or\
                         p.ipv6.dst == 'ff03::fc').\
        must_not_next()

    # Step 16
    # - Device: ROUTER
    # - Description: Harness instructs the device to send a ICMPv6 Echo (ping) Request with global scope (address
    #   ff0e::…) , encapsulated in an MPL packet. The source address is chosen as the ML-EID.
    # - Pass Criteria:
    #   - N/A
    print("Step 16: ROUTER sends an ICMPv6 Echo Request with global scope and ML-EID as source.")
    pkts.filter_ping_request().\
        filter(lambda p: p.wpan).\
        filter(lambda p: p.ipv6.src == vars['ROUTER_MLEID'] or p.ipv6inner.src == vars['ROUTER_MLEID']).\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['GLOBAL_MCAST']) or\
                         p.ipv6.dst == 'ff03::fc').\
        must_next()

    # Step 17 & 18
    # - Device: BR_1, BR_2
    # - Description: Does not forward the multicast ping packet to the LAN.
    print("Step 17 & 18: BR_1 and BR_2 do not forward the multicast ping packet to the LAN.")
    pkts.copy().\
        filter_ping_request().\
        filter(lambda p: not p.wpan).\
        filter(lambda p: p.ipv6.src == vars['ROUTER_MLEID'] or p.ipv6inner.src == vars['ROUTER_MLEID']).\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['GLOBAL_MCAST']) or\
                         p.ipv6.dst == 'ff03::fc').\
        must_not_next()

    # Step 19
    # - Device: ROUTER
    # - Description: Harness instructs the device to send a ICMPv6 Echo (ping) Request with global scope (address
    #   ff0e::…) , encapsulated in an MPL packet. The source address is chosen as the link-local address (fe80::...).
    # - Pass Criteria:
    #   - N/A
    print("Step 19: ROUTER sends an ICMPv6 Echo Request with global scope and link-local address as source.")
    pkts.filter_ping_request().\
        filter(lambda p: p.wpan).\
        filter(lambda p: p.ipv6.src == vars['ROUTER_LLA'] or p.ipv6inner.src == vars['ROUTER_LLA']).\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['GLOBAL_MCAST']) or\
                         p.ipv6.dst == 'ff03::fc').\
        must_next()

    # Step 20 & 21
    # - Device: BR_1, BR_2
    # - Description: Does not forward the multicast ping packet to the LAN.
    print("Step 20 & 21: BR_1 and BR_2 do not forward the multicast ping packet to the LAN.")
    pkts.copy().\
        filter_ping_request().\
        filter(lambda p: not p.wpan).\
        filter(lambda p: p.ipv6.src == vars['ROUTER_LLA'] or p.ipv6inner.src == vars['ROUTER_LLA']).\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['GLOBAL_MCAST']) or\
                         p.ipv6.dst == 'ff03::fc').\
        must_not_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
