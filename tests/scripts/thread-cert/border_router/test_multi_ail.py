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

from node import OtbrNode

IPV4_CIDR_ADDR_CMD = f'ip addr show {config.BACKBONE_IFNAME} | grep -w inet | grep -Eo "[0-9.]+/[0-9]+"'


class ThreeBRs_TwoInfra(thread_cert.TestCase):
    """
    Test that two border routers on different infrastructures can ping each other via Thread interface.

    Topology:

    ----(backbone0.0)---- | -------(backbone0.1)------
             |                    |            |
            BR1   .............  BR2 ........ BR3

    """
    USE_MESSAGE_FACTORY = False
    BR1, BR2, BR3 = range(1, 4)

    TOPOLOGY = {
        BR1: {
            'name': 'BR_1',
            'backbone_network_id': 0,
            'allowlist': [BR2],
            'is_otbr': True,
            'version': '1.4',
        },
        BR2: {
            'name': 'BR_2',
            'backbone_network_id': 1,
            'allowlist': [BR1, BR3],
            'is_otbr': True,
            'version': '1.4',
        },
        BR3: {
            'name': 'BR_3',
            'backbone_network_id': 1,
            'allowlist': [BR2],
            'is_otbr': True,
            'version': '1.4',
        }
    }

    def test_multi_backbone_infra(self):
        """This test ensures that the multiple backbone infra works as expected."""
        br1: OtbrNode = self.nodes[self.BR1]
        br2: OtbrNode = self.nodes[self.BR2]
        br3: OtbrNode = self.nodes[self.BR3]

        # start nodes
        for br in [br1, br2, br3]:
            br.start()
        self.simulator.go(config.BORDER_ROUTER_STARTUP_DELAY)

        # check BR1 and BR2 are not on the same subnet, but BR2 and BR3 are on the same subnet
        br1_infra_ip_addr = br1.bash(IPV4_CIDR_ADDR_CMD)
        br2_infra_ip_addr = br2.bash(IPV4_CIDR_ADDR_CMD)
        br3_infra_ip_addr = br3.bash(IPV4_CIDR_ADDR_CMD)
        assert len(br1_infra_ip_addr) == 1
        assert len(br2_infra_ip_addr) == 1
        assert len(br3_infra_ip_addr) == 1

        self.assertNotEqual(ipaddress.ip_network(br1_infra_ip_addr[0].strip(), strict=False),
                            ipaddress.ip_network(br2_infra_ip_addr[0].strip(), strict=False))
        self.assertEqual(ipaddress.ip_network(br2_infra_ip_addr[0].strip(), strict=False),
                         ipaddress.ip_network(br3_infra_ip_addr[0].strip(), strict=False))

        # ping each other using Thread MLEID
        br1_mleid = br1.get_ip6_address(config.ADDRESS_TYPE.ML_EID)
        br2_mleid = br2.get_ip6_address(config.ADDRESS_TYPE.ML_EID)
        br3_mleid = br3.get_ip6_address(config.ADDRESS_TYPE.ML_EID)

        self.assertTrue(br1.ping(br2_mleid))
        self.assertTrue(br1.ping(br3_mleid))
        self.assertTrue(br2.ping(br1_mleid))
        self.assertTrue(br2.ping(br3_mleid))
        self.assertTrue(br3.ping(br1_mleid))
        self.assertTrue(br3.ping(br2_mleid))

        # ping each other using Infra ULA
        br1_onlink_ula = br1.get_ip6_address(config.ADDRESS_TYPE.ONLINK_ULA)[0]
        br2_onlink_ula = br2.get_ip6_address(config.ADDRESS_TYPE.ONLINK_ULA)[0]
        br3_onlink_ula = br3.get_ip6_address(config.ADDRESS_TYPE.ONLINK_ULA)[0]

        self.assertFalse(br1.ping(br2_onlink_ula, backbone=True))
        self.assertFalse(br1.ping(br3_onlink_ula, backbone=True))
        self.assertFalse(br2.ping(br1_onlink_ula, backbone=True))
        self.assertFalse(br3.ping(br1_onlink_ula, backbone=True))
        self.assertTrue(br2.ping(br3_onlink_ula, backbone=True))
        self.assertTrue(br3.ping(br2_onlink_ula, backbone=True))


if __name__ == '__main__':
    unittest.main(verbosity=2)
