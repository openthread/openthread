#!/usr/bin/python
#
#  Copyright (c) 2016, Nest Labs, Inc.
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

import pexpect
import time
import unittest

import node

LEADER = 1
ROUTER = 2
ED = 3

class Cert_5_6_8_ContextManagement(unittest.TestCase):
    def setUp(self):
        self.nodes = {}
        for i in range(1,4):
            self.nodes[i] = node.Node(i)

        self.nodes[LEADER].set_panid(0xface)
        self.nodes[LEADER].set_mode('rsdn')
        self.nodes[LEADER].add_whitelist(self.nodes[ROUTER].get_addr64())
        self.nodes[LEADER].add_whitelist(self.nodes[ED].get_addr64())
        self.nodes[LEADER].enable_whitelist()
        self.nodes[LEADER].set_context_reuse_delay(10)

        self.nodes[ROUTER].set_panid(0xface)
        self.nodes[ROUTER].set_mode('rsdn')
        self.nodes[ROUTER].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[ROUTER].enable_whitelist()

        self.nodes[ED].set_panid(0xface)
        self.nodes[ED].set_mode('rsn')
        self.nodes[ED].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[ED].enable_whitelist()

    def tearDown(self):
        for node in list(self.nodes.values()):
            node.stop()
        del self.nodes

    def test(self):
        self.nodes[LEADER].start()
        self.nodes[LEADER].set_state('leader')
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[ROUTER].start()
        time.sleep(3)
        self.assertEqual(self.nodes[ROUTER].get_state(), 'router')

        self.nodes[ED].start()
        time.sleep(3)
        self.assertEqual(self.nodes[ED].get_state(), 'child')

        self.nodes[ROUTER].add_prefix('2001::/64', 'paros')
        self.nodes[ROUTER].register_netdata()
        time.sleep(2)

        addrs = self.nodes[LEADER].get_addrs()
        for addr in addrs:
            if addr[0:3] == '200':
                self.nodes[ED].ping(addr)

        self.nodes[ROUTER].remove_prefix('2001::/64')
        self.nodes[ROUTER].register_netdata()
        time.sleep(5)

        addrs = self.nodes[LEADER].get_addrs()
        for addr in addrs:
            if addr[0:3] == '200':
                self.nodes[ED].ping(addr)

        self.nodes[ROUTER].add_prefix('2002::/64', 'paros')
        self.nodes[ROUTER].register_netdata()
        time.sleep(5)

        addrs = self.nodes[LEADER].get_addrs()
        for addr in addrs:
            if addr[0:3] == '200':
                self.nodes[ED].ping(addr)

        time.sleep(5)
        self.nodes[ROUTER].add_prefix('2003::/64', 'paros')
        self.nodes[ROUTER].register_netdata()
        time.sleep(5)

        addrs = self.nodes[LEADER].get_addrs()
        for addr in addrs:
            if addr[0:3] == '200':
                self.nodes[ED].ping(addr)

if __name__ == '__main__':
    unittest.main()
