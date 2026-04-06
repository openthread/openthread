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

GUA1_PREFIX = "2005:1234:abcd:100::/64"
ETH1_ADDR = "2005:1234:abcd:100::e1"
ETH2_ADDR = "2005:1234:abcd:100::e2"

THREADGROUP1 = "threadgroup1.org"
THREADGROUP2 = "threadgroup2.org"
THREADGROUP3 = "threadgroup3.org"

ANSWER1_ETH1 = "2002:1234::e1:1"
ANSWER2_ETH2 = "2002:1234::e2:2"
ANSWER3_ETH1 = "2002:1234::e1:3"

# ICMPv6 RA Option Types
ND_OPTION_PIO = 3
ND_OPTION_RDNSS = 25


def verify(pv):
    pkts = pv.pkts

    BR_1 = pv.vars['BR_1']
    Router_1 = pv.vars['Router_1']
    ED_1 = pv.vars['ED_1']
    Eth_1 = pv.vars['Eth_1']
    Eth_2 = pv.vars['Eth_2']

    BR_1_ETH_ADDR = pv.vars['BR_1_ETH']
    Eth_1_ETH_ADDR = pv.vars['Eth_1_ETH']
    Eth_2_ETH_ADDR = pv.vars['Eth_2_ETH']

    # Step 1
    # - Device: Eth_1, Eth_2
    # - Description (DNS-11.3): Enable
    # - Pass Criteria:
    #   - N/A
    print("Step 1: Enable Eth_1, Eth_2")

    # Step 2
    # - Device: Eth_2
    # - Description (DNS-11.3): Harness instructs device to: Configure router
    #   advertisement daemon (radvd) to multicast ND RA with: Prefix Information
    #   Option (PIO) with a global on-link prefix GUA_1 (SLAAC is enabled (A=1),
    #   on-link (L=1)), Recursive DNS Server Option (25), with Addresses of IPv6
    #   Recursive DNS Servers field contains a single global IPv6 address of Eth_1.
    #   Configure DNS server with records: threadgroup1.org AAAA 2002:1234::E2:1,
    #   threadgroup2.org AAAA 2002:1234::E2:2, threadgroup3.org AAAA 2002:1234::E2:3.
    #   Note: prefix GUA_1 may be 2005:1234:abcd:100::/64.
    # - Pass Criteria:
    #   - N/A
    print("Step 2: Eth_2 sends RA with global IPv6 address of Eth_1")
    pkts.filter_eth_src(Eth_2_ETH_ADDR).\
        filter_icmpv6_nd_ra().\
        filter(lambda p: ND_OPTION_PIO in p.icmpv6.opt.type).\
        filter(lambda p: ND_OPTION_RDNSS in p.icmpv6.opt.type).\
        must_next()

    # Step 3
    # - Device: Eth_1
    # - Description (DNS-11.3): Harness instructs device to: Configure DNS server
    #   with records: threadgroup1.org AAAA 2002:1234::E1:1, threadgroup2.org AAAA
    #   2002:1234::E1:2, threadgroup3.org AAAA 2002:1234::E1:3
    # - Pass Criteria:
    #   - N/A
    print("Step 3: Configure DNS records on Eth_1")

    # Step 4
    # - Device: BR_1 (DUT), Router_1, ED_1
    # - Description (DNS-11.3): Enable
    # - Pass Criteria:
    #   - Single Thread Network forms.
    print("Step 4: Enable BR_1, Router_1, ED_1")
    pkts.filter_wpan_src64(BR_1).\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        must_next()

    pkts.filter_wpan_src64(Router_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        must_next()

    pkts.filter_wpan_src64(ED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        must_next()

    # Step 5
    # - Device: BR_1 (DUT)
    # - Description (DNS-11.3): Automatically obtains / configures an OMR prefix for
    #   the Thread Network, and assigns an address for its AIL interface using SLAAC.
    # - Pass Criteria:
    #   - N/A
    print("Step 5: BR_1 configures OMR prefix and AIL address")

    # Step 6
    # - Device: ED_1
    # - Description (DNS-11.3): Harness instructs device to perform DNS query
    #   Qtype=AAAA, name threadgroup1.org. Automatically, the DNS query gets
    #   routed to the DUT
    # - Pass Criteria:
    #   - N/A
    print("Step 6: ED_1 performs DNS query for threadgroup1.org")
    pkts.filter(lambda p: 'udp' in p.layer_names and p.udp.dstport == 53).\
        must_next()

    # Step 7
    # - Device: BR_1 (DUT)
    # - Description (DNS-11.3): Automatically processes the DNS query by requesting
    #   upstream to the Eth_1 DNS server. Then, it responds back with the answer
    #   to ED_1.
    # - Pass Criteria:
    #   - N/A
    print("Step 7: BR_1 requests upstream to Eth_1")
    pkts.filter_eth_src(BR_1_ETH_ADDR).\
        filter(lambda p: p.eth.dst == Eth_1_ETH_ADDR).\
        filter_ipv6_dst(ETH1_ADDR).\
        filter(lambda p: 'udp' in p.layer_names and p.udp.dstport == 53).\
        must_next()

    # Step 8
    # - Device: ED_1
    # - Description (DNS-11.3): Successfully receives DNS query result:
    #   threadgroup1.org AAAA 2002:1234::E1:1
    # - Pass Criteria:
    #   - ED_1 MUST receive correct DNS query answer from the DUT.
    print("Step 8: ED_1 receives correct answer")
    pkts.filter_eth_src(Eth_1_ETH_ADDR).\
        filter(lambda p: p.eth.dst == BR_1_ETH_ADDR).\
        must_next()

    # Step 9
    # - Device: Eth_2
    # - Description (DNS-11.3): Harness instructs device to configure router
    #   advertisement daemon (radvd) to multicast ND RA with updated information,
    #   pointing to another DNS server: Prefix Information Option (PIO) - as
    #   before, Recursive DNS Server Option (25), with Addresses of IPv6
    #   Recursive DNS Servers field contains a single global IPv6 address of
    #   Eth_2. Harness waits for a time of at least RA_PERIOD + 1 seconds, where
    #   RA_PERIOD is the max period between multicast ND RA transmissions by
    #   Eth_2, to ensure that the new ND RA is received by DUT.
    # - Pass Criteria:
    #   - N/A
    print("Step 9: Eth_2 sends RA with global IPv6 address of Eth_2")
    pkts.filter_eth_src(Eth_2_ETH_ADDR).\
        filter_icmpv6_nd_ra().\
        filter(lambda p: ND_OPTION_PIO in p.icmpv6.opt.type).\
        filter(lambda p: ND_OPTION_RDNSS in p.icmpv6.opt.type).\
        must_next()

    # Step 10
    # - Device: ED_1
    # - Description (DNS-11.3): Harness instructs device to perform DNS query
    #   Qtype=AAAA, name threadgroup2.org. Automatically, the DNS query gets
    #   routed to the DUT.
    # - Pass Criteria:
    #   - N/A
    print("Step 10: ED_1 performs DNS query for threadgroup2.org")
    pkts.filter(lambda p: 'udp' in p.layer_names and p.udp.dstport == 53).\
        must_next()

    # Step 11
    # - Device: BR_1 (DUT)
    # - Description (DNS-11.3): Automatically processes the DNS query by requesting
    #   upstream to the Eth_2 DNS server. Then, it responds back with the answer
    #   to ED_1.
    # - Pass Criteria:
    #   - N/A
    print("Step 11: BR_1 requests upstream to Eth_2")
    pkts.filter_eth_src(BR_1_ETH_ADDR).\
        filter(lambda p: p.eth.dst == Eth_2_ETH_ADDR).\
        filter_ipv6_dst(ETH2_ADDR).\
        filter(lambda p: 'udp' in p.layer_names and p.udp.dstport == 53).\
        must_next()

    # Step 12
    # - Device: ED_1
    # - Description (DNS-11.3): Successfully receives DNS query result:
    #   threadgroup2.org AAAA 2002:1234::E2:2
    # - Pass Criteria:
    #   - ED_1 MUST receive correct DNS query answer from the DUT.
    print("Step 12: ED_1 receives correct answer")
    pkts.filter_eth_src(Eth_2_ETH_ADDR).\
        filter(lambda p: p.eth.dst == BR_1_ETH_ADDR).\
        must_next()

    # Step 13
    # - Device: Eth_2
    # - Description (DNS-11.3): Harness instructs device to configure router
    #   advertisement daemon (radvd) to multicast ND RA with updated information,
    #   pointing to the first DNS server using link-local address: Prefix
    #   Information Option (PIO) - as before, Recursive DNS Server Option (25),
    #   with Addresses of IPv6 Recursive DNS Servers field contains a single
    #   link-local IPv6 address of Eth_1. Harness waits for a time of at least
    #   RA_PERIOD + 1 seconds, where RA_PERIOD is the max period between
    #   multicast ND RA transmissions by Eth_2, to ensure that the new RA is
    #   received by DUT.
    # - Pass Criteria:
    #   - N/A
    print("Step 13: Eth_2 sends RA with link-local IPv6 address of Eth_1")
    pkts.filter_eth_src(Eth_2_ETH_ADDR).\
        filter_icmpv6_nd_ra().\
        filter(lambda p: ND_OPTION_PIO in p.icmpv6.opt.type).\
        filter(lambda p: ND_OPTION_RDNSS in p.icmpv6.opt.type).\
        must_next()

    # Step 14
    # - Device: ED_1
    # - Description (DNS-11.3): Harness instructs device to perform DNS query
    #   Qtype=AAAA, name threadgroup3.org. Automatically, the DNS query gets
    #   routed to the DUT
    # - Pass Criteria:
    #   - N/A
    print("Step 14: ED_1 performs DNS query for threadgroup3.org")
    pkts.filter(lambda p: 'udp' in p.layer_names and p.udp.dstport == 53).\
        must_next()

    # Step 15
    # - Device: BR_1 (DUT)
    # - Description (DNS-11.3): Automatically processes the DNS query by requesting
    #   upstream to the Eth_1 DNS server. Then, it responds back with the answer
    #   to ED_1.
    # - Pass Criteria:
    #   - N/A
    print("Step 15: BR_1 requests upstream to Eth_1")
    pkts.filter_eth_src(BR_1_ETH_ADDR).\
        filter(lambda p: p.eth.dst == Eth_1_ETH_ADDR).\
        filter(lambda p: 'udp' in p.layer_names and p.udp.dstport == 53).\
        must_next()

    # Step 16
    # - Device: ED_1
    # - Description (DNS-11.3): Successfully receives DNS query result:
    #   threadgroup3.org AAAA 2002:1234::E1:3
    # - Pass Criteria:
    #   - ED_1 MUST receive correct DNS query answer from the DUT.
    print("Step 16: ED_1 receives correct answer")
    pkts.filter_eth_src(Eth_1_ETH_ADDR).\
        filter(lambda p: p.eth.dst == BR_1_ETH_ADDR).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
