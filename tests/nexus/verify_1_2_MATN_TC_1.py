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
    # 5.10.1 MATN-TC-01: Default blocking of multicast from backbone
    #
    # 5.10.1.1 Topology
    # - BR_1 (DUT)
    # - Router
    #
    # 5.10.1.2 Purpose & Description
    # Verify that a Primary BBR by default blocks IPv6 multicast from the backbone to its Thread Network when no
    #   devices have registered for multicast groups.
    #
    # Spec Reference | V1.2 Section
    # ---------------|-------------
    # Multicast      | 5.10.1

    pkts = pv.pkts
    pv.summary.show()

    BR_1 = pv.vars['BR_1']
    MA1 = pv.vars['MA1']
    MA2 = pv.vars['MA2']
    MA3 = pv.vars['MA3']
    ALL_MPL_FORWARDERS = pv.vars['ALL_MPL_FORWARDERS']
    MA6 = pv.vars['MA6']

    # Step 0
    # - Device: N/A
    # - Description: Topology - BR_1 (DUT), Router
    # - Pass Criteria:
    #   - N/A
    print("Step 0: Topology - BR_1 (DUT), Router")

    test_cases = [
        ('1', MA1, 'multicast address MA1'),
        ('2', MA2, 'multicast address MA2 (site-local scope)'),
        ('3', MA3, 'multicast address MA3 (global scope)'),
        ('4', ALL_MPL_FORWARDERS, 'site-local scope multicast address ALL_MPL_FORWARDERS'),
        ('5', MA6, 'link local address MA6'),
    ]

    for step, addr, desc in test_cases:
        print(f"Step {step}: Host sends ICMPv6 Echo (ping) Request packet to the {desc}.")
        print(f"Step {step}a: BR_1 does not forward the ping request packet to its Thread Network.")
        pkts.filter_wpan_src64(BR_1).\
            filter_ping_request().\
            filter(lambda p, addr=addr: p.ipv6.dst == Ipv6Addr(addr)).\
            must_not_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
