#!/usr/bin/python
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

ED1 = 1
BR1 = 2
LEADER = 3
ROUTER2 = 4
REED = 5
ED2 = 6
ED3 = 7

class Cert_5_2_5_AddressQuery(unittest.TestCase):
    def setUp(self):
        self.nodes = {}
        for i in range(1,8):
            self.nodes[i] = node.Node(i)

        self.nodes[LEADER].set_panid(0xface)
        self.nodes[LEADER].set_mode('rsdn')
        self.nodes[LEADER].add_whitelist(self.nodes[BR1].get_addr64())
        self.nodes[LEADER].add_whitelist(self.nodes[ROUTER2].get_addr64())
        self.nodes[LEADER].add_whitelist(self.nodes[REED].get_addr64())
        self.nodes[LEADER].enable_whitelist()

        self.nodes[BR1].set_panid(0xface)
        self.nodes[BR1].set_mode('rsdn')
        self.nodes[BR1].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[BR1].add_whitelist(self.nodes[ED1].get_addr64())
        self.nodes[BR1].enable_whitelist()

        self.nodes[ED1].set_panid(0xface)
        self.nodes[ED1].set_mode('rsn')
        self.nodes[ED1].add_whitelist(self.nodes[BR1].get_addr64())
        self.nodes[ED1].enable_whitelist()

        self.nodes[REED].set_panid(0xface)
        self.nodes[REED].set_mode('rsdn')
        self.nodes[REED].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[REED].add_whitelist(self.nodes[ROUTER2].get_addr64())
        self.nodes[REED].set_router_upgrade_threshold(0)
        self.nodes[REED].enable_whitelist()

        self.nodes[ROUTER2].set_panid(0xface)
        self.nodes[ROUTER2].set_mode('rsdn')
        self.nodes[ROUTER2].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[ROUTER2].add_whitelist(self.nodes[REED].get_addr64())
        self.nodes[ROUTER2].add_whitelist(self.nodes[ED2].get_addr64())
        self.nodes[ROUTER2].add_whitelist(self.nodes[ED3].get_addr64())
        self.nodes[ROUTER2].enable_whitelist()

        self.nodes[ED2].set_panid(0xface)
        self.nodes[ED2].set_mode('rsn')
        self.nodes[ED2].add_whitelist(self.nodes[ROUTER2].get_addr64())
        self.nodes[ED2].enable_whitelist()

        self.nodes[ED3].set_panid(0xface)
        self.nodes[ED3].set_mode('rsn')
        self.nodes[ED3].add_whitelist(self.nodes[ROUTER2].get_addr64())
        self.nodes[ED3].enable_whitelist()

    def tearDown(self):
        for node in list(self.nodes.values()):
            node.stop()
        del self.nodes

    def test(self):
        self.nodes[LEADER].start()
        self.nodes[LEADER].set_state('leader')
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[BR1].start()
        time.sleep(3)
        self.assertEqual(self.nodes[BR1].get_state(), 'router')

        self.nodes[BR1].add_prefix('2003::/64', 'paros')
        self.nodes[BR1].add_prefix('2004::/64', 'paros')
        self.nodes[BR1].register_netdata()

        self.nodes[ED1].start()
        time.sleep(3)
        self.assertEqual(self.nodes[ED1].get_state(), 'child')

        self.nodes[REED].start()
        time.sleep(3)
        self.assertEqual(self.nodes[REED].get_state(), 'child')

        self.nodes[ROUTER2].start()
        time.sleep(3)
        self.assertEqual(self.nodes[ROUTER2].get_state(), 'router')

        self.nodes[ED2].start()
        time.sleep(3)
        self.assertEqual(self.nodes[ED2].get_state(), 'child')

        self.nodes[ED3].start()
        time.sleep(3)
        self.assertEqual(self.nodes[ED3].get_state(), 'child')

        addrs = self.nodes[REED].get_addrs()
        for addr in addrs:
            if addr[0:4] != 'fe80':
                self.assertTrue(self.nodes[ED2].ping(addr))
                time.sleep(1)

if __name__ == '__main__':
    unittest.main()
