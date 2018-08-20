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

CHANNEL_INIT = 19
PANID_INIT = 0xface

CHANNEL_FINAL = 19
PANID_FINAL = 0xabcd

COMMISSIONER = 1
LEADER = 2
ROUTER1 = 3
ROUTER2 = 4

class Cert_9_2_09_PendingPartition(unittest.TestCase):
    def setUp(self):
        self.simulator = config.create_default_simulator()

        self.nodes = {}
        for i in range(1,5):
            self.nodes[i] = node.Node(i, simulator=self.simulator)

        self.nodes[COMMISSIONER].set_active_dataset(10, channel=CHANNEL_INIT, panid=PANID_INIT)
        self.nodes[COMMISSIONER].set_mode('rsdn')
        self.nodes[COMMISSIONER].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[COMMISSIONER].enable_whitelist()
        self.nodes[COMMISSIONER].set_router_selection_jitter(1)

        self.nodes[LEADER].set_active_dataset(10, channel=CHANNEL_INIT, panid=PANID_INIT)
        self.nodes[LEADER].set_mode('rsdn')
        self.nodes[LEADER].set_partition_id(0xffffffff)
        self.nodes[LEADER].add_whitelist(self.nodes[COMMISSIONER].get_addr64())
        self.nodes[LEADER].add_whitelist(self.nodes[ROUTER1].get_addr64())
        self.nodes[LEADER].enable_whitelist()
        self.nodes[LEADER].set_router_selection_jitter(1)

        self.nodes[ROUTER1].set_active_dataset(10, channel=CHANNEL_INIT, panid=PANID_INIT)
        self.nodes[ROUTER1].set_mode('rsdn')
        self.nodes[ROUTER1].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[ROUTER1].add_whitelist(self.nodes[ROUTER2].get_addr64())
        self.nodes[ROUTER1].enable_whitelist()
        self.nodes[ROUTER1].set_router_selection_jitter(1)

        self.nodes[ROUTER2].set_active_dataset(10, channel=CHANNEL_INIT, panid=PANID_INIT)
        self.nodes[ROUTER2].set_mode('rsdn')
        self.nodes[ROUTER2].add_whitelist(self.nodes[ROUTER1].get_addr64())
        self.nodes[ROUTER2].enable_whitelist()
        self.nodes[ROUTER2].set_router_selection_jitter(1)
        self.nodes[ROUTER2].set_network_id_timeout(100)

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

        self.nodes[ROUTER1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER1].get_state(), 'router')

        self.nodes[ROUTER2].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER2].get_state(), 'router')

        self.nodes[COMMISSIONER].send_mgmt_pending_set(pending_timestamp=30,
                                                       active_timestamp=210,
                                                       delay_timer=500000,
                                                       channel=20,
                                                       panid=0xafce)
        self.simulator.go(5)

        self.nodes[LEADER].remove_whitelist(self.nodes[ROUTER1].get_addr64())
        self.nodes[ROUTER1].remove_whitelist(self.nodes[LEADER].get_addr64())
        self.simulator.go(140)

        self.assertEqual(self.nodes[ROUTER1].get_state(), 'router')
        self.assertEqual(self.nodes[ROUTER2].get_state(), 'leader')

        self.nodes[ROUTER2].send_mgmt_pending_set(pending_timestamp=50,
                                                  active_timestamp=410,
                                                  delay_timer=200000,
                                                  channel=CHANNEL_FINAL,
                                                  panid=PANID_FINAL)
        self.simulator.go(5)

        self.nodes[LEADER].add_whitelist(self.nodes[ROUTER1].get_addr64())
        self.nodes[ROUTER1].add_whitelist(self.nodes[LEADER].get_addr64())
        self.simulator.go(200)

        self.assertEqual(self.nodes[ROUTER1].get_state(), 'router')
        self.assertEqual(self.nodes[ROUTER2].get_state(), 'router')

        self.assertEqual(self.nodes[COMMISSIONER].get_panid(), PANID_FINAL)
        self.assertEqual(self.nodes[LEADER].get_panid(), PANID_FINAL)
        self.assertEqual(self.nodes[ROUTER1].get_panid(), PANID_FINAL)
        self.assertEqual(self.nodes[ROUTER2].get_panid(), PANID_FINAL)

        self.assertEqual(self.nodes[COMMISSIONER].get_channel(), CHANNEL_FINAL)
        self.assertEqual(self.nodes[LEADER].get_channel(), CHANNEL_FINAL)
        self.assertEqual(self.nodes[ROUTER1].get_channel(), CHANNEL_FINAL)
        self.assertEqual(self.nodes[ROUTER2].get_channel(), CHANNEL_FINAL)

        ipaddrs = self.nodes[ROUTER2].get_addrs()
        for ipaddr in ipaddrs:
            if ipaddr[0:4] != 'fe80':
                break

        self.assertTrue(self.nodes[LEADER].ping(ipaddr))

if __name__ == '__main__':
    unittest.main()
