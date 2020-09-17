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

import thread_cert
from pktverify.consts import MLE_CHILD_ID_RESPONSE, MLE_LINK_REQUEST, MLE_LINK_ACCEPT, CHALLENGE_TLV, LEADER_DATA_TLV, SOURCE_ADDRESS_TLV, VERSION_TLV
from pktverify.packet_verifier import PacketVerifier

LEADER = 1
ROUTER1 = 2
ED = 3
ROUTER2 = 4
ROUTER3 = 5


class Cert_6_1_9_EDSynchronization(thread_cert.TestCase):
    TOPOLOGY = {
        LEADER: {
            'name': 'LEADER',
            'mode': 'rsdn',
            'panid': 0xface,
            'allowlist': [ROUTER1, ED, ROUTER2]
        },
        ROUTER1: {
            'name': 'ROUTER_1',
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'allowlist': [LEADER, ED, ROUTER3]
        },
        ED: {
            'name': 'ED',
            'panid': 0xface,
            'router_upgrade_threshold': 0,
            'allowlist': [LEADER]
        },
        ROUTER2: {
            'name': 'ROUTER_2',
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'allowlist': [LEADER, ED, ROUTER3]
        },
        ROUTER3: {
            'name': 'ROUTER_3',
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'allowlist': [ED, ROUTER1, ROUTER2]
        },
    }

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(3)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[ROUTER1].start()
        self.simulator.go(3)
        self.assertEqual(self.nodes[ROUTER1].get_state(), 'router')

        self.nodes[ROUTER2].start()
        self.simulator.go(3)
        self.assertEqual(self.nodes[ROUTER2].get_state(), 'router')

        self.nodes[ROUTER3].start()
        self.simulator.go(3)
        self.assertEqual(self.nodes[ROUTER3].get_state(), 'router')

        self.nodes[ED].start()
        self.simulator.go(3)
        self.assertEqual(self.nodes[ED].get_state(), 'child')

        self.nodes[ED].add_allowlist(self.nodes[ROUTER1].get_addr64())
        self.nodes[ED].add_allowlist(self.nodes[ROUTER2].get_addr64())
        self.nodes[ED].add_allowlist(self.nodes[ROUTER3].get_addr64())
        self.nodes[ED].enable_allowlist()
        self.simulator.go(10)

    def verify(self, pv):
        pkts = pv.pkts
        pv.summary.show()

        LEADER = pv.vars['LEADER']
        ED = pv.vars['ED']

        # Step 3: The DUT MUST send a unicast Link Request
        # to Router 1, Router 2 & Router 3
        pkts.filter_wpan_src64(LEADER).filter_wpan_dst64(ED).filter_mle_cmd(MLE_CHILD_ID_RESPONSE).must_next()
        for i in range(1, 3):
            _pkts = pkts.copy()
            _pkts.filter_wpan_src64(ED).filter_wpan_dst64(
                pv.vars['ROUTER_%d' % i]).filter_mle_cmd(MLE_LINK_REQUEST).must_next().must_verify(
                    lambda p: {CHALLENGE_TLV, LEADER_DATA_TLV, SOURCE_ADDRESS_TLV, VERSION_TLV} <= set(p.mle.tlv.type))

            # Step 4: Router_1, Router_2 & Router_3 MUST all send a
            # Link Accept message to the DUT
            _pkts.filter_wpan_src64(pv.vars['ROUTER_%d' %
                                            i]).filter_wpan_dst64(ED).filter_mle_cmd(MLE_LINK_ACCEPT).must_next()


if __name__ == '__main__':
    unittest.main()
