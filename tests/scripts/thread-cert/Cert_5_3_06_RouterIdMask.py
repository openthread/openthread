#!/usr/bin/env python
#
#  Copyright (c) 2016, The OpenThread Authors.
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

import time
import unittest

import node
import mle
import config
import command

DUT_LEADER = 1
ROUTER1 = 2
ROUTER2 = 3

class Cert_5_3_6_RouterIdMask(unittest.TestCase):
    def setUp(self):
        self.simulator = config.create_default_simulator()

        self.nodes = {}
        for i in range(1,4):
            self.nodes[i] = node.Node(i, simulator=self.simulator)

        self.nodes[DUT_LEADER].set_panid(0xface)
        self.nodes[DUT_LEADER].set_mode('rsdn')
        self.nodes[DUT_LEADER].add_whitelist(self.nodes[ROUTER1].get_addr64())
        self.nodes[DUT_LEADER].enable_whitelist()

        self.nodes[ROUTER1].set_panid(0xface)
        self.nodes[ROUTER1].set_mode('rsdn')
        self.nodes[ROUTER1].add_whitelist(self.nodes[DUT_LEADER].get_addr64())
        self.nodes[ROUTER1].add_whitelist(self.nodes[ROUTER2].get_addr64())
        self.nodes[ROUTER1].enable_whitelist()
        self.nodes[ROUTER1].set_router_selection_jitter(1)

        self.nodes[ROUTER2].set_panid(0xface)
        self.nodes[ROUTER2].set_mode('rsdn')
        self._setUpRouter2()

    def _setUpRouter2(self):
        self.nodes[ROUTER2].add_whitelist(self.nodes[ROUTER1].get_addr64())
        self.nodes[ROUTER2].enable_whitelist()
        self.nodes[ROUTER2].set_router_selection_jitter(1)

    def tearDown(self):
        for node in list(self.nodes.values()):
            node.stop()
            node.destroy()
        self.simulator.stop()

    def test(self):
        # 1
        self.nodes[DUT_LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[DUT_LEADER].get_state(), 'leader')

        self.nodes[ROUTER1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER1].get_state(), 'router')

        self.nodes[ROUTER2].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER2].get_state(), 'router')
        router2_id = self.nodes[ROUTER2].get_router_id()

        # Wait DUT_LEADER to establish routing to ROUTER2 via ROUTER1's MLE advertisement.
        self.simulator.go(config.MAX_ADVERTISEMENT_INTERVAL)

        # 2
        self.nodes[ROUTER2].reset()
        self._setUpRouter2()

        # 3 & 4
        # Flush the message queue to avoid possible impact on follow-up verification.
        dut_messages = self.simulator.get_messages_sent_by(DUT_LEADER)

        # Verify the cost from DUT_LEADER to ROUTER2 goes to infinity in 12 mins.
        routing_cost = 1
        for i in range(0, 24):
            self.simulator.go(30)
            print("%ss" %((i + 1) * 30))

            leader_messages = self.simulator.get_messages_sent_by(DUT_LEADER)
            msg = leader_messages.last_mle_message(mle.CommandType.ADVERTISEMENT, False)
            if msg == None:
                continue

            self.assertTrue(command.check_id_set(msg, router2_id))

            routing_cost = command.get_routing_cost(msg, router2_id)
            if routing_cost == 0:
                break
        self.assertTrue(routing_cost == 0)

        self.simulator.go(config.INFINITE_COST_TIMEOUT + config.MAX_ADVERTISEMENT_INTERVAL)
        leader_messages = self.simulator.get_messages_sent_by(DUT_LEADER)
        msg = leader_messages.last_mle_message(mle.CommandType.ADVERTISEMENT)
        self.assertFalse(command.check_id_set(msg, router2_id))

        # 5
        # Flush the message queue to avoid possible impact on follow-up verification.
        dut_messages = self.simulator.get_messages_sent_by(DUT_LEADER)

        self.nodes[ROUTER2].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER2].get_state(), 'router')

        self.simulator.go(config.MAX_ADVERTISEMENT_INTERVAL)
        leader_messages = self.simulator.get_messages_sent_by(DUT_LEADER)
        leader_messages.last_mle_message(mle.CommandType.ADVERTISEMENT)

        # 6
        self.nodes[ROUTER1].reset()
        self.nodes[ROUTER2].reset()

        router1_id = self.nodes[ROUTER1].get_router_id()
        router2_id = self.nodes[ROUTER2].get_router_id()

        self.simulator.go(config.MAX_NEIGHBOR_AGE + config.MAX_ADVERTISEMENT_INTERVAL)
        leader_messages = self.simulator.get_messages_sent_by(DUT_LEADER)
        msg = leader_messages.last_mle_message(mle.CommandType.ADVERTISEMENT)
        self.assertEqual(command.get_routing_cost(msg, router1_id), 0)

        self.simulator.go(config.INFINITE_COST_TIMEOUT + config.MAX_ADVERTISEMENT_INTERVAL)
        leader_messages = self.simulator.get_messages_sent_by(DUT_LEADER)
        msg = leader_messages.last_mle_message(mle.CommandType.ADVERTISEMENT)
        self.assertFalse(command.check_id_set(msg, router1_id))
        self.assertFalse(command.check_id_set(msg, router2_id))

if __name__ == '__main__':
    unittest.main()
