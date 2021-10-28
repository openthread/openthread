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

from pktverify.addrs import Ipv6Addr
import config
import thread_cert

# Test description:
# The purpose of this test case is to verify that BR acts the same way like TD
# replying on echo requests for subscribed multicast addresses.
#
# Topology:
#    ----------------(eth)-----------
#           |                    |
#          BR1 (Leader)         HOST
#        /  |
#       /   |
#     BR2   TD
#

BR_1 = 1
TD = 2
BR_2 = 3
HOST = 5

MA1 = Ipv6Addr('ff04::1234:777a:1')
MA2 = Ipv6Addr('ff04::1234:777a:2')


class MATN_02_MLRFirstUse(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR_1: {
            'name': 'BR_1',
            'is_otbr': True,
            'allowlist': [TD, BR_2],
            'version': '1.2',
        },
        TD: {
            'name': 'TD',
            'allowlist': [BR_1],
            'version': '1.2',
        },
        BR_2: {
            'name': 'BR_2',
            'allowlist': [BR_1],
            'is_otbr': True,
            'version': '1.2',
        },
        HOST: {
            'name': 'Host',
            'is_host': True
        },
    }

    def test(self):
        br1 = self.nodes[BR_1]
        td = self.nodes[TD]
        br2 = self.nodes[BR_2]
        host = self.nodes[HOST]

        br1.start()
        self.simulator.go(5)
        self.assertEqual('leader', br1.get_state())
        self.assertTrue(br1.is_primary_backbone_router)

        td.start()
        self.simulator.go(5)
        self.assertEqual('router', td.get_state())

        br2.start()
        br2.disable_ether()
        self.simulator.go(5)
        self.assertEqual('router', td.get_state())

        host.start(start_radvd=False)
        self.simulator.go(10)

        # TD registers for multicast address, MA1, at BR_1.
        td.add_ipmaddr(MA1)

        # BR_2 registers for multicast address, MA2, at BR_1.
        br2.add_ipmaddr(MA2)
        self.simulator.go(5)

        # Host sends a ping packet to the multicast address, MA1. TD should respond to the ping request.
        self.assertTrue(
            host.ping(MA1, backbone=True, ttl=10, interface=host.get_ip6_address(config.ADDRESS_TYPE.ONLINK_ULA)[0]))
        self.simulator.go(5)

        # Host sends a ping packet to the multicast address, MA2. BR_2 should respond to the ping request.
        self.assertTrue(
            host.ping(MA2, backbone=True, ttl=10, interface=host.get_ip6_address(config.ADDRESS_TYPE.ONLINK_ULA)[0]))
        self.simulator.go(5)

        self.collect_ipaddrs()
        self.collect_rloc16s()
        self.collect_rlocs()
        self.collect_leader_aloc(BR_1)
        self.collect_extra_vars()


if __name__ == '__main__':
    unittest.main()
