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

# Test description:
#   This test verifies UDP servers can be accessible using RLOC/ALOC/MLEID/LINK-LOCAL/OMR when PLAT_UDP is enabled.
#   This test uses SRP server for convince.
#
# Topology:
#    -----------(eth)------
#           |
#          BR1 (Leader)
#           |
#        ROUTER1
#

import config

BR1 = 1
ROUTER1 = 2


class TestPlatUdpAccessibility(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR1: {
            'name': 'BR_1',
            'is_otbr': True,
            'version': '1.2',
            'router_selection_jitter': 1,
        },
        ROUTER1: {
            'name': 'Router_1',
            'version': '1.2',
            'router_selection_jitter': 1,
        },
    }

    def test(self):
        self.nodes[BR1].start()
        self.simulator.go(5)
        self.assertEqual('leader', self.nodes[BR1].get_state())
        self.nodes[BR1].srp_server_set_enabled(True)

        self.nodes[ROUTER1].start()
        self.simulator.go(5)
        self.assertEqual('router', self.nodes[ROUTER1].get_state())

        # Router1 can ping to/from the Host on infra link.
        self.assertTrue(self.nodes[ROUTER1].ping(self.nodes[BR1].get_rloc()))

        server_port = self.nodes[BR1].get_srp_server_port()

        self._test_srp_server(self.nodes[BR1].get_mleid(), server_port)
        self._test_srp_server(self.nodes[BR1].get_linklocal(), server_port)
        self._test_srp_server(self.nodes[BR1].get_ip6_address(config.ADDRESS_TYPE.OMR)[0], server_port)
        self._test_srp_server(self.nodes[BR1].get_rloc(), server_port)
        for server_aloc in self.nodes[BR1].get_ip6_address(config.ADDRESS_TYPE.ALOC):
            self._test_srp_server(server_aloc, server_port)

    def _test_srp_server(self, server_addr, server_port):
        print(f'Testing SRP server: {server_addr}:{server_port}')

        # check if the SRP client can register to the SRP server
        self.nodes[ROUTER1].srp_client_start(server_addr, server_port)
        self.nodes[ROUTER1].srp_client_set_host_name('host1')
        self.nodes[ROUTER1].srp_client_set_host_address(self.nodes[ROUTER1].get_rloc())
        self.nodes[ROUTER1].srp_client_add_service('ins1', '_ipp._tcp', 11111)
        self.simulator.go(3)
        self.assertEqual(self.nodes[ROUTER1].srp_client_get_host_state(), 'Registered')

        # check if the SRP client can remove from the SRP server
        self.nodes[ROUTER1].srp_client_remove_host('host1')
        self.nodes[ROUTER1].srp_client_remove_service('ins1', '_ipp._tcp')
        self.simulator.go(3)
        self.assertEqual(self.nodes[ROUTER1].srp_client_get_host_state(), 'Removed')

        # stop the SRP client for the next round
        self.nodes[ROUTER1].srp_client_stop()
        self.simulator.go(3)


if __name__ == '__main__':
    unittest.main()
