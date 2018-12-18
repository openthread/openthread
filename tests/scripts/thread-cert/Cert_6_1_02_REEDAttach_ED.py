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

import config
import mle
import node

LEADER = 1
REED = 2
MED = 3

class Cert_6_1_2_REEDAttach_ED(unittest.TestCase):
    def setUp(self):
        self.simulator = config.create_default_simulator()

        self.nodes = {}
        for i in range(1,4):
            self.nodes[i] = node.Node(i, (i == MED), simulator=self.simulator)

        self.nodes[LEADER].set_panid(0xface)
        self.nodes[LEADER].set_mode('rsdn')
        self.nodes[LEADER].add_whitelist(self.nodes[REED].get_addr64())
        self.nodes[LEADER].enable_whitelist()

        self.nodes[REED].set_panid(0xface)
        self.nodes[REED].set_mode('rsdn')
        self.nodes[REED].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[REED].add_whitelist(self.nodes[MED].get_addr64())
        self.nodes[REED].enable_whitelist()
        self.nodes[REED].set_router_upgrade_threshold(0)

        self.nodes[MED].set_panid(0xface)
        self.nodes[MED].set_mode('rsn')
        self.nodes[MED].add_whitelist(self.nodes[REED].get_addr64())
        self.nodes[MED].enable_whitelist()
        self.nodes[MED].set_timeout(config.DEFAULT_CHILD_TIMEOUT)

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

        self.nodes[MED].start()
        self.simulator.go(config.DEFAULT_CHILD_TIMEOUT + 5)
        self.assertEqual(self.nodes[MED].get_state(), 'child')
        self.assertEqual(self.nodes[REED].get_state(), 'router')

        med_messages = self.simulator.get_messages_sent_by(MED)

        # Step 2 - DUT send MLE Parent Request
        msg = med_messages.next_mle_message(mle.CommandType.PARENT_REQUEST)
        self.assertEqual(0x02, msg.mle.aux_sec_hdr.key_id_mode)
        msg.assertSentWithHopLimit(255)
        msg.assertSentToDestinationAddress("ff02::2")
        msg.assertMleMessageContainsTlv(mle.Mode)
        msg.assertMleMessageContainsTlv(mle.Challenge)
        msg.assertMleMessageContainsTlv(mle.ScanMask)
        msg.assertMleMessageContainsTlv(mle.Version)

        scan_mask_tlv = msg.get_mle_message_tlv(mle.ScanMask)
        self.assertEqual(1, scan_mask_tlv.router)
        self.assertEqual(0, scan_mask_tlv.end_device)

        # Step 4 - DUT send MLE Parent Request again
        msg = med_messages.next_mle_message(mle.CommandType.PARENT_REQUEST)
        self.assertEqual(0x02, msg.mle.aux_sec_hdr.key_id_mode)
        msg.assertSentWithHopLimit(255)
        msg.assertSentToDestinationAddress("ff02::2")
        msg.assertMleMessageContainsTlv(mle.Mode)
        msg.assertMleMessageContainsTlv(mle.Challenge)
        msg.assertMleMessageContainsTlv(mle.ScanMask)
        msg.assertMleMessageContainsTlv(mle.Version)

        scan_mask_tlv = msg.get_mle_message_tlv(mle.ScanMask)
        self.assertEqual(1, scan_mask_tlv.router)
        self.assertEqual(1, scan_mask_tlv.end_device)

        # Step 6 - DUT send Child ID Request
        msg = med_messages.next_mle_message(mle.CommandType.CHILD_ID_REQUEST)
        self.assertEqual(0x02, msg.mle.aux_sec_hdr.key_id_mode)
        msg.assertSentToNode(self.nodes[REED])
        msg.assertMleMessageContainsTlv(mle.AddressRegistration)
        msg.assertMleMessageContainsTlv(mle.LinkLayerFrameCounter)
        msg.assertMleMessageContainsTlv(mle.Mode)
        msg.assertMleMessageContainsTlv(mle.Response)
        msg.assertMleMessageContainsTlv(mle.Timeout)
        msg.assertMleMessageContainsTlv(mle.Version)
        msg.assertMleMessageContainsTlv(mle.TlvRequest)
        msg.assertMleMessageContainsOptionalTlv(mle.MleFrameCounter)
        msg.assertMleMessageContainsOptionalTlv(mle.Route64)

        tlv_request_tlv = msg.get_mle_message_tlv(mle.TlvRequest)
        self.assertEqual(mle.TlvType.ADDRESS16, tlv_request_tlv.tlvs[0])
        self.assertEqual(mle.TlvType.NETWORK_DATA, tlv_request_tlv.tlvs[1])

        # Step 8 - DUT send Child Update messages
        msg = med_messages.next_mle_message(mle.CommandType.CHILD_UPDATE_REQUEST)
        msg.assertMleMessageContainsTlv(mle.LeaderData)
        msg.assertMleMessageContainsTlv(mle.Mode)
        msg.assertMleMessageContainsTlv(mle.SourceAddress)

        # Step 10 - Leader send ICMPv6 echo request
        ed_addrs = self.nodes[MED].get_addrs()
        for addr in ed_addrs:
            if addr[0:4] != 'fe80':
                self.assertTrue(self.nodes[LEADER].ping(addr))

if __name__ == '__main__':
    unittest.main()
