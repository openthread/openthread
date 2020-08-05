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
ROUTER = 16
DUT_REED = 17
ED = 18
MESH_LOCAL_PREFIX = 'fd00:db8:'
ROUTING_LOCATOR = ':0:ff:fe00'
REED_ADVERTISEMENT_INTERVAL = 570
REED_ADVERTISEMENT_MAX_JITTER = 60
ROUTER_SELECTION_JITTER = 1


class Cert_5_2_4_REEDUpgrade(thread_cert.TestCase):
    TOPOLOGY = {
        LEADER: {
            'mode': 'rsdn',
            'panid': 0xface,
            'whitelist': [2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, ROUTER]
        },
        2: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER]
        },
        3: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER]
        },
        4: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER]
        },
        5: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER]
        },
        6: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER]
        },
        7: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER]
        },
        8: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER]
        },
        9: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER]
        },
        10: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER]
        },
        11: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER]
        },
        12: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER]
        },
        13: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER]
        },
        14: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER]
        },
        15: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER]
        },
        ROUTER: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER, DUT_REED]
        },
        DUT_REED: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [ROUTER, ED]
        },
        ED: {
            'mode': 'rsdn',
            'panid': 0xface,
            'whitelist': [DUT_REED]
        },
    }

    def test(self):
        # 1 Ensure topology is formed correctly without the DUT_REED.
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        for i in range(2, 17):
            self.nodes[i].start()
            self.simulator.go(5)
            self.assertEqual(self.nodes[i].get_state(), 'router')

        # 2 DUT_REED: Attach to ROUTER, 2-hops from the Leader.
        self.nodes[DUT_REED].start()
        self.simulator.go(5)
        self.simulator.go(ROUTER_SELECTION_JITTER)

        reed_messages = self.simulator.get_messages_sent_by(DUT_REED)

        # The DUT_REED must not send a coap message here.
        msg = reed_messages.does_not_contain_coap_message()
        assert msg is True, "Error: The REED sent an Address Solicit Request"

        # 3 DUT_REED: Verify MLE Advertisement.
        msg = reed_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)
        msg.assertSentWithHopLimit(255)
        msg.assertSentToDestinationAddress('ff02::1')
        msg.assertMleMessageContainsTlv(mle.SourceAddress)
        msg.assertMleMessageContainsTlv(mle.LeaderData)
        msg.assertMleMessageDoesNotContainTlv(mle.Route64)

        # 4 Wait for DUT_REED to send the second packet.
        self.simulator.go(REED_ADVERTISEMENT_INTERVAL + REED_ADVERTISEMENT_MAX_JITTER)

        # 5 DUT_REED: Verify the second MLE Advertisement.
        reed_messages = self.simulator.get_messages_sent_by(DUT_REED)
        msg = reed_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)

        # 6 ED
        self.nodes[ED].start()
        self.simulator.go(5)

        # 7 DUT_REED: Verify MLE Parent Response.
        reed_messages = self.simulator.get_messages_sent_by(DUT_REED)
        msg = reed_messages.next_mle_message(mle.CommandType.PARENT_RESPONSE)
        msg.assertSentToNode(self.nodes[ED])
        msg.assertMleMessageContainsTlv(mle.SourceAddress)
        msg.assertMleMessageContainsTlv(mle.LeaderData)
        msg.assertMleMessageContainsTlv(mle.LinkLayerFrameCounter)
        msg.assertMleMessageContainsOptionalTlv(mle.MleFrameCounter)
        msg.assertMleMessageContainsTlv(mle.Response)
        msg.assertMleMessageContainsTlv(mle.Challenge)
        msg.assertMleMessageContainsTlv(mle.LinkMargin)
        msg.assertMleMessageContainsTlv(mle.Connectivity)
        msg.assertMleMessageContainsTlv(mle.Version)

        # 8 DUT_REED: Verify Address Solicit Request.
        msg = reed_messages.next_coap_message('0.02')
        msg.assertCoapMessageRequestUriPath('/a/as')
        msg.assertCoapMessageContainsTlv(network_layer.MacExtendedAddress)
        msg.assertCoapMessageContainsTlv(network_layer.Status)
        msg.assertCoapMessageContainsOptionalTlv(network_layer.Rloc16)

        # 9 DUT_REED: Verify Multicast Link Request.
        msg = reed_messages.next_mle_message(mle.CommandType.LINK_REQUEST)
        msg.assertSentToDestinationAddress('ff02::2')
        msg.assertMleMessageContainsTlv(mle.Challenge)
        msg.assertMleMessageContainsTlv(mle.LeaderData)
        msg.assertMleMessageContainsTlv(mle.SourceAddress)
        msg.assertMleMessageContainsTlv(mle.TlvRequest)
        msg.assertMleMessageContainsTlv(mle.Version)

        # 10 DUT_REED: Verify MLE Child ID Response.
        msg = reed_messages.next_mle_message(mle.CommandType.CHILD_ID_RESPONSE)
        msg.assertSentToNode(self.nodes[ED])
        msg.assertMleMessageContainsTlv(mle.Address16)

        # 11 Verify connectivity by sending an ICMPv6 Echo Request to the
        # Leader.
        mleid = None
        for addr in self.nodes[LEADER].get_addrs():
            if (addr.startswith(MESH_LOCAL_PREFIX) and addr.find(ROUTING_LOCATOR) == -1):
                mleid = addr
                break

        self.assertTrue(self.nodes[ED].ping(mleid))


if __name__ == '__main__':
    unittest.main()
