#!/usr/bin/env python3
#
#  Copyright (c) 2020, The OpenThread Authors.
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

from config import ADDRESS_TYPE
from pktverify import consts
from pktverify.packet_verifier import PacketVerifier

import thread_cert

LEADER = 1
ROUTER = 2
SSED_1 = 3

# 0.5s
CSL_PERIOD = 3125


class LowPower_5_3_01_SSEDAttachment(thread_cert.TestCase):
    TOPOLOGY = {
        LEADER: {
            'version': '1.2',
            'name': 'LEADER',
            'mode': 'rsdn',
            'panid': 0xface,
            'allowlist': [ROUTER, SSED_1]
        },
        ROUTER: {
            'version': '1.2',
            'name': 'ROUTER',
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [LEADER],
        },
        SSED_1: {
            'version': '1.2',
            'name': 'SSED_1',
            'mode': 's',
            'panid': 0xface,
            'whitelist': [LEADER],
        },
    }

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[ROUTER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER].get_state(), 'router')

        self.nodes[SSED_1].set_csl_period(CSL_PERIOD)
        self.nodes[SSED_1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[SSED_1].get_state(), 'child')

        self.simulator.go(5)

        # Verify connectivity by sending an ICMP Echo Request from the Router to SSED_1 mesh-local address via the DUT
        # Set ping timeout as two CSL periods.
        timeout = 2 * CSL_PERIOD * 160 / 1000000
        self.assertTrue(self.nodes[ROUTER].ping(self.nodes[SSED_1].get_ip6_address(ADDRESS_TYPE.RLOC),
                                                timeout=timeout))

        # Verify fragmented CSL transmission
        self.assertTrue(self.nodes[ROUTER].ping(self.nodes[SSED_1].get_ip6_address(ADDRESS_TYPE.RLOC),
                                                size=128,
                                                timeout=timeout))

    def verify(self, pv):
        pkts = pv.pkts
        pv.summary.show()
        LEADER = self.vars['LEADER']
        ROUTER = self.vars['ROUTER']
        SSED_1 = self.vars['SSED_1']

        # Step 1: ensure the Leader is sending MLE Advertisements
        pkts.filter_wpan_src64(LEADER) \
            .filter_mle_cmd(pvconsts.MLE_ADVERTISEMENT) \
            .must_next()

        # Step 2: Verify that the frame version is IEEE Std 802.15.4-2015 of the MLE Child Update Response from the
        # DUT to SSED_1
        pkts.filter_wpan_src64(LEADER) \
            .filter_mle_cmd(pvconsts.MLE_CHILD_UPDATE_RESPONSE) \
            .filter(lambda p: p.wpan.version == 2) \
            .must_next()

        # Step 3:
        # Verify that SSED_1 does not send a MAC Data Request prior to receiving the ICMP Echo Request from the Leader.
        # Verify that the frame version is IEEE Std 802.15.4-2015 of the ICMP Echo Request sent to SSED_1 from the DUT
        # Verify that the Router successfully receives an Echo Response from SSED_1
        pkts.filter_wpan_src64(LEADER) \
            .filter_ping_request() \
            .filter(lambda p : p.wpan.version == 2) \
            .must_next()
        idx1 = pkts.index
        pkts.filter_wpan_src64(SSED_1) \
            .filter_ping_reply() \
            .filter(lambda p : p.wpan.version == 2) \
            .must_next()
        idx2 = pkts.index
        pkts.range(idx1, idx2).filter(lambda p: p.wpan.cmd == MacHeader.CommandIdentifier.DATA_REQUEST) \
            .must_not_next()


if __name__ == '__main__':
    unittest.main()
