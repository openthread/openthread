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

import logging
import unittest

import pktverify
from pktverify import packet_verifier
import config
import thread_cert

# Test description:
# The purpose of this test is to verify the Router's routing behavior using LLA as source address.
#
# Topology:
#
#
#  ROUTER_2 ----- ROUTER_1 ---- ROUTER_3
#
#

ROUTER_1 = 1
ROUTER_2 = 2
ROUTER_3 = 3


class TestPing(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False
    SUPPORT_NCP = False

    TOPOLOGY = {
        ROUTER_1: {
            'name': 'Router_1',
            'allowlist': [ROUTER_2, ROUTER_3],
        },
        ROUTER_2: {
            'name': 'Router_2',
            'allowlist': [ROUTER_1],
        },
        ROUTER_3: {
            'name': 'Router_3',
            'allowlist': [ROUTER_1],
        },
    }

    def test(self):
        router1 = self.nodes[ROUTER_1]
        router2 = self.nodes[ROUTER_2]
        router3 = self.nodes[ROUTER_3]

        router1.start()
        self.simulator.go(5)
        self.assertEqual('leader', router1.get_state())

        router2.start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual('router', router2.get_state())

        router3.start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual('router', router3.get_state())

        self.simulator.go(10)

        left, center, right = router2, router1, router3

        left_lla = left.get_ip6_address(config.ADDRESS_TYPE.LINK_LOCAL)

        # Ping a neighbor with LLA src should succeed.
        self.assertTrue(left.ping(center.get_mleid(), interface=left_lla))
        self.simulator.go(3)

        # Ping a non-neighbor with LLA src should fail.
        self.assertFalse(left.ping(right.get_mleid(), interface=left_lla))
        self.simulator.go(3)

        center.add_route('2001::/64', stable=True)
        center.register_netdata()

        self.simulator.go(3)

        # Make sure external routes are not used for LLA src.
        self.assertFalse(left.ping("2001::1", interface=left_lla))
        self.simulator.go(3)

    def verify(self, pv: pktverify.packet_verifier.PacketVerifier):
        pkts = pv.pkts
        # Make sure the Ping Request to 2001::1 was not sent
        pkts.filter_ipv6_dst('2001::1').filter_ping_request().must_not_next()


if __name__ == '__main__':
    unittest.main()
