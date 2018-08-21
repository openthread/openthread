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
import mle

LEADER = 1
DUT_ROUTER1 = 2
ROUTER2 = 3
ROUTER24 = 24

class Cert_5_2_06_RouterDowngrade(unittest.TestCase):
    def setUp(self):
        self.simulator = config.create_default_simulator()

        self.nodes = {}
        for i in range(1, 25):
            self.nodes[i] = node.Node(i, simulator=self.simulator)
            self.nodes[i].set_panid()
            self.nodes[i].set_mode('rsdn')
            self.nodes[i].set_router_selection_jitter(1)
            if i != DUT_ROUTER1:
                self.nodes[i].set_router_upgrade_threshold(32)
                self.nodes[i].set_router_downgrade_threshold(32)

    def tearDown(self):
        for node in list(self.nodes.values()):
            node.stop()
            node.destroy()
        self.simulator.stop()

    def test(self):
        # 1 Ensure topology is formed correctly without ROUTER24.
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        for i in range(2, 24):
            self.nodes[i].start()
            self.simulator.go(5)
            self.assertEqual(self.nodes[i].get_state(), 'router')

        # This method flushes the message queue so calling this method again will return only the newly logged messages.
        dut_messages = self.simulator.get_messages_sent_by(DUT_ROUTER1)

        # 2 ROUTER24: Attach to network.
        # All reference testbed devices have been configured with downgrade threshold as 32 except DUT_ROUTER1,
        # so we don't need to ensure ROUTER24 has a better link quality on posix.
        self.nodes[ROUTER24].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER24].get_state(), 'router')

        # 3 DUT_ROUTER1:
        self.simulator.go(10)
        self.assertEqual(self.nodes[DUT_ROUTER1].get_state(), 'child')

        # Verify it sent a Parent Request and Child ID Request.
        dut_messages = self.simulator.get_messages_sent_by(DUT_ROUTER1)
        dut_messages.next_mle_message(mle.CommandType.PARENT_REQUEST)
        dut_messages.next_mle_message(mle.CommandType.CHILD_ID_REQUEST)

        # Verify it sent an Address Release Message to the Leader when it attached as a child.
        msg = dut_messages.next_coap_message('0.02')
        command.check_address_release(msg, self.nodes[LEADER])

        # 4 & 5
        router1_rloc = self.nodes[DUT_ROUTER1].get_ip6_address(config.ADDRESS_TYPE.RLOC)
        self.assertTrue(self.nodes[LEADER].ping(router1_rloc))

if __name__ == '__main__':
    unittest.main()
