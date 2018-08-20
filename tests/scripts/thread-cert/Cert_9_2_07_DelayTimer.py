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

PANID_INIT = 0xface

COMMISSIONER = 1
LEADER = 2
ROUTER = 3

LEADER_ACTIVE_TIMESTAMP = 10
ROUTER_ACTIVE_TIMESTAMP = 20
ROUTER_PENDING_TIMESTAMP = 30
ROUTER_PENDING_ACTIVE_TIMESTAMP = 25

COMMISSIONER_PENDING_CHANNEL = 20
COMMISSIONER_PENDING_PANID = 0xafce

class Cert_9_2_7_DelayTimer(unittest.TestCase):
    def setUp(self):
        self.simulator = config.create_default_simulator()

        self.nodes = {}
        for i in range(1,4):
            self.nodes[i] = node.Node(i, simulator=self.simulator)

        self.nodes[COMMISSIONER].set_active_dataset(LEADER_ACTIVE_TIMESTAMP)
        self.nodes[COMMISSIONER].set_mode('rsdn')
        self.nodes[COMMISSIONER].set_panid(PANID_INIT)
        self.nodes[COMMISSIONER].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[COMMISSIONER].enable_whitelist()
        self.nodes[COMMISSIONER].set_router_selection_jitter(1)

        self.nodes[LEADER].set_mode('rsdn')
        self.nodes[LEADER].set_panid(PANID_INIT)
        self.nodes[LEADER].set_partition_id(0xffffffff)
        self.nodes[LEADER].add_whitelist(self.nodes[COMMISSIONER].get_addr64())
        self.nodes[LEADER].enable_whitelist()
        self.nodes[LEADER].set_router_selection_jitter(1)

        self.nodes[ROUTER].set_active_dataset(ROUTER_ACTIVE_TIMESTAMP)
        self.nodes[ROUTER].set_pending_dataset(ROUTER_PENDING_TIMESTAMP, ROUTER_PENDING_ACTIVE_TIMESTAMP)
        self.nodes[ROUTER].set_mode('rsdn')
        self.nodes[ROUTER].set_panid(PANID_INIT)
        self.nodes[ROUTER].set_partition_id(0x1)
        self.nodes[ROUTER].enable_whitelist()
        self.nodes[ROUTER].set_router_selection_jitter(1)

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

        self.nodes[ROUTER].start()
        self.simulator.go(10)
        self.assertEqual(self.nodes[ROUTER].get_state(), 'leader')

        self.nodes[LEADER].add_whitelist(self.nodes[ROUTER].get_addr64())
        self.nodes[ROUTER].add_whitelist(self.nodes[LEADER].get_addr64())

        self.simulator.go(30)
        self.assertEqual(self.nodes[COMMISSIONER].get_state(), 'router')
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')
        self.assertEqual(self.nodes[ROUTER].get_state(), 'router')

        ipaddrs = self.nodes[ROUTER].get_addrs()
        for ipaddr in ipaddrs:
            if ipaddr[0:4] != 'fe80':
                break
        self.assertTrue(self.nodes[LEADER].ping(ipaddr))

        self.nodes[COMMISSIONER].send_mgmt_pending_set(pending_timestamp=40,
                                                       active_timestamp=80,
                                                       delay_timer=10000,
                                                       channel=COMMISSIONER_PENDING_CHANNEL,
                                                       panid=COMMISSIONER_PENDING_PANID)
        self.simulator.go(40)
        self.assertEqual(self.nodes[LEADER].get_panid(), COMMISSIONER_PENDING_PANID)
        self.assertEqual(self.nodes[COMMISSIONER].get_panid(), COMMISSIONER_PENDING_PANID)
        self.assertEqual(self.nodes[ROUTER].get_panid(), COMMISSIONER_PENDING_PANID)

        self.assertEqual(self.nodes[LEADER].get_channel(), COMMISSIONER_PENDING_CHANNEL)
        self.assertEqual(self.nodes[COMMISSIONER].get_channel(), COMMISSIONER_PENDING_CHANNEL)
        self.assertEqual(self.nodes[ROUTER].get_channel(), COMMISSIONER_PENDING_CHANNEL)

        ipaddrs = self.nodes[ROUTER].get_addrs()
        for ipaddr in ipaddrs:
            if ipaddr[0:4] != 'fe80':
                break
        self.assertTrue(self.nodes[LEADER].ping(ipaddr))

if __name__ == '__main__':
    unittest.main()
