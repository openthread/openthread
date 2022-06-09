#!/usr/bin/env python3
#
#  Copyright (c) 2022, The OpenThread Authors.
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
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS'
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
import ipaddress
import unittest

import config
import thread_cert
from pktverify.consts import ICMPV6_RA_OPT_TYPE_RIO
from pktverify.packet_verifier import PacketVerifier

# Test description:
# The purpose of this test case is to verify that BR can handle manually added OMR prefixes properly.
#
# Topology:
#     BR_1 (Leader)
#       |
#       |
#     BR_2
#

BR_1 = 1
BR_2 = 2


class ManualOmrsPrefix(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR_1: {
            'name': 'BR_1',
            'is_otbr': True,
            'version': '1.2',
        },
        BR_2: {
            'name': 'BR_2',
            'is_otbr': True,
            'version': '1.2',
        },
    }

    def test(self):
        br1 = self.nodes[BR_1]
        br2 = self.nodes[BR_2]

        br1.start()
        self.simulator.go(config.BORDER_ROUTER_STARTUP_DELAY)
        self.assertEqual('leader', br1.get_state())

        self.simulator.go(10)
        self.assertEqual(br1.get_netdata_omr_prefixes(), [br1.get_br_omr_prefix()])

        br2.disable_br()
        br2.start()
        self.simulator.go(config.BORDER_ROUTER_STARTUP_DELAY)
        self.assertEqual('router', br2.get_state())
        self.assertEqual(br1.get_netdata_omr_prefixes(), [br1.get_br_omr_prefix()])

        # Add a smaller OMR prefix. Verify BR_1 withdraws its OMR prefix.
        self._test_manual_omr_prefix('2001::/64', 'paros', expect_withdraw=True, prf='low')
        # Add a bigger OMR prefix. Verify BR_1 does not withdraw its OMR prefix.
        self._test_manual_omr_prefix('fdff:ffff:1::/64', 'paros', expect_withdraw=False, prf='low')
        # Add a high preference bigger OMR prefix. Verify BR_1 withdraws its OMR prefix.
        self._test_manual_omr_prefix('fdff:ffff:2::/64', 'paros', expect_withdraw=True, prf='med')
        # Add a smaller but invalid OMR prefix (P_on_mesh = 0). Verify BR_1 does not withdraw its OMR prefix.
        self._test_manual_omr_prefix('2002::/64', 'pars', expect_withdraw=False)
        # Add a smaller but invalid OMR prefix (P_stable = 0). Verify BR_1 does not withdraw its OMR prefix.
        self._test_manual_omr_prefix('2003::/64', 'paro', expect_withdraw=False)
        # Add a deprecating OMR prefix (P_preferred = 0). Verify BR_1 does not withdraw its OMR prefix.
        self._test_manual_omr_prefix('2004::/64', 'aros', expect_withdraw=False)

        self.collect_ipaddrs()
        self.collect_extra_vars(BR_1_OMR_PREFIX=br1.get_br_omr_prefix())

    def verify(self, pv: PacketVerifier):
        pkts = pv.pkts
        pv.summary.show()

        BR_1_ETH = pv.vars['BR_1_ETH']

        BR_1_OMR_PREFIX = pv.vars['BR_1_OMR_PREFIX']
        assert BR_1_OMR_PREFIX.endswith('::/64')
        BR_1_OMR_PREFIX = BR_1_OMR_PREFIX[:-3]

        # Filter all ICMPv6 RA messages advertised by BR_1
        ra_pkts = pkts.filter_eth_src(BR_1_ETH).filter_icmpv6_nd_ra()

        # Verify BR_1 sends RA RIO for both BR OMR prefix and deprecating OMR prefix (i.e. 2004::/64)
        ra_pkts.filter(lambda p: ICMPV6_RA_OPT_TYPE_RIO in p.icmpv6.opt.type and '2004::' in p.icmpv6.opt.prefix and
                       BR_1_OMR_PREFIX in p.icmpv6.opt.prefix).must_next()

    def _test_manual_omr_prefix(self, prefix, flags, expect_withdraw, prf='med'):
        br1 = self.nodes[BR_1]
        br2 = self.nodes[BR_2]

        br2.add_prefix(prefix, flags, prf=prf)
        br2.register_netdata()
        self.simulator.go(10)

        is_omr_prefix = ('a' in flags and 'o' in flags and 's' in flags and 'D' not in flags)

        if expect_withdraw:
            self._assert_prefixes_equal(br1.get_netdata_omr_prefixes(), [prefix])
        elif is_omr_prefix:
            self._assert_prefixes_equal(br1.get_netdata_omr_prefixes(), [br1.get_br_omr_prefix(), prefix])
        else:
            self._assert_prefixes_equal(br1.get_netdata_omr_prefixes(), [br1.get_br_omr_prefix()])

        br2.remove_prefix(prefix)
        br2.register_netdata()
        self.simulator.go(10)
        self._assert_prefixes_equal(br1.get_netdata_omr_prefixes(), [br1.get_br_omr_prefix()])
        self.simulator.go(3)

    def _assert_prefixes_equal(self, prefixes1, prefixes2):
        prefixes1 = sorted([ipaddress.IPv6Network(p) for p in prefixes1])
        prefixes2 = sorted([ipaddress.IPv6Network(p) for p in prefixes2])
        self.assertEqual(prefixes1, prefixes2)


if __name__ == '__main__':
    unittest.main()
