#!/usr/bin/env python3
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

import command
import config
import thread_cert

LEADER = 1
DUT_ROUTER1 = 2
MED1 = 3


class Cert_5_3_11_AddressQueryTimeoutIntervals(thread_cert.TestCase):
    TOPOLOGY = {
        LEADER: {
            'mode': 'rsdn',
            'panid': 0xface,
            'allowlist': [DUT_ROUTER1]
        },
        DUT_ROUTER1: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'allowlist': [LEADER, MED1]
        },
        MED1: {
            'is_mtd': True,
            'mode': 'rsn',
            'panid': 0xface,
            'allowlist': [DUT_ROUTER1]
        },
    }

    def test(self):
        # 1 ALL: Build and verify the topology
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[DUT_ROUTER1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[DUT_ROUTER1].get_state(), 'router')

        self.nodes[MED1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[MED1].get_state(), 'child')

        # 2 MED1: MED1 sends an ICMPv6 Echo Request to a non-existent
        # mesh-local address X
        X = "fd00:db8:0000:0000:aa55:aa55:aa55:aa55"
        self.assertFalse(self.nodes[MED1].ping(X))

        self.simulator.go(config.AQ_TIMEOUT)

        # Verify DUT_ROUTER1 sent an Address Query Request message
        dut_router1_messages = self.simulator.get_messages_sent_by(DUT_ROUTER1)
        msg = dut_router1_messages.next_coap_message('0.02', '/a/aq')
        command.check_address_query(
            msg,
            self.nodes[DUT_ROUTER1],
            config.REALM_LOCAL_ALL_ROUTERS_ADDRESS,
        )

        # Verify DUT_ROUTER1 didn't receive an Address Query Notification
        # message
        dut_router1_messages = self.simulator.get_messages_sent_by(DUT_ROUTER1)
        msg = dut_router1_messages.next_coap_message('0.02', '/a/an', False)
        self.assertTrue(msg is None)

        # 2 MED1: MED1 sends an ICMPv6 Echo Request to a non-existent mesh-local address X before
        #         ADDRESS_QUERY_INITIAL_RETRY_DELAY timeout expires
        self.assertFalse(self.nodes[MED1].ping(X))

        self.simulator.go(config.ADDRESS_QUERY_INITIAL_RETRY_DELAY)

        # Verify DUT_ROUTER1 didn't generate an Address Query Request message
        dut_router1_messages = self.simulator.get_messages_sent_by(DUT_ROUTER1)
        msg = dut_router1_messages.next_coap_message('0.02', '/a/aq', False)
        self.assertTrue(msg is None)

        # 3 MED1: MED1 sends an ICMPv6 Echo Request to a non-existent mesh-local address X after
        #         ADDRESS_QUERY_INITIAL_RETRY_DELAY timeout expired
        self.assertFalse(self.nodes[MED1].ping(X))

        # Verify DUT_ROUTER1 sent an Address Query Request message
        dut_router1_messages = self.simulator.get_messages_sent_by(DUT_ROUTER1)
        msg = dut_router1_messages.next_coap_message('0.02', '/a/aq')
        command.check_address_query(
            msg,
            self.nodes[DUT_ROUTER1],
            config.REALM_LOCAL_ALL_ROUTERS_ADDRESS,
        )


if __name__ == '__main__':
    unittest.main()
