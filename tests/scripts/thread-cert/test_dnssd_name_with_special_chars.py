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
import logging

import command
import config
import thread_cert

# Test description:
#   This test verifies SRP/DNS client/server support for Service Instance names with special and unicode characters.
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

# TODO: support Instance names with dots (.)
SPECIAL_INSTANCE_NAME = 'O\\T 网关'


class TestDnssdNameWithSpecialChars(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False
    SUPPORT_NCP = False

    TOPOLOGY = {
        SERVER: {
            'name': 'SERVER',
            'mode': 'rdn',
        },
        CLIENT: {
            'name': 'CLIENT',
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

        # Register a single service with the instance name containing special chars
        client.srp_client_set_host_name('host1')
        client.srp_client_set_host_address('2001::1')
        client.srp_client_add_service(SPECIAL_INSTANCE_NAME, '_srv._udp', 1977)
        self.simulator.go(5)
        self.__check_service_discovery(server, client)

    def __check_service_discovery(self, server, client):
        # Check the service on client
        client.dns_set_config(server.get_rloc())
        services = client.dns_browse('_srv._udp.default.service.arpa')
        self.assertEqual(1, len(services))
        self.assertIn(SPECIAL_INSTANCE_NAME, services)
        self.__verify_service(services[SPECIAL_INSTANCE_NAME])

        service = client.dns_resolve_service(SPECIAL_INSTANCE_NAME, '_srv._udp.default.service.arpa')
        self.__verify_service(service)

        service = client.dns_resolve_service(SPECIAL_INSTANCE_NAME.lower(), '_srv._udp.default.service.arpa')
        self.__verify_service(service)

    def __verify_service(self, service):
        logging.info('service discovered: %r', service)
        self.assertEqual(service['port'], 1977)
        self.assertEqual(service['host'], 'host1.default.service.arpa.')


if __name__ == '__main__':
    unittest.main()
