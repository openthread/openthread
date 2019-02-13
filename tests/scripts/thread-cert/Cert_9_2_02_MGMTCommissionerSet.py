#!/usr/bin/env python
#
#  Copyright (c) 2019, The OpenThread Authors.
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

import binascii
import time
import unittest

from command import check_payload_same
from command import get_sub_tlv
import config
import mesh_cop
import mle
import node

COMMISSIONER = 1
LEADER = 2


class Cert_9_2_02_MGMTCommissionerSet(unittest.TestCase):
    def setUp(self):
        self.simulator = config.create_default_simulator()

        self.nodes = {}
        for i in range(1,3):
            self.nodes[i] = node.Node(i, simulator=self.simulator)

        self.nodes[COMMISSIONER].set_panid(0xface)
        self.nodes[COMMISSIONER].set_mode('rsdn')
        self.nodes[COMMISSIONER].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[COMMISSIONER].enable_whitelist()
        self.nodes[COMMISSIONER].set_router_selection_jitter(1)

        self.nodes[LEADER].set_panid(0xface)
        self.nodes[LEADER].set_mode('rsdn')
        self.nodes[LEADER].add_whitelist(self.nodes[COMMISSIONER].get_addr64())
        self.nodes[LEADER].enable_whitelist()
        self.nodes[LEADER].set_router_selection_jitter(1)

    def tearDown(self):
        for node in list(self.nodes.values()):
            node.stop()
            node.destroy()
        self.simulator.stop()

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[COMMISSIONER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[COMMISSIONER].get_state(), 'router')
        # skip all other Coaps sent by DUT
        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        while True:
            msg = leader_messages.next_coap_message('2.04', assert_enabled=False)
            if msg is None:
                break

        self.nodes[COMMISSIONER].commissioner_start()
        self.simulator.go(3)

        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        msg = leader_messages.next_coap_message('2.04', assert_enabled=False)
        commissioner_session_id_tlv = get_sub_tlv(msg.coap.payload, mesh_cop.CommissionerSessionId)

        # Step 2 - Harness instructs commissioner to send MGMT_COMMISSIONER_SET.req to DUT
        steering_data_tlv = mesh_cop.SteeringData(bytes([0xFF]))
        self.nodes[COMMISSIONER].commissioner_mgmtset_with_tlvs([steering_data_tlv])
        self.simulator.go(5)

        # Step 3 - DUT responds to MGMT_COMMISSIONER_SET.req with MGMT_COMMISSIONER_SET.rsp
        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        msg = leader_messages.next_coap_message('2.04')
        check_payload_same(msg.coap.payload, (mesh_cop.State(0xFF),)) # (mesh_cop.State(0xFF),) <- this a tuple, don't delete the comma

        # Step 4 - Harness instructs commissioner to send MGMT_COMMISSIONER_SET.req to DUT
        self.nodes[COMMISSIONER].commissioner_mgmtset_with_tlvs([steering_data_tlv, commissioner_session_id_tlv])
        self.simulator.go(5)

        # Step 5 - DUT sends MGMT_COMMISSIONER_SET.rsp to commissioner
        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        msg = leader_messages.next_coap_message('2.04')
        check_payload_same(msg.coap.payload, (mesh_cop.State(0x1),))

        # Step 6 - DUT sends a multicast MLE Data Response
        msg = leader_messages.next_mle_message(mle.CommandType.DATA_RESPONSE)
        print('data response:{}'.format(msg.mle))

        # Step 7 - Harness instructs commissioner to send MGMT_COMMISSIONER_SET.req to DUT
        border_agent_locator_tlv = mesh_cop.BorderAgentLocator(0x0400)
        self.nodes[COMMISSIONER].commissioner_mgmtset_with_tlvs(
            [commissioner_session_id_tlv, border_agent_locator_tlv])
        self.simulator.go(5)

        # Step 8 - DUT responds to MGMT_COMMISSIONER_SET.req with MGMT_COMMISSIONER_SET.rsp
        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        msg = leader_messages.next_coap_message('2.04')
        check_payload_same(msg.coap.payload, (mesh_cop.State(0xFF),))

        # Step 9 - Harness instructs commissioner to send MGMT_COMMISSIONER_SET.req to DUT
        self.nodes[COMMISSIONER].commissioner_mgmtset_with_tlvs(
            [steering_data_tlv, commissioner_session_id_tlv, border_agent_locator_tlv])
        self.simulator.go(5)

        # Step 10 - DUT responds to MGMT_COMMISSIONER_SET.req with MGMT_COMMISSIONER_SET.rsp
        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        msg = leader_messages.next_coap_message('2.04')
        check_payload_same(msg.coap.payload, (mesh_cop.State(0xFF),))

        # Step 11 - Harness instructs commissioner to send MGMT_COMMISSIONER_SET.req to DUT
        self.nodes[COMMISSIONER].commissioner_mgmtset_with_tlvs(
            [mesh_cop.CommissionerSessionId(0xFFFF), border_agent_locator_tlv])
        self.simulator.go(5)

        # Step 12 - DUT responds to MGMT_COMMISSIONER_SET.req with MGMT_COMMISSIONER_SET.rsp
        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        msg = leader_messages.next_coap_message('2.04')
        check_payload_same(msg.coap.payload, (mesh_cop.State(0xFF),))

        # Step 13 - Harness instructs commissioner to send MGMT_COMMISSIONER_SET.req to DUT
        self.nodes[COMMISSIONER].commissioner_mgmtset_with_tlvs(
            [commissioner_session_id_tlv, steering_data_tlv, mesh_cop.Channel(0x0, 0x0)])
        self.simulator.go(5)

        # Step 14 - DUT responds to MGMT_COMMISSIONER_SET.req with MGMT_COMMISSIONER_SET.rsp
        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        msg = leader_messages.next_coap_message('2.04')
        check_payload_same(msg.coap.payload, (mesh_cop.State(0x1),))

        # Step 15 - Send ICMPv6 Echo Request to DUT
        ipaddrs = self.nodes[LEADER].get_addrs()
        for ipaddr in ipaddrs:
            self.assertTrue(self.nodes[COMMISSIONER].ping(ipaddr))

if __name__ == '__main__':
    unittest.main()
