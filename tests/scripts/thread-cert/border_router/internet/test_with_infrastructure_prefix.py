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
import logging

import config
import thread_cert

# Test description:
#   This test verifies publishing infrastructure NAT64 prefix (PREF64) in Thread network.
#
#
# Topology:
#
#   ----------------(eth)--------------------
#           |                 |
#          BR (Leader)      HOST (Router)
#           |
#        ROUTER
#

BR = 1
ROUTER = 2
HOST = 3

ETH_PREFIX = "fd00:abcd:7890::/64"
OMR_PREFIX = "2000:0:1111:4444::/64"
INFRA_NAT64_PREFIX = "fd00:beef:1234::/96"
SMALL_NAT64_PREFIX = "fd00:aaaa:2222::/96"

NAT64_PREFIX_REFRESH_DELAY = 305

NAT64_STATE_DISABLED = 'disabled'
NAT64_STATE_NOT_RUNNING = 'not_running'
NAT64_STATE_IDLE = 'idle'
NAT64_STATE_ACTIVE = 'active'


class Nat64InfrastructurePrefix(thread_cert.TestCase):
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

        # Start host with radvd advertising an infrastructure prefix but NO NAT64 prefix yet.
        host.start(start_radvd=True, prefix=ETH_PREFIX, slaac=True)
        self.simulator.go(5)

        br.start()
        br.nat64_set_enabled(True)
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual('leader', br.get_state())

        router.start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual('router', router.get_state())

        # Case 1: No infra-discovered NAT64 prefix. BR publishes its local prefix.
        # Since OMR is not yet infra-derived (no OMR published), it should publish local prefix anyway?
        # Actually, if no OMR is published, it generates one.
        self.simulator.go(10)
        local_nat64_prefix = br.get_br_nat64_prefix()
        logging.info("Local NAT64 prefix: %s", local_nat64_prefix)

        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        self.assertEqual(br.get_netdata_nat64_routes()[0], local_nat64_prefix)
        self.assertDictIncludes(br.nat64_state, {
            'PrefixManager': NAT64_STATE_ACTIVE,
            'Translator': NAT64_STATE_ACTIVE
        })

        # Case 2: Host starts advertising a NAT64 prefix (PREF64).
        # We also need to add an infra-derived OMR prefix for BR to favor infra NAT64 prefix.
        br.add_prefix(OMR_PREFIX)
        br.register_netdata()
        self.simulator.go(10)

        host.start_radvd_service(prefix=ETH_PREFIX, slaac=True, nat64_prefix=INFRA_NAT64_PREFIX)
        # Give some time for RA discovery and evaluation.
        # RoutingManager evaluates every few seconds or on RA events.
        self.simulator.go(20)

        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        self.assertEqual(br.get_netdata_nat64_routes()[0], INFRA_NAT64_PREFIX)
        self.assertDictIncludes(br.nat64_state, {
            'PrefixManager': NAT64_STATE_ACTIVE,
            'Translator': NAT64_STATE_NOT_RUNNING
        })

        # Case 3: Stop infrastructure NAT64 prefix advertisement.
        host.stop_radvd_service()
        # Wait for the discovered prefix to expire or be deprecated.
        self.simulator.go(NAT64_PREFIX_REFRESH_DELAY)

        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        self.assertEqual(br.get_netdata_nat64_routes()[0], local_nat64_prefix)
        self.assertDictIncludes(br.nat64_state, {
            'PrefixManager': NAT64_STATE_ACTIVE,
            'Translator': NAT64_STATE_ACTIVE
        })

        # Case 4: Recover infrastructure NAT64 prefix advertisement.
        host.start_radvd_service(prefix=ETH_PREFIX, slaac=True, nat64_prefix=INFRA_NAT64_PREFIX)
        self.simulator.go(20)

        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        self.assertEqual(br.get_netdata_nat64_routes()[0], INFRA_NAT64_PREFIX)
        self.assertDictIncludes(br.nat64_state, {
            'PrefixManager': NAT64_STATE_ACTIVE,
            'Translator': NAT64_STATE_NOT_RUNNING
        })

        # Case 5: Change infrastructure NAT64 prefix to a smaller one.
        host.start_radvd_service(prefix=ETH_PREFIX, slaac=True, nat64_prefix=SMALL_NAT64_PREFIX)
        self.simulator.go(20)

        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        self.assertEqual(br.get_netdata_nat64_routes()[0], SMALL_NAT64_PREFIX)
        self.assertDictIncludes(br.nat64_state, {
            'PrefixManager': NAT64_STATE_ACTIVE,
            'Translator': NAT64_STATE_NOT_RUNNING
        })


if __name__ == '__main__':
    unittest.main()
