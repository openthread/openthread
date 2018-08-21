#!/usr/bin/env python
#
#  Copyright (c) 2016, The OpenThread Authors.
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

import time
import unittest

import node
import config
import command

LEADER = 1
DUT_ROUTER1 = 2
ROUTER2 = 3
ROUTER3 = 4

class Cert_5_3_5_RoutingLinkQuality(unittest.TestCase):
    def setUp(self):
        self.simulator = config.create_default_simulator()

        self.nodes = {}
        for i in range(1,5):
            self.nodes[i] = node.Node(i, simulator=self.simulator)

        self.nodes[LEADER].set_panid(0xface)
        self.nodes[LEADER].set_mode('rsdn')
        self.nodes[LEADER].add_whitelist(self.nodes[DUT_ROUTER1].get_addr64())
        self.nodes[LEADER].add_whitelist(self.nodes[ROUTER2].get_addr64())
        self.nodes[LEADER].enable_whitelist()

        self.nodes[DUT_ROUTER1].set_panid(0xface)
        self.nodes[DUT_ROUTER1].set_mode('rsdn')
        self.nodes[DUT_ROUTER1].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[DUT_ROUTER1].add_whitelist(self.nodes[ROUTER2].get_addr64())
        self.nodes[DUT_ROUTER1].add_whitelist(self.nodes[ROUTER3].get_addr64())
        self.nodes[DUT_ROUTER1].enable_whitelist()
        self.nodes[DUT_ROUTER1].set_router_selection_jitter(1)

        self.nodes[ROUTER2].set_panid(0xface)
        self.nodes[ROUTER2].set_mode('rsdn')
        self.nodes[ROUTER2].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[ROUTER2].add_whitelist(self.nodes[DUT_ROUTER1].get_addr64())
        self.nodes[ROUTER2].enable_whitelist()
        self.nodes[ROUTER2].set_router_selection_jitter(1)

        self.nodes[ROUTER3].set_panid(0xface)
        self.nodes[ROUTER3].set_mode('rsdn')
        self.nodes[ROUTER3].add_whitelist(self.nodes[DUT_ROUTER1].get_addr64())
        self.nodes[ROUTER3].enable_whitelist()
        self.nodes[ROUTER3].set_router_selection_jitter(1)

    def tearDown(self):
        for node in list(self.nodes.values()):
            node.stop()
            node.destroy()
        self.simulator.stop()

    def test(self):
        # 1
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        for router in range(DUT_ROUTER1, ROUTER3 + 1):
            self.nodes[router].start()
        self.simulator.go(10)

        for router in range(DUT_ROUTER1, ROUTER3 + 1):
            self.assertEqual(self.nodes[router].get_state(), 'router')

        # 2 & 3
        leader_rloc = self.nodes[LEADER].get_ip6_address(config.ADDRESS_TYPE.RLOC)

        # Verify the ICMPv6 Echo Request took the least cost path.
        self.assertTrue(self.nodes[ROUTER3].ping(leader_rloc))
        path = [ROUTER3, DUT_ROUTER1, LEADER]
        command.check_icmp_path(self.simulator, path, self.nodes)

        # 4 & 5
        self.nodes[LEADER].add_whitelist(self.nodes[DUT_ROUTER1].get_addr64(), config.RSSI['LINK_QULITY_1'])
        self.nodes[DUT_ROUTER1].add_whitelist(self.nodes[LEADER].get_addr64(), config.RSSI['LINK_QULITY_1'])
        self.simulator.go(3 * config.MAX_ADVERTISEMENT_INTERVAL)

        # Verify the ICMPv6 Echo Request took the longer path because it cost less.
        self.assertTrue(self.nodes[ROUTER3].ping(leader_rloc))
        path = [ROUTER3, DUT_ROUTER1, ROUTER2, LEADER]
        command.check_icmp_path(self.simulator, path, self.nodes)

        # 6 & 7
        self.nodes[LEADER].add_whitelist(self.nodes[DUT_ROUTER1].get_addr64(), config.RSSI['LINK_QULITY_2'])
        self.nodes[DUT_ROUTER1].add_whitelist(self.nodes[LEADER].get_addr64(), config.RSSI['LINK_QULITY_2'])
        self.simulator.go(3 * config.MAX_ADVERTISEMENT_INTERVAL)

        # Verify the direct neighbor would be prioritized when there are two paths with the same cost.
        self.assertTrue(self.nodes[ROUTER3].ping(leader_rloc))
        path = [ROUTER3, DUT_ROUTER1, LEADER]
        command.check_icmp_path(self.simulator, path, self.nodes)

        # 8 & 9
        self.nodes[LEADER].add_whitelist(self.nodes[DUT_ROUTER1].get_addr64(), config.RSSI['LINK_QULITY_0'])
        self.nodes[DUT_ROUTER1].add_whitelist(self.nodes[LEADER].get_addr64(), config.RSSI['LINK_QULITY_0'])
        self.simulator.go(3 * config.MAX_ADVERTISEMENT_INTERVAL)

        # Verify the ICMPv6 Echo Request took the longer path.
        leader_rloc = self.nodes[LEADER].get_ip6_address(config.ADDRESS_TYPE.RLOC)
        self.assertTrue(self.nodes[ROUTER3].ping(leader_rloc))
        path = [ROUTER3, DUT_ROUTER1, ROUTER2, LEADER]
        command.check_icmp_path(self.simulator, path, self.nodes)

if __name__ == '__main__':
    unittest.main()
