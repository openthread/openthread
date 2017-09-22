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
import ipv6

import node
import mle
import config
import command

LEADER = 1
DUT_ROUTER1 = 2
DUT_REED = 17

class Cert_5_2_7_REEDSynchronization(unittest.TestCase):
    def setUp(self):
        self.nodes = {}
        for i in range(1, 18):
            self.nodes[i] = node.Node(i)
            self.nodes[i].set_panid(0xface)
            self.nodes[i].set_mode('rsdn')
            self.nodes[i].set_router_selection_jitter(1)

        self.sniffer = config.create_default_thread_sniffer()
        self.sniffer.start()

    def tearDown(self):
        self.sniffer.stop()
        del self.sniffer

        for node in list(self.nodes.values()):
            node.stop()
        del self.nodes

    def test(self):
        # 1. Ensure topology is formed correctly without DUT_ROUTER1.
        self.nodes[LEADER].start()
        self.nodes[LEADER].set_state('leader')
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        for i in range(2, 17):
            self.nodes[i].start()
        time.sleep(5)

        for i in range(2, 17):
            self.assertEqual(self.nodes[i].get_state(), 'router')

        # 2. DUT_REED: Attach to network. Verify it didn't send an Address Solicit Request.
        # Avoid DUT_REED attach to DUT_ROUTER1.
        self.nodes[DUT_REED].add_whitelist(self.nodes[DUT_ROUTER1].get_addr64(), config.RSSI['LINK_QULITY_1'])

        self.nodes[DUT_REED].start()
        time.sleep(5)
        self.assertEqual(self.nodes[DUT_REED].get_state(), 'child')

        # The DUT_REED must not send a coap message here.
        reed_messages = self.sniffer.get_messages_sent_by(DUT_REED)
        msg = reed_messages.does_not_contain_coap_message()
        assert msg is True, "Error: The DUT_REED sent an Address Solicit Request"

        # 3. DUT_REED: Verify sent a Link Request to at least 3 neighboring Routers.
        for i in range(0, 3):
            msg = reed_messages.next_mle_message(mle.CommandType.LINK_REQUEST)
            command.check_link_request(msg)

        # 4. DUT_ROUTER1: Verify sent a Link Accept to DUT_REED.
        time.sleep(30)
        dut_messages = self.sniffer.get_messages_sent_by(DUT_ROUTER1)
        flag_link_accept = False
        while True:
            msg = dut_messages.next_mle_message(mle.CommandType.LINK_ACCEPT, False)
            if msg == None :
                break

            destination_link_local = self.nodes[DUT_REED].get_ip6_address(config.ADDRESS_TYPE.LINK_LOCAL)
            if ipv6.ip_address(destination_link_local) == msg.ipv6_packet.ipv6_header.destination_address:
                flag_link_accept = True
                break

        assert flag_link_accept is True, "Error: DUT_ROUTER1 didn't send a Link Accept to DUT_REED"

        command.check_link_accept(msg, self.nodes[DUT_REED])

if __name__ == '__main__':
    unittest.main()
