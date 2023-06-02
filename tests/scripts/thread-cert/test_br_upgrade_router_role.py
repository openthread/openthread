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

import ipaddress
import unittest

import command
import config
import thread_cert

# Test description:
#
# This test verifies behavior of Border Routers (which provide IP connectivity) requesting router role within Thread
# mesh.

# Topology:
#
#  5 FTD nodes, all connected.
#
#  Topology is expected to change during execution of test.
#

LEADER = 1
ROUTER = 2
BR1 = 3
BR2 = 4
BR3 = 5


class BrUpgradeRouterRole(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False
    SUPPORT_NCP = False

    TOPOLOGY = {
        LEADER: {
            'name': 'leader',
            'mode': 'rdn',
        },
        ROUTER: {
            'name': 'reader',
            'mode': 'rdn',
        },
        BR1: {
            'name': 'br-1',
            'mode': 'rdn',
        },
        BR2: {
            'name': 'br-2',
            'mode': 'rdn',
        },
        BR3: {
            'name': 'br-3',
            'mode': 'rdn',
        },
    }

    def test(self):
        leader = self.nodes[LEADER]
        router = self.nodes[ROUTER]
        br1 = self.nodes[BR1]
        br2 = self.nodes[BR2]
        br3 = self.nodes[BR3]

        #-------------------------------------------------------------------------------------
        # Set the router upgrade threshold to 2 on all nodes.

        for node in [leader, router, br1, br2, br3]:
            node.set_router_upgrade_threshold(2)

        #-------------------------------------------------------------------------------------
        # Start the leader and router.

        leader.start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual(leader.get_state(), 'leader')

        router.start()
        self.simulator.go(125)
        self.assertEqual(router.get_state(), 'router')

        #-------------------------------------------------------------------------------------
        # Start all three BRs, we expect all to stay as child since there are already two
        # routers in the Thread mesh.

        br1.start()
        br2.start()
        br3.start()
        self.simulator.go(125)
        self.assertEqual(br1.get_state(), 'child')
        self.assertEqual(br2.get_state(), 'child')
        self.assertEqual(br3.get_state(), 'child')

        #-------------------------------------------------------------------------------------
        # Add an external route on `br1`, it should now try to become a router requesting
        # with status BorderRouterRequest. Verify that leader allows it to become router.

        br1.add_route('2001:dead:beef:cafe::/64', stable=True)
        br1.register_netdata()
        self.simulator.go(15)
        self.assertEqual(br1.get_state(), 'router')

        #-------------------------------------------------------------------------------------
        # Add a prefix with default route on `br2`, it should also become a router.

        br2.add_prefix('2001:dead:beef:2222::/64', 'paros')
        br2.register_netdata()
        self.simulator.go(15)
        self.assertEqual(br2.get_state(), 'router')

        #-------------------------------------------------------------------------------------
        # Add an external route on `br3`, it should not become a router since we already have
        # two that requested router role upgrade as border router reason.

        br3.add_route('2001:dead:beef:cafe::/64', stable=True)
        br3.register_netdata()
        self.simulator.go(120)
        self.assertEqual(br3.get_state(), 'child')

        #-------------------------------------------------------------------------------------
        # Remove the external route on `br1`. This should now trigger `br3` to request a
        # router role upgrade since number of BRs acting as router in network data is now
        # below the threshold of two.

        br1.remove_route('2001:dead:beef:cafe::/64')
        br1.register_netdata()
        self.simulator.go(15)
        self.assertEqual(br1.get_state(), 'router')
        self.assertEqual(br3.get_state(), 'router')


if __name__ == '__main__':
    unittest.main()
