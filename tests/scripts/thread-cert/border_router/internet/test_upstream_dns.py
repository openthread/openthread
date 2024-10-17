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
#    ----------------(eth)--------------------
#           |                 |
#          BR (Leader)      HOST
#           |
#        ROUTER
#

BR = 1
ROUTER = 2
HOST = 3

TEST_DOMAIN = 'test.domain'
TEST_DOMAIN_IP6_ADDRESSES = {'2001:db8::1'}

TEST_DOMAIN_BIND_CONF = f'''
zone "{TEST_DOMAIN}" {{ type master; file "/etc/bind/db.test.domain"; }};
'''

TEST_DOMAIN_BIND_ZONE = f'''
$TTL 24h
@ IN SOA {TEST_DOMAIN} test.{TEST_DOMAIN}. ( 20230330 86400 300 604800 3600 )
@ IN NS {TEST_DOMAIN}.
''' + '\n'.join(f'@ IN AAAA {addr}' for addr in TEST_DOMAIN_IP6_ADDRESSES)


class UpstreamDns(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR: {
            'name': 'BR',
            'allowlist': [ROUTER],
            'is_otbr': True,
            'version': '1.4',
        },
        ROUTER: {
            'name': 'Router',
            'allowlist': [BR],
            'version': '1.4',
        },
        HOST: {
            'name': 'Host',
            'is_host': True
        },
    }

    def test(self):
        br = self.nodes[BR]
        router = self.nodes[ROUTER]
        host = self.nodes[HOST]

        host.start(start_radvd=False)
        self.simulator.go(5)

        br.start()
        # When feature flag is enabled, NAT64 might be disabled by default. So
        # ensure NAT64 is enabled here.
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual('leader', br.get_state())

        br.nat64_set_enabled(True)
        br.srp_server_set_enabled(True)

        br.bash('service bind9 stop')

        br.bash(shlex.join(['echo', TEST_DOMAIN_BIND_CONF]) + ' >> /etc/bind/named.conf.local')
        br.bash(shlex.join(['echo', TEST_DOMAIN_BIND_ZONE]) + ' >> /etc/bind/db.test.domain')

        br.bash('service bind9 start')

        router.start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual('router', router.get_state())

        self.simulator.go(10)
        router.srp_client_enable_auto_start_mode()

        # verify the server can forward the DNS query to upstream server.
        self._verify_upstream_dns(br, router)

    def _verify_upstream_dns(self, br, ed):
        upstream_dns_enabled = br.dns_upstream_query_state
        if not upstream_dns_enabled:
            br.dns_upstream_query_state = True
        self.assertTrue(br.dns_upstream_query_state)

        resolved_names = ed.dns_resolve(TEST_DOMAIN)
        self.assertEqual(len(resolved_names), len(TEST_DOMAIN_IP6_ADDRESSES))
        for record in resolved_names:
            self.assertIn(ipaddress.IPv6Address(record[0]).compressed, TEST_DOMAIN_IP6_ADDRESSES)


if __name__ == '__main__':
    unittest.main()
