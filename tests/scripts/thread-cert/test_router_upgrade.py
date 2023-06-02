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

import unittest

import config
import mle
import network_layer
import thread_cert

LEADER = 1
REED = 2

RSSI_LOW = -95
RSSI_HIGH = -45

ROUTER_UPGRADE_DELAY = 150

# Test Purpose and Description:
# -----------------------------
# The purpose of this test case is to show that a REED does not
# upgrade to router if it does not have a neighbor with link margin
# above threshold.
#
# Test Topology:
# -------------
# Leader
#    |
# REED
#
# DUT Types:
# ----------
#  Leader
#  REED


class TestRouterUpgrade(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        LEADER: {
            'name': 'LEADER',
            'mode': 'rdn',
            'allowlist': [REED]
        },
        REED: {
            'name': 'REED',
            'mode': 'rdn',
            'allowlist': [LEADER]
        },
    }

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[REED].add_allowlist(self.nodes[LEADER].get_addr64(), rssi=RSSI_LOW)
        self.nodes[REED].enable_allowlist()

        self.nodes[REED].start()
        self.simulator.go(ROUTER_UPGRADE_DELAY)
        self.assertEqual(self.nodes[REED].get_state(), 'child')

        self.nodes[REED].add_allowlist(self.nodes[LEADER].get_addr64(), rssi=RSSI_HIGH)
        self.nodes[REED].enable_allowlist()

        self.simulator.go(ROUTER_UPGRADE_DELAY)
        self.assertEqual(self.nodes[REED].get_state(), 'router')


if __name__ == '__main__':
    unittest.main()
