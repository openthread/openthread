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

import ipaddress
import unittest

import command
import config
import thread_cert

# Test description:
#
#   This test verifies SRP client behavior when there are many services
#   causing SRP Update message going over the IPv6 MTU size. In such
#   a case the SRP client attempts to register a single service at
#   a time.
#
# Topology:
#
#     LEADER (SRP server)
#       |
#       |
#     ROUTER (SRP client)
#

SERVER = 1
CLIENT = 2


class SrpManyServicesMtuCheck(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False
    SUPPORT_NCP = False

    TOPOLOGY = {
        SERVER: {
            'name': 'SRP_SERVER',
            'mode': 'rdn',
        },
        CLIENT: {
            'name': 'SRP_CLIENT',
            'mode': 'rdn',
        },
    }

    def test(self):
        server = self.nodes[SERVER]
        client = self.nodes[CLIENT]

        # Start the server & client devices.

        server.start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual(server.get_state(), 'leader')

        client.start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual(client.get_state(), 'router')

        server.srp_server_set_enabled(True)
        client.srp_client_enable_auto_start_mode()
        self.simulator.go(15)

        # Register 8 services with long name, 6 sub-types and long txt record.
        # The 8 services won't be fit in a single MTU (1280 bytes) UDP message
        # SRP Client would then register services one by one.

        num_services = 8

        client.srp_client_set_host_name('hosthosthosthosthosthosthosthosthosthosthosthosthosthosthosthos')
        client.srp_client_set_host_address('2001::1')

        txt_info = ["xyz=987654321", "uvw=abcdefghijklmnopqrstu", "qwp=564321abcdefghij"]

        for num in range(num_services):
            name = chr(ord('a') + num) * 63
            client.srp_client_add_service(
                name,
                '_longlongsrvname._udp,_subtype1,_subtype2,_subtype3,_subtype4,_subtype5,_subtype6',
                1977,
                txt_entries=txt_info)

        self.simulator.go(10)

        # Make sure all services are successfully registered
        # (from both client and server perspectives).

        client_services = client.srp_client_get_services()
        self.assertEqual(len(client_services), num_services)

        for service in client_services:
            self.assertEqual(service['state'], 'Registered')

        server_services = server.srp_server_get_services()
        self.assertEqual(len(server_services), num_services)

        # Remove all 8 services.

        for num in range(num_services):
            name = chr(ord('a') + num) * 63
            client.srp_client_remove_service(name, '_longlongsrvname._udp')

        self.simulator.go(10)

        client_services = client.srp_client_get_services()
        self.assertEqual(len(client_services), 0)

        server_services = server.srp_server_get_services()
        self.assertEqual(len(server_services), num_services)

        for service in server_services:
            self.assertEqual(service['deleted'], 'true')


if __name__ == '__main__':
    unittest.main()
