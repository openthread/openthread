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

from binascii import hexlify
import time
import unittest
import mle
import config
from command import contains_tlv
from network_data import Prefix, BorderRouter, LowpanId
import node

LEADER = 1
ROUTER = 2
SED1 = 3
MED1 = 4

MTDS = [SED1, MED1]

LEADER_NOTIFY_SED_BY_CHILD_UPDATE_REQUEST = True

class Cert_7_1_3_BorderRouterAsLeader(unittest.TestCase):
    def setUp(self):
        self.simulator = config.create_default_simulator()

        self.nodes = {}
        for i in range(1,5):
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
        for node in list(self.nodes.values()):
            node.stop()
            node.destroy()
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
        self.nodes[LEADER].add_prefix('2001:2:0:1::/64', 'paros')
        self.nodes[LEADER].add_prefix('2001:2:0:2::/64', 'paro')
        self.nodes[LEADER].register_netdata()
        self.simulator.go(5)

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

        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        med1_messages = self.simulator.get_messages_sent_by(MED1)
        sed1_messages = self.simulator.get_messages_sent_by(SED1)

        # 3 - Leader
        # Ignore the first DATA_RESPONSE message sent when it became leader
        leader_messages.next_mle_message(mle.CommandType.DATA_RESPONSE)

        msg = leader_messages.next_mle_message(mle.CommandType.DATA_RESPONSE)
        network_data_tlv = msg.assertMleMessageContainsTlv(mle.NetworkData)
        prefixes = filter(lambda tlv : isinstance(tlv, Prefix), network_data_tlv.tlvs)
        self.assertTrue(len(prefixes) >= 2)
        for prefix in prefixes:
            self.assertTrue(contains_tlv(prefix.sub_tlvs, BorderRouter))
            self.assertTrue(contains_tlv(prefix.sub_tlvs, LowpanId))

        # 4 - N/A
        msg = med1_messages.next_mle_message(mle.CommandType.CHILD_UPDATE_REQUEST)
        addr_reg_tlv = msg.assertMleMessageContainsTlv(mle.AddressRegistration)
        med1_addresses = addr_reg_tlv.addresses

        # 5 - Leader
        msg = leader_messages.next_mle_message(mle.CommandType.CHILD_UPDATE_RESPONSE)
        msg.assertSentToNode(self.nodes[MED1])
        msg.assertMleMessageContainsTlv(mle.SourceAddress)
        addr_reg_tlv = msg.assertMleMessageContainsTlv(mle.AddressRegistration)
        self.assertTrue(all(addr in addr_reg_tlv.addresses for addr in med1_addresses))
        msg.assertMleMessageContainsTlv(mle.Mode)

        # 6A & 6B - Leader
        """
        if LEADER_NOTIFY_SED_BY_CHILD_UPDATE_REQUEST:
            msg = leader_messages.next_mle_message(mle.CommandType.CHILD_UPDATE_REQUEST)
        else:
            msg = leader_messages.next_mle_message(mle.CommandType.DATA_RESPONSE)
        msg.assertSentToNode(self.nodes[SED1])
        msg.assertMleMessageContainsTlv(mle.SourceAddress)
        msg.assertMleMessageContainsTlv(mle.LeaderData)
        msg.assertMleMessageContainsTlv(mle.NetworkData)
        msg.assertMleMessageContainsTlv(mle.ActiveTimestamp)
        """

        # 7 - N/A
        msg = sed1_messages.next_mle_message(mle.CommandType.CHILD_UPDATE_REQUEST)
        addr_reg_tlv = msg.assertMleMessageContainsTlv(mle.AddressRegistration)
        sed1_addresses = addr_reg_tlv.addresses

        # 8 - Leader
        msg = leader_messages.next_mle_message(mle.CommandType.CHILD_UPDATE_RESPONSE)
        msg.assertSentToNode(self.nodes[SED1])
        msg.assertMleMessageContainsTlv(mle.SourceAddress)
        addr_reg_tlv = msg.assertMleMessageContainsTlv(mle.AddressRegistration)
        self.assertTrue(all(addr in addr_reg_tlv.addresses for addr in sed1_addresses))
        msg.assertMleMessageContainsTlv(mle.Mode)


if __name__ == '__main__':
    unittest.main()
