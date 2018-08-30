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
import time
import unittest
import ipaddress

import command
import config
import ipv6
import node

LEADER = 1
ROUTER1 = 2
DUT_ROUTER2 = 3
ROUTER3 = 4
SED1 = 5

class Cert_5_3_09_AddressQuery(unittest.TestCase):
    def setUp(self):
        self.simulator = config.create_default_simulator()

        self.nodes = {}
        for i in range(1,6):
            self.nodes[i] = node.Node(i, (i == SED1), simulator=self.simulator)

        self.nodes[LEADER].set_panid(0xface)
        self.nodes[LEADER].set_mode('rsdn')
        self.nodes[LEADER].add_whitelist(self.nodes[ROUTER1].get_addr64())
        self.nodes[LEADER].add_whitelist(self.nodes[DUT_ROUTER2].get_addr64())
        self.nodes[LEADER].add_whitelist(self.nodes[ROUTER3].get_addr64())
        self.nodes[LEADER].enable_whitelist()

        self.nodes[ROUTER1].set_panid(0xface)
        self.nodes[ROUTER1].set_mode('rsdn')
        self.nodes[ROUTER1].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[ROUTER1].enable_whitelist()
        self.nodes[ROUTER1].set_router_selection_jitter(1)

        self.nodes[DUT_ROUTER2].set_panid(0xface)
        self.nodes[DUT_ROUTER2].set_mode('rsdn')
        self.nodes[DUT_ROUTER2].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[DUT_ROUTER2].add_whitelist(self.nodes[SED1].get_addr64())
        self.nodes[DUT_ROUTER2].enable_whitelist()
        self.nodes[DUT_ROUTER2].set_router_selection_jitter(1)

        self.nodes[ROUTER3].set_panid(0xface)
        self.nodes[ROUTER3].set_mode('rsdn')
        self.nodes[ROUTER3].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[ROUTER3].enable_whitelist()
        self.nodes[ROUTER3].set_router_selection_jitter(1)

        self.nodes[SED1].set_panid(0xface)
        self.nodes[SED1].set_mode('s')
        self.nodes[SED1].add_whitelist(self.nodes[DUT_ROUTER2].get_addr64())
        self.nodes[SED1].set_timeout(config.DEFAULT_CHILD_TIMEOUT)
        self.nodes[SED1].enable_whitelist()

    def tearDown(self):
        for node in list(self.nodes.values()):
            node.stop()
            node.destroy()
        self.simulator.stop()

    def test(self):
        # 1 & 2 ALL: Build and verify the topology
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        # Configure the LEADER to be a DHCPv6 Border Router for prefixes 2001:: & 2002::
        self.nodes[LEADER].add_prefix('2001::/64', 'pdros')
        self.nodes[LEADER].add_prefix('2002::/64', 'pdro')
        self.nodes[LEADER].register_netdata()

        # Set lowpan context of sniffer
        self.simulator.set_lowpan_context(1, '2001::/64')
        self.simulator.set_lowpan_context(2, '2002::/64')

        self.nodes[ROUTER1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER1].get_state(), 'router')

        self.nodes[DUT_ROUTER2].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[DUT_ROUTER2].get_state(), 'router')

        self.nodes[ROUTER3].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER3].get_state(), 'router')

        self.nodes[SED1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[SED1].get_state(), 'child')

        # 3 SED1: The SED1 sends an ICMPv6 Echo Request to ROUTER3 using GUA 2001:: address
        router3_addr = self.nodes[ROUTER3].get_addr("2001::/64")
        self.assertTrue(router3_addr is not None)
        self.assertTrue(self.nodes[SED1].ping(router3_addr))

        # Verify DUT_ROUTER2 sent an Address Query Request
        dut_router2_messages = self.simulator.get_messages_sent_by(DUT_ROUTER2)
        msg = dut_router2_messages.next_coap_message('0.02', '/a/aq')
        msg.assertSentToDestinationAddress(config.REALM_LOCAL_ALL_ROUTERS_ADDRESS)
        command.check_address_query(msg, self.nodes[DUT_ROUTER2], config.REALM_LOCAL_ALL_ROUTERS_ADDRESS)

        # Verify the DUT_ROUTER2 forwarded the ICMPv6 Echo Request to ROUTER3
        msg = dut_router2_messages.get_icmp_message(ipv6.ICMP_ECHO_REQUEST)
        assert msg is not None, "Error: The DUT_ROUTER2 didn't forward ICMPv6 Echo Request to ROUTER3"
        msg.assertSentToNode(self.nodes[ROUTER3])

        # 4 ROUTER1: ROUTER1 sends an ICMPv6 Echo Request to the SED1 using GUA 2001:: address
        sed1_addr = self.nodes[SED1].get_addr("2001::/64")
        self.assertTrue(sed1_addr is not None)
        self.assertTrue(self.nodes[ROUTER1].ping(sed1_addr))

        # Wait for sniffer got all Address Notification messages
        self.simulator.go(1)

        # Verify DUT_ROUTER2 sent an Address Notification message
        dut_router2_messages = self.simulator.get_messages_sent_by(DUT_ROUTER2)
        msg = dut_router2_messages.next_coap_message('0.02', '/a/an')
        command.check_address_notification(msg, self.nodes[DUT_ROUTER2], self.nodes[ROUTER1])

        # 5 SED1: SED1 sends an ICMPv6 Echo Request to the ROUTER3 using GUA 2001:: address
        self.assertTrue(self.nodes[SED1].ping(router3_addr))

        # Wait for sniffer got the ICMPv6 Echo Reply
        self.simulator.go(1)

        # Verify DUT_ROUTER2 didn't generate Address Query Request
        dut_router2_messages = self.simulator.get_messages_sent_by(DUT_ROUTER2)
        dut_router2_messages_temp = copy.deepcopy(dut_router2_messages)
        msg = dut_router2_messages.next_coap_message('0.02', '/a/aq', False)
        assert msg is None, "Error: The DUT_ROUTER2 sent an unexpected Address Query Request"

        # Verify DUT_ROUTER2 forwarded the ICMPv6 Echo Reply to SED1
        msg = dut_router2_messages_temp.get_icmp_message(ipv6.ICMP_ECHO_RESPONSE)
        assert msg is not None, "Error: The DUT_ROUTER2 didn't forward ICMPv6 Echo Reply to SED1"
        msg.assertSentToNode(self.nodes[SED1])

        # 6 DUT_ROUTER2: Power off ROUTER3 and wait 580s to alow LEADER to expire its Router ID
        self.nodes[ROUTER3].stop()
        self.simulator.go(580)

        # The SED1 sends an ICMPv6 Echo Request to ROUTER3 GUA 2001:: address
        self.assertFalse(self.nodes[SED1].ping(router3_addr))

        # Verify DUT_ROUTER2 sent an Address Query Request
        dut_router2_messages = self.simulator.get_messages_sent_by(DUT_ROUTER2)
        msg = dut_router2_messages.next_coap_message('0.02', '/a/aq')
        msg.assertSentToDestinationAddress(config.REALM_LOCAL_ALL_ROUTERS_ADDRESS)

        # 7 SED1: Power off SED1 and wait to allow DUT_ROUTER2 to timeout the child
        self.nodes[SED1].stop()
        self.simulator.go(5)

        # ROUTER1 sends two ICMPv6 Echo Requests to SED1 GUA 2001:: address
        self.assertFalse(self.nodes[ROUTER1].ping(sed1_addr))
        self.assertFalse(self.nodes[ROUTER1].ping(sed1_addr))

        # Verify DUT_ROUTER2 didn't generate an Address Notification message
        dut_router2_messages = self.simulator.get_messages_sent_by(DUT_ROUTER2)
        msg = dut_router2_messages.next_coap_message('0.02', '/a/an', False)
        assert msg is None, "Error: The DUT_ROUTER2 sent an unexpected Address Notification message"

if __name__ == '__main__':
    unittest.main()
