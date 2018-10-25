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

import config
import mle
import node

LEADER = 1
ED = 2

class Cert_6_1_1_RouterAttach(unittest.TestCase):
    def setUp(self):
        self.simulator = config.create_default_simulator()

        self.nodes = {}
        for i in range(1,3):
            self.nodes[i] = node.Node(i, (i == ED), simulator=self.simulator)

        self.nodes[LEADER].set_panid(0xface)
        self.nodes[LEADER].set_mode('rsdn')
        self.nodes[LEADER].add_whitelist(self.nodes[ED].get_addr64())
        self.nodes[LEADER].enable_whitelist()

        self.nodes[ED].set_panid(0xface)
        self.nodes[ED].set_mode('rsn')
        self.nodes[ED].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[ED].enable_whitelist()

    def tearDown(self):
        for node in list(self.nodes.values()):
            node.stop()
            node.destroy()
        self.simulator.stop()

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[ED].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ED].get_state(), 'child')

        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        ed_messages = self.simulator.get_messages_sent_by(ED)

        # 1 - leader
        msg = leader_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)

        # 2 - ed
        msg = ed_messages.next_mle_message(mle.CommandType.PARENT_REQUEST)
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

        # 3 - leader
        msg = leader_messages.next_mle_message(mle.CommandType.PARENT_RESPONSE)
        msg.assertSentToNode(self.nodes[ED])

        # 4 - ed
        msg = ed_messages.next_mle_message(mle.CommandType.CHILD_ID_REQUEST)
        self.assertEqual(0x02, msg.mle.aux_sec_hdr.key_id_mode)
        msg.assertSentToNode(self.nodes[LEADER])
        msg.assertMleMessageContainsTlv(mle.AddressRegistration)
        msg.assertMleMessageContainsTlv(mle.LinkLayerFrameCounter)
        msg.assertMleMessageContainsTlv(mle.Mode)
        msg.assertMleMessageContainsTlv(mle.Response)
        msg.assertMleMessageContainsTlv(mle.Timeout)
        msg.assertMleMessageContainsTlv(mle.TlvRequest)
        msg.assertMleMessageContainsTlv(mle.Version)
        msg.assertMleMessageContainsOptionalTlv(mle.MleFrameCounter)

        # 5 - leader
        msg = leader_messages.next_mle_message(mle.CommandType.CHILD_ID_RESPONSE)
        msg.assertSentToNode(self.nodes[ED])

        # 6 - leader
        ed_addrs = self.nodes[ED].get_addrs()
        for addr in ed_addrs:
            if addr[0:4] != 'fe80':
                self.assertTrue(self.nodes[LEADER].ping(addr))

if __name__ == '__main__':
    unittest.main()
