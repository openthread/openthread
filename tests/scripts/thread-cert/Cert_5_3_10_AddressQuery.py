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

import copy
import ipaddress
import time
import unittest

import command
import config
import ipv6
import network_layer
import node

LEADER = 1
BR = 2
ROUTER1 = 3
DUT_ROUTER2 = 4
MED1 = 5

class Cert_5_3_10_AddressQuery(unittest.TestCase):
    def setUp(self):
        self.simulator = config.create_default_simulator()

        self.nodes = {}
        for i in range(1, 6):
            self.nodes[i] = node.Node(i, (i == MED1), simulator=self.simulator)

        self.nodes[LEADER].set_panid(0xface)
        self.nodes[LEADER].set_mode('rsdn')
        self.nodes[LEADER].add_whitelist(self.nodes[BR].get_addr64())
        self.nodes[LEADER].add_whitelist(self.nodes[ROUTER1].get_addr64())
        self.nodes[LEADER].add_whitelist(self.nodes[DUT_ROUTER2].get_addr64())
        self.nodes[LEADER].enable_whitelist()

        self.nodes[BR].set_panid(0xface)
        self.nodes[BR].set_mode('rsdn')
        self.nodes[BR].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[BR].enable_whitelist()
        self.nodes[BR].set_router_selection_jitter(1)

        self.nodes[ROUTER1].set_panid(0xface)
        self.nodes[ROUTER1].set_mode('rsdn')
        self.nodes[ROUTER1].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[ROUTER1].add_whitelist(self.nodes[DUT_ROUTER2].get_addr64())
        self.nodes[ROUTER1].enable_whitelist()
        self.nodes[ROUTER1].set_router_selection_jitter(1)

        self.nodes[DUT_ROUTER2].set_panid(0xface)
        self.nodes[DUT_ROUTER2].set_mode('rsdn')
        self.nodes[DUT_ROUTER2].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[DUT_ROUTER2].add_whitelist(self.nodes[ROUTER1].get_addr64())
        self.nodes[DUT_ROUTER2].add_whitelist(self.nodes[MED1].get_addr64())
        self.nodes[DUT_ROUTER2].enable_whitelist()
        self.nodes[DUT_ROUTER2].set_router_selection_jitter(1)

        self.nodes[MED1].set_panid(0xface)
        self.nodes[MED1].set_mode('rsn')
        self.nodes[MED1].add_whitelist(self.nodes[DUT_ROUTER2].get_addr64())
        self.nodes[MED1].enable_whitelist()

    def tearDown(self):
        for node in list(self.nodes.values()):
            node.stop()
            node.destroy()
        self.simulator.stop()

    def test(self):
        # 1 & 2
        # Build and verify the topology
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[BR].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[BR].get_state(), 'router')

        # Configure two On-Mesh Prefixes on the BR
        self.nodes[BR].add_prefix('2003::/64', 'paros')
        self.nodes[BR].add_prefix('2004::/64', 'paros')
        self.nodes[BR].register_netdata()

        # Set lowpan context of sniffer
        self.simulator.set_lowpan_context(1, '2003::/64')
        self.simulator.set_lowpan_context(2, '2004::/64')

        self.nodes[DUT_ROUTER2].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[DUT_ROUTER2].get_state(), 'router')

        self.nodes[ROUTER1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER1].get_state(), 'router')

        self.nodes[MED1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[MED1].get_state(), 'child')

        # 3 MED1: MED1 sends an ICMPv6 Echo Request to Router1 using GUA 2003:: address
        router1_addr = self.nodes[ROUTER1].get_addr("2003::/64")
        self.assertTrue(router1_addr is not None)
        self.assertTrue(self.nodes[MED1].ping(router1_addr))

        # Wait for sniffer got Address Notification messages
        self.simulator.go(1)

        # Verify DUT_ROUTER2 sent an Address Query Request
        dut_router2_messages = self.simulator.get_messages_sent_by(DUT_ROUTER2)
        msg = dut_router2_messages.next_coap_message('0.02', '/a/aq')
        command.check_address_query(msg, self.nodes[DUT_ROUTER2], config.REALM_LOCAL_ALL_ROUTERS_ADDRESS)

        # Verify the DUT_ROUTER2 forwarded ICMPv6 Echo Request to ROUTER1
        msg = dut_router2_messages.get_icmp_message(ipv6.ICMP_ECHO_REQUEST)
        assert msg is not None, "Error: The DUT_ROUTER2 didn't forward ICMPv6 Echo Request to ROUTER1"
        msg.assertSentToNode(self.nodes[ROUTER1])

        # 4 BR: BR sends an ICMPv6 Echo Request to MED1 using GUA 2003:: address
        med1_addr = self.nodes[MED1].get_addr("2003::/64")
        self.assertTrue(med1_addr is not None)
        self.assertTrue(self.nodes[BR].ping(med1_addr))

        # Wait for sniffer got Address Notification messages
        self.simulator.go(1)

        # Verify DUT_ROUTER2 sent an Address Notification message
        dut_router2_messages = self.simulator.get_messages_sent_by(DUT_ROUTER2)
        msg = dut_router2_messages.next_coap_message('0.02', '/a/an')
        command.check_address_notification(msg, self.nodes[DUT_ROUTER2], self.nodes[BR])

        # 5 MED1: MED1 sends an ICMPv6 Echo Request to ROUTER1 using GUA 2003:: address
        addr = self.nodes[ROUTER1].get_addr("2003::/64")
        self.assertTrue(addr is not None)
        self.assertTrue(self.nodes[MED1].ping(addr))

        # Wait for sniffer got ICMPv6 Echo Reply
        self.simulator.go(1)

        # Verify DUT_ROUTER2 didn't generate an Address Query Request
        dut_router2_messages = self.simulator.get_messages_sent_by(DUT_ROUTER2)
        dut_router2_messages_temp = copy.deepcopy(dut_router2_messages)
        msg = dut_router2_messages.next_coap_message('0.02', '/a/aq', False)
        assert msg is None, "Error: The DUT_ROUTER2 sent an unexpected Address Query Request"

        # Verify DUT_ROUTER2 forwarded ICMPv6 Echo Reply to MED1
        msg = dut_router2_messages_temp.get_icmp_message(ipv6.ICMP_ECHO_RESPONSE)
        assert msg is not None, "Error: The DUT_ROUTER2 didn't forward ICMPv6 Echo Reply to MED1"
        msg.assertSentToNode(self.nodes[MED1])

        # 6 DUT_ROUTER2: Power off ROUTER1 and wait 580 seconds to allow the LEADER to expire its Router ID
        router1_id = self.nodes[ROUTER1].get_router_id()
        self.nodes[ROUTER1].stop()
        self.simulator.go(580)

        # Send an ICMPv6 Echo Request from MED1 to ROUTER1 GUA 2003:: address
        self.assertFalse(self.nodes[MED1].ping(router1_addr))

        # Verify the DUT_ROUTER2 has removed all entries based on ROUTER1's Router ID
        command.check_router_id_cached(self.nodes[DUT_ROUTER2], router1_id, False)

        # Verify DUT_ROUTER2 sent an Address Query Request
        dut_router2_messages = self.simulator.get_messages_sent_by(DUT_ROUTER2)
        msg = dut_router2_messages.next_coap_message('0.02', '/a/aq')
        msg.assertSentToDestinationAddress(config.REALM_LOCAL_ALL_ROUTERS_ADDRESS)

        # 7 MED1: Power off MED1 and wait to allow DUT_ROUTER2 to timeout the child
        self.nodes[MED1].stop()
        self.simulator.go(config.MLE_END_DEVICE_TIMEOUT)

        # BR sends two ICMPv6 Echo Requests to MED1 GUA 2003:: address
        self.assertFalse(self.nodes[BR].ping(med1_addr))
        self.assertFalse(self.nodes[BR].ping(med1_addr))

        # Verify DUT_ROUTER2 didn't generate an Address Notification message
        dut_router2_messages = self.simulator.get_messages_sent_by(DUT_ROUTER2)
        msg = dut_router2_messages.next_coap_message('0.02', '/a/an/', False)
        assert msg is None, "Error: The DUT_ROUTER2 sent an unexpected Address Notification message"

if __name__ == '__main__':
    unittest.main()
