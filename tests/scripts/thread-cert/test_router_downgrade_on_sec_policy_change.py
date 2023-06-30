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

# Test description:
#   This test verifies leader and router downgrade delay on security policy version threshold change.
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


class DowngradeOnSecPolicyChange(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False
    SUPPORT_NCP = False

    TOPOLOGY = {
        LEADER: {
            'name': 'LEADER',
            'mode': 'rdn',
        },
        ROUTER: {
            'name': 'ROUTER',
            'mode': 'rdn',
        },
    }

    def test(self):
        leader = self.nodes[LEADER]
        router = self.nodes[ROUTER]

        # Start the leader & router devices.

        leader.start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual(leader.get_state(), 'leader')

        router.start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual(router.get_state(), 'router')

        self.assertTrue(leader.get_router_eligible())
        self.assertTrue(router.get_router_eligible())

        # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        # Change security policy, disable `R` bit and set version threshold to max value (7).

        leader.send_mgmt_active_set(
            active_timestamp=30,
            security_policy=[3600, 'onc', 7],
        )

        self.assertEqual(leader.get_state(), 'leader')
        self.assertEqual(router.get_state(), 'router')

        # Leader should take at least 10 seconds before downgrading.

        self.simulator.go(8)
        self.assertEqual(leader.get_state(), 'leader')
        self.assertFalse(leader.get_router_eligible())
        self.assertFalse(router.get_router_eligible())

        # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        # Change back security policy. This should cancel the ongoing downgrade delay.

        leader.send_mgmt_active_set(
            active_timestamp=60,
            security_policy=[3600, 'ornc', 0],
        )

        self.simulator.go(1)
        self.assertEqual(leader.get_state(), 'leader')
        self.assertTrue(leader.get_router_eligible())

        # Make sure both device stay as leader and router.

        self.simulator.go(600)
        self.assertEqual(leader.get_state(), 'leader')
        self.assertEqual(router.get_state(), 'router')
        self.assertTrue(leader.get_router_eligible())
        self.assertTrue(router.get_router_eligible())

        # - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        # Change security policy again, disable `R` bit and set version threshold to max value (7).

        leader.send_mgmt_active_set(
            active_timestamp=90,
            security_policy=[3600, 'onc', 7],
        )

        self.assertEqual(leader.get_state(), 'leader')

        # Leader should take at least 10 seconds before downgrading.

        self.simulator.go(8)
        self.assertEqual(leader.get_state(), 'leader')
        self.assertFalse(leader.get_router_eligible())

        # Make sure both leader and router are downgraded and are now `detached`.

        self.simulator.go(80)
        self.assertEqual(leader.get_state(), 'detached')
        self.assertEqual(router.get_state(), 'detached')

        self.assertFalse(leader.get_router_eligible())
        self.assertFalse(router.get_router_eligible())


if __name__ == '__main__':
    unittest.main()
