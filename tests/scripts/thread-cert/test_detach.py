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

import thread_cert
import config
from pktverify.consts import MLE_CHILD_UPDATE_REQUEST, TIMEOUT_TLV, ADDR_REL_URI
from pktverify.packet_verifier import PacketVerifier

# Test description:
#   This test verifies that detaching function can send correct "goodbye" messages.
#
# Topology:
#
#   CHILD_1 ----- ROUTER_1 ----- LEADER
#
#

LEADER = 1
ROUTER_1 = 2
CHILD_1 = 3


class TestDetach(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False
    SUPPORT_NCP = False

    TOPOLOGY = {
        LEADER: {
            'name': 'Leader',
            'allowlist': [ROUTER_1],
            'mode': 'rdn',
        },
        ROUTER_1: {
            'name': 'Router_1',
            'allowlist': [LEADER, CHILD_1],
            'mode': 'rdn',
        },
        CHILD_1: {
            'name': 'Child_1',
            'is_mtd': True,
            'allowlist': [ROUTER_1],
            'mode': '-',
            'timeout': 10,
        },
    }

    def test(self):
        leader = self.nodes[LEADER]
        router1 = self.nodes[ROUTER_1]
        child1 = self.nodes[CHILD_1]

        leader.start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual(leader.get_state(), 'leader')

        router1.start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual(router1.get_state(), 'router')
        router1_rloc16 = router1.get_addr16()
        self.assertTrue(list(filter(lambda x: x[1]['rloc16'] == router1_rloc16, leader.router_table().items())))

        self.collect_rloc16s()

        child1.start()
        self.simulator.go(7)
        self.assertEqual(child1.get_state(), 'child')
        child_table = router1.get_child_table()
        self.assertEqual(len(child_table), 1)
        self.assertEqual(child_table[1]['timeout'], 10)

        child1.detach()
        self.assertEqual(child1.get_state(), 'disabled')
        self.assertFalse(router1.get_child_table())

        router1.detach()
        self.assertEqual(router1.get_state(), 'disabled')
        self.assertFalse(list(filter(lambda x: x[1]['rloc16'] == router1_rloc16, leader.router_table().items())))

        router1.start()
        self.simulator.go(5)
        self.assertEqual(router1.get_state(), 'router')

        child1.start()
        self.simulator.go(7)
        self.assertEqual(child1.get_state(), 'child')
        child_table = router1.get_child_table()
        self.assertEqual(len(child_table), 1)
        self.assertEqual(child_table[2]['timeout'], 10)

        router1.thread_stop()
        self.assertEqual(router1.get_state(), 'disabled')
        child1.detach()
        self.assertEqual(child1.get_state(), 'disabled')

        router1.start()
        self.simulator.go(5)
        self.assertEqual(router1.get_state(), 'router')

        child1.start()
        self.simulator.go(7)
        self.assertEqual(child1.get_state(), 'child')

        leader.detach()
        self.assertEqual(leader.get_state(), 'disabled')

        self.assertTrue(child1.ping(router1.get_mleid(), timeout=20))

        router1.detach()
        self.assertEqual(router1.get_state(), 'disabled')

        leader.detach()
        self.assertEqual(leader.get_state(), 'disabled')

        leader.start()
        self.assertEqual(leader.get_state(), 'detached')
        leader.detach()
        self.assertEqual(leader.get_state(), 'disabled')

        leader.start()
        # leader didn't become leader after the last start(), so it re-syncs in a non-critical manner thus taking ROUTER_RESET_DELAY to recover
        self.simulator.go(config.ROUTER_RESET_DELAY / 2)
        self.assertEqual(leader.get_state(), 'detached')
        self.simulator.go(config.ROUTER_RESET_DELAY / 2)
        self.assertEqual(leader.get_state(), 'leader')
        router1.start()
        self.simulator.go(config.ROUTER_RESET_DELAY)
        self.assertEqual(router1.get_state(), 'router')

        leader.thread_stop()
        router1.detach(is_async=True)
        self.assertEqual(router1.get_state(), 'router')
        router1.thread_stop()
        self.assertEqual(router1.get_state(), 'disabled')
        router1.detach()
        self.assertEqual(router1.get_state(), 'disabled')

    def verify(self, pv: PacketVerifier):
        pkts = pv.pkts
        pv.summary.show()

        leader = pv.vars['Leader']
        router1 = pv.vars['Router_1']
        child1 = pv.vars['Child_1']
        leader_rloc16 = pv.vars['Leader_RLOC16']

        pkts.filter_wpan_src64(child1).filter_mle_cmd(MLE_CHILD_UPDATE_REQUEST).filter_wpan_dst64(
            router1).must_next().must_verify(lambda p: TIMEOUT_TLV in set(p.mle.tlv.type) and p.mle.tlv.timeout == 0)
        pkts.filter_wpan_src64(router1).filter_coap_request(ADDR_REL_URI).filter_wpan_dst16(leader_rloc16).must_next()
        pkts.filter_wpan_src64(child1).filter_mle_cmd(MLE_CHILD_UPDATE_REQUEST).filter_wpan_dst64(
            router1).must_next().must_verify(lambda p: TIMEOUT_TLV in set(p.mle.tlv.type) and p.mle.tlv.timeout == 0)
        pkts.filter_wpan_src64(leader).filter_coap_request(ADDR_REL_URI).must_not_next()
        pkts.filter_wpan_src64(router1).filter_coap_request(ADDR_REL_URI).filter_wpan_dst16(leader_rloc16).must_next()


if __name__ == '__main__':
    unittest.main()
