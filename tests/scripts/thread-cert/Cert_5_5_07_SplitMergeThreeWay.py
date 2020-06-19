#!/usr/bin/env python3
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

import unittest

import thread_cert

LEADER1 = 1
ROUTER1 = 2
ROUTER2 = 3
ROUTER3 = 4


class Cert_5_5_7_SplitMergeThreeWay(thread_cert.TestCase):
    TOPOLOGY = {
        LEADER1: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [ROUTER1, ROUTER2, ROUTER3]
        },
        ROUTER1: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER1]
        },
        ROUTER2: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER1]
        },
        ROUTER3: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER1]
        },
    }

    def _setUpLeader1(self):
        self.nodes[LEADER1].add_whitelist(self.nodes[ROUTER1].get_addr64())
        self.nodes[LEADER1].add_whitelist(self.nodes[ROUTER2].get_addr64())
        self.nodes[LEADER1].add_whitelist(self.nodes[ROUTER3].get_addr64())
        self.nodes[LEADER1].enable_whitelist()
        self.nodes[LEADER1].set_router_selection_jitter(1)

    def test(self):
        self.nodes[LEADER1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER1].get_state(), 'leader')

        self.nodes[ROUTER1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER1].get_state(), 'router')

        self.nodes[ROUTER2].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER2].get_state(), 'router')

        self.nodes[ROUTER3].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER3].get_state(), 'router')

        self.nodes[LEADER1].reset()
        self._setUpLeader1()
        self.simulator.go(140)

        self.nodes[LEADER1].start()
        self.simulator.go(30)

        addrs = self.nodes[LEADER1].get_addrs()
        for addr in addrs:
            if addr[0:4] != 'fe80':
                self.assertTrue(self.nodes[ROUTER1].ping(addr))

        addrs = self.nodes[ROUTER2].get_addrs()
        for addr in addrs:
            if addr[0:4] != 'fe80':
                self.assertTrue(self.nodes[ROUTER1].ping(addr))

        addrs = self.nodes[ROUTER3].get_addrs()
        for addr in addrs:
            if addr[0:4] != 'fe80':
                self.assertTrue(self.nodes[ROUTER1].ping(addr))


if __name__ == '__main__':
    unittest.main()
