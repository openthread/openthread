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
ROUTER = 2


class Cert_5_1_01_RouterAttach(thread_cert.TestCase):
    TOPOLOGY = {
        LEADER: {
            'mode': 'rdn',
            'panid': 0xface,
            'allowlist': [ROUTER]
        },
        ROUTER: {
            'mode': 'rdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'allowlist': [LEADER]
        },
    }

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[ROUTER].start()
        self.simulator.go(7)
        self.assertEqual(self.nodes[ROUTER].get_state(), 'router')

        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        router_messages = self.simulator.get_messages_sent_by(ROUTER)

        # 1 - Leader
        msg = leader_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)
        msg.assertSentWithHopLimit(255)
        msg.assertSentToDestinationAddress("ff02::1")
        msg.assertMleMessageContainsTlv(mle.SourceAddress)
        msg.assertMleMessageContainsTlv(mle.LeaderData)
        msg.assertMleMessageContainsTlv(mle.Route64)

        # 2 - Router
        msg = router_messages.next_mle_message(mle.CommandType.PARENT_REQUEST)
        msg.assertSentWithHopLimit(255)
        msg.assertSentToDestinationAddress("ff02::2")
        msg.assertMleMessageContainsTlv(mle.Mode)
        msg.assertMleMessageContainsTlv(mle.Challenge)
        msg.assertMleMessageContainsTlv(mle.ScanMask)
        msg.assertMleMessageContainsTlv(mle.Version)

        scan_mask_tlv = msg.get_mle_message_tlv(mle.ScanMask)
        self.assertEqual(1, scan_mask_tlv.router)
        self.assertEqual(0, scan_mask_tlv.end_device)

        # 3 - Leader
        msg = leader_messages.next_mle_message(mle.CommandType.PARENT_RESPONSE)
        msg.assertSentToNode(self.nodes[ROUTER])
        msg.assertMleMessageContainsTlv(mle.SourceAddress)
        msg.assertMleMessageContainsTlv(mle.LeaderData)
        msg.assertMleMessageContainsTlv(mle.LinkLayerFrameCounter)
        msg.assertMleMessageContainsOptionalTlv(mle.MleFrameCounter)
        msg.assertMleMessageContainsTlv(mle.Response)
        msg.assertMleMessageContainsTlv(mle.Challenge)
        msg.assertMleMessageContainsTlv(mle.LinkMargin)
        msg.assertMleMessageContainsTlv(mle.Connectivity)
        msg.assertMleMessageContainsTlv(mle.Version)

        # 4 - Router
        msg = router_messages.next_mle_message(mle.CommandType.CHILD_ID_REQUEST)
        msg.assertSentToNode(self.nodes[LEADER])
        msg.assertMleMessageContainsTlv(mle.Response)
        msg.assertMleMessageContainsTlv(mle.LinkLayerFrameCounter)
        msg.assertMleMessageContainsOptionalTlv(mle.MleFrameCounter)
        msg.assertMleMessageContainsTlv(mle.Mode)
        msg.assertMleMessageContainsTlv(mle.Timeout)
        msg.assertMleMessageContainsTlv(mle.Version)
        msg.assertMleMessageContainsTlv(mle.TlvRequest)
        msg.assertMleMessageDoesNotContainTlv(mle.AddressRegistration)

        # 5 - Leader
        msg = leader_messages.next_mle_message(mle.CommandType.CHILD_ID_RESPONSE)
        msg.assertSentToNode(self.nodes[ROUTER])
        msg.assertMleMessageContainsTlv(mle.SourceAddress)
        msg.assertMleMessageContainsTlv(mle.LeaderData)
        msg.assertMleMessageContainsTlv(mle.Address16)
        msg.assertMleMessageContainsOptionalTlv(mle.NetworkData)
        msg.assertMleMessageContainsOptionalTlv(mle.Route64)
        msg.assertMleMessageContainsOptionalTlv(mle.AddressRegistration)

        # 6 - Router
        msg = router_messages.next_coap_message("0.02")
        msg.assertCoapMessageRequestUriPath("/a/as")
        msg.assertCoapMessageContainsTlv(network_layer.MacExtendedAddress)
        msg.assertCoapMessageContainsTlv(network_layer.Status)

        # 7 - Leader
        msg = leader_messages.next_coap_message("2.04")
        msg.assertCoapMessageContainsTlv(network_layer.Status)
        msg.assertCoapMessageContainsOptionalTlv(network_layer.RouterMask)

        status_tlv = msg.get_coap_message_tlv(network_layer.Status)
        self.assertEqual(network_layer.StatusValues.SUCCESS, status_tlv.status)

        # 8 - Router
        msg = router_messages.next_mle_message(mle.CommandType.LINK_REQUEST)
        msg.assertMleMessageContainsTlv(mle.SourceAddress)
        msg.assertMleMessageContainsTlv(mle.LeaderData)
        msg.assertMleMessageContainsTlv(mle.Challenge)
        msg.assertMleMessageContainsTlv(mle.Version)
        msg.assertMleMessageContainsTlv(mle.TlvRequest)

        tlv_request = msg.get_mle_message_tlv(mle.TlvRequest)
        self.assertIn(mle.TlvType.LINK_MARGIN, tlv_request.tlvs)

        # 9 - Leader
        msg = leader_messages.next_mle_message(mle.CommandType.LINK_ACCEPT_AND_REQUEST)
        msg.assertMleMessageContainsTlv(mle.SourceAddress)
        msg.assertMleMessageContainsTlv(mle.LeaderData)
        msg.assertMleMessageContainsTlv(mle.Response)
        msg.assertMleMessageContainsTlv(mle.LinkLayerFrameCounter)
        msg.assertMleMessageContainsTlv(mle.Version)
        msg.assertMleMessageContainsTlv(mle.LinkMargin)
        msg.assertMleMessageContainsOptionalTlv(mle.MleFrameCounter)
        msg.assertMleMessageContainsOptionalTlv(mle.Challenge)

        # 10 - Router
        msg = router_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)
        msg.assertSentWithHopLimit(255)
        msg.assertSentToDestinationAddress("ff02::1")
        msg.assertMleMessageContainsTlv(mle.SourceAddress)
        msg.assertMleMessageContainsTlv(mle.LeaderData)
        msg.assertMleMessageContainsTlv(mle.Route64)

        # 11 - Leader, Router1
        for addr in self.nodes[LEADER].get_addrs():
            self.assertTrue(self.nodes[ROUTER].ping(addr))


if __name__ == '__main__':
    unittest.main()
