#!/usr/bin/env python3
#
#  Copyright (c) 2024, The OpenThread Authors.
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
import ipaddress

import config
import thread_cert

IPV4_CIDR_ADDR_CMD = f'ip addr show {config.BACKBONE_IFNAME} | grep -w inet | grep -Eo "[0-9.]+/[0-9]+"'


class TwoBorderRoutersOnTwoInfrastructures(thread_cert.TestCase):
    """
    Test that two border routers on different infrastructures can ping each other via Thread interface.

    Topology:

    -------(backbone0)-------- | ---------(backbone1)-------
              |                             |
           BR1 (Leader)  ..............  BR2 (Router)

    """
    USE_MESSAGE_FACTORY = False

    BR1 = 1
    BR2 = 2

    TOPOLOGY = {
        BR1: {
            'name': 'BR_1',
            'backbone_network': 'backbone0',
            'allowlist': [BR2],
            'is_otbr': True,
            'version': '1.3',
        },
        BR2: {
            'name': 'BR_2',
            'backbone_network': 'backbone1',
            'allowlist': [BR1],
            'is_otbr': True,
            'version': '1.3',
        }
    }

    def test(self):
        br1 = self.nodes[self.BR1]
        br2 = self.nodes[self.BR2]

        # start nodes
        br1.start()
        self.simulator.go(2)
        br2.start()
        self.simulator.go(config.BORDER_ROUTER_STARTUP_DELAY)

        # check roles
        self.assertEqual('leader', br1.get_state())
        self.assertEqual('router', br2.get_state())

        # check two BRs AIL are in different subnets
        br1_infra_ip_addr = br1.bash(IPV4_CIDR_ADDR_CMD)
        br2_infra_ip_addr = br2.bash(IPV4_CIDR_ADDR_CMD)

        self.assertEqual(len(br1_infra_ip_addr), 1)
        self.assertEqual(len(br2_infra_ip_addr), 1)
        self.assertNotEqual(ipaddress.ip_network(br1_infra_ip_addr[0].strip(), strict=False),
                            ipaddress.ip_network(br2_infra_ip_addr[0].strip(), strict=False))

        # ping each other
        self.assertTrue(br1.ping(br2.get_ip6_address(config.ADDRESS_TYPE.ML_EID)))
        self.assertTrue(br2.ping(br1.get_ip6_address(config.ADDRESS_TYPE.ML_EID)))


if __name__ == '__main__':
    unittest.main(verbosity=2)
