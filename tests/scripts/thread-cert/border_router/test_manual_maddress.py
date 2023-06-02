#!/usr/bin/env python3
#
#  Copyright (c) 2021, The OpenThread Authors.
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

import pktverify
from pktverify.consts import MA1

# Test description:
# The purpose of this test case is to verify that a Border Router device is able to properly
# forward traffic in response to multicast destination when it comes from another BBR.
#
# Topology:
#    ----------------(eth)------------------
#           |                  |     |
#          BR1 (Leader) ------ TD   HOST
#

BR_1 = 1
TD = 2
HOST = 3


class ManualMulticastAddressConfig(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR_1: {
            'name': 'BR_1',
            'is_otbr': True,
            'version': '1.2',
        },
        TD: {
            'name': 'TD',
            'is_otbr': True,
            'router_eligible': False,
            'version': '1.2',
        },
        HOST: {
            'name': 'Host',
            'is_host': True,
        },
    }

    def test(self):
        br1 = self.nodes[BR_1]
        td = self.nodes[TD]
        host = self.nodes[HOST]

        br1.start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual('leader', br1.get_state())
        self.assertTrue(br1.is_primary_backbone_router)

        td.start()
        self.simulator.go(5)
        self.assertEqual('child', td.get_state())

        # TD registers for multicast address, MA1, at BR_1.
        td.add_ipmaddr_tun(MA1)
        self.simulator.go(5)

        # Host sends a ping packet to the multicast address, MA1. TD should
        # respond to the ping request.
        host.ping(MA1, backbone=True, ttl=10, interface=host.get_ip6_address(config.ADDRESS_TYPE.ONLINK_ULA)[0])
        self.simulator.go(5)

    def verify(self, pv: pktverify.packet_verifier.PacketVerifier):
        pkts = pv.pkts
        vars = pv.vars
        pv.summary.show()

        # 1. Host sends a ping packet to the multicast address, MA1.
        _pkt = pkts.filter_eth_src(vars['Host_ETH']) \
            .filter_ipv6_dst(MA1) \
            .filter_ping_request() \
            .must_next()

        # 1. TD receives the multicast ping packet and sends a ping response
        # packet back to Host.
        # TD receives the MPL packet containing an encapsulated ping packet to
        # MA1, sent by Host, and unicasts a ping response packet back to Host.
        pkts.filter_ipv6_dst(_pkt.ipv6.src) \
            .filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier) \
            .must_next()


if __name__ == '__main__':
    unittest.main()
