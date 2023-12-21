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
#   This test verifies the SRP server and client properly handle SRP host
#   and service instance TTLs.
#
# Topology:
#     LEADER (SRP server)
#       |
#       |
#     ROUTER (SRP client)
#

SERVER = 1
CLIENT = 2
KEY_LEASE = 240  # Seconds


class SrpTtl(thread_cert.TestCase):
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

        #
        # Start the server and client devices.
        #

        server.srp_server_set_enabled(True)
        server.srp_server_set_lease_range(120, 240, KEY_LEASE, KEY_LEASE)
        server.start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual(server.get_state(), 'leader')
        self.simulator.go(5)

        client.srp_server_set_enabled(False)
        client.start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual(client.get_state(), 'router')

        self.assertEqual(client.srp_client_get_auto_start_mode(), 'Enabled')

        client.srp_client_set_host_name('my-host')
        client.srp_client_set_host_address('2001::1')
        client.srp_client_add_service('my-service', '_ipps._tcp', 12345)
        self.simulator.go(2)

        #
        # CLIENT_TTL < TTL_MIN < LEASE_MAX ==> TTL_MIN
        #

        client.srp_client_set_ttl(100)
        server.srp_server_set_ttl_range(120, 240)
        server.srp_server_set_lease_range(120, 240, KEY_LEASE, KEY_LEASE)
        self.simulator.go(KEY_LEASE)
        self.check_ttl(120)

        #
        # TTL_MIN < CLIENT_TTL < TTL_MAX < LEASE_MAX ==> CLIENT_TTL
        #

        client.srp_client_set_ttl(100)
        server.srp_server_set_ttl_range(60, 120)
        server.srp_server_set_lease_range(120, 240, KEY_LEASE, KEY_LEASE)
        self.simulator.go(KEY_LEASE)
        self.check_ttl(100)

        #
        # TTL_MAX < LEASE_MAX < CLIENT_TTL ==> TTL_MAX
        #

        client.srp_client_set_ttl(240)
        server.srp_server_set_ttl_range(60, 120)
        server.srp_server_set_lease_range(120, 240, KEY_LEASE, KEY_LEASE)
        self.simulator.go(KEY_LEASE)
        self.check_ttl(120)

        #
        # LEASE_MAX < TTL_MAX < CLIENT_TTL ==> LEASE_MAX
        #

        client.srp_client_set_ttl(240)
        server.srp_server_set_ttl_range(60, 120)
        server.srp_server_set_lease_range(30, 60, KEY_LEASE, KEY_LEASE)
        self.simulator.go(KEY_LEASE)
        self.check_ttl(60)

    def check_ttl(self, ttl):
        """Check that we have properly registered host and service instance.
        """

        server = self.nodes[SERVER]

        server_services = server.srp_server_get_services()
        print(server_services)
        self.assertEqual(len(server_services), 1)
        server_service = server_services[0]

        # Verify that the server accepted the SRP registration and stored
        # the same service resources.
        self.assertEqual(int(server_service['ttl']), ttl)


if __name__ == '__main__':
    unittest.main()
