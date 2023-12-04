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
import ipv6

import ipaddress

# For NAT64 connectivity tests
import socket
import select

# Test description:
#   This test verifies publishing the local NAT64 prefix in Thread network
#   when no NAT64 prefix found on infrastructure interface.
#   It also verifies the outbound connectivity from a Thread device to
#   an IPv4 host address.
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

# The prefix is set small enough that a random-generated ULA NAT64 prefix is very
# likely greater than it. So the BR will remove the random-generated one.
SMALL_NAT64_PREFIX = "fd00:00:00:01:00:00::/96"
# The prefix is set larger than a random-generated ULA NAT64 prefix.
# So the BR will remove the random-generated one.
LARGE_NAT64_PREFIX = "ff00:00:00:01:00:00::/96"

DEFAULT_NAT64_CIDR = ipaddress.IPv4Network("192.168.255.0/24")
UPDATED_NAT64_CIDR = ipaddress.IPv4Network("100.64.64.0/24")


class Nat64SingleBorderRouter(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR: {
            'name': 'BR',
            'allowlist': [ROUTER],
            'is_otbr': True,
            'version': '1.2',
        },
        ROUTER: {
            'name': 'Router',
            'allowlist': [BR],
            'version': '1.2',
        },
        HOST: {
            'name': 'Host',
            'is_host': True
        },
    }

    def get_host_ip(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setblocking(False)
        # Note: The address is not important, we use this function to get the host ip address.
        sock.connect(('8.8.8.8', 54321))
        host_ip, _ = sock.getsockname()
        sock.close()
        return host_ip

    def receive_from(self, sock, timeout_seconds):
        ready = select.select([sock], [], [], timeout_seconds)
        if ready[0]:
            return sock.recv(1024)
        else:
            raise AssertionError("No data received")

    def listen_udp(self, addr, port):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setblocking(False)
        sock.bind((addr, port))
        return sock

    def test(self):
        br = self.nodes[BR]
        router = self.nodes[ROUTER]
        host = self.nodes[HOST]

        host.start(start_radvd=False)
        self.simulator.go(5)

        br.start()
        # When feature flag is enabled, NAT64 might be disabled by default. So
        # ensure NAT64 is enabled here.
        br.nat64_set_enabled(True)
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        br.bash("service bind9 stop")
        self.simulator.go(330)
        self.assertEqual('leader', br.get_state())

        router.start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual('router', router.get_state())

        #
        # Case 1. Border router advertises its local NAT64 prefix.
        #
        self.simulator.go(5)
        local_nat64_prefix = br.get_br_nat64_prefix()

        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        nat64_prefix = br.get_netdata_nat64_routes()[0]
        self.assertEqual(nat64_prefix, local_nat64_prefix)

        #
        # Case 2.
        # User adds a smaller NAT64 prefix (same preference) and the local prefix is withdrawn.
        # User removes the smaller NAT64 prefix and the local prefix is re-added.
        #
        br.add_route(SMALL_NAT64_PREFIX, stable=False, nat64=True, prf='low')
        br.register_netdata()
        self.simulator.go(5)

        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        self.assertNotEqual(local_nat64_prefix, br.get_netdata_nat64_routes()[0])

        br.remove_route(SMALL_NAT64_PREFIX)
        br.register_netdata()
        self.simulator.go(10)

        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        self.assertEqual(local_nat64_prefix, br.get_netdata_nat64_routes()[0])

        #
        # Case 3.
        # User adds a larger NAT64 prefix (higher preference) and the local prefix is withdrawn.
        # User removes the larger NAT64 prefix and the local prefix is re-added.
        #
        br.add_route(LARGE_NAT64_PREFIX, stable=False, nat64=True, prf='med')
        br.register_netdata()
        self.simulator.go(5)

        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        self.assertNotEqual(local_nat64_prefix, br.get_netdata_nat64_routes()[0])

        br.remove_route(LARGE_NAT64_PREFIX)
        br.register_netdata()
        self.simulator.go(10)

        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        self.assertEqual(local_nat64_prefix, br.get_netdata_nat64_routes()[0])

        #
        # Case 4. Disable and re-enable border routing on the border router.
        #
        br.disable_br()
        self.simulator.go(5)

        # NAT64 prefix is withdrawn from Network Data.
        self.assertEqual(len(br.get_netdata_nat64_routes()), 0)

        br.enable_br()
        self.simulator.go(config.BORDER_ROUTER_STARTUP_DELAY)

        #
        # Case 5. NAT64 connectivity and counters.
        #
        host_ip = self.get_host_ip()
        self.assertTrue(router.ping(ipaddr=host_ip))

        mappings = br.nat64_mappings
        self.assertEqual(mappings[0]['counters']['ICMP']['4to6']['packets'], 1)
        self.assertEqual(mappings[0]['counters']['ICMP']['6to4']['packets'], 1)
        self.assertEqual(mappings[0]['counters']['total']['4to6']['packets'], 1)
        self.assertEqual(mappings[0]['counters']['total']['6to4']['packets'], 1)

        counters = br.nat64_counters
        self.assertEqual(counters['protocol']['ICMP']['4to6']['packets'], 1)
        self.assertEqual(counters['protocol']['ICMP']['6to4']['packets'], 1)

        sock = self.listen_udp('0.0.0.0', 54321)
        router.udp_start('::', 54321)
        # We can use IPv4 addresses for commands like UDP send.
        # The address will be converted to an IPv6 address by CLI.
        router.udp_send(10, host_ip, 54321)
        self.assertTrue(len(self.receive_from(sock, timeout_seconds=1)) == 10)

        sock.close()

        counters = br.nat64_counters
        self.assertEqual(counters['protocol']['UDP']['6to4']['packets'], 1)
        mappings = br.nat64_mappings
        self.assertEqual(mappings[0]['counters']['UDP']['6to4']['packets'], 1)

        # We should be able to get a IPv4 mapped IPv6 address.
        # 203.0.113.1, RFC5737 TEST-NET-3, should be unreachable.
        mapped_ip6_address = str(
            ipv6.synthesize_ip6_address(ipaddress.IPv6Network(nat64_prefix), ipaddress.IPv4Address('203.0.113.1')))
        self.assertFalse(router.ping(ipaddr=mapped_ip6_address))

        mappings = br.nat64_mappings
        self.assertEqual(mappings[0]['counters']['ICMP']['4to6']['packets'], 1)
        self.assertEqual(mappings[0]['counters']['ICMP']['6to4']['packets'], 2)
        self.assertEqual(mappings[0]['counters']['total']['4to6']['packets'], 1)
        self.assertEqual(mappings[0]['counters']['total']['6to4']['packets'], 3)

        counters = br.nat64_counters
        self.assertEqual(counters['protocol']['ICMP']['4to6']['packets'], 1)
        self.assertEqual(counters['protocol']['ICMP']['6to4']['packets'], 2)

        #
        # Case 6. Change the CIDR used by NAT64 during runtime
        #
        self.assertEqual(br.nat64_cidr, DEFAULT_NAT64_CIDR)

        br.nat64_cidr = UPDATED_NAT64_CIDR
        self.assertEqual(br.nat64_cidr, UPDATED_NAT64_CIDR)
        self.assertTrue(router.ping(ipaddr=host_ip))

        #
        # Case 7. Disable and re-enable ethernet on the border router.
        # Note: disable_ether will remove default route but enable_ether won't add it back,
        # NAT64 connectivity tests will fail after this.
        # TODO: Add a default IPv4 route after enable_ether.
        #
        br.disable_ether()
        self.simulator.go(5)

        # NAT64 prefix is withdrawn from Network Data.
        self.assertEqual(len(br.get_netdata_nat64_routes()), 0)

        br.enable_ether()
        self.simulator.go(80)

        # Same NAT64 prefix is advertised to Network Data.
        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        self.assertEqual(nat64_prefix, br.get_netdata_nat64_routes()[0])
        # Same NAT64 prefix is advertised to Network Data.
        self.assertEqual(len(br.get_netdata_nat64_routes()), 1)
        self.assertEqual(nat64_prefix, br.get_netdata_nat64_routes()[0])


if __name__ == '__main__':
    unittest.main()
