#!/usr/bin/env python3
#
#  Copyright (c) 2024, The OpenThread Authors.
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
import typing
import unittest

import config
import thread_cert

# Test description:
#
#   This test verifies DHCP6 PD pauses requesting PD prefix when there is another BR advertising PD prefix.
#

LEADER = 1
ROUTER = 2


class TestDhcp6Pd(thread_cert.TestCase):
    SUPPORT_NCP = False
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        LEADER: {
            'name': 'Leader',
            'allowlist': [ROUTER],
            'is_otbr': True,
            'mode': 'rdn',
        },
        ROUTER: {
            'name': 'Router',
            'allowlist': [LEADER],
            'is_otbr': True,
            'mode': 'rdn',
        },
    }

    def test(self):
        leader = self.nodes[LEADER]
        router = self.nodes[ROUTER]

        #---------------------------------------------------------------
        # Start the server & client devices.

        # Case 1: Only 1 BR receives PD prefix

        leader.start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual(leader.get_state(), 'leader')
        leader.pd_set_enabled(True)

        router.start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual(router.get_state(), 'router')
        router.pd_set_enabled(True)

        leader.start_pd_radvd_service("2001:db8:abcd:1234::/64")
        self.simulator.go(30)

        self.assertSetEqual({leader.pd_state, router.pd_state}, {"running", "idle"})

        self.assertEqual(leader.pd_get_prefix(), "2001:db8:abcd:1234::/64")

        # Case 2: More than one BR receives PD prefix
        # The BR with "smaller" PD prefix wins

        # Clean up and ensure no prefix is currently published.
        leader.pd_set_enabled(False)
        router.pd_set_enabled(False)
        self.simulator.go(30)

        leader.start_pd_radvd_service("2001:db8:abcd:1234::/64")
        router.start_pd_radvd_service("2001:db8:1234:abcd::/64")
        leader.pd_set_enabled(True)
        router.pd_set_enabled(True)
        self.simulator.go(30)

        self.assertSetEqual({leader.pd_state, router.pd_state}, {"running", "idle"})

        # Case 3: When the other BR lost PD prefix, the remaining BR should try to request one.

        if leader.pd_state == 'running':
            br_to_stop = leader
            br_to_continue = router
            expected_prefix = "2001:db8:1234:abcd::/64"
        else:
            br_to_stop = router
            br_to_continue = leader
            expected_prefix = "2001:db8:abcd:1234::/64"
        br_to_stop.pd_set_enabled(False)
        self.simulator.go(30)

        self.assertEqual(br_to_continue.pd_state, "running")
        self.assertEqual(br_to_continue.pd_get_prefix(), expected_prefix)


if __name__ == '__main__':
    unittest.main()
