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

import copy
import time
import unittest

import command
import config
import mle
import node

DUT_LEADER = 1
DUT_ROUTER1 = 2

class Cert_5_5_1_LeaderReboot(unittest.TestCase):
    def setUp(self):
        self.simulator = config.create_default_simulator()

        self.nodes = {}
        for i in range(1,3):
            self.nodes[i] = node.Node(i, simulator = self.simulator)

        self.nodes[DUT_LEADER].set_panid(0xface)
        self.nodes[DUT_LEADER].set_mode('rsdn')
        self._setUpLeader()

        self.nodes[DUT_ROUTER1].set_panid(0xface)
        self.nodes[DUT_ROUTER1].set_mode('rsdn')
        self.nodes[DUT_ROUTER1].add_whitelist(self.nodes[DUT_LEADER].get_addr64())
        self.nodes[DUT_ROUTER1].enable_whitelist()
        self.nodes[DUT_ROUTER1].set_router_selection_jitter(1)

    def _setUpLeader(self):
        self.nodes[DUT_LEADER].add_whitelist(self.nodes[DUT_ROUTER1].get_addr64())
        self.nodes[DUT_LEADER].enable_whitelist()

    def tearDown(self):
        for node in list(self.nodes.values()):
            node.stop()
            node.destroy()
        self.simulator.stop()

    def test(self):
        # 1 ALL: Build and verify the topology
        self.nodes[DUT_LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[DUT_LEADER].get_state(), 'leader')

        self.nodes[DUT_ROUTER1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[DUT_ROUTER1].get_state(), 'router')

        # 2 DUT_LEADER, DUT_ROUTER1: Verify both DUT_LEADER and DUT_ROUTER1 send MLE Advertisement message
        leader_messages = self.simulator.get_messages_sent_by(DUT_LEADER)
        msg = leader_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)
        command.check_mle_advertisement(msg)

        router1_messages = self.simulator.get_messages_sent_by(DUT_ROUTER1)
        msg = router1_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)
        command.check_mle_advertisement(msg)

        # Send a harness helper ping to the DUT
        router1_rloc = self.nodes[DUT_ROUTER1].get_ip6_address(config.ADDRESS_TYPE.RLOC)
        self.assertTrue(self.nodes[DUT_LEADER].ping(router1_rloc))

        leader_rloc = self.nodes[DUT_LEADER].get_ip6_address(config.ADDRESS_TYPE.RLOC)
        self.assertTrue(self.nodes[DUT_ROUTER1].ping(leader_rloc))

        # 3 DUT_LEADER: Reset DUT_LEADER
        leader_rloc16 = self.nodes[DUT_LEADER].get_addr16()
        self.nodes[DUT_LEADER].reset()
        self._setUpLeader()

        # Clean sniffer's buffer
        self.simulator.get_messages_sent_by(DUT_LEADER)
        self.simulator.get_messages_sent_by(DUT_ROUTER1)

        # DUT_LEADER sleep time is less than leader timeout value
        self.simulator.go(config.MAX_ADVERTISEMENT_INTERVAL)

        # Verify DUT_LEADER didn't send MLE Advertisement messages
        leader_messages = self.simulator.get_messages_sent_by(DUT_LEADER)
        msg = leader_messages.next_mle_message(mle.CommandType.ADVERTISEMENT, False)
        self.assertTrue(msg is None)

        self.nodes[DUT_LEADER].start()

        # Verify the DUT_LEADER is still a leader
        self.simulator.go(5)
        self.assertEqual(self.nodes[DUT_LEADER].get_state(), 'leader')
        self.assertEqual(self.nodes[DUT_LEADER].get_addr16(), leader_rloc16)

        # 4 DUT_LEADER: Verify DUT_LEADER sent a multicast Link Request message
        leader_messages = self.simulator.get_messages_sent_by(DUT_LEADER)
        leader_messages_temp = copy.deepcopy(leader_messages)

        msg = leader_messages.next_mle_message(mle.CommandType.LINK_REQUEST)
        command.check_link_request(msg, tlv_request_address16 = command.CheckType.CONTAIN, \
            tlv_request_route64 = command.CheckType.CONTAIN)

        # 5 DUT_ROUTER1: Verify DUT_ROUTER1 replied with Link Accept message
        router1_messages = self.simulator.get_messages_sent_by(DUT_ROUTER1)
        router1_messages_temp = copy.deepcopy(router1_messages)
        msg = router1_messages.next_mle_message(mle.CommandType.LINK_ACCEPT)
        if msg is not None:
          command.check_link_accept(msg, self.nodes[DUT_LEADER], address16 = command.CheckType.CONTAIN, \
              leader_data = command.CheckType.CONTAIN, route64 = command.CheckType.CONTAIN)
        else:
          msg = router1_messages_temp.next_mle_message(mle.CommandType.LINK_ACCEPT_AND_REQUEST)
          self.assertTrue(msg is not None)
          command.check_link_accept(msg, self.nodes[DUT_LEADER], address16 = command.CheckType.CONTAIN, \
              leader_data = command.CheckType.CONTAIN, route64 = command.CheckType.CONTAIN, \
              challenge = command.CheckType.CONTAIN)

        # 6 DUT_LEADER: Verify DUT_LEADER didn't send a Parent Request message
        msg = leader_messages_temp.next_mle_message(mle.CommandType.PARENT_REQUEST, False)
        self.assertTrue(msg is None)

        # 7 ALL: Verify connectivity by sending an ICMPv6 Echo Request from DUT_LEADER to DUT_ROUTER1 link local address
        router1_link_local_address = self.nodes[DUT_ROUTER1].get_ip6_address(config.ADDRESS_TYPE.LINK_LOCAL)
        self.assertTrue(self.nodes[DUT_LEADER].ping(router1_link_local_address))

if __name__ == '__main__':
    unittest.main()
