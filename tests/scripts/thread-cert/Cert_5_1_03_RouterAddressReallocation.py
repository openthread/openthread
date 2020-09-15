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

LEADER = 1
ROUTER1 = 2
ROUTER2 = 3


class Cert_5_1_03_RouterAddressReallocation(thread_cert.TestCase):
    TOPOLOGY = {
        LEADER: {
            'mode': 'rsdn',
            'panid': 0xface,
            'allowlist': [ROUTER1, ROUTER2]
        },
        ROUTER1: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'allowlist': [LEADER, ROUTER2]
        },
        ROUTER2: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'allowlist': [LEADER, ROUTER1]
        },
    }

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[ROUTER1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER1].get_state(), 'router')

        self.nodes[ROUTER2].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER2].get_state(), 'router')

        rloc16 = self.nodes[ROUTER1].get_addr16()

        self.nodes[ROUTER2].set_network_id_timeout(110)
        self.nodes[LEADER].stop()
        self.simulator.go(140)

        self.assertEqual(self.nodes[ROUTER2].get_state(), 'leader')
        self.assertEqual(self.nodes[ROUTER1].get_state(), 'router')
        self.assertEqual(self.nodes[ROUTER1].get_addr16(), rloc16)

        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        router1_messages = self.simulator.get_messages_sent_by(ROUTER1)
        router2_messages = self.simulator.get_messages_sent_by(ROUTER2)

        # 2 - All
        leader_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)

        router1_messages.next_mle_message(mle.CommandType.PARENT_REQUEST)
        leader_messages.next_mle_message(mle.CommandType.PARENT_RESPONSE)

        router1_messages.next_mle_message(mle.CommandType.CHILD_ID_REQUEST)
        leader_messages.next_mle_message(mle.CommandType.CHILD_ID_RESPONSE)

        msg = router1_messages.next_coap_message("0.02")
        msg.assertCoapMessageRequestUriPath("/a/as")

        msg = leader_messages.next_coap_message("2.04")

        router1_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)

        router2_messages.next_mle_message(mle.CommandType.PARENT_REQUEST)
        leader_messages.next_mle_message(mle.CommandType.PARENT_RESPONSE)
        router1_messages.next_mle_message(mle.CommandType.PARENT_RESPONSE)

        router2_messages.next_mle_message(mle.CommandType.CHILD_ID_REQUEST)

        # Leader or Router1 can be parent of Router2
        if leader_messages.contains_mle_message(mle.CommandType.CHILD_ID_RESPONSE):
            leader_messages.next_mle_message(mle.CommandType.CHILD_ID_RESPONSE)

            msg = router2_messages.next_coap_message("0.02")
            msg.assertCoapMessageRequestUriPath("/a/as")

            msg = leader_messages.next_coap_message("2.04")

        elif router1_messages.contains_mle_message(mle.CommandType.CHILD_ID_RESPONSE):
            router1_messages.next_mle_message(mle.CommandType.CHILD_ID_RESPONSE)

            msg = router2_messages.next_coap_message("0.02")
            msg.assertCoapMessageRequestUriPath("/a/as")

            msg = router1_messages.next_coap_message("0.02")
            msg.assertCoapMessageRequestUriPath("/a/as")

            msg = leader_messages.next_coap_message("2.04")

            msg = router1_messages.next_coap_message("2.04")

        # 5 - Router1
        # Router1 make two attempts to reconnect to its current Partition.
        for _ in range(4):
            msg = router1_messages.next_mle_message(mle.CommandType.PARENT_REQUEST)
            msg.assertSentWithHopLimit(255)
            msg.assertSentToDestinationAddress("ff02::2")
            msg.assertMleMessageContainsTlv(mle.Mode)
            msg.assertMleMessageContainsTlv(mle.Challenge)
            msg.assertMleMessageContainsTlv(mle.ScanMask)
            msg.assertMleMessageContainsTlv(mle.Version)

            scan_mask_tlv = msg.get_mle_message_tlv(mle.ScanMask)
            self.assertEqual(1, scan_mask_tlv.router)
            self.assertEqual(1, scan_mask_tlv.end_device)

        # 6 - Router1
        msg = router1_messages.next_mle_message(mle.CommandType.PARENT_REQUEST)
        msg.assertSentWithHopLimit(255)
        msg.assertSentToDestinationAddress("ff02::2")
        msg.assertMleMessageContainsTlv(mle.Mode)
        msg.assertMleMessageContainsTlv(mle.Challenge)
        msg.assertMleMessageContainsTlv(mle.ScanMask)
        msg.assertMleMessageContainsTlv(mle.Version)

        scan_mask_tlv = msg.get_mle_message_tlv(mle.ScanMask)
        self.assertEqual(1, scan_mask_tlv.router)
        self.assertEqual(0, scan_mask_tlv.end_device)

        # 7 - Router1
        msg = router1_messages.next_mle_message(mle.CommandType.CHILD_ID_REQUEST)
        msg.assertSentToNode(self.nodes[ROUTER2])
        msg.assertMleMessageContainsTlv(mle.Response)
        msg.assertMleMessageContainsTlv(mle.LinkLayerFrameCounter)
        msg.assertMleMessageContainsOptionalTlv(mle.MleFrameCounter)
        msg.assertMleMessageContainsTlv(mle.Mode)
        msg.assertMleMessageContainsTlv(mle.Timeout)
        msg.assertMleMessageContainsTlv(mle.Version)
        msg.assertMleMessageContainsTlv(mle.TlvRequest)
        msg.assertMleMessageDoesNotContainTlv(mle.AddressRegistration)

        # 8 - Router1
        msg = router1_messages.next_coap_message("0.02")
        msg.assertCoapMessageRequestUriPath("/a/as")
        msg.assertCoapMessageContainsTlv(network_layer.MacExtendedAddress)
        msg.assertCoapMessageContainsOptionalTlv(network_layer.Rloc16)
        msg.assertCoapMessageContainsTlv(network_layer.Status)

        # 8 - Router2
        msg = router2_messages.next_coap_message("2.04")


if __name__ == '__main__':
    unittest.main()
