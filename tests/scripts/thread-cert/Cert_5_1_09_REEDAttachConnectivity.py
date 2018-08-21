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
REED0 = 3
REED1 = 4
ROUTER2 = 5


class Cert_5_1_09_REEDAttachConnectivity(unittest.TestCase):

    def setUp(self):
        self.simulator = config.create_default_simulator()

        self.nodes = {}
        for i in range(1, 6):
            self.nodes[i] = node.Node(i, simulator=self.simulator)

        self.nodes[LEADER].set_panid(0xface)
        self.nodes[LEADER].set_mode('rsdn')
        self.nodes[LEADER].add_whitelist(self.nodes[ROUTER1].get_addr64())
        self.nodes[LEADER].add_whitelist(self.nodes[REED0].get_addr64())
        self.nodes[LEADER].add_whitelist(self.nodes[REED1].get_addr64())
        self.nodes[LEADER].enable_whitelist()

        self.nodes[ROUTER1].set_panid(0xface)
        self.nodes[ROUTER1].set_mode('rsdn')
        self.nodes[ROUTER1].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[ROUTER1].add_whitelist(self.nodes[REED1].get_addr64())
        self.nodes[ROUTER1].enable_whitelist()
        self.nodes[ROUTER1].set_router_selection_jitter(1)

        self.nodes[REED0].set_panid(0xface)
        self.nodes[REED0].set_mode('rsdn')
        self.nodes[REED0].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[REED0].add_whitelist(self.nodes[ROUTER2].get_addr64())
        self.nodes[REED0].set_router_upgrade_threshold(0)
        self.nodes[REED0].enable_whitelist()

        self.nodes[REED1].set_panid(0xface)
        self.nodes[REED1].set_mode('rsdn')
        self.nodes[REED1].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[REED1].add_whitelist(self.nodes[ROUTER1].get_addr64())
        self.nodes[REED1].add_whitelist(self.nodes[ROUTER2].get_addr64())
        self.nodes[REED1].set_router_upgrade_threshold(0)
        self.nodes[REED1].enable_whitelist()

        self.nodes[ROUTER2].set_panid(0xface)
        self.nodes[ROUTER2].set_mode('rsdn')
        self.nodes[ROUTER2].add_whitelist(self.nodes[REED0].get_addr64())
        self.nodes[ROUTER2].add_whitelist(self.nodes[REED1].get_addr64())
        self.nodes[ROUTER2].enable_whitelist()
        self.nodes[ROUTER2].set_router_selection_jitter(1)

    def tearDown(self):
        for node in list(self.nodes.values()):
            node.stop()
            node.destroy()
        self.simulator.stop()

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        for i in range(2, 5):
            self.nodes[i].start()

        self.simulator.go(5)

        self.assertEqual(self.nodes[ROUTER1].get_state(), 'router')
        self.assertEqual(self.nodes[REED0].get_state(), 'child')
        self.assertEqual(self.nodes[REED1].get_state(), 'child')

        self.simulator.go(config.MAX_ADVERTISEMENT_INTERVAL)

        self.nodes[ROUTER2].start()
        self.simulator.go(10)
        self.assertEqual(self.nodes[ROUTER2].get_state(), 'router')
        self.assertEqual(self.nodes[REED1].get_state(), 'router')

        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        router1_messages = self.simulator.get_messages_sent_by(ROUTER1)
        reed0_messages = self.simulator.get_messages_sent_by(REED0)
        reed1_messages = self.simulator.get_messages_sent_by(REED1)
        router2_messages = self.simulator.get_messages_sent_by(ROUTER2)

        # 1 - Leader, Router1, REED1, REED2
        leader_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)
        router1_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)

        # 2 - Router2
        msg = router2_messages.next_mle_message(mle.CommandType.PARENT_REQUEST)
        msg.assertSentWithHopLimit(255)
        msg.assertSentToDestinationAddress("ff02::2")
        msg.assertMleMessageContainsTlv(mle.Mode)
        msg.assertMleMessageContainsTlv(mle.Challenge)
        msg.assertMleMessageContainsTlv(mle.ScanMask)
        msg.assertMleMessageContainsTlv(mle.Version)

        scan_mask_tlv = msg.get_mle_message_tlv(mle.ScanMask)
        self.assertEqual(1, scan_mask_tlv.router)
        self.assertEqual(0, scan_mask_tlv.end_device)

        # 4 - Router2
        msg = router2_messages.next_mle_message(mle.CommandType.PARENT_REQUEST)
        msg.assertSentWithHopLimit(255)
        msg.assertSentToDestinationAddress("ff02::2")
        msg.assertMleMessageContainsTlv(mle.Mode)
        msg.assertMleMessageContainsTlv(mle.Challenge)
        msg.assertMleMessageContainsTlv(mle.ScanMask)
        msg.assertMleMessageContainsTlv(mle.Version)

        scan_mask_tlv = msg.get_mle_message_tlv(mle.ScanMask)
        self.assertEqual(1, scan_mask_tlv.router)
        self.assertEqual(1, scan_mask_tlv.end_device)

        # 5 - REED1, REED2
        msg = reed0_messages.next_mle_message(mle.CommandType.PARENT_RESPONSE)
        connectivity_tlv_reed0 = msg.get_mle_message_tlv(mle.Connectivity)

        msg = reed1_messages.next_mle_message(mle.CommandType.PARENT_RESPONSE)
        connectivity_tlv_reed1 = msg.get_mle_message_tlv(mle.Connectivity)

        self.assertGreater(connectivity_tlv_reed1.link_quality_3, connectivity_tlv_reed0.link_quality_3)

        # 6 - Router2
        msg = router2_messages.next_mle_message(mle.CommandType.CHILD_ID_REQUEST)
        msg.assertSentToNode(self.nodes[REED1])
        msg.assertMleMessageContainsTlv(mle.Response)
        msg.assertMleMessageContainsTlv(mle.LinkLayerFrameCounter)
        msg.assertMleMessageContainsOptionalTlv(mle.MleFrameCounter)
        msg.assertMleMessageContainsTlv(mle.Mode)
        msg.assertMleMessageContainsTlv(mle.Timeout)
        msg.assertMleMessageContainsTlv(mle.Version)
        msg.assertMleMessageContainsTlv(mle.TlvRequest)
        msg.assertMleMessageDoesNotContainTlv(mle.AddressRegistration)


if __name__ == '__main__':
    unittest.main()
