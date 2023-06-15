#!/usr/bin/env python3
#
#  Copyright (c) 2023, The OpenThread Authors.
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

# Test description: This test validate the "leader override" behavior.
#
# Topology:
#
#     LEADER
#       |
#       |
#     ROUTER
#

LEADER = 1
ROUTER = 2


class LeaderOverride(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False
    SUPPORT_NCP = False

    TOPOLOGY = {
        LEADER: {
            'name': 'SRP_LEADER',
            'mode': 'rdn',
        },
        ROUTER: {
            'name': 'SRP_ROUTER',
            'mode': 'rdn',
        },
    }

    def test(self):
        leader = self.nodes[LEADER]
        router = self.nodes[ROUTER]

        leader.start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual(leader.get_state(), 'leader')

        router.start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual(router.get_state(), 'router')

        # Validate default status of "leader override"

        self.assertEqual(router.get_leader_override(), 'Disabled')

        # Configure `leader` to misbehave and ignore netdata registrations

        self.assertEqual(leader.get_ignore_netdata_reg(), 'Disabled')
        leader.enable_ignore_netdata_reg()
        self.assertEqual(leader.get_ignore_netdata_reg(), 'Enabled')

        # Enable "leader override" on `router`

        router.enable_leader_override()
        self.assertEqual(router.get_leader_override(), 'Enabled')

        # Update "leader weight" on `router` and validate that it is
        # larger than weight of `leader`.

        router.set_weight(75)
        self.assertEqual(int(router.get_weight()), 75)
        self.assertTrue(int(router.get_weight()) > int(leader.get_weight()))

        # Add a route prefix on `router` to netdata.

        router.add_route('2001:dead:beef:cafe::/64', stable=True)
        router.register_netdata()

        self.simulator.go(10)

        # Make sure that router is not seen in leader netdata (since
        # `leader` is configured to to ignore netdata registrations)

        self.assertEqual(len(router.get_routes()), 0)

        # Wait for at least 3 failed netdata registration attempts
        # (300 sec delay between attempts). Check that router has
        # taken the leader role.

        self.simulator.go(300 * 4)

        self.assertEqual(router.get_state(), 'leader')
        self.assertEqual(leader.get_state(), 'router')
        self.assertEqual(len(router.get_routes()), 1)

        # Repeat the test but this time we use same weight and validate
        # that the leader override does not happen.

        router.set_weight(int(leader.get_weight()))

        leader.stop()
        router.stop()

        leader.start()
        self.simulator.go(120)
        self.assertEqual(leader.get_state(), 'leader')
        self.assertEqual(leader.get_ignore_netdata_reg(), 'Enabled')

        router.start()
        self.simulator.go(120)
        self.assertEqual(router.get_state(), 'router')

        self.assertEqual(router.get_leader_override(), 'Enabled')
        self.assertEqual(router.get_weight(), leader.get_weight())
        router.add_route('2001:dead:beef:cafe::/64', stable=True)
        router.register_netdata()

        self.simulator.go(300 * 10)

        self.assertEqual(router.get_state(), 'router')
        self.assertEqual(leader.get_state(), 'leader')
        self.assertEqual(len(router.get_routes()), 0)

        # Update the weight on `router` to be higher than `leader` but
        # disable the "leader override" feature on `router`. Validate
        # that leader override does not happen.

        router.disable_leader_override()
        self.assertEqual(router.get_leader_override(), 'Disabled')

        router.set_weight(75)
        self.assertEqual(int(router.get_weight()), 75)
        self.assertTrue(int(router.get_weight()) > int(leader.get_weight()))

        self.simulator.go(300 * 10)

        self.assertEqual(router.get_state(), 'router')
        self.assertEqual(leader.get_state(), 'leader')
        self.assertEqual(len(router.get_routes()), 0)

        # Enable "leader override" on `router` again. Now it should
        # succeed. Validate that `router` takes over as `leader`.

        router.enable_leader_override()
        self.assertEqual(router.get_leader_override(), 'Enabled')

        self.simulator.go(300 * 5)

        self.assertEqual(router.get_state(), 'leader')
        self.assertEqual(leader.get_state(), 'router')
        self.assertEqual(len(router.get_routes()), 1)


if __name__ == '__main__':
    unittest.main()
