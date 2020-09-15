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

LEADER = 1
ROUTER1 = 2
ROUTER2 = 3
ED1 = 4
ED2 = 5
ED3 = 6

MTDS = [ED1, ED2, ED3]


class Cert_5_5_3_SplitMergeChildren(thread_cert.TestCase):
    TOPOLOGY = {
        LEADER: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'allowlist': [ROUTER1, ROUTER2, ED1]
        },
        ROUTER1: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'allowlist': [LEADER, ED2, ED3]
        },
        ROUTER2: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'allowlist': [LEADER]
        },
        ED1: {
            'is_mtd': True,
            'mode': 'rsn',
            'panid': 0xface,
            'allowlist': [LEADER]
        },
        ED2: {
            'is_mtd': True,
            'mode': 'rsn',
            'panid': 0xface,
            'allowlist': [ROUTER1]
        },
        ED3: {
            'is_mtd': True,
            'mode': 'rsn',
            'panid': 0xface,
            'allowlist': [ROUTER1]
        },
    }

    def _setUpLeader(self):
        self.nodes[LEADER].add_allowlist(self.nodes[ROUTER1].get_addr64())
        self.nodes[LEADER].add_allowlist(self.nodes[ROUTER2].get_addr64())
        self.nodes[LEADER].add_allowlist(self.nodes[ED1].get_addr64())
        self.nodes[LEADER].enable_allowlist()
        self.nodes[LEADER].set_router_selection_jitter(1)

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[ROUTER1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER1].get_state(), 'router')

        self.nodes[ROUTER2].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER2].get_state(), 'router')

        self.nodes[ED1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ED1].get_state(), 'child')

        self.nodes[ED2].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ED2].get_state(), 'child')

        self.nodes[ED3].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ED3].get_state(), 'child')

        self.nodes[LEADER].reset()
        self._setUpLeader()

        self.nodes[ED1].add_allowlist(self.nodes[ROUTER1].get_addr64())
        self.nodes[ROUTER1].add_allowlist(self.nodes[ED1].get_addr64())

        self.simulator.go(140)
        self.assertEqual(self.nodes[ROUTER1].get_state(), 'leader')
        self.assertEqual(self.nodes[ROUTER2].get_state(), 'leader')

        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'router')

        self.simulator.go(30)

        addrs = self.nodes[ED1].get_addrs()
        for addr in addrs:
            if addr[0:4] != 'fe80':
                self.assertTrue(self.nodes[ROUTER2].ping(addr))


if __name__ == '__main__':
    unittest.main()
