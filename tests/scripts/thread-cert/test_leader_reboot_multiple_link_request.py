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

DUT_LEADER = 1
DUT_ROUTER1 = 2

# Test Purpose and Description:
# -----------------------------
# The purpose of this test case is to show that when the Leader is rebooted, it sends MLE_MAX_CRITICAL_TRANSMISSION_COUNT MLE link request packets if no response is received.
#
# Test Topology:
# -------------
#   Leader
#     |
#   Router
#
# DUT Types:
# ----------
#  Leader
#  Router


class Test_LeaderRebootMultipleLinkRequest(thread_cert.TestCase):
    #USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        DUT_LEADER: {
            'name': 'LEADER',
            'mode': 'rdn',
            'allowlist': [DUT_ROUTER1]
        },
        DUT_ROUTER1: {
            'name': 'ROUTER',
            'mode': 'rdn',
            'allowlist': [DUT_LEADER]
        },
    }

    def _setUpLeader(self):
        self.nodes[DUT_LEADER].add_allowlist(self.nodes[DUT_ROUTER1].get_addr64())
        self.nodes[DUT_LEADER].enable_allowlist()

    def test(self):
        self.nodes[DUT_LEADER].start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual(self.nodes[DUT_LEADER].get_state(), 'leader')

        self.nodes[DUT_ROUTER1].start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual(self.nodes[DUT_ROUTER1].get_state(), 'router')

        leader_rloc = self.nodes[DUT_LEADER].get_ip6_address(config.ADDRESS_TYPE.RLOC)

        leader_rloc16 = self.nodes[DUT_LEADER].get_addr16()
        self.nodes[DUT_LEADER].reset()
        self.assertFalse(self.nodes[DUT_ROUTER1].ping(leader_rloc))
        self._setUpLeader()

        # Router1 will not reply to leader's link request
        self.nodes[DUT_ROUTER1].clear_allowlist()

        self.nodes[DUT_LEADER].start()

        self.simulator.go(config.LEADER_RESET_DELAY)

    def verify(self, pv):
        pkts = pv.pkts
        pv.summary.show()

        LEADER = pv.vars['LEADER']
        ROUTER = pv.vars['ROUTER']

        # Verify topology is formed correctly.
        pv.verify_attached('ROUTER', 'LEADER')

        # The DUT MUST send properly formatted MLE Advertisements with
        # an IP Hop Limit of 255 to the Link-Local All Nodes multicast
        # address (FF02::1).
        #  The following TLVs MUST be present in the MLE Advertisements:
        #      - Leader Data TLV
        #      - Route64 TLV
        #      - Source Address TLV
        with pkts.save_index():
            pkts.filter_wpan_src64(LEADER).\
                filter_mle_advertisement('Leader').\
                must_next()
        pkts.filter_wpan_src64(ROUTER).\
            filter_mle_advertisement('Router').\
            must_next()

        pkts.filter_ping_request().\
            filter_wpan_src64(ROUTER).\
            must_next()

        # The Leader MUST send MLE_MAX_CRITICAL_TRANSMISSION_COUNT multicast Link Request
        # The following TLVs MUST be present in the Link Request:
        #     - Challenge TLV
        #     - Version TLV
        #     - TLV Request TLV: Address16 TLV, Route64 TLV
        for i in range(0, config.MLE_MAX_CRITICAL_TRANSMISSION_COUNT):
            pkts.filter_wpan_src64(LEADER).\
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
