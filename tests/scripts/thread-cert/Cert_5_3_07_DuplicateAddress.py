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

import command
import config
import mle
import thread_cert

DUT_LEADER = 1
ROUTER1 = 2
ROUTER2 = 3
MED1 = 4
SED1 = 5
MED3 = 6

MTDS = [MED1, SED1, MED3]


class Cert_5_3_7_DuplicateAddress(thread_cert.TestCase):
    TOPOLOGY = {
        DUT_LEADER: {
            'mode': 'rsdn',
            'panid': 0xface,
            'allowlist': [ROUTER1, ROUTER2, MED3]
        },
        ROUTER1: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'allowlist': [DUT_LEADER, MED1]
        },
        ROUTER2: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'allowlist': [DUT_LEADER, SED1]
        },
        MED1: {
            'is_mtd': True,
            'mode': 'rsn',
            'panid': 0xface,
            'allowlist': [ROUTER1]
        },
        SED1: {
            'is_mtd': True,
            'mode': 's',
            'panid': 0xface,
            'allowlist': [ROUTER2]
        },
        MED3: {
            'is_mtd': True,
            'mode': 'rsn',
            'panid': 0xface,
            'allowlist': [DUT_LEADER]
        },
    }

    def test(self):
        # 1
        self.nodes[DUT_LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[DUT_LEADER].get_state(), 'leader')

        for i in range(ROUTER1, MED3 + 1):
            self.nodes[i].start()

        self.simulator.go(5)

        for i in [ROUTER1, ROUTER2]:
            self.assertEqual(self.nodes[i].get_state(), 'router')

        for i in MTDS:
            self.assertEqual(self.nodes[i].get_state(), 'child')

        # 2
        leader_messages = self.simulator.get_messages_sent_by(DUT_LEADER)
        msg = leader_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)
        command.check_mle_advertisement(msg)

        # 3
        self.nodes[ROUTER2].add_prefix('2001:2:0:1::/64', 'paros')
        self.nodes[ROUTER2].register_netdata()

        # Set lowpan context of sniffer
        self.simulator.set_lowpan_context(1, '2001:2:0:1::/64')

        self.nodes[MED1].add_ipaddr('2001:2:0:1::1234')
        self.nodes[SED1].add_ipaddr('2001:2:0:1::1234')

        self.simulator.go(5)

        # 4
        # Flush the message queue to avoid possible impact on follow-up
        # verification.
        self.simulator.get_messages_sent_by(DUT_LEADER)

        self.nodes[MED3].ping('2001:2:0:1::1234')

        # Verify DUT_LEADER sent an Address Query Request to the Realm local
        # address.
        dut_messages = self.simulator.get_messages_sent_by(DUT_LEADER)
        msg = dut_messages.next_coap_message('0.02', '/a/aq')
        command.check_address_query(msg, self.nodes[DUT_LEADER], config.REALM_LOCAL_ALL_ROUTERS_ADDRESS)

        # 5 & 6
        # Verify DUT_LEADER sent an Address Error Notification to the Realm
        # local address.
        self.simulator.go(5)
        dut_messages = self.simulator.get_messages_sent_by(DUT_LEADER)
        msg = dut_messages.next_coap_message('0.02', '/a/ae')
        command.check_address_error_notification(msg, self.nodes[DUT_LEADER], config.REALM_LOCAL_ALL_ROUTERS_ADDRESS)


if __name__ == '__main__':
    unittest.main()
