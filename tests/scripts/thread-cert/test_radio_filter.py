#!/usr/bin/env python3
#
#  Copyright (c) 2021, The OpenThread Authors.
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

import config
import command
import thread_cert

# Test description:
#
# This test verifies the radio filter mechanism.
#
# Topology:
#
#   Leader -- Router
#     |
#     |
#   Child
#
#

LEADER = 1
ROUTER = 2
SED = 3

WAIT_TIME = 5


class RadioFilter(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False
    SUPPORT_NCP = False

    TOPOLOGY = {
        LEADER: {
            'mode': 'rdn',
            'allowlist': [ROUTER, SED]
        },
        ROUTER: {
            'mode': 'rdn',
            'allowlist': [LEADER]
        },
        SED: {
            'is_mtd': True,
            'mode': '-',
            'timeout': config.DEFAULT_CHILD_TIMEOUT,
            'allowlist': [LEADER]
        },
    }

    def test(self):
        leader = self.nodes[LEADER]
        router = self.nodes[ROUTER]
        sed = self.nodes[SED]

        nodes = [leader, router, sed]

        leader.start()
        self.simulator.go(WAIT_TIME)
        self.assertEqual(leader.get_state(), 'leader')

        router.start()
        self.simulator.go(WAIT_TIME)
        self.assertEqual(router.get_state(), 'router')

        sed.start()
        sed.set_pollperiod(40)
        self.simulator.go(WAIT_TIME)
        self.assertEqual(sed.get_state(), 'child')

        # Validate initial state of `radiofilter` upon start
        for node in nodes:
            self.assertFalse(node.radiofilter_is_enabled())

        leader_mleid = leader.get_mleid()
        router_mleid = router.get_mleid()
        sed_mleid = sed.get_mleid()

        # Validate connections by pinging from leader, SED, and router
        self.assertTrue(leader.ping(router_mleid))
        self.assertTrue(sed.ping(leader_mleid))
        self.assertTrue(router.ping(leader_mleid))

        # Validate behavior when `radiofilter` is enabled on router
        router.radiofilter_enable()
        self.assertTrue(router.radiofilter_is_enabled())
        self.assertFalse(leader.ping(router_mleid))
        self.assertFalse(router.ping(leader_mleid))

        # Validate behavior when `radiofilter` is disabled on router
        router.radiofilter_disable()
        self.assertFalse(router.radiofilter_is_enabled())
        self.assertTrue(leader.ping(router_mleid))
        self.assertTrue(router.ping(leader_mleid))

        # Validate `radiofilter` behavior on sed.
        self.assertTrue(sed.ping(leader_mleid))

        sed.radiofilter_enable()
        self.assertTrue(sed.radiofilter_is_enabled())
        self.assertFalse(sed.ping(leader_mleid))

        self.assertTrue(sed.radiofilter_is_enabled())
        self.assertEqual(sed.get_state(), 'detached')

        sed.radiofilter_disable()
        self.assertFalse(sed.radiofilter_is_enabled())

        self.simulator.go(WAIT_TIME)
        self.assertEqual(sed.get_state(), 'child')
        self.assertTrue(leader.ping(sed_mleid))
        self.assertTrue(sed.ping(leader_mleid))

        # Validate energy scan when `radiofilter` is enabled
        scan_result = leader.scan_energy()
        self.assertTrue(len(scan_result) > 0)
        leader.radiofilter_enable()
        self.assertTrue(leader.radiofilter_is_enabled())
        scan_result = leader.scan_energy()
        self.assertTrue(len(scan_result) == 0)


if __name__ == '__main__':
    unittest.main()
