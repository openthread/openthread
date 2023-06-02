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
import unittest

import config
import thread_cert

# Test description:
#   This test verifies TREL connectivity.
#
# Topology:
#    ----------------(eth)--------------------------
#                 |                  |
#       FED1 --- BR1 (Leader) ----- BR2 --- ROUTER2
#                /\
#               /  \
#             MED1 SED1
#
from config import PACKET_VERIFICATION_TREL
from pktverify.packet_filter import PacketFilter
from pktverify.packet_verifier import PacketVerifier

BR1 = 1
FED1 = 2
MED1 = 3
SED1 = 4
BR2 = 5
ROUTER2 = 6


class TestTrelConnectivity(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False
    PACKET_VERIFICATION = PACKET_VERIFICATION_TREL

    TOPOLOGY = {
        BR1: {
            'name': 'BR1',
            'allowlist': [BR2, FED1, MED1, SED1],
            'is_otbr': True,
            'version': '1.2',
        },
        FED1: {
            'name': 'ROUTER1',
            'allowlist': [BR1],
            'version': '1.2',
            'router_eligible': False,
        },
        MED1: {
            'name': 'MED1',
            'allowlist': [BR1],
            'version': '1.2',
            'mode': 'rn',
        },
        SED1: {
            'name': 'SED1',
            'allowlist': [BR1],
            'version': '1.2',
            'mode': 'n',
        },
        BR2: {
            'name': 'BR2',
            'allowlist': [BR1, ROUTER2],
            'is_otbr': True,
            'version': '1.2',
        },
        ROUTER2: {
            'name': 'ROUTER2',
            'allowlist': [BR2],
            'version': '1.2',
        },
    }

    def test(self):
        br1 = self.nodes[BR1]
        fed1 = self.nodes[FED1]
        med1 = self.nodes[MED1]
        sed1 = self.nodes[SED1]
        br2 = self.nodes[BR2]
        router2 = self.nodes[ROUTER2]

        if br1.get_trel_state() is None:
            self.skipTest("TREL is not enabled")

        br1.start()
        self.wait_node_state(br1, 'leader', 10)

        fed1.start()
        self.wait_node_state(fed1, 'child', 10)

        med1.start()
        self.wait_node_state(med1, 'child', 10)

        sed1.start()
        self.wait_node_state(sed1, 'child', 10)

        br2.start()
        self.wait_node_state(br2, 'router', 10)

        router2.start()
        self.wait_node_state(router2, 'router', 10)

        # Allow the network to stabilize
        self.simulator.go(config.BORDER_ROUTER_STARTUP_DELAY)

        self.collect_ipaddrs()
        self.collect_rloc16s()

        router2_mleid = router2.get_mleid()
        self.assertTrue(br1.ping(router2_mleid))
        self.assertTrue(fed1.ping(router2_mleid))
        self.assertTrue(med1.ping(router2_mleid))
        self.assertTrue(sed1.ping(router2_mleid))

    def verify(self, pv: PacketVerifier):
        pkts: PacketFilter = pv.pkts
        BR1_RLOC16 = pv.vars['BR1_RLOC16']
        BR2_RLOC16 = pv.vars['BR2_RLOC16']

        print('BR1_RLOC16:', hex(BR1_RLOC16))
        print('BR2_RLOC16:', hex(BR2_RLOC16))

        # Make sure BR1 and BR2 always use TREL for transmitting ping request and reply
        pkts.filter_wpan_src16_dst16(BR1_RLOC16, BR2_RLOC16).filter_ping_request().must_not_next()
        pkts.filter_wpan_src16_dst16(BR1_RLOC16, BR2_RLOC16).filter_ping_reply().must_not_next()

        pkts.filter_wpan_src16_dst16(BR2_RLOC16, BR1_RLOC16).filter_ping_request().must_not_next()
        pkts.filter_wpan_src16_dst16(BR2_RLOC16, BR1_RLOC16).filter_ping_reply().must_not_next()


if __name__ == '__main__':
    unittest.main()
