#!/usr/bin/env python3
#
#  Copyright (c) 2020, The OpenThread Authors.
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
ROUTER = 2
ED = 3
SED = 4


class TestReset(thread_cert.TestCase):
    TOPOLOGY = {
        LEADER: {
            'mode': 'rsdn',
            'panid': 0xface,
            'allowlist': [ROUTER]
        },
        ROUTER: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'allowlist': [LEADER, ED]
        },
        ED: {
            'is_mtd': True,
            'mode': 'rsn',
            'panid': 0xface,
            'allowlist': [ROUTER]
        },
    }

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[ROUTER].start()
        self.simulator.go(7)
        self.assertEqual(self.nodes[ROUTER].get_state(), 'router')

        self.nodes[ED].start()
        self.simulator.go(7)
        self.assertEqual(self.nodes[ED].get_state(), 'child')

        leader_addrs = self.nodes[LEADER].get_addrs()
        router_addrs = self.nodes[ROUTER].get_addrs()

        for i in range(0, 1010):
            self.assertTrue(self.nodes[ED].ping(leader_addrs[0]))
        self.simulator.go(1)

        # 1 - Leader
        self.nodes[LEADER].reset()
        self.nodes[LEADER].start()
        self.simulator.go(7)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        for addr in router_addrs:
            self.assertTrue(self.nodes[LEADER].ping(addr))

        # 2 - Router
        self.nodes[ROUTER].reset()
        self.nodes[ROUTER].start()
        self.simulator.go(7)
        self.assertEqual(self.nodes[ROUTER].get_state(), 'router')

        for addr in leader_addrs:
            self.assertTrue(self.nodes[ROUTER].ping(addr))

        # 3 - Child
        self.nodes[ED].reset()
        self.nodes[ED].start()
        self.simulator.go(7)
        self.assertEqual(self.nodes[ED].get_state(), 'child')

        for addr in router_addrs:
            self.assertTrue(self.nodes[ED].ping(addr))


if __name__ == '__main__':
    unittest.main()
