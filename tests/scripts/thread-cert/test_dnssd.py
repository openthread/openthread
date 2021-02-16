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
import ipaddress
import unittest

import thread_cert

SERVER = 1
CLIENT1 = 2
CLIENT2 = 3

DOMAIN = 'default.service.arpa.'
SERVICE = '_ipps._tcp'

#
# Topology:
# LEADER -- CLIENT1
#   |
# CLIENT2
#


class TestDnssd(thread_cert.TestCase):
    SUPPORT_NCP = False
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        SERVER: {
            'mode': 'rdn',
            'panid': 0xface,
        },
        CLIENT1: {
            'mode': 'rdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
        },
        CLIENT2: {
            'mode': 'rdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
        },
    }

    def test(self):
        self.nodes[SERVER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[SERVER].get_state(), 'leader')
        self.nodes[SERVER].srp_server_set_enabled(True)

        self.nodes[CLIENT1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[CLIENT1].get_state(), 'router')

        self.nodes[CLIENT2].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[CLIENT1].get_state(), 'router')

        client1_addrs = [self.nodes[CLIENT1].get_mleid(), self.nodes[CLIENT1].get_rloc()]
        self._config_srp_client_services(CLIENT1, 'ins1', 'host1', 11111, 1, 1, client1_addrs)

        client2_addrs = [self.nodes[CLIENT2].get_mleid(), self.nodes[CLIENT2].get_rloc()]
        self._config_srp_client_services(CLIENT2, 'ins2', 'host2', 22222, 2, 2, client2_addrs)

        # Test AAAA query using DNS client
        response = self.nodes[CLIENT1].dns_resolve(f"host1.{DOMAIN}", self.nodes[SERVER].get_mleid(), 53)
        self.assertIn(ipaddress.IPv6Address(response[0][0]), map(ipaddress.IPv6Address, client1_addrs))

        response = self.nodes[CLIENT1].dns_resolve(f"host2.{DOMAIN}", self.nodes[SERVER].get_mleid(), 53)
        self.assertIn(ipaddress.IPv6Address(response[0][0]), map(ipaddress.IPv6Address, client2_addrs))

        # TODO: test other query types using DNS-SD client

    def _config_srp_client_services(self, client, instancename, hostname, port, priority, weight, addrs):
        self.nodes[client].netdata_show()
        srp_server_port = self.nodes[client].get_srp_server_port()

        self.nodes[client].srp_client_start(self.nodes[SERVER].get_mleid(), srp_server_port)
        self.nodes[client].srp_client_set_host_name(hostname)
        self.nodes[client].srp_client_set_host_address(*addrs)
        self.nodes[client].srp_client_add_service(instancename, SERVICE, port, priority, weight)

        self.simulator.go(5)
        self.assertEqual(self.nodes[client].srp_client_get_host_state(), 'Registered')


if __name__ == '__main__':
    unittest.main()
