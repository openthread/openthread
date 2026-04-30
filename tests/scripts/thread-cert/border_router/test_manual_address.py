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
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS'
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

# Test description:
# The purpose of this test case is to verify that BR can manually configure addresses and comply
# with the manual address configuration rules.
#
# Topology:
#     BR_1 (Leader)
#       |
#       |
#     ROUTER
#

BR_1 = 1
ROUTER = 2

PREFIX = '2002::/64'
GUA_1 = '2002::a:b:c:d'
GUA_2 = '2002::a:b:c:e'


class ManualAddressConfig(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR_1: {
            'name': 'BR_1',
            'is_otbr': True,
            'version': '1.2',
        },
        ROUTER: {
            'name': 'ROUTER',
            'version': '1.2',
        },
    }

    def test(self):
        br1 = self.nodes[BR_1]
        router = self.nodes[ROUTER]

        br1.start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual('leader', br1.get_state())
        self.assertTrue(br1.is_primary_backbone_router)

        router.start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual('router', router.get_state())

        # Add prefix
        br1.add_prefix(PREFIX, flags='po', prf='med')
        br1.register_netdata()
        self.simulator.go(5)

        # Configure GUA addresses
        br1.add_ipaddr(GUA_1)
        router.add_ipaddr(GUA_2)

        # BR_1 sends a ping packet to the GUA_2 address. Router should respond to the ping request.
        self.assertTrue(br1.ping(GUA_2))
        self.simulator.go(5)

        # Router sends a ping packet to the GUA_1 address. BR_1 should respond to the ping request.
        self.assertTrue(router.ping(GUA_1))
        self.simulator.go(5)


if __name__ == '__main__':
    unittest.main()
