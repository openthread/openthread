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
#   This test verifies the advertisement of NAT64 prefix in Thread network.
#
#   TODO: add checks for outbound connectivity from Thread device to IPv4 host
#         after OTBR change is ready.
#
# Topology:
#    ----------------(eth)--------------------
#           |                 |
#          BR (Leader)      HOST
#           |
#        ROUTER
#

BR = 1
ROUTER = 2
HOST = 3

# The prefix is set small enough that a random-generated NAT64 prefix is very
# likely greater than it. So that the BR will remove the random-generated one.
SMALL_NAT64_PREFIX = "fd00:00:00:01:00:00::/96"


class Nat64SingleBorderRouter(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR: {
            'name': 'BR',
            'allowlist': [ROUTER],
            'is_otbr': True,
            'version': '1.2',
        },
        ROUTER: {
            'name': 'Router',
            'allowlist': [BR],
            'version': '1.2',
        },
        HOST: {
            'name': 'Host',
            'is_host': True
        },
    }

    def test(self):
        br = self.nodes[BR]
        router = self.nodes[ROUTER]
        host = self.nodes[HOST]

        host.start(start_radvd=False)
        self.simulator.go(5)

        br.start()
        self.simulator.go(config.BORDER_ROUTER_STARTUP_DELAY)
        self.assertEqual('leader', br.get_state())

        router.start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual('router', router.get_state())

        #
        # Case 1. Border router advertises its local NAT64 prefix.
        #
        self.simulator.go(5)
        local_nat64_prefix = br.get_br_nat64_prefix()

        self.assertEqual(len(br.get_netdata_nat64_prefix()), 1)
        nat64_prefix = br.get_netdata_nat64_prefix()[0]
        self.assertEqual(nat64_prefix, local_nat64_prefix)

        #
        # Case 2.
        # User adds a smaller NAT64 prefix and the local prefix is withdrawn.
        # User removes the smaller NAT64 prefix and the local prefix is re-added.
        #
        br.add_route(SMALL_NAT64_PREFIX, stable=False, nat64=True)
        br.register_netdata()
        self.simulator.go(5)

        self.assertEqual(len(br.get_netdata_nat64_prefix()), 1)
        self.assertNotEqual(local_nat64_prefix, br.get_netdata_nat64_prefix()[0])

        br.remove_route(SMALL_NAT64_PREFIX)
        br.register_netdata()
        self.simulator.go(5)

        self.assertEqual(len(br.get_netdata_nat64_prefix()), 1)
        self.assertEqual(local_nat64_prefix, br.get_netdata_nat64_prefix()[0])

        #
        # Case 3. Disable and re-enable border routing on the border router.
        #
        br.disable_br()
        self.simulator.go(5)

        # NAT64 prefix is withdrawn from Network Data.
        self.assertEqual(len(br.get_netdata_nat64_prefix()), 0)

        br.enable_br()
        self.simulator.go(config.BORDER_ROUTER_STARTUP_DELAY)

        # Same NAT64 prefix is advertised to Network Data.
        self.assertEqual(len(br.get_netdata_nat64_prefix()), 1)
        self.assertEqual(nat64_prefix, br.get_netdata_nat64_prefix()[0])

        #
        # Case 4. Disable and re-enable ethernet on the border router.
        #
        br.disable_ether()
        self.simulator.go(5)

        # NAT64 prefix is withdrawn from Network Data.
        self.assertEqual(len(br.get_netdata_nat64_prefix()), 0)

        br.enable_ether()
        self.simulator.go(80)

        # Same NAT64 prefix is advertised to Network Data.
        self.assertEqual(len(br.get_netdata_nat64_prefix()), 1)
        self.assertEqual(nat64_prefix, br.get_netdata_nat64_prefix()[0])


if __name__ == '__main__':
    unittest.main()
