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
#   This test verifies that a single NAT64 prefix is advertised when there are
#   multiple Border Routers in the same Thread and infrastructure network.
#
#   TODO: add checks for outbound connectivity from Thread device to IPv4 host
#         after OTBR change is ready.
#
# Topology:
#    ----------------(eth)--------------------------
#           |                 |             |
#          BR1 (Leader) ---- BR2           HOST
#           |
#        ROUTER
#

BR1 = 1
ROUTER = 2
BR2 = 3
HOST = 4

NAT64_PREFIX_REFRESH_DELAY = 305


class Nat64MultiBorderRouter(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR1: {
            'name': 'BR1',
            'allowlist': [ROUTER, BR2],
            'is_otbr': True,
            'version': '1.2',
        },
        ROUTER: {
            'name': 'Router',
            'allowlist': [BR1],
            'version': '1.2',
        },
        BR2: {
            'name': 'BR2',
            'allowlist': [BR1],
            'is_otbr': True,
            'version': '1.2'
        },
        HOST: {
            'name': 'Host',
            'is_host': True
        },
    }

    def test(self):
        br1 = self.nodes[BR1]
        router = self.nodes[ROUTER]
        br2 = self.nodes[BR2]
        host = self.nodes[HOST]

        host.start(start_radvd=False)
        self.simulator.go(5)

        br1.start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        br1.bash("service bind9 stop")
        self.simulator.go(NAT64_PREFIX_REFRESH_DELAY)
        self.assertEqual('leader', br1.get_state())

        router.start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual('router', router.get_state())

        #
        # Case 1. BR2 with an infrastructure prefix joins the network later and
        #         it will add the infrastructure nat64 prefix to Network Data.
        #
        br2.start()
        self.simulator.go(config.BORDER_ROUTER_STARTUP_DELAY)
        self.assertEqual('router', br2.get_state())

        self.simulator.go(10)
        self.assertNotEqual(br1.get_br_favored_nat64_prefix(), br2.get_br_favored_nat64_prefix())
        br1_local_nat64_prefix = br1.get_br_nat64_prefix()
        br2_infra_nat64_prefix = br2.get_br_favored_nat64_prefix()

        self.assertEqual(len(br1.get_netdata_nat64_prefix()), 1)
        nat64_prefix = br1.get_netdata_nat64_prefix()[0]
        self.assertEqual(nat64_prefix, br2_infra_nat64_prefix)
        self.assertNotEqual(nat64_prefix, br1_local_nat64_prefix)

        #
        # Case 2. Disable border routing on BR2.
        #         BR1 will add its local nat64 prefix.
        #
        br2.disable_br()
        self.simulator.go(10)

        self.assertEqual(len(br1.get_netdata_nat64_prefix()), 1)
        nat64_prefix = br1.get_netdata_nat64_prefix()[0]
        self.assertEqual(nat64_prefix, br1_local_nat64_prefix)

        #
        # Case 3. Re-enables BR2 with a local prefix and it will not add
        #         its local nat64 prefix to Network Data.
        #
        br2.bash("service bind9 stop")
        self.simulator.go(5)
        br2.enable_br()

        self.simulator.go(10)
        self.assertNotEqual(br2_infra_nat64_prefix, br2.get_br_favored_nat64_prefix())
        br2_local_nat64_prefix = br2.get_br_nat64_prefix()

        self.assertEqual(len(br1.get_netdata_nat64_prefix()), 1)
        nat64_prefix = br1.get_netdata_nat64_prefix()[0]
        self.assertEqual(nat64_prefix, br1_local_nat64_prefix)
        self.assertNotEqual(nat64_prefix, br2_local_nat64_prefix)

        #
        # Case 4. Disable border routing on BR1.
        #         BR1 withdraws its local prefix and BR2 advertises its local prefix.
        #
        br1.disable_br()

        self.simulator.go(10)
        self.assertEqual(len(br1.get_netdata_nat64_prefix()), 1)
        nat64_prefix = br1.get_netdata_nat64_prefix()[0]
        self.assertEqual(br2_local_nat64_prefix, nat64_prefix)
        self.assertNotEqual(br1_local_nat64_prefix, nat64_prefix)

        #
        # Case 5. Re-enable border routing on BR1.
        #         NAT64 prefix in Network Data is still BR2's local prefix.
        #
        br1.enable_br()

        self.simulator.go(10)
        self.assertEqual(len(br1.get_netdata_nat64_prefix()), 1)
        nat64_prefix = br1.get_netdata_nat64_prefix()[0]
        self.assertEqual(br2_local_nat64_prefix, nat64_prefix)
        self.assertNotEqual(br1_local_nat64_prefix, nat64_prefix)


if __name__ == '__main__':
    unittest.main()
