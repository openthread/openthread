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
import ipaddress
import logging
import unittest

import config
import thread_cert

# Test description:
#   This test verifies Vicarious Router Solicitation.
#
# Topology:
#    -------------(eth)----------------------------
#           |               |           |
#          BR1             BR2         HOST
#           |               |
#        ROUTER1         ROUTER2
#
#     Thread Net1       Thread Net2
#

BR1 = 1
ROUTER1 = 2
BR2 = 3
ROUTER2 = 4
HOST = 5

CHANNEL1 = 18
CHANNEL2 = 19


class MultiThreadNetworks(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR1: {
            'name': 'BR_1',
            'allowlist': [ROUTER1],
            'is_otbr': True,
            'version': '1.2',
            'channel': CHANNEL1,
            'router_selection_jitter': 1,
        },
        ROUTER1: {
            'name': 'Router_1',
            'allowlist': [BR1],
            'version': '1.2',
            'channel': CHANNEL1,
            'router_selection_jitter': 1,
        },
        BR2: {
            'name': 'BR_2',
            'allowlist': [ROUTER2],
            'is_otbr': True,
            'version': '1.2',
            'channel': CHANNEL2,
            'router_selection_jitter': 1,
        },
        ROUTER2: {
            'name': 'Router_2',
            'allowlist': [BR2],
            'version': '1.2',
            'channel': CHANNEL2,
            'router_selection_jitter': 1,
        },
        HOST: {
            'name': 'Host',
            'is_host': True
        },
    }

    def test(self):
        ON_LINK_PREFIX = 'fd00::/64'
        br1 = self.nodes[BR1]
        router1 = self.nodes[ROUTER1]
        br2 = self.nodes[BR2]
        router2 = self.nodes[ROUTER2]
        host = self.nodes[HOST]

        host.start(start_radvd=True, prefix=ON_LINK_PREFIX, slaac=True)
        self.simulator.go(5)

        br1.start()
        self.simulator.go(5)
        self.assertEqual('leader', br1.get_state())

        router1.start()
        self.simulator.go(5)
        self.assertEqual('router', router1.get_state())

        self.simulator.go(10)
        self.collect_ipaddrs()

        logging.info("BR1     addrs: %r", br1.get_addrs())
        logging.info("ROUTER1 addrs: %r", router1.get_addrs())
        logging.info("HOST    addrs: %r", host.get_addrs())

        self.assertEqual(len(br1.get_routes()), 1)
        br1_on_link_prefix = br1.get_routes()[0].split(' ')[0]
        self.assertEqual(ipaddress.IPv6Network(br1_on_link_prefix), ipaddress.IPv6Network(ON_LINK_PREFIX))

        host_on_link_addr = host.get_matched_ula_addresses(br1_on_link_prefix)[0]
        self.assertTrue(router1.ping(host_on_link_addr))
        self.assertTrue(
            host.ping(router1.get_ip6_address(config.ADDRESS_TYPE.OMR)[0], backbone=True, interface=host_on_link_addr))

        # Force kill the radvd host so that BR2 will start advertising its own on-link prefix.
        host.kill_radvd_service()

        br2.start()
        self.simulator.go(5)
        self.assertEqual('leader', br2.get_state())

        router2.start()
        self.simulator.go(5)
        self.assertEqual('router', router2.get_state())

        self.simulator.go(25)
        self.collect_ipaddrs()

        logging.info("BR1     addrs: %r", br1.get_addrs())
        logging.info("ROUTER1 addrs: %r", router1.get_addrs())
        logging.info("BR2     addrs: %r", br2.get_addrs())
        logging.info("ROUTER2 addrs: %r", router2.get_addrs())
        logging.info("HOST    addrs: %r", host.get_addrs())

        self.assertTrue(len(br1.get_prefixes()) == 1)
        self.assertTrue(len(router1.get_prefixes()) == 1)
        self.assertTrue(len(br2.get_prefixes()) == 1)
        self.assertTrue(len(router2.get_prefixes()) == 1)

        br1_omr_prefix = br1.get_prefixes()[0].split(' ')[0]
        br2_omr_prefix = br2.get_prefixes()[0].split(' ')[0]
        self.assertNotEqual(br1_omr_prefix, br2_omr_prefix)

        # Verify that BR1 removed ON_LINK_PREFIX from its
        # external routes list.
        self.assertEqual(len(br1.get_routes()), 2)
        self.assertEqual(len(router1.get_routes()), 2)
        self.assertEqual(len(br2.get_routes()), 2)
        self.assertEqual(len(router2.get_routes()), 2)

        br1_external_routes = [route.split(' ')[0] for route in br1.get_routes()]
        br2_external_routes = [route.split(' ')[0] for route in br2.get_routes()]

        on_link_prefixes = list(set(br1_external_routes).intersection(br2_external_routes))
        self.assertEqual(len(on_link_prefixes), 1)
        on_link_prefix = on_link_prefixes[0]

        # Verify that both BR1 and BR2 are using a new on-link prefix.
        # BR1 is depending on the Vicarious Router Solicitation to find
        # out that the ON_LINK_PREFIX is stale and starts Router Solicitation
        # on its own.
        self.assertNotEqual(ipaddress.IPv6Network(on_link_prefix), ipaddress.IPv6Network(ON_LINK_PREFIX))

        host_on_link_addr = host.get_matched_ula_addresses(on_link_prefix)[0]
        self.assertTrue(router1.ping(host_on_link_addr))
        self.assertTrue(
            host.ping(router1.get_ip6_address(config.ADDRESS_TYPE.OMR)[0], backbone=True, interface=host_on_link_addr))
        self.assertTrue(router2.ping(host.get_matched_ula_addresses(on_link_prefix)[0]))
        self.assertTrue(
            host.ping(router2.get_ip6_address(config.ADDRESS_TYPE.OMR)[0], backbone=True, interface=host_on_link_addr))
        self.assertTrue(router1.ping(router2.get_ip6_address(config.ADDRESS_TYPE.OMR)[0]))
        self.assertTrue(router2.ping(router1.get_ip6_address(config.ADDRESS_TYPE.OMR)[0]))


if __name__ == '__main__':
    unittest.main()
