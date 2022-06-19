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

import unittest

import config
import thread_cert

# Test description:
#   This test verifies SRP function that SRP clients can register up to 500 services to one SRP server.
#
# Topology:
#     LEADER (SRP server)
#       |
#       |
#     ROUTER1 ... ROUTER12 ROUTER13     (SRP clients)
#       |            |
#      FED14  ...   FED25               (SRP clients)
#

NUM_SRP_CLIENTS = 25
INSTANCE_NUM_PER_CLIENT = 20
NUM_ROUTERS = (NUM_SRP_CLIENTS // 2) + (NUM_SRP_CLIENTS % 2)
NUM_FEDS = NUM_SRP_CLIENTS - NUM_ROUTERS
assert (NUM_FEDS <= NUM_ROUTERS)

SERVER = NUM_SRP_CLIENTS + 1
CLIENT1 = 1

ROUTER_IDS = tuple(range(1, NUM_ROUTERS + 1))
FED_IDS = tuple(range(NUM_ROUTERS + 1, NUM_SRP_CLIENTS + 1))
assert len(FED_IDS) == NUM_FEDS
CLIENT_IDS = tuple(range(1, NUM_SRP_CLIENTS + 1))
INSTANCE_IDS = tuple(range(1, INSTANCE_NUM_PER_CLIENT + 1))
SERVICE_NAME = '_srpclient._tcp'
SERVICE_PORT = 12345


class SrpRegister500Services(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False
    SUPPORT_NCP = False

    TOPOLOGY = {
        SERVER: {
            'name': 'SRP_SERVER',
            'mode': 'rdn',
            'allowlist': [],
        },
    }

    for nodeid in ROUTER_IDS:
        TOPOLOGY[nodeid] = {
            'name': f'SRP_CLIENT_{nodeid}',
            'mode': 'rdn',
            'allowlist': [SERVER],
        }
        TOPOLOGY[SERVER]['allowlist'].append(nodeid)

    for nodeid in FED_IDS:
        parentid = nodeid - NUM_ROUTERS
        TOPOLOGY[nodeid] = {
            'name': f'SRP_CLIENT_{nodeid}',
            'mode': 'rdn',
            'router_eligible': False,
            'allowlist': [parentid],
        }
        TOPOLOGY[parentid]['allowlist'].append(nodeid)

    def test(self):
        self._test_impl()

    def _test_impl(self):
        server = self.nodes[SERVER]

        def routers():
            for nodeid in ROUTER_IDS:
                yield self.nodes[nodeid]

        def feds():
            for nodeid in FED_IDS:
                yield self.nodes[nodeid]

        def clients():
            for nodeid in CLIENT_IDS:
                yield self.nodes[nodeid]

        # Start the server & clients.
        server.srp_server_set_enabled(True)
        server.start()
        self.simulator.go(5)
        self.assertEqual(server.get_state(), 'leader')

        for router in routers():
            router.srp_server_set_enabled(False)
            router.start()
            self.simulator.go(config.ROUTER_STARTUP_DELAY)

        for router in routers():
            self.assertEqual(router.get_state(), 'router')

        for fed in feds():
            fed.srp_server_set_enabled(False)
            fed.start()
            self.simulator.go(3)

        for fed in feds():
            self.assertEqual(fed.get_state(), 'child')

        server_addr = server.get_rloc()

        for clientid in CLIENT_IDS:
            client = self.nodes[clientid]
            client.srp_client_set_host_name(f'client{clientid}')
            client.srp_client_set_host_address(f'2001::{clientid}')
            client.srp_client_start(server_addr, client.get_srp_server_port())

            for instanceid in INSTANCE_IDS:
                client.srp_client_add_service(f'client{clientid}_{instanceid}', SERVICE_NAME, SERVICE_PORT)

                if instanceid % 10 == 0 or instanceid == INSTANCE_IDS[-1]:
                    self._wait_client_all_services_registered(client)

            self._check_client_host_and_services(client, clientid)

        # Make sure all service are registered successfully
        self._check_server_hosts_and_services(server)

    def _wait_client_all_services_registered(self, client, timeout=60):
        all_registered = False
        while timeout > 0:
            self.simulator.go(3)
            timeout -= 3

            client_services = client.srp_client_get_services()
            all_registered = all(service['state'] == 'Registered' for service in client_services)
            if all_registered:
                break

        self.assertTrue(all_registered, "Some services are not registered successfully")

    def _check_client_host_and_services(self, client, clientid):
        client_services = client.srp_client_get_services()
        self.assertEqual(len(client_services), INSTANCE_NUM_PER_CLIENT)
        service_instance_names = {f'client{clientid}_{instanceid}' for instanceid in INSTANCE_IDS}
        for service in client_services:
            self.assertEqual(service['name'], SERVICE_NAME)
            self.assertEqual(service['state'], 'Registered')
            self.assertEqual(int(service['port']), SERVICE_PORT)
            self.assertIn(service['instance'], service_instance_names)
            service_instance_names.discard(service['instance'])

    def _check_server_hosts_and_services(self, server):
        server_services = server.srp_server_get_services()
        self.assertEqual(len(server_services), NUM_SRP_CLIENTS * INSTANCE_NUM_PER_CLIENT)

        service_instance_names = {
            f'client{clientid}_{instanceid}' for clientid in CLIENT_IDS for instanceid in INSTANCE_IDS
        }

        for service in server_services:
            self.assertEqual(service['name'], SERVICE_NAME)
            self.assertIn(service['instance'], service_instance_names)
            service_instance_names.discard(service['instance'])


if __name__ == '__main__':
    unittest.main()
