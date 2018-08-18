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

import unittest

import node
import config
import command

LEADER = 1
ROUTER1 = 2
BR = 3
ED1 = 17
DUT_REED = 18
ROUTER_SELECTION_JITTER = 1

class Cert_5_2_5_AddressQuery(unittest.TestCase):
    def setUp(self):
        self.simulator = config.create_default_simulator()

        self.nodes = {}
        for i in range(1, 19):
            self.nodes[i] = node.Node(i, (i == ED1), simulator=self.simulator)

        self.nodes[LEADER].set_panid()
        self.nodes[LEADER].set_mode('rsdn')
        self.nodes[LEADER].enable_whitelist()

        for i in range(2, 17):
            self.nodes[i].set_panid()
            self.nodes[i].set_mode('rsdn')
            self.nodes[i].add_whitelist(self.nodes[LEADER].get_addr64())
            self.nodes[LEADER].add_whitelist(self.nodes[i].get_addr64())
            self.nodes[i].enable_whitelist()
            self.nodes[i].set_router_selection_jitter(1)

        self.nodes[ED1].set_panid()
        self.nodes[ED1].set_mode('rsn')
        self.nodes[ED1].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[LEADER].add_whitelist(self.nodes[ED1].get_addr64())
        self.nodes[ED1].enable_whitelist()

        self.nodes[DUT_REED].set_panid()
        self.nodes[DUT_REED].set_mode('rsdn')
        self.nodes[DUT_REED].add_whitelist(self.nodes[ROUTER1].get_addr64())
        self.nodes[ROUTER1].add_whitelist(self.nodes[DUT_REED].get_addr64())
        self.nodes[DUT_REED].enable_whitelist()
        self.nodes[DUT_REED].set_router_selection_jitter(1)

    def tearDown(self):
        for node in list(self.nodes.values()):
            node.stop()
            node.destroy()
        self.simulator.stop()

    def test(self):
        # 1. LEADER: DHCPv6 Server for prefix 2001::/64.
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')
        self.nodes[LEADER].add_prefix('2001::/64', 'pdros')
        self.nodes[LEADER].register_netdata()

        # 2. BR: SLAAC Server for prefix 2002::/64.
        self.nodes[BR].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[BR].get_state(), 'router')
        self.nodes[BR].add_prefix('2002::/64', 'paros')
        self.nodes[BR].register_netdata()

        # 3. Bring up remaining devices except DUT_REED.
        for i in range(2, 17):
            if i == BR:
                continue
            self.nodes[i].start()
            self.simulator.go(5)
            self.assertEqual(self.nodes[i].get_state(), 'router')

        self.nodes[ED1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ED1].get_state(), 'child')

        # 4. Bring up DUT_REED.
        self.nodes[DUT_REED].start()
        self.simulator.go(5)
        self.simulator.go(ROUTER_SELECTION_JITTER)

        reed_messages = self.simulator.get_messages_sent_by(DUT_REED)

        # Verify DUT_REED doesn't try to become router.
        msg = reed_messages.does_not_contain_coap_message()
        assert msg is True, "Error: The REED sent an Address Solicit Request"

        # 5. Enable a link between the DUT and BR to create a one-way link.
        self.nodes[DUT_REED].add_whitelist(self.nodes[BR].get_addr64())
        self.nodes[BR].add_whitelist(self.nodes[DUT_REED].get_addr64())

        # 6. Verify DUT_REED would send Address Notification when ping to its ML-EID.
        mleid = self.nodes[DUT_REED].get_ip6_address(config.ADDRESS_TYPE.ML_EID)
        self.assertTrue(self.nodes[ED1].ping(mleid))

        # Wait for sniffer collecting packets
        self.simulator.go(1)

        reed_messages = self.simulator.get_messages_sent_by(DUT_REED)
        msg = reed_messages.next_coap_message('0.02', '/a/an')
        command.check_address_notification(msg, self.nodes[DUT_REED], self.nodes[LEADER])

        # 7 & 8. Verify DUT_REED would send Address Notification when ping to its 2001::EID and 2002::EID.
        flag2001 = 0
        flag2002 = 0
        for global_address in self.nodes[DUT_REED].get_ip6_address(config.ADDRESS_TYPE.GLOBAL):
            if global_address[0:4] == '2001':
                flag2001 += 1
            elif global_address[0:4] == '2002':
                flag2002 += 1
            else:
                raise "Error: Address is unexpected."
            self.assertTrue(self.nodes[ED1].ping(global_address))

            # Wait for sniffer collecting packets
            self.simulator.go(1)

            reed_messages = self.simulator.get_messages_sent_by(DUT_REED)
            msg = reed_messages.next_coap_message('0.02', '/a/an')
            command.check_address_notification(msg, self.nodes[DUT_REED], self.nodes[LEADER])

        assert flag2001 == 1 , "Error: Expecting address 2001::EID not appear."
        assert flag2002 == 1 , "Error: Expecting address 2002::EID not appear."

if __name__ == '__main__':
    unittest.main()
