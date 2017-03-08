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

LEADER = 1
ROUTER = 2
SED1 = 6

class Cert_5_1_07_MaxChildCount(unittest.TestCase):
    def setUp(self):
        self.nodes = {}
        for i in range(1, 13):
            self.nodes[i] = node.Node(i)

        self.nodes[LEADER].set_panid(0xface)
        self.nodes[LEADER].set_mode('rsdn')
        self.nodes[LEADER].add_whitelist(self.nodes[ROUTER].get_addr64())
        self.nodes[LEADER].enable_whitelist()

        self.nodes[ROUTER].set_panid(0xface)
        self.nodes[ROUTER].set_mode('rsdn')
        self.nodes[ROUTER].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[ROUTER].enable_whitelist()
        self.nodes[ROUTER].set_router_selection_jitter(1)
        self.nodes[ROUTER].set_max_children(10)

        for i in range(3, 13):
            if i in range(3, 7):
                self.nodes[i].set_mode('rsn')
            else:
                self.nodes[i].set_mode('s')
            self.nodes[i].set_panid(0xface)
            self.nodes[i].add_whitelist(self.nodes[ROUTER].get_addr64())
            self.nodes[ROUTER].add_whitelist(self.nodes[i].get_addr64())
            self.nodes[i].enable_whitelist()
            self.nodes[i].set_timeout(3)

    def tearDown(self):
        for node in list(self.nodes.values()):
            node.stop()
        del self.nodes

    def test(self):
        self.nodes[LEADER].start()
        self.nodes[LEADER].set_state('leader')
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')
        time.sleep(4)

        self.nodes[ROUTER].start()
        time.sleep(5)
        self.assertEqual(self.nodes[ROUTER].get_state(), 'router')

        for i in range(3, 13):
            self.nodes[i].start()
            time.sleep(7)
            self.assertEqual(self.nodes[i].get_state(), 'child')

        ipaddrs = self.nodes[SED1].get_addrs()
        for addr in ipaddrs:
            if addr[0:4] != 'fe80' and 'ff:fe00' not in addr:
                self.assertTrue(self.nodes[LEADER].ping(addr, size=1232))
                break

        for i in range(3, 13):
            ipaddrs = self.nodes[i].get_addrs()
            for addr in ipaddrs:
                if addr[0:4] != 'fe80' and 'ff:fe00' not in addr:
                    self.assertTrue(self.nodes[LEADER].ping(addr, size=106))
                    break
        
if __name__ == '__main__':
    unittest.main()
