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
ROUTER = 2
ED = 3
SED = 4

MTDS = [ED, SED]


class Cert_5_1_02_ChildAddressTimeout(thread_cert.TestCase):
    TOPOLOGY = {
        LEADER: {
            'mode': 'rsdn',
            'panid': 0xface,
            'allowlist': [ROUTER]
        },
        ROUTER: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'allowlist': [LEADER, ED, SED]
        },
        ED: {
            'is_mtd': True,
            'mode': 'rsn',
            'panid': 0xface,
            'timeout': config.DEFAULT_CHILD_TIMEOUT,
            'allowlist': [ROUTER]
        },
        SED: {
            'is_mtd': True,
            'mode': 'sn',
            'panid': 0xface,
            'timeout': config.DEFAULT_CHILD_TIMEOUT,
            'allowlist': [ROUTER]
        },
    }

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[ROUTER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER].get_state(), 'router')

        self.nodes[ED].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ED].get_state(), 'child')

        self.nodes[SED].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[SED].get_state(), 'child')

        ed_addrs = self.nodes[ED].get_addrs()
        sed_addrs = self.nodes[SED].get_addrs()

        self.nodes[ED].stop()
        self.simulator.go(5)
        for addr in ed_addrs:
            if addr[0:4] != 'fe80':
                self.assertFalse(self.nodes[LEADER].ping(addr))

        self.nodes[SED].stop()
        self.simulator.go(5)
        for addr in sed_addrs:
            if addr[0:4] != 'fe80':
                self.assertFalse(self.nodes[LEADER].ping(addr))

        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        router1_messages = self.simulator.get_messages_sent_by(ROUTER)
        ed_messages = self.simulator.get_messages_sent_by(ED)
        sed_messages = self.simulator.get_messages_sent_by(SED)

        # 1 - All
        leader_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)

        router1_messages.next_mle_message(mle.CommandType.PARENT_REQUEST)
        leader_messages.next_mle_message(mle.CommandType.PARENT_RESPONSE)

        router1_messages.next_mle_message(mle.CommandType.CHILD_ID_REQUEST)
        leader_messages.next_mle_message(mle.CommandType.CHILD_ID_RESPONSE)

        msg = router1_messages.next_coap_message("0.02")
        msg.assertCoapMessageRequestUriPath("/a/as")

        msg = leader_messages.next_coap_message("2.04")

        router1_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)

        ed_messages.next_mle_message(mle.CommandType.PARENT_REQUEST)
        router1_messages.next_mle_message(mle.CommandType.PARENT_RESPONSE)
        ed_messages.next_mle_message(mle.CommandType.CHILD_ID_REQUEST)
        router1_messages.next_mle_message(mle.CommandType.CHILD_ID_RESPONSE)

        sed_messages.next_mle_message(mle.CommandType.PARENT_REQUEST)
        router1_messages.next_mle_message(mle.CommandType.PARENT_RESPONSE)
        sed_messages.next_mle_message(mle.CommandType.CHILD_ID_REQUEST)
        router1_messages.next_mle_message(mle.CommandType.CHILD_ID_RESPONSE)

        # 3 - Leader
        msg = leader_messages.next_coap_message("0.02")
        msg.assertCoapMessageRequestUriPath("/a/aq")

        msg = leader_messages.next_coap_message("0.02")
        msg.assertCoapMessageRequestUriPath("/a/aq")

        # 4 - Router1
        msg = router1_messages.does_not_contain_coap_message()

        # 6 - Leader
        msg = leader_messages.next_coap_message("0.02")
        msg.assertCoapMessageRequestUriPath("/a/aq")

        msg = leader_messages.next_coap_message("0.02")
        msg.assertCoapMessageRequestUriPath("/a/aq")

        # 7 - Router1
        msg = router1_messages.does_not_contain_coap_message()


if __name__ == '__main__':
    unittest.main()
