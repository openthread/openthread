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
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS'
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
import config
from pktverify.consts import ICMPV6_TYPE_DESTINATION_UNREACHABLE, MA1, MA2, MA5, MA6
from pktverify.packet_verifier import PacketVerifier

PBBR = 1
ROUTER = 2


# Test description:
#   This test verifies that the multicast packet originated from BBR is only
#   forwarded to thread network when the destination group address is in the
#   multicast listener table.
#
# Topology:
#
#    ------(eth)----------
#           |
#         PBBR---ROUTER
#
class TestMlrMulticastRoutingSelfOriginatedPacket(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        PBBR: {
            'name': 'PBBR',
            'allowlist': [ROUTER],
            'is_otbr': True,
            'version': '1.2',
        },
        ROUTER: {
            'name': 'ROUTER',
            'allowlist': [PBBR],
            'version': '1.2',
        },
    }

    def test(self):
        self.nodes[PBBR].start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual('leader', self.nodes[PBBR].get_state())
        self.nodes[PBBR].enable_backbone_router()
        self.simulator.go(3)
        self.assertTrue(self.nodes[PBBR].is_primary_backbone_router)

        self.nodes[ROUTER].start()
        self.simulator.go(5)
        self.assertEqual('router', self.nodes[ROUTER].get_state())

        self.simulator.go(5)

        self.collect_ipaddrs()

        # Verify PBBR can send packets from host to MLR registered multicast address on Thread network
        self.nodes[ROUTER].add_ipmaddr(MA1)
        self.simulator.go(3)
        self.assertTrue(self.nodes[PBBR].ping(MA1, backbone=True, ttl=10, interface=config.THREAD_IFNAME))

        # Verify PBBR can't send packets from host to not registered multicast address on Thread network
        self.assertFalse(self.nodes[PBBR].ping(MA2, backbone=True, ttl=10, interface=config.THREAD_IFNAME))

        # Verify PBBR can't send packets from host to multicast address with scope <= realm-local to Thread network
        self.assertFalse(self.nodes[PBBR].ping(MA5, backbone=True, ttl=10, interface=config.THREAD_IFNAME))
        self.assertFalse(self.nodes[PBBR].ping(MA6, backbone=True, ttl=10, interface=config.THREAD_IFNAME))

    def verify(self, pv: PacketVerifier):
        pkts = pv.pkts
        pv.add_common_vars()
        pv.summary.show()

        with pkts.save_index():
            pv.verify_attached('ROUTER')

        PBBR = pv.vars['PBBR']

        # PBBR should send ping request to MA1 to Thread network
        pkts.filter_wpan_src64(PBBR).filter_AMPLFMA().filter(
            lambda p: p.ipv6inner.dst == MA1).filter_ping_request().must_next()

        # PBBR should not send ping request to MA2 to Thread network
        pkts.filter_wpan_src64(PBBR).filter_AMPLFMA().filter(
            lambda p: p.ipv6inner.dst == MA2).filter_ping_request().must_not_next()

        # PBBR should not send ping request to MA5 to Thread network
        pkts.filter_wpan_src64(PBBR).filter_AMPLFMA().filter(
            lambda p: p.ipv6inner.dst == MA5).filter_ping_request().must_not_next()

        # PBBR should not send ping request to MA6 to Thread network
        pkts.filter_wpan_src64(PBBR).filter_AMPLFMA().filter(
            lambda p: p.ipv6inner.dst == MA6).filter_ping_request().must_not_next()


if __name__ == '__main__':
    unittest.main()
