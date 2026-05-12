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
    # 5.10.9 MATN-TC-12: Hop limit processing
    #
    # 5.10.9.1 Topology
    # - BR_1 (DUT)
    # - Router
    # - Host
    #
    # 5.10.9.2 Purpose and Description
    # The purpose of this test case is to verify that a Primary BBR correctly decrements the Hop Limit field by 1 if
    #   packet is forwarded with MPL, and if forwarded from Thread Network (using MPL) to LAN. This test also verifies
    #   that the BR drops a packet with Hop Limit 0. It also checks the use of IPv6 packets that span multiple 6LoWPAN
    #   fragments.
    #
    # Spec Reference | V1.1 Section | V1.2 Section | V1.3.0 Section
    # ---------------|--------------|--------------|---------------
    # Multicast      | N/A          | 5.10.9       | 5.10.9

    pkts = pv.pkts
    vars = pv.vars
    pv.summary.show()

    BR_1 = vars['BR_1']
    ROUTER = vars['Router']
    MA1 = Ipv6Addr(vars['MA1'])
    MA2 = Ipv6Addr(vars['MA2'])
    ECHO_ID1 = 0x1231
    ECHO_ID2 = 0x1232
    ECHO_ID3 = 0x1233
    ECHO_ID4 = 0x1234
    ECHO_ID5 = 0x1235
    ECHO_ID6 = 0x1236

    HL_59 = 59
    HL_58 = 58
    HL_1 = 1
    HL_159 = 159
    HL_158 = 158
    HL_2 = 2
    HL_0 = 0

    # Step 0
    # - Device: N/A
    # - Description: Topology formation – BR_1 (DUT), Router
    # - Pass Criteria:
    #   - N/A
    print("Step 0: Topology formation – BR_1 (DUT), Router")

    # Step 1
    # - Device: Router
    # - Description: Harness instructs the device to register a multicast address, MA1
    # - Pass Criteria:
    #   - N/A
    print("Step 1: Router registers a multicast address, MA1")
    pkts.filter_wpan_src64(ROUTER).\
        filter_coap_request("/n/mr").\
        filter(lambda p: MA1 in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 2
    # - Device: Host
    # - Description: Harness instructs the device to multicast a ICMPv6 Echo (ping) Request packet to the multicast
    #   address, MA1, with the IPv6 Hop Limit field set to 59. The size of the payload is 130 bytes.
    # - Pass Criteria:
    #   - N/A
    print("Step 2: Host multicasts a ICMPv6 Echo Request to MA1, HL=59")
    pkts.filter_eth_src(vars['Host_ETH']).\
        filter_ping_request(identifier=ECHO_ID1).\
        filter(lambda p: p.ipv6.dst == MA1).\
        filter(lambda p: p.ipv6.hlim == HL_59).\
        must_next()

    # Step 3
    # - Device: BR_1 (DUT)
    # - Description: Automatically forwards the ping request packet onto the Thread Network
    # - Pass Criteria:
    #   - The DUT MUST forward the ICMPv6 Echo (ping) Request packet to Router_1 as an MPL packet encapsulating the
    #     IPv6 packet with the Hop Limit field of the inner packet set to 58.
    print("Step 3: BR_1 forwards to Thread as MPL, inner HL=58")
    pkts.filter_wpan_src64(BR_1).\
        filter_AMPLFMA().\
        filter(lambda p: p.ipv6inner.dst == MA1).\
        filter(lambda p: p.ipv6inner.hlim == HL_58).\
        filter(lambda p: p.icmpv6.echo.identifier == ECHO_ID1).\
        filter(lambda p: p.lowpan.frag).\
        must_next()

    # Step 4
    # - Device: Router
    # - Description: Receives the multicast ping request packet.
    # - Pass Criteria:
    #   - N/A
    print("Step 4: Router receives the multicast ping request packet.")

    # Step 5
    # - Device: Host
    # - Description: Harness instructs the device to multicast a ICMPv6 Echo (ping) Request packet to the multicast
    #   address, MA1, with the IPv6 Hop Limit field set to 1. The size of the payload is 130 bytes.
    # - Pass Criteria:
    #   - N/A
    print("Step 5: Host multicasts a ICMPv6 Echo Request to MA1, HL=1")
    pkts.filter_eth_src(vars['Host_ETH']).\
        filter_ping_request(identifier=ECHO_ID2).\
        filter(lambda p: p.ipv6.dst == MA1).\
        filter(lambda p: p.ipv6.hlim == HL_1).\
        must_next()

    # Step 6
    # - Device: BR_1 (DUT)
    # - Description: Does not forward the ping request packet.
    # - Pass Criteria:
    #   - The DUT MUST NOT forward the ICMPv6 Echo (ping) Request packet to the Thread Network.
    print("Step 6: BR_1 MUST NOT forward to Thread")
    pkts.copy().\
        filter_wpan_src64(BR_1).\
        filter_AMPLFMA().\
        filter(lambda p: p.ipv6inner.dst == MA1).\
        filter(lambda p: p.icmpv6.echo.identifier == ECHO_ID2).\
        must_not_next()

    # Step 7
    # - Device: Router
    # - Description: Harness instructs the device to send a ICMPv6 Echo (ping) Request packet encapsulated in an MPL
    #   packet to the multicast address, MA2, with the Hop Limit field of the inner (encapsulated) packet set to 159.
    #   The size of the payload is 130 bytes.
    # - Pass Criteria:
    #   - N/A
    print("Step 7: Router sends Echo Request in MPL to MA2, inner HL=159")
    pkts.filter_wpan_src64(ROUTER).\
        filter_AMPLFMA().\
        filter(lambda p: p.ipv6inner.dst == MA2).\
        filter(lambda p: p.ipv6inner.hlim == HL_159).\
        filter(lambda p: p.icmpv6.echo.identifier == ECHO_ID3).\
        filter(lambda p: p.lowpan.frag).\
        must_next()

    # Step 8
    # - Device: BR_1 (DUT)
    # - Description: Automatically forwards the ping request packet to the LAN.
    # - Pass Criteria:
    #   - The DUT MUST forward the multicast ICMPv6 Echo (ping) Request packet to the LAN with the Hop Limit field set
    #     to 158.
    print("Step 8: BR_1 forwards to LAN, HL=158")
    pkts.filter_eth_src(vars['BR_1_ETH']).\
        filter_ping_request(identifier=ECHO_ID3).\
        filter(lambda p: p.ipv6.dst == MA2).\
        filter(lambda p: p.ipv6.hlim == HL_158).\
        must_next()

    # Step 9
    # - Device: Router
    # - Description: Harness instructs the device to send a ICMPv6 Echo (ping) Request packet encapsulated in an MPL
    #   multicast packet to the multicast address, MA2, with the Hop Limit field of the inner (encapsulated) packet set
    #   to 2. The size of the payload is 130 bytes.
    # - Pass Criteria:
    #   - N/A
    print("Step 9: Router sends Echo Request in MPL to MA2, inner HL=2")
    pkts.filter_wpan_src64(ROUTER).\
        filter_AMPLFMA().\
        filter(lambda p: p.ipv6inner.dst == MA2).\
        filter(lambda p: p.ipv6inner.hlim == HL_2).\
        filter(lambda p: p.icmpv6.echo.identifier == ECHO_ID4).\
        filter(lambda p: p.lowpan.frag).\
        must_next()

    # Step 10
    # - Device: BR_1 (DUT)
    # - Description: Automatically forwards the ping request packet to the LAN.
    # - Pass Criteria:
    #   - The DUT MUST forward the multicast ICMPv6 Echo (ping) Request packet to the LAN with the Hop Limit field set
    #     to 1.
    print("Step 10: BR_1 forwards to LAN, HL=1")
    pkts.filter_eth_src(vars['BR_1_ETH']).\
        filter_ping_request(identifier=ECHO_ID4).\
        filter(lambda p: p.ipv6.dst == MA2).\
        filter(lambda p: p.ipv6.hlim == HL_1).\
        must_next()

    # Step 11
    # - Device: Router
    # - Description: Harness instructs the device to send an ICMPv6 Echo (ping) Request packet encapsulated in an MPL
    #   multicast packet to the multicast address, MA2, with the Hop Limit field of the inner (encapsulated) packet set
    #   to 1. The size of the payload is 130 bytes.
    # - Pass Criteria:
    #   - N/A
    print("Step 11: Router sends Echo Request in MPL to MA2, inner HL=1")
    pkts.filter_wpan_src64(ROUTER).\
        filter_AMPLFMA().\
        filter(lambda p: p.ipv6inner.dst == MA2).\
        filter(lambda p: p.ipv6inner.hlim == HL_1).\
        filter(lambda p: p.icmpv6.echo.identifier == ECHO_ID5).\
        filter(lambda p: p.lowpan.frag).\
        must_next()

    # Step 12
    # - Device: BR_1 (DUT)
    # - Description: Does not forward the ping request packet to the LAN.
    # - Pass Criteria:
    #   - The DUT MUST NOT forward the ICMPv6 Echo (ping) Request packet to the LAN.
    print("Step 12: BR_1 MUST NOT forward to LAN")
    pkts.copy().\
        filter_eth_src(vars['BR_1_ETH']).\
        filter_ping_request(identifier=ECHO_ID5).\
        must_not_next()

    # Step 13
    # - Device: Router
    # - Description: Harness instructs the device to send a ICMPv6 Echo (ping) Request packet encapsulated in an MPL
    #   multicast packet to the multicast address, MA2, with the Hop Limit field of the inner (encapsulated) packet set
    #   to 0.
    # - Pass Criteria:
    #   - N/A
    print("Step 13: Router sends Echo Request in MPL to MA2, inner HL=0")
    pkts.filter_wpan_src64(ROUTER).\
        filter_AMPLFMA().\
        filter(lambda p: p.ipv6inner.dst == MA2).\
        filter(lambda p: p.ipv6inner.hlim == HL_0).\
        filter(lambda p: p.icmpv6.echo.identifier == ECHO_ID6).\
        filter(lambda p: p.lowpan.frag).\
        must_next()

    # Step 14
    # - Device: BR_1 (DUT)
    # - Description: Does not forward the ping request packet to the LAN.
    # - Pass Criteria:
    #   - The DUT MUST NOT forward the ICMPv6 Echo (ping) Request packet to the LAN.
    print("Step 14: BR_1 MUST NOT forward to LAN")
    pkts.copy().\
        filter_eth_src(vars['BR_1_ETH']).\
        filter_ping_request(identifier=ECHO_ID6).\
        must_not_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
