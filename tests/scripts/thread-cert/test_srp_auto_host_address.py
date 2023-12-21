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
#   This test verifies SRP client auto host address mode.
#
# Topology:
#     SRP client (leader)
#       |
#       |
#     SRP server (router)
#

CLIENT = 1
SERVER = 2


class SrpAutoHostAddress(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False
    SUPPORT_NCP = False

    TOPOLOGY = {
        CLIENT: {
            'name': 'SRP_CLIENT',
            'mode': 'rdn',
        },
        SERVER: {
            'name': 'SRP_SERVER',
            'mode': 'rdn',
        },
    }

    def test(self):
        client = self.nodes[CLIENT]
        server = self.nodes[SERVER]

        #-------------------------------------------------------------------
        # Form the network.

        client.srp_server_set_enabled(False)
        client.start()
        self.simulator.go(15)
        self.assertEqual(client.get_state(), 'leader')

        server.start()
        self.simulator.go(5)
        self.assertEqual(server.get_state(), 'router')

        #-------------------------------------------------------------------
        # Enable SRP server

        server.srp_server_set_enabled(True)
        self.simulator.go(5)

        #-------------------------------------------------------------------
        # Check auto start mode on SRP client

        self.assertEqual(client.srp_client_get_auto_start_mode(), 'Enabled')
        self.simulator.go(2)

        self.assertEqual(client.srp_client_get_state(), 'Enabled')

        #-------------------------------------------------------------------
        # Set host name and enable auto host address on client

        client.srp_client_set_host_name('host')
        client.srp_client_enable_auto_host_address()

        #-------------------------------------------------------------------
        # Register a service on client

        client.srp_client_add_service('test_srv', '_test._udo', 12345, 0, 0)
        self.simulator.go(2)
        self.check_registered_addresses(client, server)

        #-------------------------------------------------------------------
        # Add an address and check the SRP client re-registered and updated
        # server with new address.

        client.add_ipaddr('fd00:1:2:3:4:5:6:7')

        self.simulator.go(5)
        client_addresses = [addr.strip() for addr in client.get_addrs()]
        self.assertIn('fd00:1:2:3:4:5:6:7', client_addresses)
        self.check_registered_addresses(client, server)

        #-------------------------------------------------------------------
        # Remove the address and check the SRP client re-registered and updated
        # server.

        client.del_ipaddr('fd00:1:2:3:4:5:6:7')

        self.simulator.go(5)
        client_addresses = [addr.strip() for addr in client.get_addrs()]
        self.assertNotIn('fd00:1:2:3:4:5:6:7', client_addresses)
        self.check_registered_addresses(client, server)

        #-------------------------------------------------------------------
        # Add an SLAAC on-mesh prefix (which will trigger an address to be
        # added) and check that the SRP client re-registered and updated
        # server with the new address.

        client.add_prefix('fd00:abba:cafe:bee::/64', 'paos')
        client.register_netdata()
        self.simulator.go(5)

        slaac_addr = [addr.strip() for addr in client.get_addrs() if addr.strip().startswith('fd00:abba:cafe:bee:')]
        self.assertEqual(len(slaac_addr), 1)
        self.check_registered_addresses(client, server)

        #-------------------------------------------------------------------
        # Add another SLAAC on-mesh prefix and check that the SRP client
        # re-registered and updated server with all address.

        client.add_prefix('fd00:9:8:7::/64', 'paos')
        client.register_netdata()
        self.simulator.go(5)

        slaac_addr = [addr.strip() for addr in client.get_addrs() if addr.strip().startswith('fd00:9:8:7:')]
        self.assertEqual(len(slaac_addr), 1)
        self.check_registered_addresses(client, server)

        #-------------------------------------------------------------------
        # Add a non-preferred SLAAC on-mesh prefix and check that the
        # set of registered addresses remains unchanged and that the
        # non-preferred  address is not registered by SRP client.

        client.add_prefix('fd00:a:b:c::/64', 'aos')
        client.register_netdata()
        self.simulator.go(5)

        slaac_addr = [addr.strip() for addr in client.get_addrs() if addr.strip().startswith('fd00:a:b:c:')]
        self.assertEqual(len(slaac_addr), 1)
        self.check_registered_addresses(client, server)

        #-------------------------------------------------------------------
        # Remove the on-mesh prefix (which will trigger an address to be
        # removed) and check that the SRP client re-registered and updated
        # server with the remaining address.

        client.remove_prefix('fd00:abba:cafe:bee::/64')
        client.register_netdata()
        self.simulator.go(5)

        slaac_addr = [addr.strip() for addr in client.get_addrs() if addr.strip().startswith('fd00:abba:cafe:bee:')]
        self.assertEqual(len(slaac_addr), 0)
        self.check_registered_addresses(client, server)

        #-------------------------------------------------------------------
        # Remove the next on-mesh prefix. Check that SRP client re-registered
        # now with only ML-EID.

        client.remove_prefix('fd00:9:8:7::/64')
        client.register_netdata()
        self.simulator.go(5)

        slaac_addr = [addr.strip() for addr in client.get_addrs() if addr.strip().startswith('fd00:9:8:7:')]
        self.assertEqual(len(slaac_addr), 0)
        self.check_registered_addresses(client, server)

        #-------------------------------------------------------------------
        # Explicitly set the host addresses (which disables the auto host
        # address mode) and check that only the new addresses are registered.

        client.srp_client_set_host_address('fd00:f:e:d:c:b:a:9')
        self.simulator.go(5)

        self.assertEqual(client.srp_client_get_host_state(), 'Registered')
        server_hosts = server.srp_server_get_hosts()
        self.assertEqual(len(server_hosts), 1)
        server_host = server_hosts[0]
        self.assertEqual(server_host['deleted'], 'false')
        self.assertEqual(server_host['fullname'], 'host.default.service.arpa.')
        host_addresses = [addr.strip() for addr in server_host['addresses']]
        self.assertEqual(len(host_addresses), 1)
        self.assertEqual(host_addresses[0], 'fd00:f:e:d:c:b:a:9')

        #-------------------------------------------------------------------
        # Re-enable auto host address mode and check that addresses are
        # updated and registered properly.

        client.srp_client_enable_auto_host_address()
        self.simulator.go(5)
        self.check_registered_addresses(client, server)

    def check_registered_addresses(self, client, server):
        # Ensure client has registered successfully.
        self.assertEqual(client.srp_client_get_host_state(), 'Registered')

        # Check the host info on server.
        server_hosts = server.srp_server_get_hosts()
        self.assertEqual(len(server_hosts), 1)
        server_host = server_hosts[0]
        self.assertEqual(server_host['deleted'], 'false')
        self.assertEqual(server_host['fullname'], 'host.default.service.arpa.')

        # Check the host addresses on server to match client.

        host_addresses = [addr.strip() for addr in server_host['addresses']]

        client_mleid = client.get_mleid()
        client_addresses = [addr.split(' ')[0] for addr in client.get_addrs(verbose=True) if 'preferred:1' in addr]
        client_addresses += [client_mleid]

        # All registered addresses must be in client list of addresses.

        for addr in host_addresses:
            self.assertIn(addr, client_addresses)

        # All preferred addresses on client excluding link-local and
        # mesh-local addresses must be seen on server side. But if there
        # was no address, then mesh-local address should be the only
        # one registered.

        checked_address = False

        for addr in client_addresses:
            if not self.is_address_link_local(addr) and not self.is_address_locator(addr) and addr != client_mleid:
                self.assertIn(addr, host_addresses)
                checked_address = True

        if not checked_address:
            self.assertEqual(len(host_addresses), 1)
            self.assertIn(client_mleid, host_addresses)

    def is_address_locator(self, addr):
        # Checks if an IPv6 address is a locator (IID should match `0:ff:fe00:xxxx`)
        u32s = addr.split(':')
        self.assertEqual(len(u32s), 8)
        return ':'.join(u32s[4:]).startswith('0:ff:fe00:')

    def is_address_link_local(self, addr):
        # Checks if an IPv6 address is link-local
        return addr.startswith('fe80:')


if __name__ == '__main__':
    unittest.main()
