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
DNS_NAT64_PREFIX = "2000:0:0:1:0:0::/96"
RA_NAT64_PREFIX = "2000:0:0:2::/96"

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

        # Case 1: No infra-derived OMR prefix. BR publishes its local NAT64 prefix.
        local_nat64_prefix = br.get_br_nat64_prefix()

        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        nat64_prefix = br.get_netdata_nat64_routes()[0]
        self.assertEqual(nat64_prefix, local_nat64_prefix)

        self.assertDictIncludes(br.nat64_state, {
            'PrefixManager': NAT64_STATE_ACTIVE,
            'Translator': NAT64_STATE_ACTIVE
        })

        # Case 2: Add OMR prefix. BR should now discover and publish the NAT64 prefix from DNS.
        br.add_prefix(OMR_PREFIX)
        br.register_netdata()
        self.simulator.go(NAT64_PREFIX_REFRESH_DELAY)

        # Wait for the BR to discover and favor the DNS64 prefix.
        self.wait_for(lambda: br.get_br_favored_nat64_prefix() != local_nat64_prefix)
        favored_nat64_prefix = br.get_br_favored_nat64_prefix()
        dns_nat64_prefix = favored_nat64_prefix

        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        nat64_prefix = br.get_netdata_nat64_routes()[0]
        self.assertEqual(nat64_prefix, dns_nat64_prefix)
        self.assertDictIncludes(br.nat64_state, {
            'PrefixManager': NAT64_STATE_ACTIVE,
            'Translator': NAT64_STATE_NOT_RUNNING
        })

        # Case 3: A smaller prefix with medium preference is added to Network Data.
        # The BR should yield to it and stop publishing its discovered prefix.
        br.add_route(DNS_NAT64_PREFIX, stable=False, nat64=True, prf='med')
        br.register_netdata()
        self.simulator.go(5)

        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        self.assertNotEqual(dns_nat64_prefix, br.get_netdata_nat64_routes()[0])
        self.assertDictIncludes(br.nat64_state, {
            'PrefixManager': NAT64_STATE_IDLE,
            'Translator': NAT64_STATE_NOT_RUNNING
        })

        br.remove_route(DNS_NAT64_PREFIX)
        br.register_netdata()
        self.simulator.go(10)

        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        self.assertEqual(nat64_prefix, dns_nat64_prefix)

        # Case 4: A smaller prefix with low preference is added.
        # The BR should not yield and continue publishing its higher-preference prefix.
        br.add_route(DNS_NAT64_PREFIX, stable=False, nat64=True, prf='low')
        br.register_netdata()
        self.simulator.go(5)

        self.assertEqual(len(br.get_netdata_nat64_routes()), 2)
        self.assertEqual(br.get_netdata_nat64_routes(), [dns_nat64_prefix, DNS_NAT64_PREFIX])
        self.assertDictIncludes(br.nat64_state, {
            'PrefixManager': NAT64_STATE_ACTIVE,
            'Translator': NAT64_STATE_NOT_RUNNING
        })

        br.remove_route(DNS_NAT64_PREFIX)
        br.register_netdata()
        self.simulator.go(5)
        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)

        # Case 5: Start radvd to advertise a PREF64 prefix via RA.
        # The RA-discovered NAT64 prefix should be favored over the DNS one.
        br.start_radvd_service(prefix=None, nat64_prefix=RA_NAT64_PREFIX)
        self.simulator.go(10)

        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        nat64_prefix = br.get_netdata_nat64_routes()[0]
        self.assertEqual(nat64_prefix, RA_NAT64_PREFIX)
        self.assertDictIncludes(br.nat64_state, {'PrefixManager': NAT64_STATE_ACTIVE, 'Translator': NAT64_STATE_IDLE})

        # Case 6: Stop radvd.
        # The BR should fall back to the DNS-discovered NAT64 prefix.
        br.stop_radvd_service()
        self.simulator.go(310)

        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        nat64_prefix = br.get_netdata_nat64_routes()[0]
        self.assertEqual(nat64_prefix, dns_nat64_prefix)

        # Case 7: Infrastructure DNS64 server is stopped.
        # The BR should fall back to its local NAT64 prefix.
        br.bash("service bind9 stop || true")
        self.simulator.go(NAT64_PREFIX_REFRESH_DELAY)

        local_nat64_prefix = br.get_br_nat64_prefix()
        self.assertNotEqual(local_nat64_prefix, dns_nat64_prefix)
        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        self.assertEqual(br.get_netdata_nat64_routes()[0], local_nat64_prefix)
        self.assertDictIncludes(br.nat64_state, {
            'PrefixManager': NAT64_STATE_ACTIVE,
            'Translator': NAT64_STATE_ACTIVE
        })

        # Case 8: Infrastructure DNS64 server is recovered.
        # The BR should switch back to the DNS-discovered prefix.
        br.bash("service bind9 start")
        self.simulator.go(NAT64_PREFIX_REFRESH_DELAY)

        self.assertEqual(br.get_br_favored_nat64_prefix(), dns_nat64_prefix)
        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        self.assertEqual(br.get_netdata_nat64_routes()[0], dns_nat64_prefix)
        self.assertDictIncludes(br.nat64_state, {
            'PrefixManager': NAT64_STATE_ACTIVE,
            'Translator': NAT64_STATE_NOT_RUNNING
        })

        # Case 9: Change the NAT64 prefix on the DNS64 server.
        # The BR should discover and publish the new prefix.
        br.bash(r"sed -i 's/dns64 /\/\/dns64 /' /etc/bind/named.conf.options")
        br.bash(r"sed -i '/\/\/dns64 /a dns64 " + DNS_NAT64_PREFIX + " {};' /etc/bind/named.conf.options")
        br.bash("service bind9 restart")
        self.simulator.go(NAT64_PREFIX_REFRESH_DELAY)

        self.assertEqual(br.get_br_favored_nat64_prefix(), DNS_NAT64_PREFIX)
        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        self.assertEqual(br.get_netdata_nat64_routes()[0], DNS_NAT64_PREFIX)
        self.assertDictIncludes(br.nat64_state, {
            'PrefixManager': NAT64_STATE_ACTIVE,
            'Translator': NAT64_STATE_NOT_RUNNING
        })


if __name__ == '__main__':
    unittest.main()
