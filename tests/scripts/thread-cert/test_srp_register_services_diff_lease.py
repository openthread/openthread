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
#   This test verifies the SRP client and server behavior when services
#   with different lease (and/or key lease) intervals are registered.
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


class SrpRegisterServicesDiffLease(thread_cert.TestCase):
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

        #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        # Start the server and client.

        server.start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual(server.get_state(), 'leader')

        client.start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual(client.get_state(), 'router')

        server.srp_server_set_enabled(True)
        client.srp_client_enable_auto_start_mode()

        self.simulator.go(15)

        client.srp_client_set_host_name('host')
        client.srp_client_enable_auto_host_address()

        #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        # Add a service with specific lease and key lease and verify that
        # it is successfully registered and seen with same lease/key-lease
        # on server.

        client.srp_client_add_service('ins1', '_test._udp', 1111, lease=60, key_lease=800)

        self.simulator.go(5)

        self.check_services_on_client(client, 1)
        services = server.srp_server_get_services()
        self.assertEqual(len(services), 1)
        service = services[0]
        self.assertEqual(service['fullname'], 'ins1._test._udp.default.service.arpa.')
        self.assertEqual(service['deleted'], 'false')
        self.assertEqual(int(service['ttl']), 60)
        self.assertEqual(int(service['lease']), 60)
        self.assertEqual(int(service['key-lease']), 800)

        #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        # Register two more services with different lease intervals.

        client.srp_client_add_service('ins2', '_test._udp', 2222, lease=30, key_lease=200)
        client.srp_client_add_service('ins3', '_test._udp', 3333, lease=100, key_lease=1000)

        self.simulator.go(10)

        self.check_services_on_client(client, 3)
        server_services = server.srp_server_get_services()
        self.assertEqual(len(server_services), 3)
        for service in server_services:
            if service['fullname'] == 'ins1._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 60)
                self.assertEqual(int(service['lease']), 60)
                self.assertEqual(int(service['key-lease']), 800)
            elif service['fullname'] == 'ins2._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 30)
                self.assertEqual(int(service['lease']), 30)
                self.assertEqual(int(service['key-lease']), 200)
            elif service['fullname'] == 'ins3._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 100)
                self.assertEqual(int(service['lease']), 100)
                self.assertEqual(int(service['key-lease']), 1000)
            else:
                self.assertTrue(False)

        #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        # Wait for longest lease time to validate that all services renew their
        # lease successfully.

        self.simulator.go(105)

        self.check_services_on_client(client, 3)
        server_services = server.srp_server_get_services()
        self.assertEqual(len(server_services), 3)
        for service in server_services:
            self.assertEqual(service['deleted'], 'false')

        #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        # Remove two services.

        client.srp_client_remove_service('ins2', '_test._udp')
        client.srp_client_remove_service('ins3', '_test._udp')

        self.simulator.go(10)

        self.check_services_on_client(client, 1)
        server_services = server.srp_server_get_services()
        self.assertEqual(len(server_services), 3)
        for service in server_services:
            if service['fullname'] == 'ins1._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 60)
                self.assertEqual(int(service['lease']), 60)
                self.assertEqual(int(service['key-lease']), 800)
            elif service['fullname'] == 'ins2._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'true')
            elif service['fullname'] == 'ins3._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'true')
            else:
                self.assertTrue(False)

        #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        # Wait for longer than key-lease of `ins2` service and check that it is
        # removed on server.

        self.simulator.go(201)

        self.check_services_on_client(client, 1)
        server_services = server.srp_server_get_services()
        self.assertEqual(len(server_services), 2)
        for service in server_services:
            if service['fullname'] == 'ins1._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 60)
                self.assertEqual(int(service['lease']), 60)
                self.assertEqual(int(service['key-lease']), 800)
            elif service['fullname'] == 'ins3._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'true')
            else:
                self.assertTrue(False)

        #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        # Add both services again now with same lease intervals.

        client.srp_client_add_service('ins2', '_test._udp', 2222, lease=30, key_lease=100)
        client.srp_client_add_service('ins3', '_test._udp', 3333, lease=30, key_lease=100)

        self.simulator.go(10)

        self.check_services_on_client(client, 3)
        server_services = server.srp_server_get_services()
        self.assertEqual(len(server_services), 3)
        for service in server_services:
            if service['fullname'] == 'ins1._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 60)
                self.assertEqual(int(service['lease']), 60)
                self.assertEqual(int(service['key-lease']), 800)
            elif service['fullname'] == 'ins2._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 30)
                self.assertEqual(int(service['lease']), 30)
                self.assertEqual(int(service['key-lease']), 100)
            elif service['fullname'] == 'ins3._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 30)
                self.assertEqual(int(service['lease']), 30)
                self.assertEqual(int(service['key-lease']), 100)
            else:
                self.assertTrue(False)

        #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        # Remove `ins1` while adding a new service with same key-lease as
        # `ins1` but different lease interval.

        client.srp_client_remove_service('ins1', '_test._udp')
        client.srp_client_add_service('ins4', '_test._udp', 4444, lease=90, key_lease=800)

        self.simulator.go(5)

        self.check_services_on_client(client, 3)
        server_services = server.srp_server_get_services()
        self.assertEqual(len(server_services), 4)
        for service in server_services:
            if service['fullname'] == 'ins1._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'true')
            elif service['fullname'] == 'ins2._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 30)
                self.assertEqual(int(service['lease']), 30)
                self.assertEqual(int(service['key-lease']), 100)
            elif service['fullname'] == 'ins3._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 30)
                self.assertEqual(int(service['lease']), 30)
                self.assertEqual(int(service['key-lease']), 100)
            elif service['fullname'] == 'ins4._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 90)
                self.assertEqual(int(service['lease']), 90)
                self.assertEqual(int(service['key-lease']), 800)
            else:
                self.assertTrue(False)

        #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        # Remove two services `ins2` and `ins3` (they now have same key lease).

        client.srp_client_remove_service('ins2', '_test._udp')
        client.srp_client_remove_service('ins3', '_test._udp')

        self.simulator.go(10)

        self.check_services_on_client(client, 1)
        server_services = server.srp_server_get_services()
        self.assertEqual(len(server_services), 4)
        for service in server_services:
            if service['fullname'] == 'ins1._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'true')
            elif service['fullname'] == 'ins2._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'true')
            elif service['fullname'] == 'ins3._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'true')
            elif service['fullname'] == 'ins4._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 90)
                self.assertEqual(int(service['lease']), 90)
                self.assertEqual(int(service['key-lease']), 800)
            else:
                self.assertTrue(False)

        #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        # Add `ins1` with key-lease smaller than lease and check that
        # client handles this properly (uses the lease value for
        # key-lease).

        client.srp_client_add_service('ins1', '_test._udp', 1111, lease=100, key_lease=90)

        self.simulator.go(10)

        self.check_services_on_client(client, 2)
        server_services = server.srp_server_get_services()
        self.assertEqual(len(server_services), 4)
        for service in server_services:
            if service['fullname'] == 'ins1._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 100)
                self.assertEqual(int(service['lease']), 100)
                self.assertEqual(int(service['key-lease']), 100)
            elif service['fullname'] == 'ins2._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'true')
            elif service['fullname'] == 'ins3._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'true')
            elif service['fullname'] == 'ins4._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 90)
                self.assertEqual(int(service['lease']), 90)
                self.assertEqual(int(service['key-lease']), 800)
            else:
                self.assertTrue(False)

        #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        # Change default lease and key-lease intervals on client.

        client.srp_client_set_lease_interval(40)
        self.assertEqual(client.srp_client_get_lease_interval(), 40)

        client.srp_client_set_key_lease_interval(330)
        self.assertEqual(client.srp_client_get_key_lease_interval(), 330)

        #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        # Add `ins2` and `ins3`. `ins2` specifies the key-lease explicitly but
        # leaves lease as default. `ins3` does the opposite.

        client.srp_client_add_service('ins2', '_test._udp', 2222, key_lease=330)
        client.srp_client_add_service('ins3', '_test._udp', 3333, lease=40)

        self.simulator.go(10)

        self.check_services_on_client(client, 4)
        server_services = server.srp_server_get_services()
        self.assertEqual(len(server_services), 4)
        for service in server_services:
            if service['fullname'] == 'ins1._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 100)
                self.assertEqual(int(service['lease']), 100)
                self.assertEqual(int(service['key-lease']), 100)
            elif service['fullname'] == 'ins2._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 40)
                self.assertEqual(int(service['lease']), 40)
                self.assertEqual(int(service['key-lease']), 330)
            elif service['fullname'] == 'ins3._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 40)
                self.assertEqual(int(service['lease']), 40)
                self.assertEqual(int(service['key-lease']), 330)
            elif service['fullname'] == 'ins4._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 90)
                self.assertEqual(int(service['lease']), 90)
                self.assertEqual(int(service['key-lease']), 800)
            else:
                self.assertTrue(False)

        #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        # Change the default lease to 50 and wait for long enough for `ins2`
        # and `ins3` to do lease refresh. Validate that `ins2` now requests
        # new default lease of 50 while `ins3` should stay as before.

        client.srp_client_set_lease_interval(50)
        self.assertEqual(client.srp_client_get_lease_interval(), 50)

        self.simulator.go(45)

        self.check_services_on_client(client, 4)
        server_services = server.srp_server_get_services()
        self.assertEqual(len(server_services), 4)
        for service in server_services:
            if service['fullname'] == 'ins1._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 100)
                self.assertEqual(int(service['lease']), 100)
                self.assertEqual(int(service['key-lease']), 100)
            elif service['fullname'] == 'ins2._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 50)
                self.assertEqual(int(service['lease']), 50)
                self.assertEqual(int(service['key-lease']), 330)
            elif service['fullname'] == 'ins3._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 40)
                self.assertEqual(int(service['lease']), 40)
                self.assertEqual(int(service['key-lease']), 330)
            elif service['fullname'] == 'ins4._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 90)
                self.assertEqual(int(service['lease']), 90)
                self.assertEqual(int(service['key-lease']), 800)
            else:
                self.assertTrue(False)

        #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        # Change the default key lease to 30. `ins3` should adopt this but
        # since it is shorter than its explicitly specified lease the
        # client should use same value for both lease and key-lease.

        client.srp_client_set_key_lease_interval(35)
        self.assertEqual(client.srp_client_get_key_lease_interval(), 35)

        self.simulator.go(45)

        self.check_services_on_client(client, 4)
        server_services = server.srp_server_get_services()
        self.assertEqual(len(server_services), 4)
        for service in server_services:
            if service['fullname'] == 'ins1._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 100)
                self.assertEqual(int(service['lease']), 100)
                self.assertEqual(int(service['key-lease']), 100)
            elif service['fullname'] == 'ins2._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 50)
                self.assertEqual(int(service['lease']), 50)
                self.assertEqual(int(service['key-lease']), 330)
            elif service['fullname'] == 'ins3._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 40)
                self.assertEqual(int(service['lease']), 40)
                self.assertEqual(int(service['key-lease']), 40)
            elif service['fullname'] == 'ins4._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 90)
                self.assertEqual(int(service['lease']), 90)
                self.assertEqual(int(service['key-lease']), 800)
            else:
                self.assertTrue(False)

        #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        # Change the requested TTL. Wait for long enough for all
        # services to refresh and check that the new TTL is correctly
        # requested by the client (when it is not larger than
        # service lease).

        client.srp_client_set_ttl(65)
        self.assertEqual(client.srp_client_get_ttl(), 65)

        self.simulator.go(110)

        self.check_services_on_client(client, 4)
        server_services = server.srp_server_get_services()
        self.assertEqual(len(server_services), 4)
        for service in server_services:
            if service['fullname'] == 'ins1._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 65)
                self.assertEqual(int(service['lease']), 100)
                self.assertEqual(int(service['key-lease']), 100)
            elif service['fullname'] == 'ins2._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 50)
                self.assertEqual(int(service['lease']), 50)
                self.assertEqual(int(service['key-lease']), 330)
            elif service['fullname'] == 'ins3._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 40)
                self.assertEqual(int(service['lease']), 40)
                self.assertEqual(int(service['key-lease']), 40)
            elif service['fullname'] == 'ins4._test._udp.default.service.arpa.':
                self.assertEqual(service['deleted'], 'false')
                self.assertEqual(int(service['ttl']), 65)
                self.assertEqual(int(service['lease']), 90)
                self.assertEqual(int(service['key-lease']), 800)
            else:
                self.assertTrue(False)

    def check_services_on_client(self, client, expected_num_services):
        services = client.srp_client_get_services()
        self.assertEqual(len(services), expected_num_services)
        for service in client.srp_client_get_services():
            self.assertIn(service['state'], ['Registered', 'ToRefresh', 'Refreshing'])


if __name__ == '__main__':
    unittest.main()
