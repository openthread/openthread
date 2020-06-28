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
ROUTER = 2
REED = 3


class Cert_5_6_7_NetworkDataRequestREED(thread_cert.TestCase):
    TOPOLOGY = {
        LEADER: {
            'mode': 'rsdn',
            'panid': 0xface,
            'whitelist': [ROUTER, REED]
        },
        ROUTER: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER]
        },
        REED: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_upgrade_threshold': 0,
            'whitelist': [LEADER]
        },
    }

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(4)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[ROUTER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER].get_state(), 'router')

        self.nodes[REED].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[REED].get_state(), 'child')

        self.nodes[LEADER].remove_whitelist(self.nodes[REED].get_addr64())
        self.nodes[REED].remove_whitelist(self.nodes[LEADER].get_addr64())

        self.nodes[ROUTER].add_prefix('2001:2:0:3::/64', 'paros')
        self.nodes[ROUTER].register_netdata()

        # Set lowpan context of sniffer
        self.simulator.set_lowpan_context(1, '2001:2:0:3::/64')

        self.simulator.go(2)

        self.nodes[LEADER].add_whitelist(self.nodes[REED].get_addr64())
        self.nodes[REED].add_whitelist(self.nodes[LEADER].get_addr64())

        self.simulator.go(35)

        addrs = self.nodes[REED].get_addrs()
        self.assertTrue(any('2001:2:0:3' in addr[0:10] for addr in addrs))
        for addr in addrs:
            if addr[0:10] == '2001:2:0:3':
                self.assertTrue(self.nodes[LEADER].ping(addr))


if __name__ == '__main__':
    unittest.main()
