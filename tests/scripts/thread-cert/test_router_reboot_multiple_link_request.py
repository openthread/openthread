#!/usr/bin/env python3
#
#  Copyright (c) 2023, The OpenThread Authors.
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

import copy
import unittest

import command
import config
import mle
import thread_cert
from pktverify.consts import MLE_PARENT_REQUEST, MLE_LINK_REQUEST, MLE_LINK_ACCEPT, MLE_LINK_ACCEPT_AND_REQUEST, SOURCE_ADDRESS_TLV, CHALLENGE_TLV, RESPONSE_TLV, LINK_LAYER_FRAME_COUNTER_TLV, ROUTE64_TLV, ADDRESS16_TLV, LEADER_DATA_TLV, TLV_REQUEST_TLV, VERSION_TLV
from pktverify.packet_verifier import PacketVerifier
from pktverify.null_field import nullField

LEADER = 1
DUT_ROUTER = 2
MED1 = 3
MED2 = 4
MED3 = 5
MED4 = 6
MED5 = 7
MED6 = 8

# Test Purpose and Description:
# -----------------------------
# The purpose of this test case is to show that when a router with > 5 children is rebooted, it sends MLE_MAX_CRITICAL_TRANSMISSION_COUNT MLE link request packets if no response is received.
#
# Test Topology:
# -------------
#   Leader
#     |
#   Router ------------------------+
#   |       |     |     |    |     |
#   MED1  MED2  MED3  MED4  MED5  MED6
#
# DUT Types:
# ----------
#  Router


class Test_LeaderRebootMultipleLinkRequest(thread_cert.TestCase):
    #USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        LEADER: {
            'name': 'LEADER',
            'mode': 'rdn',
            'allowlist': [DUT_ROUTER]
        },
        DUT_ROUTER: {
            'name': 'ROUTER',
            'mode': 'rdn',
            'allowlist': [LEADER, MED1, MED2, MED3, MED4, MED5, MED6]
        },
        MED1: {
            'name': 'MED1',
            'mode': 'rn',
            'allowlist': [DUT_ROUTER]
        },
        MED2: {
            'name': 'MED2',
            'mode': 'rn',
            'allowlist': [DUT_ROUTER]
        },
        MED3: {
            'name': 'MED3',
            'mode': 'rn',
            'allowlist': [DUT_ROUTER]
        },
        MED4: {
            'name': 'MED4',
            'mode': 'rn',
            'allowlist': [DUT_ROUTER]
        },
        MED5: {
            'name': 'MED5',
            'mode': 'rn',
            'allowlist': [DUT_ROUTER]
        },
        MED6: {
            'name': 'MED6',
            'mode': 'rn',
            'allowlist': [DUT_ROUTER]
        },
    }

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[DUT_ROUTER].start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual(self.nodes[DUT_ROUTER].get_state(), 'router')

        for medid in range(MED1, MED6 + 1):
            self.nodes[medid].start()
            self.simulator.go(config.ED_STARTUP_DELAY)
            self.assertEqual(self.nodes[medid].get_state(), 'child')

        self.simulator.go(config.MAX_ADVERTISEMENT_INTERVAL)

        self.nodes[DUT_ROUTER].reset()
        # Leader will not reply to router's link request
        self.nodes[LEADER].clear_allowlist()

        self.nodes[DUT_ROUTER].start()

        self.simulator.go(config.LEADER_RESET_DELAY)

    def verify(self, pv):
        pkts = pv.pkts
        pv.summary.show()

        LEADER = pv.vars['LEADER']
        ROUTER = pv.vars['ROUTER']

        # Verify topology is formed correctly.
        pv.verify_attached('ROUTER', 'LEADER')
        for i in range(1, 7):
            pv.verify_attached('MED%d' % i, 'ROUTER', 'MTD')

        pkts.filter_wpan_src64(ROUTER).\
            filter_mle_advertisement('Router').\
            must_next()

        # The router MUST send MLE_MAX_CRITICAL_TRANSMISSION_COUNT multicast Link Request
        # The following TLVs MUST be present in the Link Request:
        #     - Challenge TLV
        #     - Version TLV
        #     - TLV Request TLV: Address16 TLV, Route64 TLV
        for i in range(0, config.MLE_MAX_CRITICAL_TRANSMISSION_COUNT):
            pkts.filter_wpan_src64(ROUTER).\
                filter_LLARMA().\
                filter_mle_cmd(MLE_LINK_REQUEST).\
                filter(lambda p: {
                                CHALLENGE_TLV,
                                VERSION_TLV,
                                TLV_REQUEST_TLV,
                                ADDRESS16_TLV,
                                ROUTE64_TLV
                                } <= set(p.mle.tlv.type) and\
                    p.mle.tlv.addr16 is nullField and\
                    p.mle.tlv.route64.id_mask is nullField
                    ).\
                must_next()


if __name__ == '__main__':
    unittest.main()
