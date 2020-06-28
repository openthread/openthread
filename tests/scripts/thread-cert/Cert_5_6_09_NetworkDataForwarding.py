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

import config
import thread_cert

LEADER = 1
ROUTER1 = 2
ROUTER2 = 3
ED = 4
SED = 5

MTDS = [ED, SED]


class Cert_5_6_9_NetworkDataForwarding(thread_cert.TestCase):
    TOPOLOGY = {
        LEADER: {
            'mode': 'rsdn',
            'panid': 0xface,
            'whitelist': [ROUTER1, ROUTER2]
        },
        ROUTER1: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER, ED, SED]
        },
        ROUTER2: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER]
        },
        ED: {
            'is_mtd': True,
            'mode': 'rsn',
            'panid': 0xface,
            'whitelist': [ROUTER1]
        },
        SED: {
            'is_mtd': True,
            'mode': 's',
            'panid': 0xface,
            'timeout': config.DEFAULT_CHILD_TIMEOUT,
            'whitelist': [ROUTER1]
        },
    }

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(4)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[ROUTER1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER1].get_state(), 'router')

        self.nodes[ROUTER2].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER2].get_state(), 'router')

        self.nodes[ED].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ED].get_state(), 'child')

        self.nodes[SED].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[SED].get_state(), 'child')

        self.nodes[LEADER].add_prefix('2001:2:0:1::/64', 'paros', 'med')
        self.nodes[LEADER].add_route('2001:2:0:2::/64', 'med')
        self.nodes[LEADER].register_netdata()

        # Set lowpan context of sniffer
        self.simulator.set_lowpan_context(1, '2001:2:0:1::/64')

        self.simulator.go(10)

        self.nodes[ROUTER2].add_prefix('2001:2:0:1::/64', 'paros', 'low')
        self.nodes[ROUTER2].add_route('2001:2:0:2::/64', 'high')
        self.nodes[ROUTER2].register_netdata()
        self.simulator.go(10)

        self.assertFalse(self.nodes[SED].ping('2001:2:0:2::1'))

        self.assertFalse(self.nodes[SED].ping('2007::1'))

        self.nodes[ROUTER2].remove_prefix('2001:2:0:1::/64')
        self.nodes[ROUTER2].add_prefix('2001:2:0:1::/64', 'paros', 'high')
        self.nodes[ROUTER2].register_netdata()
        self.simulator.go(10)

        self.assertFalse(self.nodes[SED].ping('2007::1'))

        self.nodes[ROUTER2].remove_prefix('2001:2:0:1::/64')
        self.nodes[ROUTER2].add_prefix('2001:2:0:1::/64', 'paros', 'med')
        self.nodes[ROUTER2].register_netdata()
        self.simulator.go(10)

        self.assertFalse(self.nodes[SED].ping('2007::1'))


if __name__ == '__main__':
    unittest.main()
