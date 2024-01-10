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
#   This test verifies the SRP client lease changing works as expected.
#
# Topology:
#     LEADER (SRP server)
#       |
#       |
#     ROUTER (SRP client)
#
from pktverify.packet_filter import PacketFilter

SERVER = 1
CLIENT = 2

DEFAULT_LEASE_TIME = 7200
LEASE_TIME = 60
NEW_LEASE_TIME = 120
KEY_LEASE_TIME = 240
NEW_TTL = 10

assert LEASE_TIME < KEY_LEASE_TIME
assert NEW_LEASE_TIME < KEY_LEASE_TIME
assert NEW_TTL < NEW_LEASE_TIME


class SrpClientChangeLeaseTime(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False
    SUPPORT_NCP = False

    TOPOLOGY = {
        SERVER: {
            'name': 'SRP_SERVER',
            'networkkey': '00112233445566778899aabbccddeeff',
            'mode': 'rdn',
        },
        CLIENT: {
            'name': 'SRP_CLIENT',
            'networkkey': '00112233445566778899aabbccddeeff',
            'mode': 'rdn',
        },
    }

    def test(self):
        server = self.nodes[SERVER]
        client = self.nodes[CLIENT]

        #
        # 0. Start the server and client devices.
        #

        server.srp_server_set_enabled(True)
        server.srp_server_set_lease_range(LEASE_TIME, LEASE_TIME, KEY_LEASE_TIME, KEY_LEASE_TIME)
        server.start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual(server.get_state(), 'leader')
        self.simulator.go(5)

        client.start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual(client.get_state(), 'router')

        #
        # 1. Register a single service and verify that it works.
        #

        self.assertEqual(client.srp_client_get_auto_start_mode(), 'Enabled')

        client.srp_client_set_host_name('my-host')
        client.srp_client_set_host_address('2001::1')
        client.srp_client_add_service('my-service', '_ipps._tcp', 12345)
        self.simulator.go(2)

        self.check_host_and_service(server, client)

        server.srp_server_set_lease_range(NEW_LEASE_TIME, NEW_LEASE_TIME, KEY_LEASE_TIME, KEY_LEASE_TIME)
        client.srp_client_set_lease_interval(NEW_LEASE_TIME)
        self.simulator.go(NEW_LEASE_TIME)
        self.assertEqual(client.srp_client_get_host_state(), 'Registered')

        self.simulator.go(KEY_LEASE_TIME * 2)
        self.assertEqual(client.srp_client_get_host_state(), 'Registered')

        client.srp_client_set_ttl(NEW_TTL)
        self.simulator.go(NEW_LEASE_TIME)
        self.assertEqual(client.srp_client_get_host_state(), 'Registered')

        client.srp_client_set_ttl(0)
        self.simulator.go(NEW_LEASE_TIME)
        self.assertEqual(client.srp_client_get_host_state(), 'Registered')

    def verify(self, pv):
        pkts: PacketFilter = pv.pkts
        pv.summary.show()

        CLIENT_SRC64 = pv.vars['SRP_CLIENT']

        pkts.filter_wpan_src64(CLIENT_SRC64).filter('dns.flags.response == 0 and dns.resp.ttl == {DEFAULT_LEASE_TIME}',
                                                    DEFAULT_LEASE_TIME=DEFAULT_LEASE_TIME).must_next()
        pkts.filter_wpan_src64(CLIENT_SRC64).filter('dns.flags.response == 0 and dns.resp.ttl == {NEW_LEASE_TIME}',
                                                    NEW_LEASE_TIME=NEW_LEASE_TIME).must_next()
        pkts.filter_wpan_src64(CLIENT_SRC64).filter('dns.flags.response == 0 and dns.resp.ttl == {NEW_TTL}',
                                                    NEW_TTL=NEW_TTL).must_next()
        pkts.filter_wpan_src64(CLIENT_SRC64).filter('dns.flags.response == 0 and dns.resp.ttl == {NEW_LEASE_TIME}',
                                                    NEW_LEASE_TIME=NEW_LEASE_TIME).must_next()

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


if __name__ == '__main__':
    unittest.main()
