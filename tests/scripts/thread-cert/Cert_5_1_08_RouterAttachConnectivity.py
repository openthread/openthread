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

import config
import mle
import thread_cert

LEADER = 1
ROUTER1 = 2
ROUTER2 = 3
ROUTER3 = 4
ROUTER4 = 5


class Cert_5_1_08_RouterAttachConnectivity(thread_cert.TestCase):
    TOPOLOGY = {
        LEADER: {
            'mode': 'rsdn',
            'panid': 0xface,
            'whitelist': [ROUTER1, ROUTER2, ROUTER3]
        },
        ROUTER1: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER, ROUTER3]
        },
        ROUTER2: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER, ROUTER4]
        },
        ROUTER3: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER, ROUTER1, ROUTER4]
        },
        ROUTER4: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [ROUTER2, ROUTER3]
        },
    }

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        for i in range(2, 5):
            self.nodes[i].start()

        self.simulator.go(5)

        for i in range(2, 5):
            self.assertEqual(self.nodes[i].get_state(), 'router')

        self.simulator.go(config.MAX_ADVERTISEMENT_INTERVAL)

        self.nodes[ROUTER4].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER4].get_state(), 'router')

        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        router1_messages = self.simulator.get_messages_sent_by(ROUTER1)
        router2_messages = self.simulator.get_messages_sent_by(ROUTER2)
        router3_messages = self.simulator.get_messages_sent_by(ROUTER3)
        router4_messages = self.simulator.get_messages_sent_by(ROUTER4)

        # 1 - Leader, Router1, Router2, Router3
        leader_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)
        router1_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)
        router2_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)
        router3_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)

        # 2 - Router4
        msg = router4_messages.next_mle_message(mle.CommandType.PARENT_REQUEST)
        msg.assertSentWithHopLimit(255)
        msg.assertSentToDestinationAddress("ff02::2")
        msg.assertMleMessageContainsTlv(mle.Mode)
        msg.assertMleMessageContainsTlv(mle.Challenge)
        msg.assertMleMessageContainsTlv(mle.ScanMask)
        msg.assertMleMessageContainsTlv(mle.Version)

        scan_mask_tlv = msg.get_mle_message_tlv(mle.ScanMask)
        self.assertEqual(1, scan_mask_tlv.router)
        self.assertEqual(0, scan_mask_tlv.end_device)

        # 3 - Router2, Router3
        msg = router2_messages.next_mle_message(mle.CommandType.PARENT_RESPONSE)
        msg.assertSentToNode(self.nodes[ROUTER4])

        msg = router3_messages.next_mle_message(mle.CommandType.PARENT_RESPONSE)
        msg.assertSentToNode(self.nodes[ROUTER4])

        # 4 - Router4
        msg = router4_messages.next_mle_message(
            mle.CommandType.CHILD_ID_REQUEST)
        msg.assertSentToNode(self.nodes[ROUTER3])
        msg.assertMleMessageContainsTlv(mle.Response)
        msg.assertMleMessageContainsTlv(mle.LinkLayerFrameCounter)
        msg.assertMleMessageContainsOptionalTlv(mle.MleFrameCounter)
        msg.assertMleMessageContainsTlv(mle.Mode)
        msg.assertMleMessageContainsTlv(mle.Timeout)
        msg.assertMleMessageContainsTlv(mle.Version)
        msg.assertMleMessageContainsTlv(mle.TlvRequest)
        msg.assertMleMessageDoesNotContainTlv(mle.AddressRegistration)


if __name__ == '__main__':
    unittest.main()
