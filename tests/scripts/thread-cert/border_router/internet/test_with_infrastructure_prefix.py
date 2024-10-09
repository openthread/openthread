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
#   This test verifies publishing infrastructure NAT64 prefix in Thread network.
#
#
# Topology:
#
#   ----------------(eth)--------------------
#           |
#          BR (with DNS64 on infrastructure interface)
#           |
#        ROUTER
#

BR = 1
ROUTER = 2

OMR_PREFIX = "2000:0:1111:4444::/64"
# The prefix is set smaller than the default infrastructure NAT64 prefix.
SMALL_NAT64_PREFIX = "2000:0:0:1:0:0::/96"

NAT64_PREFIX_REFRESH_DELAY = 305

NAT64_STATE_DISABLED = 'disabled'
NAT64_STATE_NOT_RUNNING = 'not_running'
NAT64_STATE_IDLE = 'idle'
NAT64_STATE_ACTIVE = 'active'


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
    }

    def test(self):
        br = self.nodes[BR]
        router = self.nodes[ROUTER]

        br.start()
        # When feature flag is enabled, NAT64 might be disabled by default. So
        # ensure NAT64 is enabled here.
        br.nat64_set_enabled(True)
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual('leader', br.get_state())

        router.start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual('router', router.get_state())

        # Case 1 No infra-derived OMR prefix. BR publishes its local prefix.
        local_nat64_prefix = br.get_br_nat64_prefix()

        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        nat64_prefix = br.get_netdata_nat64_routes()[0]
        self.assertEqual(nat64_prefix, local_nat64_prefix)

        self.assertDictIncludes(br.nat64_state, {
            'PrefixManager': NAT64_STATE_ACTIVE,
            'Translator': NAT64_STATE_ACTIVE
        })

        # Case 2 Add OMR prefix. BR publishes the infrastructure nat64 prefix
        br.add_prefix(OMR_PREFIX)
        br.register_netdata()
        self.simulator.go(10)

        favored_nat64_prefix = br.get_br_favored_nat64_prefix()
        self.assertNotEqual(favored_nat64_prefix, local_nat64_prefix)
        infra_nat64_prefix = favored_nat64_prefix

        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        nat64_prefix = br.get_netdata_nat64_routes()[0]
        self.assertEqual(nat64_prefix, infra_nat64_prefix)
        self.assertDictIncludes(br.nat64_state, {
            'PrefixManager': NAT64_STATE_ACTIVE,
            'Translator': NAT64_STATE_NOT_RUNNING
        })

        # Case 3 Unpublish infrastructure prefix when a smaller prefix in medium
        # preference is present
        br.add_route(SMALL_NAT64_PREFIX, stable=False, nat64=True, prf='med')
        br.register_netdata()
        self.simulator.go(5)

        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        self.assertNotEqual(infra_nat64_prefix, br.get_netdata_nat64_routes()[0])
        self.assertDictIncludes(br.nat64_state, {
            'PrefixManager': NAT64_STATE_IDLE,
            'Translator': NAT64_STATE_NOT_RUNNING
        })

        br.remove_route(SMALL_NAT64_PREFIX)
        br.register_netdata()
        self.simulator.go(10)

        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        self.assertEqual(nat64_prefix, infra_nat64_prefix)

        # Case 4 No change when a smaller prefix in low preference is present
        br.add_route(SMALL_NAT64_PREFIX, stable=False, nat64=True, prf='low')
        br.register_netdata()
        self.simulator.go(5)

        self.assertEqual(len(br.get_netdata_nat64_routes()), 2)
        self.assertEqual(br.get_netdata_nat64_routes(), [infra_nat64_prefix, SMALL_NAT64_PREFIX])
        self.assertDictIncludes(br.nat64_state, {
            'PrefixManager': NAT64_STATE_ACTIVE,
            'Translator': NAT64_STATE_NOT_RUNNING
        })

        br.remove_route(SMALL_NAT64_PREFIX)
        br.register_netdata()
        self.simulator.go(5)

        # Case 5 Infrastructure nat64 prefix no longer presents
        br.bash("service bind9 stop")
        self.simulator.go(NAT64_PREFIX_REFRESH_DELAY)

        local_nat64_prefix = br.get_br_nat64_prefix()
        self.assertNotEqual(local_nat64_prefix, infra_nat64_prefix)
        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        self.assertEqual(br.get_netdata_nat64_routes()[0], local_nat64_prefix)
        self.assertDictIncludes(br.nat64_state, {
            'PrefixManager': NAT64_STATE_ACTIVE,
            'Translator': NAT64_STATE_ACTIVE
        })

        # Case 6 Infrastructure nat64 prefix is recovered
        br.bash("service bind9 start")
        self.simulator.go(NAT64_PREFIX_REFRESH_DELAY)

        self.assertEqual(br.get_br_favored_nat64_prefix(), infra_nat64_prefix)
        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        self.assertEqual(br.get_netdata_nat64_routes()[0], infra_nat64_prefix)
        self.assertDictIncludes(br.nat64_state, {
            'PrefixManager': NAT64_STATE_ACTIVE,
            'Translator': NAT64_STATE_NOT_RUNNING
        })

        # Case 7 Change infrastructure nat64 prefix
        br.bash("sed -i 's/dns64 /\/\/dns64 /' /etc/bind/named.conf.options")
        br.bash("sed -i '/\/\/dns64 /a dns64 " + SMALL_NAT64_PREFIX + " {};' /etc/bind/named.conf.options")
        br.bash("service bind9 restart")
        self.simulator.go(NAT64_PREFIX_REFRESH_DELAY)

        self.assertEqual(br.get_br_favored_nat64_prefix(), SMALL_NAT64_PREFIX)
        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        self.assertEqual(br.get_netdata_nat64_routes()[0], SMALL_NAT64_PREFIX)
        self.assertDictIncludes(br.nat64_state, {
            'PrefixManager': NAT64_STATE_ACTIVE,
            'Translator': NAT64_STATE_NOT_RUNNING
        })


if __name__ == '__main__':
    unittest.main()
