#!/usr/bin/env python3
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

import unittest

from command import (
    check_child_update_request_from_child,
    check_child_update_request_from_parent,
    check_child_update_response,
    check_data_response,
)
from command import CheckType
from command import NetworkDataCheck, PrefixesCheck, SinglePrefixCheck

import config
import mle
import node

LEADER = 1
ROUTER = 2
SED1 = 3
MED1 = 4

MTDS = [SED1, MED1]


class Cert_7_1_3_BorderRouterAsLeader(unittest.TestCase):

    def setUp(self):
        self.simulator = config.create_default_simulator()

        self.nodes = {}
        for i in range(1, 5):
            self.nodes[i] = node.Node(i, (i in MTDS), simulator=self.simulator)

        self.nodes[LEADER].set_panid(0xface)
        self.nodes[LEADER].set_mode('rsdn')
        self.nodes[LEADER].add_whitelist(self.nodes[ROUTER].get_addr64())
        self.nodes[LEADER].add_whitelist(self.nodes[SED1].get_addr64())
        self.nodes[LEADER].add_whitelist(self.nodes[MED1].get_addr64())
        self.nodes[LEADER].enable_whitelist()

        self.nodes[ROUTER].set_panid(0xface)
        self.nodes[ROUTER].set_mode('rsdn')
        self.nodes[ROUTER].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[ROUTER].enable_whitelist()
        self.nodes[ROUTER].set_router_selection_jitter(1)

        self.nodes[SED1].set_panid(0xface)
        self.nodes[SED1].set_mode('s')
        self.nodes[SED1].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[SED1].enable_whitelist()
        self.nodes[SED1].set_timeout(config.DEFAULT_CHILD_TIMEOUT)

        self.nodes[MED1].set_panid(0xface)
        self.nodes[MED1].set_mode('rsn')
        self.nodes[MED1].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[MED1].enable_whitelist()

    def tearDown(self):
        for n in list(self.nodes.values()):
            n.stop()
            n.destroy()
        self.simulator.stop()

    def test(self):
        # 1 - All
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[ROUTER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER].get_state(), 'router')

        self.nodes[SED1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[SED1].get_state(), 'child')

        self.nodes[MED1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[MED1].get_state(), 'child')

        # 2 - N/A
        # Clear collected messages
        self.simulator.get_messages_sent_by(LEADER)
        self.simulator.get_messages_sent_by(MED1)
        self.simulator.get_messages_sent_by(SED1)

        self.nodes[LEADER].add_prefix('2001:2:0:1::/64', 'paros')
        self.nodes[LEADER].add_prefix('2001:2:0:2::/64', 'paro')
        self.nodes[LEADER].register_netdata()
        self.simulator.go(5)

        # Set lowpan context of sniffer
        self.simulator.set_lowpan_context(1, '2001:2:0:1::/64')
        self.simulator.set_lowpan_context(2, '2001:2:0:2::/64')

        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        med1_messages = self.simulator.get_messages_sent_by(MED1)
        sed1_messages = self.simulator.get_messages_sent_by(SED1)

        addrs = self.nodes[SED1].get_addrs()
        self.assertTrue(any('2001:2:0:1' in addr[0:10] for addr in addrs))
        self.assertFalse(any('2001:2:0:2' in addr[0:10] for addr in addrs))
        for addr in addrs:
            if addr[0:10] == '2001:2:0:1' or addr[0:10] == '2001:2:0:2':
                self.assertTrue(self.nodes[LEADER].ping(addr))

        addrs = self.nodes[MED1].get_addrs()
        self.assertTrue(any('2001:2:0:1' in addr[0:10] for addr in addrs))
        self.assertTrue(any('2001:2:0:2' in addr[0:10] for addr in addrs))
        for addr in addrs:
            if addr[0:10] == '2001:2:0:1' or addr[0:10] == '2001:2:0:2':
                self.assertTrue(self.nodes[LEADER].ping(addr))

        # 3 - Leader
        msg = leader_messages.next_mle_message(mle.CommandType.DATA_RESPONSE)
        check_data_response(
            msg,
            network_data_check=NetworkDataCheck(prefixes_check=PrefixesCheck(
                prefix_check_list=[
                    SinglePrefixCheck(b'2001000200000001'),
                    SinglePrefixCheck(b'2001000200000002'),
                ])),
        )

        # 4 - N/A
        # Get addresses registered by MED1
        msg = med1_messages.next_mle_message(
            mle.CommandType.CHILD_UPDATE_REQUEST)
        check_child_update_request_from_child(
            msg, address_registration=CheckType.CONTAIN, CIDs=[0, 1, 2])

        # 5 - Leader
        # Make a copy of leader's messages to ensure that we don't miss
        # messages to SED1
        leader_messages_copy = leader_messages.clone()
        msg = leader_messages_copy.next_mle_message(
            mle.CommandType.CHILD_UPDATE_RESPONSE,
            sent_to_node=self.nodes[MED1],
        )
        check_child_update_response(msg,
                                    address_registration=CheckType.CONTAIN,
                                    CIDs=[1, 2])

        # 6A & 6B - Leader
        if config.LEADER_NOTIFY_SED_BY_CHILD_UPDATE_REQUEST:
            msg = leader_messages.next_mle_message(
                mle.CommandType.CHILD_UPDATE_REQUEST,
                sent_to_node=self.nodes[SED1],
            )
            check_child_update_request_from_parent(
                msg,
                leader_data=CheckType.CONTAIN,
                network_data=CheckType.CONTAIN,
                active_timestamp=CheckType.CONTAIN,
            )
        else:
            msg = leader_messages.next_mle_message(
                mle.CommandType.DATA_RESPONSE, sent_to_node=self.nodes[SED1])
            check_data_response(msg, network_data_check=NetworkDataCheck())

        # 7 - N/A
        # Get addresses registered by SED1
        msg = sed1_messages.next_mle_message(
            mle.CommandType.CHILD_UPDATE_REQUEST)
        check_child_update_request_from_child(
            msg, address_registration=CheckType.CONTAIN, CIDs=[0, 1])

        # 8 - Leader
        msg = leader_messages.next_mle_message(
            mle.CommandType.CHILD_UPDATE_RESPONSE,
            sent_to_node=self.nodes[SED1],
        )
        check_child_update_response(msg,
                                    address_registration=CheckType.CONTAIN,
                                    CIDs=[1])


if __name__ == '__main__':
    unittest.main()
