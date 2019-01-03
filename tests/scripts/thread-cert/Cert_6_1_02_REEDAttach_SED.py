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

import unittest

from command import check_parent_request
from command import check_child_id_request
from command import CheckType
import config
import mac802154
import message
import mle
import node

LEADER = 1
REED = 2
SED = 3

class Cert_6_1_2_REEDAttach_SED(unittest.TestCase):
    def setUp(self):
        self.simulator = config.create_default_simulator()

        self.nodes = {}
        for i in range(1,4):
            self.nodes[i] = node.Node(i, (i == SED), simulator=self.simulator)

        self.nodes[LEADER].set_panid(0xface)
        self.nodes[LEADER].set_mode('rsdn')
        self.nodes[LEADER].add_whitelist(self.nodes[REED].get_addr64())
        self.nodes[LEADER].enable_whitelist()

        self.nodes[REED].set_panid(0xface)
        self.nodes[REED].set_mode('rsdn')
        self.nodes[REED].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[REED].add_whitelist(self.nodes[SED].get_addr64())
        self.nodes[REED].enable_whitelist()
        self.nodes[REED].set_router_upgrade_threshold(0)

        self.nodes[SED].set_panid(0xface)
        self.nodes[SED].set_mode('s')
        self.nodes[SED].add_whitelist(self.nodes[REED].get_addr64())
        self.nodes[SED].enable_whitelist()
        self.nodes[SED].set_timeout(config.DEFAULT_CHILD_TIMEOUT)

    def tearDown(self):
        for node in list(self.nodes.values()):
            node.stop()
            node.destroy()
        self.simulator.stop()

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[REED].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[REED].get_state(), 'child')

        self.nodes[SED].start()
        self.simulator.go(5) 
        self.assertEqual(self.nodes[SED].get_state(), 'child')
        self.assertEqual(self.nodes[REED].get_state(), 'router')

        sed_messages = self.simulator.get_messages_sent_by(SED)

        # Step 2 - DUT sends MLE Parent Request
        msg = sed_messages.next_mle_message(mle.CommandType.PARENT_REQUEST)
        check_parent_request(msg, is_first_request=True)

        # Step 4 - DUT sends MLE Parent Request again
        msg = sed_messages.next_mle_message(mle.CommandType.PARENT_REQUEST)
        check_parent_request(msg, is_first_request=False)

        # Step 6 - DUT sends Child ID Request
        msg = sed_messages.next_mle_message(mle.CommandType.CHILD_ID_REQUEST, sent_to_node=self.nodes[REED])
        check_child_id_request(msg, address_registration=CheckType.CONTAIN,
            tlv_request=CheckType.CONTAIN, mle_frame_counter=CheckType.OPTIONAL,
            route64=CheckType.OPTIONAL)

        # Wait DEFAULT_CHILD_TIMEOUT seconds,
        # ensure SED has received the CHILD_ID_RESPONSE,
        # and the next data requests would be keep-alive messages
        self.simulator.go(config.DEFAULT_CHILD_TIMEOUT)
        sed_messages = self.simulator.get_messages_sent_by(SED)

        # Step 11 - SED sends periodic 802.15.4 Data Request messages
        msg = sed_messages.next_message()
        self.assertEqual(False, msg.isMacAddressTypeLong())    # Extra check, keep-alive messages are of short types of mac address
        self.assertEqual(msg.type, message.MessageType.COMMAND)
        self.assertEqual(msg.mac_header.command_type, mac802154.MacHeader.CommandIdentifier.DATA_REQUEST)

        # Step 12 - REED sends ICMPv6 echo request, to DUT link local address
        sed_addrs = self.nodes[SED].get_addrs()
        for addr in sed_addrs:
            if addr[0:4] == 'fe80':
                self.assertTrue(self.nodes[REED].ping(addr))

if __name__ == '__main__':
    unittest.main()
