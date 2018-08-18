#!/usr/bin/env python
#
#  Copyright (c) 2018, The OpenThread Authors.
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

import shutil
import os

LEADER = 1
DUT_ROUTER1 = 2
REED1 = 3
MED1 = 4

class Cert_5_2_01_REEDAttach(unittest.TestCase):
    def setUp(self):
        self.simulator = config.create_default_simulator()

        self.nodes = {}
        for i in range(1, 5):
            self.nodes[i] = node.Node(i, i == MED1, simulator=self.simulator)

        self.nodes[LEADER].set_panid(0xface)
        self.nodes[LEADER].set_mode('rsdn')
        self.nodes[LEADER].add_whitelist(self.nodes[DUT_ROUTER1].get_addr64())
        self.nodes[LEADER].enable_whitelist()

        self.nodes[DUT_ROUTER1].set_panid(0xface)
        self.nodes[DUT_ROUTER1].set_mode('rsdn')
        self.nodes[DUT_ROUTER1].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[DUT_ROUTER1].add_whitelist(self.nodes[REED1].get_addr64())
        self.nodes[DUT_ROUTER1].enable_whitelist()
        self.nodes[DUT_ROUTER1].set_router_selection_jitter(1)

        self.nodes[REED1].set_panid(0xface)
        self.nodes[REED1].set_mode('rsdn')
        self.nodes[REED1].add_whitelist(self.nodes[DUT_ROUTER1].get_addr64())
        self.nodes[REED1].add_whitelist(self.nodes[MED1].get_addr64())
        self.nodes[REED1].enable_whitelist()
        self.nodes[REED1].set_router_selection_jitter(1)
        self.nodes[REED1].set_router_upgrade_threshold(1)

        self.nodes[MED1].set_panid(0xface)
        self.nodes[MED1].set_mode('rsn')
        self.nodes[MED1].add_whitelist(self.nodes[REED1].get_addr64())
        self.nodes[MED1].enable_whitelist()

    def tearDown(self):
        for node in list(self.nodes.values()):
            node.stop()
            node.destroy()
        self.simulator.stop()

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        # 1 DUT_ROUTER1: Attach to LEADER
        self.nodes[DUT_ROUTER1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[DUT_ROUTER1].get_state(), 'router')

        # DUT_ROUTER1: Verify MLE advertisements
        router1_messages = self.simulator.get_messages_sent_by(DUT_ROUTER1)
        msg = router1_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)
        command.check_mle_advertisement(msg)

        # 2 REED1: Attach to DUT_ROUTER1
        self.nodes[REED1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[REED1].get_state(), 'child')

        # 3 DUT_ROUTER1: Verify MLE Parent Response
        router1_messages = self.simulator.get_messages_sent_by(DUT_ROUTER1)
        msg = router1_messages.next_mle_message(mle.CommandType.PARENT_RESPONSE)
        msg.assertSentToNode(self.nodes[REED1])
        command.check_parent_response(msg)

        # 4 DUT_ROUTER1: Verify MLE Child ID Response
        msg = router1_messages.next_mle_message(mle.CommandType.CHILD_ID_RESPONSE)
        msg.assertSentToNode(self.nodes[REED1])
        command.check_child_id_response(msg)

        # 5 Omitted

        # 6 MED1: Attach to REED1
        self.nodes[MED1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[MED1].get_state(), 'child')

        # 7 REED1: Verify sending Address Solicit Request to DUT_ROUTER1
        reed1_messages = self.simulator.get_messages_sent_by(REED1)
        msg = reed1_messages.next_coap_message('0.02')
        reed1_ipv6_address = msg.ipv6_packet.ipv6_header.source_address.compressed
        msg.assertSentToNode(self.nodes[DUT_ROUTER1]);
        msg.assertCoapMessageRequestUriPath('/a/as')

        # 8 DUT_ROUTER1: Verify forwarding REED1's Address Solicit Request to LEADER
        router1_messages = self.simulator.get_messages_sent_by(DUT_ROUTER1)
        msg = router1_messages.next_coap_message('0.02')
        msg.assertSentToNode(self.nodes[LEADER]);
        msg.assertCoapMessageRequestUriPath('/a/as')

        # DUT_ROUTER1: Verify forwarding LEADER's Address Solicit Response to REED1
        msg = router1_messages.next_coap_message('2.04')
        msg.assertSentToDestinationAddress(reed1_ipv6_address)

        self.simulator.go(config.MAX_ADVERTISEMENT_INTERVAL)

        # 9 LEADER: Verify connectivity by sending an ICMPv6 Echo Request to REED1
        for addr in self.nodes[REED1].get_addrs():
            if addr[0:4] != 'fe80':
                self.assertTrue(self.nodes[LEADER].ping(addr))

if __name__ == '__main__':
    unittest.main()
