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
import typing
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
        },
        CLIENT1: {
            'mode': 'rdn',
        },
        CLIENT2: {
            'mode': 'rdn',
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
        answers = self.nodes[CLIENT1].dns_resolve(f"host1.{DOMAIN}", self.nodes[SERVER].get_mleid(), 53)
        self.assertEqual(set(ipaddress.IPv6Address(ip) for ip, _ in answers),
                         set(map(ipaddress.IPv6Address, client1_addrs)))

        answers = self.nodes[CLIENT1].dns_resolve(f"host2.{DOMAIN}", self.nodes[SERVER].get_mleid(), 53)
        self.assertEqual(set(ipaddress.IPv6Address(ip) for ip, _ in answers),
                         set(map(ipaddress.IPv6Address, client2_addrs)))

        service_instances = self.nodes[CLIENT1].dns_browse(f'{SERVICE}.{DOMAIN}', self.nodes[SERVER].get_mleid(), 53)
        self.assertEqual({'ins1', 'ins2'}, set(service_instances.keys()), service_instances)

        instance1_verify_info = {
            'port': 11111,
            'priority': 1,
            'weight': 1,
            'host': 'host1.default.service.arpa.',
            'address': client1_addrs,
            'txt_data': '',
            'srv_ttl': lambda x: x > 0,
            'txt_ttl': lambda x: x > 0,
            'aaaa_ttl': lambda x: x > 0,
        }

        instance2_verify_info = {
            'port': 22222,
            'priority': 2,
            'weight': 2,
            'host': 'host2.default.service.arpa.',
            'address': client2_addrs,
            'txt_data': '',
            'srv_ttl': lambda x: x > 0,
            'txt_ttl': lambda x: x > 0,
            'aaaa_ttl': lambda x: x > 0,
        }

        self._assert_service_instance_equal(service_instances['ins1'], instance1_verify_info)
        self._assert_service_instance_equal(service_instances['ins2'], instance2_verify_info)

        service_instance = self.nodes[CLIENT1].dns_resolve_service('ins1', f'{SERVICE}.{DOMAIN}',
                                                                   self.nodes[SERVER].get_mleid(), 53)
        self._assert_service_instance_equal(service_instance, instance1_verify_info)

        service_instance = self.nodes[CLIENT1].dns_resolve_service('ins2', f'{SERVICE}.{DOMAIN}',
                                                                   self.nodes[SERVER].get_mleid(), 53)
        self._assert_service_instance_equal(service_instance, instance2_verify_info)

    def _assert_service_instance_equal(self, instance, info):
        for f in ('port', 'priority', 'weight', 'host', 'txt_data'):
            self.assertEqual(instance[f], info[f], instance)

        verify_addresses = info['address']
        if not isinstance(verify_addresses, typing.Collection):
            verify_addresses = [verify_addresses]
        self.assertIn(ipaddress.IPv6Address(instance['address']), map(ipaddress.IPv6Address, verify_addresses),
                      instance)

        for ttl_f in ('srv_ttl', 'txt_ttl', 'aaaa_ttl'):
            check_ttl = info[ttl_f]
            if not callable(check_ttl):
                check_ttl = lambda x: x == check_ttl

            self.assertTrue(check_ttl(instance[ttl_f]), instance)

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
