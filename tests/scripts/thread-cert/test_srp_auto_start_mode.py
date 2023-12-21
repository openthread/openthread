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

import command
import config
import thread_cert

# Test description:
#   This test verifies SRP client auto-start functionality that SRP client can
#   correctly discovers and connects to SRP server.
#
# Topology:
#
#   Four routers, one acting as SRP client, others as SRP server.
#

CLIENT = 1
SERVER1 = 2
SERVER2 = 3
SERVER3 = 4


class SrpAutoStartMode(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False
    SUPPORT_NCP = False

    TOPOLOGY = {
        CLIENT: {
            'name': 'SRP_CLIENT',
            'mode': 'rdn',
        },
        SERVER1: {
            'name': 'SRP_SERVER1',
            'mode': 'rdn',
        },
        SERVER2: {
            'name': 'SRP_SERVER2',
            'mode': 'rdn',
        },
        SERVER3: {
            'name': 'SRP_SERVER3',
            'mode': 'rdn',
        },
    }

    def test(self):
        client = self.nodes[CLIENT]
        server1 = self.nodes[SERVER1]
        server2 = self.nodes[SERVER2]
        server3 = self.nodes[SERVER3]

        #-------------------------------------------------------------------
        # Form the network.

        client.srp_server_set_enabled(False)
        client.start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual(client.get_state(), 'leader')

        server1.start()
        server2.start()
        server3.start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual(server1.get_state(), 'router')
        self.assertEqual(server2.get_state(), 'router')
        self.assertEqual(server3.get_state(), 'router')

        server1_mleid = server1.get_mleid()
        server2_mleid = server2.get_mleid()
        server3_mleid = server3.get_mleid()
        anycast_port = 53

        #-------------------------------------------------------------------
        # Enable server1 with unicast address mode

        server1.srp_server_set_addr_mode('unicast')
        server1.srp_server_set_enabled(True)
        self.simulator.go(5)

        #-------------------------------------------------------------------
        # Check auto start mode on client and check that server1 is selected

        self.assertEqual(client.srp_client_get_auto_start_mode(), 'Enabled')
        self.simulator.go(2)

        self.assertEqual(client.srp_client_get_state(), 'Enabled')
        self.assertEqual(client.srp_client_get_server_address(), server1_mleid)

        #-------------------------------------------------------------------
        # Disable server1 and check client is stopped/disabled.

        server1.srp_server_set_enabled(False)
        self.simulator.go(5)
        self.assertEqual(client.srp_client_get_state(), 'Disabled')

        #-------------------------------------------------------------------
        # Enable server2 with unicast address mode and check client starts
        # again.

        server1.srp_server_set_addr_mode('unicast')
        server2.srp_server_set_enabled(True)
        self.simulator.go(5)
        self.assertEqual(client.srp_client_get_state(), 'Enabled')
        self.assertEqual(client.srp_client_get_server_address(), server2_mleid)

        #-------------------------------------------------------------------
        # Enable server1 and check that client stays with server2

        server1.srp_server_set_enabled(True)
        self.simulator.go(5)
        self.assertEqual(client.srp_client_get_state(), 'Enabled')
        self.assertEqual(client.srp_client_get_server_address(), server2_mleid)

        #-------------------------------------------------------------------
        # Disable server2 and check client switches to server1.

        server2.srp_server_set_enabled(False)
        self.simulator.go(5)
        self.assertEqual(client.srp_client_get_state(), 'Enabled')
        self.assertEqual(client.srp_client_get_server_address(), server1_mleid)

        #-------------------------------------------------------------------
        # Enable server2 with anycast mode seq-num 1, and check that client
        # switched to it.

        server2.srp_server_set_addr_mode('anycast')
        server2.srp_server_set_anycast_seq_num(1)
        server2.srp_server_set_enabled(True)
        self.simulator.go(5)
        server2_alocs = server2.get_ip6_address(config.ADDRESS_TYPE.ALOC)
        self.assertEqual(server2.srp_server_get_anycast_seq_num(), 1)
        self.assertEqual(client.srp_client_get_state(), 'Enabled')
        self.assertIn(client.srp_client_get_server_address(), server2_alocs)
        self.assertEqual(client.srp_client_get_server_port(), anycast_port)

        #-------------------------------------------------------------------
        # Enable server3 with anycast mode seq-num 2, and check that client
        # switched to it since seq number is higher.

        server3.srp_server_set_addr_mode('anycast')
        server3.srp_server_set_anycast_seq_num(2)
        server3.srp_server_set_enabled(True)
        self.simulator.go(5)
        server3_alocs = server3.get_ip6_address(config.ADDRESS_TYPE.ALOC)
        self.assertEqual(server3.srp_server_get_anycast_seq_num(), 2)
        self.assertEqual(client.srp_client_get_state(), 'Enabled')
        self.assertIn(client.srp_client_get_server_address(), server3_alocs)
        self.assertEqual(client.srp_client_get_server_port(), anycast_port)

        #-------------------------------------------------------------------
        # Disable server3 and check that client goes back to server2.

        server3.srp_server_set_enabled(False)
        self.simulator.go(5)
        self.assertEqual(client.srp_client_get_state(), 'Enabled')
        self.assertIn(client.srp_client_get_server_address(), server2_alocs)
        self.assertEqual(client.srp_client_get_server_port(), anycast_port)

        #-------------------------------------------------------------------
        # Enable server3 with anycast mode seq-num 0 (which is smaller than
        # server2 seq-num 1) and check that client stays with server2.

        server3.srp_server_set_anycast_seq_num(0)
        server3.srp_server_set_enabled(True)
        self.simulator.go(5)
        self.assertEqual(server3.srp_server_get_anycast_seq_num(), 0)
        self.assertEqual(client.srp_client_get_state(), 'Enabled')
        self.assertIn(client.srp_client_get_server_address(), server2_alocs)
        self.assertEqual(client.srp_client_get_server_port(), anycast_port)

        #-------------------------------------------------------------------
        # Disable server2 and check that client goes back to server3.

        server2.srp_server_set_enabled(False)
        self.simulator.go(5)
        self.assertEqual(client.srp_client_get_state(), 'Enabled')
        server3_alocs = server3.get_ip6_address(config.ADDRESS_TYPE.ALOC)
        self.assertIn(client.srp_client_get_server_address(), server3_alocs)
        self.assertEqual(client.srp_client_get_server_port(), anycast_port)

        #-------------------------------------------------------------------
        # Disable server3 and check that client goes back to server1 with
        # unicast address.

        server3.srp_server_set_enabled(False)
        self.simulator.go(5)
        self.assertEqual(client.srp_client_get_state(), 'Enabled')
        self.assertEqual(client.srp_client_get_server_address(), server1_mleid)

        #-------------------------------------------------------------------
        # Enable server2 with anycast mode seq-num 5, and check that client
        # switched to it.

        server2.srp_server_set_addr_mode('anycast')
        server2.srp_server_set_anycast_seq_num(5)
        server2.srp_server_set_enabled(True)
        self.simulator.go(5)
        server2_alocs = server2.get_ip6_address(config.ADDRESS_TYPE.ALOC)
        self.assertEqual(server2.srp_server_get_anycast_seq_num(), 5)
        self.assertEqual(client.srp_client_get_state(), 'Enabled')
        self.assertIn(client.srp_client_get_server_address(), server2_alocs)
        self.assertEqual(client.srp_client_get_server_port(), anycast_port)

        #-------------------------------------------------------------------
        # Publish an entry on server3 with specific unicast address
        # This entry should be now preferred over anycast of server2.

        unicast_addr3 = 'fd00:0:0:0:0:3333:beef:cafe'
        unicast_port3 = 1234
        server3.netdata_publish_dnssrp_unicast(unicast_addr3, unicast_port3)
        self.simulator.go(65)
        self.assertEqual(client.srp_client_get_state(), 'Enabled')
        self.assertEqual(client.srp_client_get_server_address(), unicast_addr3)
        self.assertEqual(client.srp_client_get_server_port(), unicast_port3)

        #-------------------------------------------------------------------
        # Publish an entry on server1 with specific unicast address
        # Client should still stay with server3 which it originally selected.

        unicast_addr1 = 'fd00:0:0:0:0:2222:beef:cafe'
        unicast_port1 = 10203
        server1.srp_server_set_enabled(False)
        server1.netdata_publish_dnssrp_unicast(unicast_addr1, unicast_port1)
        self.simulator.go(65)
        self.assertEqual(client.srp_client_get_state(), 'Enabled')
        self.assertEqual(client.srp_client_get_server_address(), unicast_addr3)
        self.assertEqual(client.srp_client_get_server_port(), unicast_port3)

        #-------------------------------------------------------------------
        # Unpublish the entry on server3. Now client should switch to entry
        # from server1.

        server3.netdata_unpublish_dnssrp()
        self.simulator.go(65)
        self.assertEqual(client.srp_client_get_state(), 'Enabled')
        self.assertEqual(client.srp_client_get_server_address(), unicast_addr1)
        self.assertEqual(client.srp_client_get_server_port(), unicast_port1)

        #-------------------------------------------------------------------
        # Unpublish the entry on server1 and check client goes back to anycast
        # entry from server2.

        server1.netdata_unpublish_dnssrp()
        self.simulator.go(65)
        self.assertEqual(client.srp_client_get_state(), 'Enabled')
        self.assertIn(client.srp_client_get_server_address(), server2_alocs)
        self.assertEqual(client.srp_client_get_server_port(), anycast_port)

        #-------------------------------------------------------------------
        # Finally disable server2, and check that client is disabled.

        server2.srp_server_set_enabled(False)
        self.simulator.go(5)
        self.assertEqual(client.srp_client_get_state(), 'Disabled')


if __name__ == '__main__':
    unittest.main()
