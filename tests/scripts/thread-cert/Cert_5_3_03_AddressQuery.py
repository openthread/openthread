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
import config
import command

LEADER = 1
ROUTER1 = 2
DUT_ROUTER2 = 3
ROUTER3 = 4
MED1 = 5
MED1_TIMEOUT = 3

class Cert_5_3_3_AddressQuery(unittest.TestCase):
    def setUp(self):
        self.simulator = config.create_default_simulator()

        self.nodes = {}
        for i in range(1,6):
            self.nodes[i] = node.Node(i, (i == MED1), simulator=self.simulator)

        self.nodes[LEADER].set_panid()
        self.nodes[LEADER].set_mode('rsdn')
        self.nodes[LEADER].add_whitelist(self.nodes[ROUTER1].get_addr64())
        self.nodes[LEADER].add_whitelist(self.nodes[DUT_ROUTER2].get_addr64())
        self.nodes[LEADER].add_whitelist(self.nodes[ROUTER3].get_addr64())
        self.nodes[LEADER].enable_whitelist()

        self.nodes[ROUTER1].set_panid()
        self.nodes[ROUTER1].set_mode('rsdn')
        self.nodes[ROUTER1].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[ROUTER1].enable_whitelist()
        self.nodes[ROUTER1].set_router_selection_jitter(1)

        self.nodes[DUT_ROUTER2].set_panid()
        self.nodes[DUT_ROUTER2].set_mode('rsdn')
        self.nodes[DUT_ROUTER2].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[DUT_ROUTER2].add_whitelist(self.nodes[ROUTER3].get_addr64())
        self.nodes[DUT_ROUTER2].add_whitelist(self.nodes[MED1].get_addr64())
        self.nodes[DUT_ROUTER2].enable_whitelist()
        self.nodes[DUT_ROUTER2].set_router_selection_jitter(1)

        self.nodes[ROUTER3].set_panid()
        self.nodes[ROUTER3].set_mode('rsdn')
        self.nodes[ROUTER3].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[ROUTER3].add_whitelist(self.nodes[DUT_ROUTER2].get_addr64())
        self.nodes[ROUTER3].enable_whitelist()
        self.nodes[ROUTER3].set_router_selection_jitter(1)

        self.nodes[MED1].set_panid()
        self.nodes[MED1].set_mode('rsn')
        self.nodes[MED1].add_whitelist(self.nodes[DUT_ROUTER2].get_addr64())
        self.nodes[MED1].set_timeout(MED1_TIMEOUT)
        self.nodes[MED1].enable_whitelist()

    def tearDown(self):
        for node in list(self.nodes.values()):
            node.stop()
            node.destroy()
        self.simulator.stop()

    def test(self):
        # 1
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[ROUTER1].start()
        self.nodes[DUT_ROUTER2].start()
        self.nodes[ROUTER3].start()
        self.nodes[MED1].start()
        self.simulator.go(5)

        self.assertEqual(self.nodes[ROUTER1].get_state(), 'router')
        self.assertEqual(self.nodes[DUT_ROUTER2].get_state(), 'router')
        self.assertEqual(self.nodes[ROUTER3].get_state(), 'router')
        self.assertEqual(self.nodes[MED1].get_state(), 'child')


        # 2
        # Flush the message queue to avoid possible impact on follow-up verification.
        dut_messages = self.simulator.get_messages_sent_by(DUT_ROUTER2)

        router3_mleid = self.nodes[ROUTER3].get_ip6_address(config.ADDRESS_TYPE.ML_EID)
        self.assertTrue(self.nodes[MED1].ping(router3_mleid))

        # Verify DUT_ROUTER2 sent an Address Query Request to the Realm local address.
        dut_messages = self.simulator.get_messages_sent_by(DUT_ROUTER2)
        msg = dut_messages.next_coap_message('0.02', '/a/aq')
        command.check_address_query(msg, self.nodes[DUT_ROUTER2], config.REALM_LOCAL_ALL_ROUTERS_ADDRESS)

        # 3
        # Wait the finish of address resolution traffic triggerred by previous ping.
        self.simulator.go(5)

        # Flush the message queue to avoid possible impact on follow-up verification.
        dut_messages = self.simulator.get_messages_sent_by(DUT_ROUTER2)

        med1_mleid = self.nodes[MED1].get_ip6_address(config.ADDRESS_TYPE.ML_EID)
        self.assertTrue(self.nodes[ROUTER1].ping(med1_mleid))

        # Verify DUT_ROUTER2 responded with an Address Notification.
        dut_messages = self.simulator.get_messages_sent_by(DUT_ROUTER2)
        msg = dut_messages.next_coap_message('0.02', '/a/an')
        command.check_address_notification(msg, self.nodes[DUT_ROUTER2], self.nodes[ROUTER1])

        # 4
        # Wait the finish of address resolution traffic triggerred by previous ping.
        self.simulator.go(5)

        # Flush the message queue to avoid possible impact on follow-up verification.
        dut_messages = self.simulator.get_messages_sent_by(DUT_ROUTER2)

        self.assertTrue(self.nodes[MED1].ping(router3_mleid))

        # Verify DUT_ROUTER2 didn't send an Address Query Request.
        dut_messages = self.simulator.get_messages_sent_by(DUT_ROUTER2)
        msg = dut_messages.next_coap_message('0.02', '/a/aq', False)
        assert msg is None, "The Address Query Request is not expected."

        # 5
        # Power off ROUTER3 and wait for leader to expire its Router ID.
        # In this topology, ROUTER3 has two neighbors (Leader and DUT_ROUTER2),
        # so the wait time is (MAX_NEIGHBOR_AGE (100s) + worst propagation time (32s * 15) for bad routing +\
        # INFINITE_COST_TIMEOUT (90s) + transmission time + extra redundancy), totally ~700s.
        self.nodes[ROUTER3].stop()
        self.simulator.go(700)

        # Flush the message queue to avoid possible impact on follow-up verification.
        dut_messages = self.simulator.get_messages_sent_by(DUT_ROUTER2)

        self.assertFalse(self.nodes[MED1].ping(router3_mleid))

        # Verify DUT_ROUTER2 sent an Address Query Request to the Realm local address.
        dut_messages = self.simulator.get_messages_sent_by(DUT_ROUTER2)
        msg = dut_messages.next_coap_message('0.02', '/a/aq')
        command.check_address_query(msg, self.nodes[DUT_ROUTER2], config.REALM_LOCAL_ALL_ROUTERS_ADDRESS)

        # 6
        self.nodes[MED1].stop()
        self.simulator.go(MED1_TIMEOUT)

        # Flush the message queue to avoid possible impact on follow-up verification.
        dut_messages = self.simulator.get_messages_sent_by(DUT_ROUTER2)

        self.assertFalse(self.nodes[ROUTER1].ping(med1_mleid))
        self.assertFalse(self.nodes[ROUTER1].ping(med1_mleid))

        # Verify DUT_ROUTER2 didn't respond with an Address Notification.
        dut_messages = self.simulator.get_messages_sent_by(DUT_ROUTER2)
        msg = dut_messages.next_coap_message('0.02', '/a/an', False)
        assert msg is None, "The Address Notification is not expected."

if __name__ == '__main__':
    unittest.main()
