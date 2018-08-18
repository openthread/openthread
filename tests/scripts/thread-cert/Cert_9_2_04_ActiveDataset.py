#!/usr/bin/env python
#
#  Copyright (c) 2016, The OpenThread Authors.
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

import time
import unittest

import config
import node

COMMISSIONER = 1
LEADER = 2

class Cert_9_2_15_PendingPartition(unittest.TestCase):
    def setUp(self):
        self.simulator = config.create_default_simulator()

        self.nodes = {}
        for i in range(1,3):
            self.nodes[i] = node.Node(i, simulator=self.simulator)

        self.nodes[COMMISSIONER].set_active_dataset(10, panid=0xface, master_key='000102030405060708090a0b0c0d0e0f')
        self.nodes[COMMISSIONER].set_mode('rsdn')
        self.nodes[COMMISSIONER].set_router_selection_jitter(1)

        self.nodes[LEADER].set_active_dataset(10, panid=0xface, master_key='000102030405060708090a0b0c0d0e0f')
        self.nodes[LEADER].set_mode('rsdn')
        self.nodes[LEADER].set_router_selection_jitter(1)

    def tearDown(self):
        for node in list(self.nodes.values()):
            node.stop()
            node.destroy()
        self.simulator.stop()

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[COMMISSIONER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[COMMISSIONER].get_state(), 'router')

        self.nodes[COMMISSIONER].commissioner_start()
        self.simulator.go(3)

        self.nodes[COMMISSIONER].send_mgmt_active_set(active_timestamp=101,
                                                      channel_mask=0x001fffe0,
                                                      extended_panid='000db70000000000',
                                                      network_name='GRL')
        self.simulator.go(3)
        self.assertEqual(self.nodes[LEADER].get_network_name(), 'GRL')

        # Step 6
        # Attempt to set Channel TLV
        self.nodes[COMMISSIONER].send_mgmt_active_set(active_timestamp=102,
                                                      channel=18,
                                                      channel_mask=0x001fffe0,
                                                      extended_panid='000db70000000001',
                                                      network_name='threadcert')
        self.simulator.go(3)
        self.assertEqual(self.nodes[LEADER].get_network_name(), 'GRL')

        # Step 8
        # Attempt to set Mesh Local Prefix TLV
        self.nodes[COMMISSIONER].send_mgmt_active_set(active_timestamp=103,
                                                      channel_mask=0x001ffee0,
                                                      extended_panid='000db70000000000',
                                                      mesh_local='fd00:0db7::',
                                                      network_name='UL')
        self.simulator.go(3)
        self.assertEqual(self.nodes[LEADER].get_network_name(), 'GRL')

        # Step 10
        # Attempt to set Network Master Key TLV
        self.nodes[COMMISSIONER].send_mgmt_active_set(active_timestamp=104,
                                                      channel_mask=0x001fffe0,
                                                      extended_panid='000db70000000000',
                                                      master_key='00112233445566778899aabbccddeeff',
                                                      mesh_local='fd00:0db7::',
                                                      network_name='UL')
        self.simulator.go(3)
        self.assertEqual(self.nodes[LEADER].get_network_name(), 'GRL')

        # Step 12
        # Attempt to set PAN ID TLV
        self.nodes[COMMISSIONER].send_mgmt_active_set(active_timestamp=105,
                                                      channel_mask=0x001fffe0,
                                                      extended_panid='000db70000000000',
                                                      master_key='00112233445566778899aabbccddeeff',
                                                      mesh_local='fd00:0db7::',
                                                      network_name='UL',
                                                      panid=0xafce)
        self.simulator.go(3)
        self.assertEqual(self.nodes[LEADER].get_network_name(), 'GRL')

        # Step 14
        # Invalid Commissioner Session ID
        self.nodes[COMMISSIONER].send_mgmt_active_set(active_timestamp=106,
                                                      channel_mask=0x001fffe0,
                                                      extended_panid='000db70000000000',
                                                      network_name='UL',
                                                      binary='0b02abcd')
        self.simulator.go(3)
        self.assertEqual(self.nodes[LEADER].get_network_name(), 'GRL')

        # Step 16
        # Old Active Timestamp
        self.nodes[COMMISSIONER].send_mgmt_active_set(active_timestamp=101,
                                                      channel_mask=0x001fffe0,
                                                      extended_panid='000db70000000000',
                                                      network_name='UL')
        self.simulator.go(3)
        self.assertEqual(self.nodes[LEADER].get_network_name(), 'GRL')

        # Step 18
        # Unexpected Steering Data TLV
        self.nodes[COMMISSIONER].send_mgmt_active_set(active_timestamp=107,
                                                      channel_mask=0x001fffe0,
                                                      extended_panid='000db70000000000',
                                                      network_name='UL',
                                                      binary='0806113320440000')
        self.simulator.go(3)
        self.assertEqual(self.nodes[LEADER].get_network_name(), 'UL')

        # Step 20
        # Undefined TLV
        self.nodes[COMMISSIONER].send_mgmt_active_set(active_timestamp=108,
                                                      channel_mask=0x001fffe0,
                                                      extended_panid='000db70000000000',
                                                      network_name='GRL',
                                                      binary='8202aa55')
        self.simulator.go(3)
        self.assertEqual(self.nodes[LEADER].get_network_name(), 'GRL')

        ipaddrs = self.nodes[COMMISSIONER].get_addrs()
        for ipaddr in ipaddrs:
            self.assertTrue(self.nodes[LEADER].ping(ipaddr))

if __name__ == '__main__':
    unittest.main()
