#!/usr/bin/env python3
#
#  Copyright (c) 2022, The OpenThread Authors.
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
import logging
import unittest

import config
import thread_cert
from pktverify.consts import MLE_LINK_REQUEST, MLE_LINK_ACCEPT
from pktverify.packet_verifier import PacketVerifier

LEADER = 1
REED = 2
ROUTER1, ROUTER2, ROUTER3 = 3, 4, 5

# Test Purpose and Description:
# -----------------------------
# This test verifies if a device can quickly and efficiently establish links with
# neighboring routers by sending a multicast Link Request message after becoming
# a router
#
# Test Topology:
# -------------
#     Leader------+
#    |     \       \
# Router1 Router2 Router3
#    \     |        |
#     +---REED-----+
#

# REED should establish links to the three Routers within the delay threshold.
# The max delay consists:
#   Leader may delay 1 second for MLE Advertisement
#   Router may delay 1 second for MLE Advertisement
#   Router may delay 1 second for MLE Link Accept after receiving MLE Link Request (multicast)
LINK_ESTABLISH_DELAY_THRESHOLD = 3


class TestRouterMulticastLinkRequest(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        LEADER: {
            'name': 'LEADER',
            'mode': 'rdn',
            'allowlist': [ROUTER1, ROUTER2, ROUTER3]
        },
        ROUTER1: {
            'name': 'ROUTER1',
            'mode': 'rdn',
            'allowlist': [LEADER, REED]
        },
        ROUTER2: {
            'name': 'ROUTER2',
            'mode': 'rdn',
            'allowlist': [LEADER, REED]
        },
        ROUTER3: {
            'name': 'ROUTER3',
            'mode': 'rdn',
            'allowlist': [LEADER, REED]
        },
        REED: {
            'name': 'REED',
            'mode': 'rdn',
            'allowlist': [ROUTER1, ROUTER2, ROUTER3]
        },
    }

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        for routerid in (ROUTER1, ROUTER2, ROUTER3):
            self.nodes[routerid].start()
            self.simulator.go(config.ROUTER_STARTUP_DELAY)
            self.assertEqual(self.nodes[routerid].get_state(), 'router')

        # Wait for the network to stabilize
        self.simulator.go(60)

        self.nodes[REED].start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual(self.nodes[REED].get_state(), 'router')
        self.simulator.go(LINK_ESTABLISH_DELAY_THRESHOLD + 3)

    def verify(self, pv: PacketVerifier):
        pkts = pv.pkts
        print(pv.vars)
        pv.summary.show()

        REED = pv.vars['REED']
        as_pkt = pkts.filter_wpan_src64(REED).filter_coap_request('/a/as', confirmable=True).must_next()
        parent_rloc16 = as_pkt.wpan.dst16
        as_ack_pkt = pkts.filter_wpan_src16(parent_rloc16).filter_coap_ack('/a/as').must_next()
        become_router_timestamp = as_ack_pkt.sniff_timestamp

        # REED has just received `/a/as` and become a Router
        # REED should send Multicast Link Request after becoming Router
        link_request_pkt = pkts.filter_wpan_src64(REED).filter_mle_cmd(MLE_LINK_REQUEST).must_next()
        link_request_pkt.must_verify('ipv6.dst == "ff02::2"')

        # REED should send Link Accept to the three Routers
        for router in ('ROUTER1', 'ROUTER2', 'ROUTER3'):
            with pkts.save_index():
                pkt = pkts.filter_wpan_src64(REED).filter_wpan_dst64(
                    pv.vars[router]).filter_mle_cmd(MLE_LINK_ACCEPT).must_next()
                link_establish_delay = pkt.sniff_timestamp - become_router_timestamp
                logging.info("Link to %s established in %.3f seconds", router, link_establish_delay)
                self.assertLess(link_establish_delay, LINK_ESTABLISH_DELAY_THRESHOLD)


if __name__ == '__main__':
    unittest.main()
