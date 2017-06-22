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
import network_layer
import node

LEADER = 1
ROUTER1 = 2
SNIFFER = 3


class Cert_5_1_05_RouterAddressTimeout(unittest.TestCase):

    def setUp(self):
        self.nodes = {}
        for i in range(1, 3):
            self.nodes[i] = node.Node(i)

        self.nodes[LEADER].set_panid(0xface)
        self.nodes[LEADER].set_mode('rsdn')
        self.nodes[LEADER].add_whitelist(self.nodes[ROUTER1].get_addr64())
        self.nodes[LEADER].enable_whitelist()

        self.nodes[ROUTER1].set_panid(0xface)
        self.nodes[ROUTER1].set_mode('rsdn')
        self._setUpRouter1()

        self.sniffer = config.create_default_thread_sniffer(SNIFFER)
        self.sniffer.start()

    def _setUpRouter1(self):
        self.nodes[ROUTER1].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[ROUTER1].enable_whitelist()
        self.nodes[ROUTER1].set_router_selection_jitter(1)

    def tearDown(self):
        self.sniffer.stop()
        del self.sniffer

        for node in list(self.nodes.values()):
            node.stop()
        del self.nodes

    def test(self):
        self.nodes[LEADER].start()
        self.nodes[LEADER].set_state('leader')
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')
        time.sleep(4)

        self.nodes[ROUTER1].start()
        time.sleep(5)
        self.assertEqual(self.nodes[ROUTER1].get_state(), 'router')

        rloc16 = self.nodes[ROUTER1].get_addr16()

        self.nodes[ROUTER1].reset()
        self._setUpRouter1()
        time.sleep(200)
        self.nodes[ROUTER1].start()
        time.sleep(5)
        self.assertEqual(self.nodes[ROUTER1].get_state(), 'router')
        self.assertNotEqual(self.nodes[ROUTER1].get_addr16(), rloc16)

        rloc16 = self.nodes[ROUTER1].get_addr16()
        self.nodes[ROUTER1].reset()
        self._setUpRouter1()
        time.sleep(300)
        self.nodes[ROUTER1].start()
        time.sleep(5)
        self.assertEqual(self.nodes[ROUTER1].get_state(), 'router')
        self.assertEqual(self.nodes[ROUTER1].get_addr16(), rloc16)

        leader_messages = self.sniffer.get_messages_sent_by(LEADER)
        router1_messages = self.sniffer.get_messages_sent_by(ROUTER1)

        # 2 - All
        leader_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)

        router1_messages.next_mle_message(mle.CommandType.PARENT_REQUEST)
        leader_messages.next_mle_message(mle.CommandType.PARENT_RESPONSE)

        router1_messages.next_mle_message(mle.CommandType.CHILD_ID_REQUEST)
        leader_messages.next_mle_message(mle.CommandType.CHILD_ID_RESPONSE)

        # 3 - Router1
        router1_messages.next_mle_message(mle.CommandType.LINK_REQUEST)
        router1_messages.next_mle_message(mle.CommandType.PARENT_REQUEST)
        router1_messages.next_mle_message(mle.CommandType.CHILD_ID_REQUEST)

        msg = router1_messages.next_coap_message("0.02")
        msg.assertCoapMessageRequestUriPath("/a/as")

        # 4 - Leader
        leader_messages.next_mle_message(mle.CommandType.PARENT_RESPONSE)
        leader_messages.next_mle_message(mle.CommandType.CHILD_ID_RESPONSE)

        msg = leader_messages.next_coap_message("2.04")
        msg.assertCoapMessageContainsTlv(network_layer.Status)
        msg.assertCoapMessageContainsOptionalTlv(network_layer.RouterMask)

        status_tlv = msg.get_coap_message_tlv(network_layer.Status)
        self.assertEqual(network_layer.StatusValues.SUCCESS, status_tlv.status)

        # 6 - Router1
        router1_messages.next_mle_message(mle.CommandType.LINK_REQUEST)
        router1_messages.next_mle_message(mle.CommandType.PARENT_REQUEST)
        router1_messages.next_mle_message(mle.CommandType.CHILD_ID_REQUEST)

        msg = router1_messages.next_coap_message("0.02")
        msg.assertCoapMessageRequestUriPath("/a/as")

        # 7 - Leader
        leader_messages.next_mle_message(mle.CommandType.PARENT_RESPONSE)
        leader_messages.next_mle_message(mle.CommandType.CHILD_ID_RESPONSE)

        msg = leader_messages.next_coap_message("2.04")
        msg.assertCoapMessageContainsTlv(network_layer.Status)
        msg.assertCoapMessageContainsOptionalTlv(network_layer.RouterMask)

        status_tlv = msg.get_coap_message_tlv(network_layer.Status)
        self.assertEqual(network_layer.StatusValues.SUCCESS, status_tlv.status)


if __name__ == '__main__':
    unittest.main()
