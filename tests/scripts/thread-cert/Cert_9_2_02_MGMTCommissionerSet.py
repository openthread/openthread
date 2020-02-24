#!/usr/bin/env python3
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

from ipaddress import ip_address
import unittest

import command
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
        for i in range(1, 3):
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
        for n in list(self.nodes.values()):
            n.stop()
            n.destroy()
        self.simulator.stop()

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[COMMISSIONER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[COMMISSIONER].get_state(), 'router')

        # Skip all other Coaps sent by Leader
        self.simulator.get_messages_sent_by(COMMISSIONER)
        self.simulator.get_messages_sent_by(LEADER)

        # Commissioner start
        self.nodes[COMMISSIONER].commissioner_start()
        self.simulator.go(3)
        self.simulator.get_messages_sent_by(COMMISSIONER)  # Skip LEAD_PET.req

        # Get CommissionerSesssionId from LEAD_PET.rsp
        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        msg = leader_messages.next_coap_message('2.04', assert_enabled=True)
        commissioner_session_id_tlv = command.get_sub_tlv(
            msg.coap.payload, mesh_cop.CommissionerSessionId)

        # Step 2 - Harness instructs commissioner to send
        # MGMT_COMMISSIONER_SET.req to Leader
        steering_data_tlv = mesh_cop.SteeringData(bytes([0xff]))
        self.nodes[COMMISSIONER].commissioner_mgmtset_with_tlvs(
            [steering_data_tlv])
        self.simulator.go(5)

        # Step 3 - Leader responds to MGMT_COMMISSIONER_SET.req with
        # MGMT_COMMISSIONER_SET.rsp
        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        msg = leader_messages.next_coap_message('2.04')
        # (mesh_cop.State(mesh_cop.MeshCopState.REJECT),) <- this a tuple, don't delete the comma
        command.check_coap_message(
            msg, [mesh_cop.State(mesh_cop.MeshCopState.REJECT)])
        self.simulator.get_messages_sent_by(COMMISSIONER)  # Skip LEAD_PET.req

        # Step 4 - Harness instructs commissioner to send
        # MGMT_COMMISSIONER_SET.req to Leader
        self.nodes[COMMISSIONER].commissioner_mgmtset_with_tlvs(
            [steering_data_tlv, commissioner_session_id_tlv])
        self.simulator.go(5)
        commissioner_messages = self.simulator.get_messages_sent_by(
            COMMISSIONER)
        msg = commissioner_messages.next_coap_message('0.02', uri_path='/c/cs')
        rloc = ip_address(self.nodes[LEADER].get_rloc())
        leader_aloc = ip_address(self.nodes[LEADER].get_addr_leader_aloc())
        command.check_coap_message(
            msg,
            [steering_data_tlv, commissioner_session_id_tlv],
            dest_addrs=[rloc, leader_aloc],
        )

        # Step 5 - Leader sends MGMT_COMMISSIONER_SET.rsp to commissioner
        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        msg = leader_messages.next_coap_message('2.04')
        command.check_coap_message(
            msg, [mesh_cop.State(mesh_cop.MeshCopState.ACCEPT)])

        # Step 6 - Leader sends a multicast MLE Data Response
        msg = leader_messages.next_mle_message(mle.CommandType.DATA_RESPONSE)
        command.check_data_response(
            msg,
            command.NetworkDataCheck(
                commissioning_data_check=command.CommissioningDataCheck(
                    stable=0,
                    sub_tlv_type_list=[
                        mesh_cop.CommissionerSessionId,
                        mesh_cop.SteeringData,
                        mesh_cop.BorderAgentLocator,
                    ],
                )),
        )

        # Step 7 - Harness instructs commissioner to send
        # MGMT_COMMISSIONER_SET.req to Leader
        border_agent_locator_tlv = mesh_cop.BorderAgentLocator(0x0400)
        self.nodes[COMMISSIONER].commissioner_mgmtset_with_tlvs(
            [commissioner_session_id_tlv, border_agent_locator_tlv])
        self.simulator.go(5)

        # Step 8 - Leader responds to MGMT_COMMISSIONER_SET.req with
        # MGMT_COMMISSIONER_SET.rsp
        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        msg = leader_messages.next_coap_message('2.04')
        command.check_coap_message(
            msg, [mesh_cop.State(mesh_cop.MeshCopState.REJECT)])

        # Step 9 - Harness instructs commissioner to send
        # MGMT_COMMISSIONER_SET.req to Leader
        self.nodes[COMMISSIONER].commissioner_mgmtset_with_tlvs([
            steering_data_tlv,
            commissioner_session_id_tlv,
            border_agent_locator_tlv,
        ])
        self.simulator.go(5)

        # Step 10 - Leader responds to MGMT_COMMISSIONER_SET.req with
        # MGMT_COMMISSIONER_SET.rsp
        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        msg = leader_messages.next_coap_message('2.04')
        command.check_coap_message(
            msg, [mesh_cop.State(mesh_cop.MeshCopState.REJECT)])

        # Step 11 - Harness instructs commissioner to send
        # MGMT_COMMISSIONER_SET.req to Leader
        self.nodes[COMMISSIONER].commissioner_mgmtset_with_tlvs(
            [mesh_cop.CommissionerSessionId(0xffff), steering_data_tlv])
        self.simulator.go(5)

        # Step 12 - Leader responds to MGMT_COMMISSIONER_SET.req with
        # MGMT_COMMISSIONER_SET.rsp
        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        msg = leader_messages.next_coap_message('2.04')
        command.check_coap_message(
            msg, [mesh_cop.State(mesh_cop.MeshCopState.REJECT)])

        # Step 13 - Harness instructs commissioner to send
        # MGMT_COMMISSIONER_SET.req to Leader
        self.nodes[COMMISSIONER].commissioner_mgmtset_with_tlvs([
            commissioner_session_id_tlv,
            steering_data_tlv,
            mesh_cop.Channel(0x0, 0x0),
        ])
        self.simulator.go(5)

        # Step 14 - Leader responds to MGMT_COMMISSIONER_SET.req with
        # MGMT_COMMISSIONER_SET.rsp
        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        msg = leader_messages.next_coap_message('2.04')
        command.check_coap_message(
            msg, [mesh_cop.State(mesh_cop.MeshCopState.ACCEPT)])

        # Step 15 - Send ICMPv6 Echo Request to Leader
        leader_rloc = self.nodes[LEADER].get_rloc()
        self.assertTrue(self.nodes[COMMISSIONER].ping(leader_rloc))


if __name__ == '__main__':
    unittest.main()
