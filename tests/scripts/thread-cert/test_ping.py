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

import logging
import unittest

import pktverify
from pktverify import packet_verifier, packet_filter
from pktverify.consts import MA1, MA1g, MA2
import config
import thread_cert

# Test description:
# The purpose of this test is to verify the functionality of ping command.
#
# Topology:
#    ----------------(eth)------------------
#           |                  |     |
#          BR1 (Leader) ----- BR2   HOST
#           |
#           |
#          TD
#

BR_1 = 1
BR_2 = 2
TD = 3
HOST = 4


class TestPing(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR_1: {
            'name': 'BR_1',
            'is_otbr': True,
            'allowlist': [BR_2, TD],
            'version': '1.2',
            'router_selection_jitter': 2,
        },
        BR_2: {
            'name': 'BR_2',
            'allowlist': [BR_1],
            'is_otbr': True,
            'version': '1.2',
            'router_selection_jitter': 2,
        },
        TD: {
            'name': 'TD',
            'allowlist': [BR_1],
            'version': '1.2',
            'router_selection_jitter': 2,
        },
        HOST: {
            'name': 'Host',
            'is_host': True
        },
    }

    def test(self):
        br1 = self.nodes[BR_1]
        br2 = self.nodes[BR_2]
        td = self.nodes[TD]
        host = self.nodes[HOST]

        br1.start()
        self.simulator.go(5)
        self.assertEqual('leader', br1.get_state())
        self.assertTrue(br1.is_primary_backbone_router)

        td.start()
        self.simulator.go(5)
        self.assertEqual('router', td.get_state())

        br2.start()
        self.simulator.go(5)
        self.assertEqual('router', br2.get_state())
        self.assertFalse(br2.is_primary_backbone_router)

        host.start(start_radvd=False)
        self.simulator.go(10)

        # 1. TD pings Host.
        self.assertTrue(
            td.ping(host.get_ip6_address(config.ADDRESS_TYPE.ONLINK_ULA)[0]))
        self.simulator.go(5)

        # 2. TD pings Host using specified interface.
        self.assertTrue(
            td.ping(host.get_ip6_address(config.ADDRESS_TYPE.ONLINK_ULA)[0],
                    interface=td.get_ip6_address(config.ADDRESS_TYPE.OMR)[0]))
        self.simulator.go(5)

        # 3. TD pings BR_2 using RLOC address. TD sends 5 echo requests and
        # expects 5 responses.
        self.assertTrue(td.ping(br2.get_ip6_address(config.ADDRESS_TYPE.RLOC),
                                interface=td.get_ip6_address(
                                    config.ADDRESS_TYPE.RLOC), count=5,
                                num_responses=5))
        self.simulator.go(5)

        # 4. TD pings BR_2 using link local address. TD should receive no
        # response.
        self.assertFalse(td.ping(br2.get_ip6_address(config.ADDRESS_TYPE.RLOC),
                                 interface=td.get_ip6_address(
                                     config.ADDRESS_TYPE.LINK_LOCAL), count=5,
                                 num_responses=5))
        self.simulator.go(5)

        # 5. TD pings BR_2's link local address. TD should receive no
        # response.
        self.assertFalse(
            td.ping(br2.get_ip6_address(config.ADDRESS_TYPE.LINK_LOCAL),
                    interface=td.get_ip6_address(
                        config.ADDRESS_TYPE.RLOC), count=5,
                    num_responses=5))
        self.simulator.go(5)

        # 6. TD pings BR_1's link local address using TD's link local address.
        self.assertFalse(
            td.ping(br1.get_ip6_address(config.ADDRESS_TYPE.LINK_LOCAL),
                    interface=td.get_ip6_address(
                        config.ADDRESS_TYPE.LINK_LOCAL), count=5,
                    num_responses=5))
        self.simulator.go(5)

        # 7. TD subscribes multicast address MA1.
        td.add_ipmaddr(MA1)
        self.simulator.go(5)

        # 8. BR_2 pings MA1. BR_2 should receive a response from TD.
        self.assertTrue(br2.ping(MA1))
        self.simulator.go(5)

        # 9. BR_2 pings MA1 using a non-existent address. BR_2 should receive no
        # response.
        self.assertFalse(td.ping(MA1, interface='1::1'))
        self.simulator.go(5)

        # 10. Host pings TD's OMR address. Host should receive a response.
        self.assertTrue(
            host.ping(td.get_ip6_address(config.ADDRESS_TYPE.OMR)[0],
                      backbone=True))
        self.simulator.go(5)

        # 11. Host pings TD's RLOC address. Host should receive no
        # response.
        self.assertFalse(
            host.ping(td.get_ip6_address(config.ADDRESS_TYPE.RLOC),
                      backbone=True))
        self.simulator.go(5)

        self.collect_ipaddrs()
        self.collect_rloc16s()
        self.collect_rlocs()
        self.collect_leader_aloc(BR_1)
        self.collect_extra_vars()

    def verify(self, pv: pktverify.packet_verifier.PacketVerifier):
        pkts = pv.pkts
        vars = pv.vars
        pv.summary.show()

        # Ensure the topology is formed correctly
        pv.verify_attached('TD', 'BR_1')
        pv.verify_attached('BR_2')


if __name__ == '__main__':
    unittest.main()
