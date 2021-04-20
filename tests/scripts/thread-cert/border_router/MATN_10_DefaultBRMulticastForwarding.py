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
from pktverify import packet_verifier, packet_filter, consts
from pktverify.consts import MA1
import config
import thread_cert

# Test description:
# The purpose of this test case is to verify that a Secondary BBR can take over
# forwarding of inbound multicast transmissionsThread Network when the Primary
# BBR disconnects. The Secondary in that case becomes Primary.
#
# Topology:
#    ----------------(eth)------------------
#           |                  |      |
#         BR_1 (Leader) ---- BR_2     |
#           |                  |     Host
#           |                  |
#          ROUTER_1 -----------+
#

BR_1 = 1
BR_2 = 2
ROUTER_1 = 3
HOST = 4


class MATN_10_FailureOfPrimaryBBRInboundMulticast(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR_1: {
            'name': 'BR_1',
            'is_otbr': True,
            'allowlist': [BR_2, ROUTER_1],
            'version': '1.2',
            'router_selection_jitter': 2,
        },
        BR_2: {
            'name': 'BR_2',
            'is_otbr': True,
            'allowlist': [BR_1, ROUTER_1],
            'version': '1.2',
            'router_selection_jitter': 2,
        },
        ROUTER_1: {
            'name': 'Router_1',
            'allowlist': [BR_1, BR_2],
            'version': '1.2',
            'router_selection_jitter': 2,
            'partition_id': 1,
        },
        HOST: {
            'name': 'Host',
            'is_host': True,
        },
    }

    def test(self):
        br1 = self.nodes[BR_1]
        br2 = self.nodes[BR_2]
        router1 = self.nodes[ROUTER_1]
        host = self.nodes[HOST]

        br1.start()
        self.simulator.go(10)
        self.assertEqual('leader', br1.get_state())
        self.assertTrue(br1.is_primary_backbone_router)

        router1.start()
        self.simulator.go(10)
        self.assertEqual('router', router1.get_state())

        br2.start()
        self.simulator.go(10)
        self.assertEqual('router', br2.get_state())
        self.assertFalse(br2.is_primary_backbone_router)

        # 1. Router_1 registers for multicast address, MA1, at BR_1.
        router1.add_ipmaddr(MA1)
        self.simulator.go(5)

        # 5. Host sends a ping packet to the multicast address, MA1.
        self.assertTrue(host.ping(MA1, backbone=True))
        self.simulator.go(5)

        # 8a. Switch off BR_1.
        br1.thread_stop()
        self.simulator.go(180)

        # 8b. BR_2 detects the missing Primary BBR and becomes the Primary BBR
        # of the Thread Network, distributing Dataset (with BR_2’s RLOC16) to
        # Router_1.
        self.assertTrue(br2.is_primary_backbone_router)

        # 10. Host sends a ping packet to the multicast address, MA1.
        self.assertTrue(host.ping(MA1, backbone=True))
        self.simulator.go(5)

        # 14. Router_1 re-registers for multicast address, MA1, at BR_2.
        router1.add_ipmaddr(MA1)

        self.collect_ipaddrs()
        self.collect_rloc16s()
        self.collect_rlocs()
        self.collect_leader_aloc(BR_2)
        self.collect_extra_vars()

    def verify(self, pv: pktverify.packet_verifier.PacketVerifier):
        pkts = pv.pkts
        vars = pv.vars
        pv.summary.show()
        logging.info(f'vars = {vars}')

        # Ensure the topology is formed correctly
        pv.verify_attached('Router_1', 'BR_1')
        pv.verify_attached('BR_2')



if __name__ == '__main__':
    unittest.main()
