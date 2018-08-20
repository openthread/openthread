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
ROUTER = 2


class Cert_5_1_13_RouterReset(unittest.TestCase):

    def setUp(self):
        self.simulator = config.create_default_simulator()

        self.nodes = {}
        for i in range(1, 3):
            self.nodes[i] = node.Node(i, simulator=self.simulator)

        self.nodes[LEADER].set_panid(0xface)
        self.nodes[LEADER].set_mode('rsdn')
        self.nodes[LEADER].add_whitelist(self.nodes[ROUTER].get_addr64())
        self.nodes[LEADER].enable_whitelist()

        self.nodes[ROUTER].set_panid(0xface)
        self.nodes[ROUTER].set_mode('rsdn')
        self._setUpRouter()

    def _setUpRouter(self):
        self.nodes[ROUTER].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[ROUTER].enable_whitelist()
        self.nodes[ROUTER].set_router_selection_jitter(1)

    def tearDown(self):
        for node in list(self.nodes.values()):
            node.stop()
            node.destroy()
        self.simulator.stop()

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[ROUTER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER].get_state(), 'router')

        rloc16 = self.nodes[ROUTER].get_addr16()

        self.nodes[ROUTER].reset()
        self._setUpRouter()
        self.simulator.go(5)

        self.nodes[ROUTER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER].get_state(), 'router')
        self.assertEqual(self.nodes[ROUTER].get_addr16(), rloc16)

        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        router1_messages = self.simulator.get_messages_sent_by(ROUTER)

        # 1 - All
        leader_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)

        router1_messages.next_mle_message(mle.CommandType.PARENT_REQUEST)
        leader_messages.next_mle_message(mle.CommandType.PARENT_RESPONSE)

        router1_messages.next_mle_message(mle.CommandType.CHILD_ID_REQUEST)
        leader_messages.next_mle_message(mle.CommandType.CHILD_ID_RESPONSE)

        msg = router1_messages.next_coap_message("0.02")
        msg.assertCoapMessageRequestUriPath("/a/as")

        msg = leader_messages.next_coap_message("2.04")

        router1_messages.next_mle_message(mle.CommandType.LINK_REQUEST)
        msg = leader_messages.next_mle_message_of_one_of_command_types(mle.CommandType.LINK_ACCEPT_AND_REQUEST,
                                                                       mle.CommandType.LINK_ACCEPT)
        self.assertIsNotNone(msg)

        # 2 - Router1 / Leader
        msg = router1_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)
        msg.assertSentWithHopLimit(255)
        msg.assertSentToDestinationAddress("ff02::1")
        msg.assertMleMessageContainsTlv(mle.SourceAddress)
        msg.assertMleMessageContainsTlv(mle.LeaderData)
        msg.assertMleMessageContainsTlv(mle.Route64)

        msg = leader_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)
        msg.assertSentWithHopLimit(255)
        msg.assertSentToDestinationAddress("ff02::1")
        msg.assertMleMessageContainsTlv(mle.SourceAddress)
        msg.assertMleMessageContainsTlv(mle.LeaderData)
        msg.assertMleMessageContainsTlv(mle.Route64)

        # 4 - Router1
        msg = router1_messages.next_mle_message(mle.CommandType.LINK_REQUEST)
        msg.assertSentToDestinationAddress("ff02::2")
        msg.assertMleMessageContainsTlv(mle.Challenge)
        msg.assertMleMessageContainsTlv(mle.Version)
        msg.assertMleMessageContainsTlv(mle.TlvRequest)

        tlv_request = msg.get_mle_message_tlv(mle.TlvRequest)
        self.assertIn(mle.TlvType.ROUTE64, tlv_request.tlvs)
        self.assertIn(mle.TlvType.ADDRESS16, tlv_request.tlvs)

        # 5 - Leader
        msg = leader_messages.next_mle_message(mle.CommandType.LINK_ACCEPT)
        msg.assertSentToNode(self.nodes[ROUTER])
        msg.assertMleMessageContainsTlv(mle.SourceAddress)
        msg.assertMleMessageContainsTlv(mle.LeaderData)
        msg.assertMleMessageContainsTlv(mle.Response)
        msg.assertMleMessageContainsTlv(mle.LinkLayerFrameCounter)
        msg.assertMleMessageContainsOptionalTlv(mle.MleFrameCounter)
        msg.assertMleMessageContainsTlv(mle.Address16)
        msg.assertMleMessageContainsTlv(mle.Version)
        msg.assertMleMessageContainsTlv(mle.Route64)
        msg.assertMleMessageContainsOptionalTlv(mle.Challenge)


if __name__ == '__main__':
    unittest.main()
