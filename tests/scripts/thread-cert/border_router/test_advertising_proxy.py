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
import ipaddress
import logging
import unittest

import config
import thread_cert

# Test description:
#   This test verifies the basic functionality of advertising proxy.
#
# Topology:
#    ----------------(eth)--------------------
#           |                   |
#          BR (Leader)    HOST (mDNS Browser)
#           |
#        ROUTER
#

BR = 1
ROUTER = 2
HOST = 3


class SingleHostAndService(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR: {
            'name': 'BR_1',
            'allowlist': [ROUTER],
            'is_otbr': True,
            'version': '1.2',
            'router_selection_jitter': 2,
        },
        ROUTER: {
            'name': 'Router_1',
            'allowlist': [BR],
            'version': '1.2',
            'router_selection_jitter': 2,
        },
        HOST: {
            'name': 'Host',
            'is_host': True
        },
    }

    def test(self):
        host = self.nodes[HOST]
        server = self.nodes[BR]
        client = self.nodes[ROUTER]

        host.start(start_radvd=False)
        self.simulator.go(5)

        server.srp_server_set_enabled(True)
        server.start()
        self.simulator.go(5)
        self.assertEqual('leader', server.get_state())

        client.start()
        self.simulator.go(5)
        self.assertEqual('router', client.get_state())

        #
        # 1. Register a single service.
        #

        client.srp_client_set_host_name('my-host')
        client.srp_client_set_host_address('2001::1')
        client.srp_client_add_service('my-service', '_ipps._tcp', 12345)
        client.srp_client_start(server.get_addrs()[0], client.get_srp_server_port())
        self.simulator.go(2)

        self.check_host_and_service(server, client)

        #
        # 2. Discover the service by the HOST on the ethernet. This makes sure
        #    the Advertising Proxy multicasts the same service on ethernet.
        #

        self.host_check_mdns_service(host)

        #
        # 3. Check if the Advertising Proxy removes the service from ethernet
        #    when the SRP client removes it.
        #

        client.srp_client_remove_host()
        self.simulator.go(2)

        self.assertIsNone(host.discover_mdns_service('my-service', '_ipps._tcp', 'my-host'))

        #
        # 4. Check if we can discover the mDNS service again when re-registering the
        #    service from the SRP client.
        #

        client.srp_client_set_host_name('my-host')
        client.srp_client_set_host_address('2001::1')
        client.srp_client_add_service('my-service', '_ipps._tcp', 12345)
        self.simulator.go(2)

        self.host_check_mdns_service(host)

    def host_check_mdns_service(self, host):
        service = host.discover_mdns_service('my-service', '_ipps._tcp', 'my-host')
        self.assertIsNotNone(service)
        self.assertEqual(service['instance'], 'my-service')
        self.assertEqual(service['name'], '_ipps._tcp')
        self.assertEqual(service['port'], 12345)
        self.assertEqual(service['priority'], 0)
        self.assertEqual(service['weight'], 0)
        self.assertEqual(service['host'], 'my-host')

    def check_host_and_service(self, server, client):
        """Check that we have properly registered host and service instance.
        """

        client_services = client.srp_client_get_services()
        print(client_services)
        self.assertEqual(len(client_services), 1)
        client_service = client_services[0]

        # Verify that the client possesses correct service resources.
        self.assertEqual(client_service['instance'], 'my-service')
        self.assertEqual(client_service['name'], '_ipps._tcp')
        self.assertEqual(int(client_service['port']), 12345)
        self.assertEqual(int(client_service['priority']), 0)
        self.assertEqual(int(client_service['weight']), 0)

        # Verify that the client received a SUCCESS response for the server.
        self.assertEqual(client_service['state'], 'Registered')

        server_services = server.srp_server_get_services()
        print(server_services)
        self.assertEqual(len(server_services), 1)
        server_service = server_services[0]

        # Verify that the server accepted the SRP registration and stores
        # the same service resources.
        self.assertEqual(server_service['deleted'], 'false')
        self.assertEqual(server_service['instance'], client_service['instance'])
        self.assertEqual(server_service['name'], client_service['name'])
        self.assertEqual(int(server_service['port']), int(client_service['port']))
        self.assertEqual(int(server_service['priority']), int(client_service['priority']))
        self.assertEqual(int(server_service['weight']), int(client_service['weight']))
        self.assertEqual(server_service['host'], 'my-host')

        server_hosts = server.srp_server_get_hosts()
        print(server_hosts)
        self.assertEqual(len(server_hosts), 1)
        server_host = server_hosts[0]

        self.assertEqual(server_host['deleted'], 'false')
        self.assertEqual(server_host['fullname'], server_service['host_fullname'])
        self.assertEqual(len(server_host['addresses']), 1)
        self.assertEqual(ipaddress.ip_address(server_host['addresses'][0]), ipaddress.ip_address('2001::1'))


if __name__ == '__main__':
    unittest.main()
