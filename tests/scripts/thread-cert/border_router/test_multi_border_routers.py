#!/usr/bin/env python3
#
#  Copyright (c) 2020, The OpenThread Authors.
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
import logging
import unittest
from ipaddress import IPv6Network

import config
import thread_cert

# Test description:
#   This test verifies that a single OMR and on-link prefix is chosen
#   and advertised when there are multiple Border Routers in the same
#   Thread and infrastructure network.
#
# Topology:
#    ----------------(eth)------------------
#           |                  |     |
#          BR1 (Leader) ----- BR2   HOST
#           |                  |
#        ROUTER1            ROUTER2
#

BR1 = 1
ROUTER1 = 2
BR2 = 3
ROUTER2 = 4
HOST = 5

CHANNEL = 18


class MultiBorderRouters(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR1: {
            'name': 'BR_1',
            'allowlist': [ROUTER1, BR2],
            'is_otbr': True,
            'version': '1.2',
            'channel': CHANNEL,
        },
        ROUTER1: {
            'name': 'Router_1',
            'allowlist': [BR1],
            'version': '1.2',
            'channel': CHANNEL,
        },
        BR2: {
            'name': 'BR_2',
            'allowlist': [BR1, ROUTER2],
            'is_otbr': True,
            'version': '1.2',
            'channel': CHANNEL,
        },
        ROUTER2: {
            'name': 'Router_2',
            'allowlist': [BR2],
            'version': '1.2',
            'channel': CHANNEL,
        },
        HOST: {
            'name': 'Host',
            'is_host': True
        },
    }

    def test(self):
        br1 = self.nodes[BR1]
        router1 = self.nodes[ROUTER1]
        br2 = self.nodes[BR2]
        router2 = self.nodes[ROUTER2]
        host = self.nodes[HOST]

        host.start(start_radvd=False)
        self.simulator.go(5)

        br1.start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual('leader', br1.get_state())

        router1.start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual('router', router1.get_state())

        self.simulator.go(5)

        br2.start()
        self.simulator.go(config.BORDER_ROUTER_STARTUP_DELAY)
        self.assertEqual('router', br2.get_state())

        router2.start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual('router', router2.get_state())

        #
        # Case 1. bi-directional connectivity when there are two BRs.
        #

        self.simulator.go(10)
        self.collect_ipaddrs()

        logging.info("BR1     addrs: %r", br1.get_addrs())
        logging.info("ROUTER1 addrs: %r", router1.get_addrs())
        logging.info("BR2     addrs: %r", br2.get_addrs())
        logging.info("ROUTER2 addrs: %r", router2.get_addrs())
        logging.info("HOST    addrs: %r", host.get_addrs())

        self.assertEqual(len(br1.get_netdata_omr_prefixes()), 1)
        self.assertEqual(len(router1.get_netdata_omr_prefixes()), 1)
        self.assertEqual(len(br2.get_netdata_omr_prefixes()), 1)
        self.assertEqual(len(router2.get_netdata_omr_prefixes()), 1)

        br1_omr_prefix = br1.get_br_omr_prefix()
        self.assertEqual(br1_omr_prefix, br1.get_netdata_omr_prefixes()[0])

        # Each BR should independently register an external route for the on-link prefix.
        self.assertEqual(len(br1.get_netdata_non_nat64_prefixes()), 2)
        self.assertEqual(len(router1.get_netdata_non_nat64_prefixes()), 2)
        self.assertEqual(len(br2.get_netdata_non_nat64_prefixes()), 2)
        self.assertEqual(len(router2.get_netdata_non_nat64_prefixes()), 2)

        br1_on_link_prefix = br1.get_br_on_link_prefix()
        self.assertEqual(br1_on_link_prefix, br1.get_netdata_non_nat64_prefixes()[0])
        self.assertEqual(br1_on_link_prefix, br1.get_netdata_non_nat64_prefixes()[0])

        self.assertEqual(len(br1.get_ip6_address(config.ADDRESS_TYPE.OMR)), 1)
        self.assertEqual(len(router1.get_ip6_address(config.ADDRESS_TYPE.OMR)), 1)
        self.assertEqual(len(br2.get_ip6_address(config.ADDRESS_TYPE.OMR)), 1)
        self.assertEqual(len(router2.get_ip6_address(config.ADDRESS_TYPE.OMR)), 1)
        self.assertEqual(len(host.get_matched_ula_addresses(br1_on_link_prefix)), 1)

        # Router1 and Router2 can ping each other inside the Thread network.
        self.assertTrue(router1.ping(router2.get_ip6_address(config.ADDRESS_TYPE.OMR)[0]))
        self.assertTrue(router2.ping(router1.get_ip6_address(config.ADDRESS_TYPE.OMR)[0]))

        # Both Router1 and Router2 can ping to/from the Host on infra link.
        self.assertTrue(router1.ping(host.get_matched_ula_addresses(br1_on_link_prefix)[0]))
        self.assertTrue(host.ping(router1.get_ip6_address(config.ADDRESS_TYPE.OMR)[0], backbone=True))
        self.assertTrue(router2.ping(host.get_matched_ula_addresses(br1_on_link_prefix)[0]))
        self.assertTrue(host.ping(router2.get_ip6_address(config.ADDRESS_TYPE.OMR)[0], backbone=True))

        #
        # Case 2. Another BR continues providing Border Routing when current one is disabled.
        #

        br1.disable_br()

        self.simulator.go(15)
        self.collect_ipaddrs()

        logging.info("BR1     addrs: %r", br1.get_addrs())
        logging.info("ROUTER1 addrs: %r", router1.get_addrs())
        logging.info("BR2     addrs: %r", br2.get_addrs())
        logging.info("ROUTER2 addrs: %r", router2.get_addrs())
        logging.info("HOST    addrs: %r", host.get_addrs())

        self.assertGreaterEqual(len(host.get_addrs()), 3)

        self.assertEqual(len(br1.get_netdata_omr_prefixes()), 1)
        self.assertEqual(len(router1.get_netdata_omr_prefixes()), 1)
        self.assertEqual(len(br2.get_netdata_omr_prefixes()), 1)
        self.assertEqual(len(router2.get_netdata_omr_prefixes()), 1)

        br2_omr_prefix = br2.get_br_omr_prefix()
        self.assertEqual(br2_omr_prefix, br2.get_netdata_omr_prefixes()[0])

        # There should be no changes to the external route for the
        # on-link prefix, given that the on-link prefix is derived
        # from the Extended PAN ID.
        self.assertEqual(len(br1.get_netdata_non_nat64_prefixes()), 1)
        self.assertEqual(len(router1.get_netdata_non_nat64_prefixes()), 1)
        self.assertEqual(len(br2.get_netdata_non_nat64_prefixes()), 1)
        self.assertEqual(len(router2.get_netdata_non_nat64_prefixes()), 1)

        br2_on_link_prefix = br2.get_br_on_link_prefix()
        self.assertEqual(set(map(IPv6Network, br2.get_netdata_non_nat64_prefixes())),
                         set(map(IPv6Network, [br1_on_link_prefix, br2_on_link_prefix])))

        self.assertEqual(len(br1.get_ip6_address(config.ADDRESS_TYPE.OMR)), 1)
        self.assertEqual(len(router1.get_ip6_address(config.ADDRESS_TYPE.OMR)), 1)
        self.assertEqual(len(br2.get_ip6_address(config.ADDRESS_TYPE.OMR)), 1)
        self.assertEqual(len(router2.get_ip6_address(config.ADDRESS_TYPE.OMR)), 1)

        self.assertEqual(len(host.get_matched_ula_addresses(br2_on_link_prefix)), 1)

        # Router1 and Router2 can ping each other inside the Thread network.
        self.assertTrue(router1.ping(router2.get_ip6_address(config.ADDRESS_TYPE.OMR)[0]))
        self.assertTrue(router2.ping(router1.get_ip6_address(config.ADDRESS_TYPE.OMR)[0]))

        # Both Router1 and Router2 can ping to/from the Host on infra link.
        for router in [router1, router2]:
            self.assertTrue(router.ping(host.get_matched_ula_addresses(br2_on_link_prefix)[0]))
            self.assertTrue(host.ping(router.get_ip6_address(config.ADDRESS_TYPE.OMR)[0], backbone=True))


if __name__ == '__main__':
    unittest.main()
