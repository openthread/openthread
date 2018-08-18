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
import network_layer
import config

DUT_LEADER = 1
ROUTER_1 = 2
ROUTER_31 = 32
ROUTER_32 = 33

class Cert_5_2_3_LeaderReject2Hops(unittest.TestCase):
    def setUp(self):
        self.simulator = config.create_default_simulator()

        self.nodes = {}

        self.nodes[DUT_LEADER] = node.Node(DUT_LEADER, simulator=self.simulator)
        self.nodes[DUT_LEADER].set_panid(0xface)
        self.nodes[DUT_LEADER].set_mode('rsdn')
        self.nodes[DUT_LEADER].enable_whitelist()
        self.nodes[DUT_LEADER].set_router_upgrade_threshold(32)
        self.nodes[DUT_LEADER].set_router_downgrade_threshold(33)

        for i in range(2, 33):
            self.nodes[i] = node.Node(i, simulator=self.simulator)
            self.nodes[i].set_panid(0xface)
            self.nodes[i].set_mode('rsdn')
            self.nodes[i].add_whitelist(self.nodes[DUT_LEADER].get_addr64())
            self.nodes[DUT_LEADER].add_whitelist(self.nodes[i].get_addr64())
            self.nodes[i].enable_whitelist()
            self.nodes[i].set_router_upgrade_threshold(33)
            self.nodes[i].set_router_downgrade_threshold(33)
            self.nodes[i].set_router_selection_jitter(1)

        self.nodes[ROUTER_32] = node.Node(ROUTER_32, simulator=self.simulator)
        self.nodes[ROUTER_32].set_panid(0xface)
        self.nodes[ROUTER_32].set_mode('rsdn')
        self.nodes[ROUTER_32].add_whitelist(self.nodes[ROUTER_1].get_addr64())
        self.nodes[ROUTER_1].add_whitelist(self.nodes[ROUTER_32].get_addr64())
        self.nodes[ROUTER_32].enable_whitelist()
        self.nodes[ROUTER_32].set_router_upgrade_threshold(33)
        self.nodes[ROUTER_32].set_router_downgrade_threshold(33)
        self.nodes[ROUTER_32].set_router_selection_jitter(1)

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

        for i in range(2, 32):
            self.nodes[i].start()
            self.simulator.go(5)
            self.assertEqual(self.nodes[i].get_state(), 'router')

        leader_messages = self.simulator.get_messages_sent_by(DUT_LEADER)

        # 2
        self.nodes[ROUTER_31].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER_31].get_state(), 'router')

        # 3 - DUT_LEADER
        # This method flushes the message queue so calling this method again will return only the newly logged messages.
        leader_messages = self.simulator.get_messages_sent_by(DUT_LEADER)
        msg = leader_messages.next_coap_message('2.04')
        msg.assertCoapMessageContainsTlv(network_layer.Status)
        msg.assertCoapMessageContainsTlv(network_layer.RouterMask)
        msg.assertCoapMessageContainsTlv(network_layer.Rloc16)

        status_tlv = msg.get_coap_message_tlv(network_layer.Status)
        self.assertEqual(network_layer.StatusValues.SUCCESS, status_tlv.status)

        # 4 - DUT_LEADER
        msg = leader_messages.last_mle_message(mle.CommandType.ADVERTISEMENT)
        msg.assertAssignedRouterQuantity(32)

        # 5 - Router_32
        self.nodes[ROUTER_32].start()
        self.simulator.go(5)

        # 6 - DUT_LEADER
        leader_messages = self.simulator.get_messages_sent_by(DUT_LEADER)
        msg = leader_messages.next_coap_message('2.04')
        msg.assertCoapMessageContainsTlv(network_layer.Status)

        status_tlv = msg.get_coap_message_tlv(network_layer.Status)
        self.assertEqual(network_layer.StatusValues.NO_ADDRESS_AVAILABLE, status_tlv.status)

if __name__ == '__main__':
    unittest.main()
