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

import mle
import network_layer
import thread_cert

DUT_LEADER = 1
ROUTER_1 = 2
ROUTER_31 = 32
ROUTER_32 = 33


class Cert_5_2_3_LeaderReject2Hops(thread_cert.TestCase):
    TOPOLOGY = {
        DUT_LEADER: {
            'mode':
                'rsdn',
            'panid':
                0xface,
            'router_downgrade_threshold':
                33,
            'router_upgrade_threshold':
                32,
            'allowlist': [
                ROUTER_1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
                28, 29, 30, 31, ROUTER_31
            ]
        },
        ROUTER_1: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER, ROUTER_32]
        },
        3: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        4: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        5: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        6: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        7: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        8: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        9: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        10: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        11: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        12: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        13: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        14: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        15: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        16: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        17: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        18: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        19: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        20: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        21: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        22: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        23: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        24: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        25: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        26: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        27: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        28: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        29: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        30: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        31: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        ROUTER_31: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [DUT_LEADER]
        },
        ROUTER_32: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 33,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 33,
            'allowlist': [ROUTER_1]
        },
    }

    def test(self):
        # 1
        self.nodes[DUT_LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[DUT_LEADER].get_state(), 'leader')

        for i in range(2, 32):
            self.nodes[i].start()
            self.simulator.go(5)
            self.assertEqual(self.nodes[i].get_state(), 'router')

        leader_messages = self.simulator.get_messages_sent_by(DUT_LEADER)

        # 2
        self.nodes[ROUTER_31].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER_31].get_state(), 'router')

        # 3 - DUT_LEADER
        # This method flushes the message queue so calling this method again
        # will return only the newly logged messages.
        leader_messages = self.simulator.get_messages_sent_by(DUT_LEADER)
        msg = leader_messages.next_coap_message('2.04')
        msg.assertCoapMessageContainsTlv(network_layer.Status)
        msg.assertCoapMessageContainsTlv(network_layer.RouterMask)
        msg.assertCoapMessageContainsTlv(network_layer.Rloc16)

        status_tlv = msg.get_coap_message_tlv(network_layer.Status)
        self.assertEqual(network_layer.StatusValues.SUCCESS, status_tlv.status)

        # 4 - DUT_LEADER
        msg = leader_messages.last_mle_message(mle.CommandType.ADVERTISEMENT)
        msg.assertAssignedRouterQuantity(32)

        # 5 - Router_32
        self.nodes[ROUTER_32].start()
        self.simulator.go(5)

        # 6 - DUT_LEADER
        leader_messages = self.simulator.get_messages_sent_by(DUT_LEADER)
        msg = leader_messages.next_coap_message('2.04')
        msg.assertCoapMessageContainsTlv(network_layer.Status)

        status_tlv = msg.get_coap_message_tlv(network_layer.Status)
        self.assertEqual(network_layer.StatusValues.NO_ADDRESS_AVAILABLE, status_tlv.status)


if __name__ == '__main__':
    unittest.main()
