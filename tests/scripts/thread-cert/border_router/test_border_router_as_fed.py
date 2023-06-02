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

import config
import thread_cert

# Test description:
#   This test verifies bi-directional connectivity between Thread end device
#   and infra host via an Border Router (FED)
#
# Topology:
#    ----------------(eth)--------------------
#           |                 |
#          BR (FED)          HOST
#           |
#        Leader
#

BR = 1
LEADER = 2
HOST = 3


class TestBorderRouterAsFed(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR: {
            'name': 'BR',
            'allowlist': [LEADER],
            'is_otbr': True,
            'version': '1.2',
            'router_eligible': False,
        },
        LEADER: {
            'name': 'LEADER',
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
        leader = self.nodes[LEADER]
        host = self.nodes[HOST]

        host.start(start_radvd=False)
        self.simulator.go(5)

        leader.start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual('leader', leader.get_state())

        br.start()
        self.simulator.go(5)
        self.assertEqual('child', br.get_state())

        self.simulator.go(config.BORDER_ROUTER_STARTUP_DELAY)
        self.assertEqual('child', br.get_state())

        # Leader can ping to/from the Host on infra link.
        self.assertTrue(leader.ping(host.get_ip6_address(config.ADDRESS_TYPE.ONLINK_ULA)[0]))
        self.assertTrue(host.ping(leader.get_ip6_address(config.ADDRESS_TYPE.OMR)[0], backbone=True))

        self.simulator.go(5)


if __name__ == '__main__':
    unittest.main()
