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
import logging
import unittest

import config
import thread_cert
from pktverify.consts import MLE_LINK_REQUEST
from pktverify.packet_verifier import PacketVerifier

LEADER = 1
REED = 2
ROUTER1, ROUTER2, ROUTER3 = 3, 4, 5

# Test Purpose and Description:
# -----------------------------
# This test verifies if a device can quickly and efficiently establish links with
# neighboring routers by sending a multicast Link Request message after becoming
# a router
#
# Test Topology:
# -------------
#     Leader------+
#    |     \       \
# Router1 Router2 Router3
#    \     |        |
#     +---REED-----+
#

# REED should establish links to the three Routers within the delay threshold.
# The max delay consists:
#   Leader may delay 1 second for MLE Advertisement
#   Router may delay 1 second for MLE Advertisement
#   Router may delay 1 second for MLE Link Accept after receiving MLE Link Request (multicast)
LINK_ESTABLISH_DELAY_THRESHOLD = 3


class TestRouterMulticastLinkRequest(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        LEADER: {
            'name': 'LEADER',
            'mode': 'rdn',
            'allowlist': [ROUTER1, ROUTER2, ROUTER3]
        },
        ROUTER1: {
            'name': 'ROUTER1',
            'mode': 'rdn',
            'allowlist': [LEADER, REED]
        },
        ROUTER2: {
            'name': 'ROUTER2',
            'mode': 'rdn',
            'allowlist': [LEADER, REED]
        },
        ROUTER3: {
            'name': 'ROUTER3',
            'mode': 'rdn',
            'allowlist': [LEADER, REED]
        },
        REED: {
            'name': 'REED',
            'mode': 'rdn',
            'allowlist': [ROUTER1, ROUTER2, ROUTER3]
        },
    }

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        for routerid in (ROUTER1, ROUTER2, ROUTER3):
            self.nodes[routerid].start()
            self.simulator.go(config.ROUTER_STARTUP_DELAY)
            self.assertEqual(self.nodes[routerid].get_state(), 'router')

        # Wait for the network to stabilize
        self.simulator.go(60)

        self.nodes[REED].start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual(self.nodes[REED].get_state(), 'router')
        self.simulator.go(LINK_ESTABLISH_DELAY_THRESHOLD + 3)

        # Verify that REED has established link with all routers
        reed_table = self.nodes[REED].router_table()
        reed_id = self.nodes[REED].get_router_id()
        for router in [self.nodes[ROUTER1], self.nodes[ROUTER2], self.nodes[ROUTER3]]:
            self.assertEqual(reed_table[router.get_router_id()]['link'], 1)
            self.assertEqual(router.router_table()[reed_id]['link'], 1)


if __name__ == '__main__':
    unittest.main()
