#!/usr/bin/env python3
#
#  Copyright (c) 2023, The OpenThread Authors.
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

import config
import thread_cert

import ipaddress
import shlex

# Test description:
#   This test verifies forwarding DNS queries sent by 'Router' by using
# a record resolved by BIND9 server.
#
# Topology:
#    ----------------(eth)----------------------------------
#           |                 |                  |
#        BR (Leader)     DNS SERVER 1       DNS SERVER 2
#           |
#        ROUTER
#

BR = 1
ROUTER = 2
DNS_SERVER_1 = 3
DNS_SERVER_2 = 4

TEST_DOMAIN_1 = 'test.domain.resolv'
TEST_DOMAIN_IP6_ADDRESSES_1 = {'2001:db8::1'}

TEST_DOMAIN_2 = 'test.domain.rdnss'
TEST_DOMAIN_IP6_ADDRESSES_2 = {'2001:db8::2'}


class UpstreamDns(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR: {
            'name': 'BR',
            'is_otbr': True,
            'version': '1.4',
        },
        ROUTER: {
            'name': 'Router',
            'version': '1.4',
        },
        DNS_SERVER_1: {
            'name': 'DNS Server 1',
            'is_host': True
        },
        DNS_SERVER_2: {
            'name': 'DNS Server 2',
            'is_host': True
        },
    }

    def test(self):
        br = self.nodes[BR]
        router = self.nodes[ROUTER]
        dns_server_1 = self.nodes[DNS_SERVER_1]
        dns_server_2 = self.nodes[DNS_SERVER_2]

        self._start_dns_server(dns_server_1, TEST_DOMAIN_1, TEST_DOMAIN_IP6_ADDRESSES_1)
        self._start_dns_server(dns_server_2, TEST_DOMAIN_2, TEST_DOMAIN_IP6_ADDRESSES_2)

        # Disable the bind9 service on the BR otherwise bind9 may respond to Thread devices' DNS queries
        br.bash('service bind9 stop || true')

        br.start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual('leader', br.get_state())

        # When feature flag is enabled, NAT64 might be disabled by default. So
        # ensure NAT64 is enabled here.
        br.nat64_set_enabled(True)
        br.srp_server_set_enabled(True)

        router.start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual('router', router.get_state())

        self.simulator.go(10)
        router.srp_client_enable_auto_start_mode()

        br.dns_upstream_query_state = True
        self.assertTrue(br.dns_upstream_query_state)

        # Update BR's /etc/resolv.conf with the address of DNS server 1 and force BR to reload it
        dns_server_addr_1 = dns_server_1.get_ether_addrs(ipv4=True, ipv6=False)[0]
        br.bash(shlex.join(['echo', 'nameserver ' + dns_server_addr_1]) + ' > /etc/resolv.conf')
        br.stop_otbr_service()
        br.start_otbr_service()

        # Publish RDNSS with the address of DNS server 2 through RA
        dns_server_addr_2 = dns_server_2.get_ip6_address(config.ADDRESS_TYPE.ONLINK_ULA)[0]
        br.start_rdnss_radvd_service(dns_server_addr_2)

        self.simulator.go(10)
        # verify the server can forward the DNS query to the two upstream servers.
        resolved_addresses_1 = set(
            ipaddress.IPv6Address(record[0]).compressed for record in router.dns_resolve(TEST_DOMAIN_1))
        self.assertEqual(resolved_addresses_1, TEST_DOMAIN_IP6_ADDRESSES_1)

        resolved_addresses_2 = set(
            ipaddress.IPv6Address(record[0]).compressed for record in router.dns_resolve(TEST_DOMAIN_2))
        self.assertEqual(resolved_addresses_2, TEST_DOMAIN_IP6_ADDRESSES_2)

    def _start_dns_server(self, dns_server, test_domain, test_domain_ip6_addresses):
        test_domain_bind_conf = f'''
zone "{test_domain}" {{ type master; file "/etc/bind/db.test.domain"; }};
'''

        test_domain_bind_zone = f'''
$TTL 24h
@ IN SOA {test_domain} test.{test_domain}. ( 20230330 86400 300 604800 3600 )
@ IN NS {test_domain}.
''' + '\n'.join(f'@ IN AAAA {addr}' for addr in test_domain_ip6_addresses)

        dns_server.start(start_radvd=False)
        dns_server.bash('service bind9 stop || true')

        dns_server.bash(shlex.join(['echo', test_domain_bind_conf]) + ' >> /etc/bind/named.conf.local')
        dns_server.bash(shlex.join(['echo', test_domain_bind_zone]) + ' >> /etc/bind/db.test.domain')

        dns_server.bash('service bind9 start')


if __name__ == '__main__':
    unittest.main()
