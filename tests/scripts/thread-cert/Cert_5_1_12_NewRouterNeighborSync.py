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

import config
import mle
import node

LEADER = 1
ROUTER1 = 2
ROUTER2 = 3


class Cert_5_1_12_NewRouterSync(unittest.TestCase):

    def setUp(self):
        self.simulator = config.create_default_simulator()

        self.nodes = {}
        for i in range(1, 4):
            self.nodes[i] = node.Node(i, simulator=self.simulator)

        self.nodes[LEADER].set_panid(0xface)
        self.nodes[LEADER].set_mode('rsdn')
        self.nodes[LEADER].add_whitelist(self.nodes[ROUTER1].get_addr64())
        self.nodes[LEADER].add_whitelist(self.nodes[ROUTER2].get_addr64())
        self.nodes[LEADER].enable_whitelist()

        self.nodes[ROUTER1].set_panid(0xface)
        self.nodes[ROUTER1].set_mode('rsdn')
        self.nodes[ROUTER1].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[ROUTER1].enable_whitelist()
        self.nodes[ROUTER1].set_router_selection_jitter(1)

        self.nodes[ROUTER2].set_panid(0xface)
        self.nodes[ROUTER2].set_mode('rsdn')
        self.nodes[ROUTER2].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[ROUTER2].enable_whitelist()
        self.nodes[ROUTER2].set_router_selection_jitter(1)

    def tearDown(self):
        for node in list(self.nodes.values()):
            node.stop()
            node.destroy()
        self.simulator.stop()

    def verify_step_4(self, router1_messages, router2_messages, req_receiver, accept_receiver):
        if router2_messages.contains_mle_message(mle.CommandType.LINK_REQUEST) and \
            (router1_messages.contains_mle_message(mle.CommandType.LINK_ACCEPT) or
                router1_messages.contains_mle_message(mle.CommandType.LINK_ACCEPT_AND_REQUEST)):

            msg = router2_messages.next_mle_message(mle.CommandType.LINK_REQUEST)

            msg.assertSentToNode(self.nodes[req_receiver])
            msg.assertMleMessageContainsTlv(mle.SourceAddress)
            msg.assertMleMessageContainsTlv(mle.LeaderData)
            msg.assertMleMessageContainsTlv(mle.Challenge)
            msg.assertMleMessageContainsTlv(mle.Version)
            msg.assertMleMessageContainsTlv(mle.TlvRequest)

            msg = router1_messages.next_mle_message_of_one_of_command_types(mle.CommandType.LINK_ACCEPT_AND_REQUEST,
                                                                            mle.CommandType.LINK_ACCEPT)
            self.assertIsNotNone(msg)

            msg.assertSentToNode(self.nodes[accept_receiver])
            msg.assertMleMessageContainsTlv(mle.SourceAddress)
            msg.assertMleMessageContainsTlv(mle.LeaderData)
            msg.assertMleMessageContainsTlv(mle.LinkLayerFrameCounter)
            msg.assertMleMessageContainsOptionalTlv(mle.MleFrameCounter)
            msg.assertMleMessageContainsTlv(mle.Version)
            msg.assertMleMessageContainsTlv(mle.LinkMargin)

            if msg.mle.command.type == mle.CommandType.LINK_ACCEPT_AND_REQUEST:
                msg.assertMleMessageContainsTlv(mle.TlvRequest)
                msg.assertMleMessageContainsTlv(mle.Challenge)

            return True

        else:
            return False

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[ROUTER1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER1].get_state(), 'router')

        self.nodes[ROUTER2].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER2].get_state(), 'router')

        self.simulator.go(10)

        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        router1_messages = self.simulator.get_messages_sent_by(ROUTER1)
        router2_messages = self.simulator.get_messages_sent_by(ROUTER2)

        # 2 - Router1
        msg = router1_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)
        msg.assertSentWithHopLimit(255)
        msg.assertSentToDestinationAddress("ff02::1")
        msg.assertMleMessageContainsTlv(mle.SourceAddress)
        msg.assertMleMessageContainsTlv(mle.LeaderData)
        msg.assertMleMessageContainsTlv(mle.Route64)

        self.nodes[ROUTER1].add_whitelist(self.nodes[ROUTER2].get_addr64())
        self.nodes[ROUTER2].add_whitelist(self.nodes[ROUTER1].get_addr64())

        self.simulator.go(35)

        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        router1_messages = self.simulator.get_messages_sent_by(ROUTER1)
        router2_messages = self.simulator.get_messages_sent_by(ROUTER2)

        # 4 - Router1, Router2
        self.assertTrue(self.verify_step_4(router1_messages, router2_messages, ROUTER1, ROUTER2) or
                        self.verify_step_4(router2_messages, router1_messages, ROUTER2, ROUTER1))

if __name__ == '__main__':
    unittest.main()
