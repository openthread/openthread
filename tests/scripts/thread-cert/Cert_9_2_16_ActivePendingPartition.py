#!/usr/bin/env python3
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

import unittest

import thread_cert

CHANNEL_INIT = 19
PANID_INIT = 0xface

NETWORK_NAME_FINAL = 'threadCert'
PANID_FINAL = 0xabcd

COMMISSIONER = 1
LEADER = 2
ROUTER1 = 3
ROUTER2 = 4


class Cert_9_2_16_ActivePendingPartition(thread_cert.TestCase):
    SUPPORT_NCP = False

    TOPOLOGY = {
        COMMISSIONER: {
            'active_dataset': {
                'timestamp': 1,
                'panid': PANID_INIT,
                'channel': CHANNEL_INIT
            },
            'mode': 'rsdn',
            'router_selection_jitter': 1,
            'whitelist': [LEADER]
        },
        LEADER: {
            'active_dataset': {
                'timestamp': 1,
                'panid': PANID_INIT,
                'channel': CHANNEL_INIT
            },
            'mode': 'rsdn',
            'partition_id': 0xffffffff,
            'router_selection_jitter': 1,
            'whitelist': [COMMISSIONER, ROUTER1]
        },
        ROUTER1: {
            'active_dataset': {
                'timestamp': 1,
                'panid': PANID_INIT,
                'channel': CHANNEL_INIT
            },
            'mode': 'rsdn',
            'router_selection_jitter': 1,
            'whitelist': [LEADER, ROUTER2]
        },
        ROUTER2: {
            'active_dataset': {
                'timestamp': 1,
                'panid': PANID_INIT,
                'channel': CHANNEL_INIT
            },
            'mode': 'rsdn',
            'router_selection_jitter': 1,
            'whitelist': [ROUTER1]
        },
    }

    def _setUpRouter2(self):
        self.nodes[ROUTER2].add_whitelist(self.nodes[ROUTER1].get_addr64())
        self.nodes[ROUTER2].enable_whitelist()
        self.nodes[ROUTER2].set_router_selection_jitter(1)

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

        self.nodes[COMMISSIONER].send_mgmt_pending_set(
            pending_timestamp=10,
            active_timestamp=10,
            delay_timer=600000,
            mesh_local='fd00:0db9::',
        )
        self.simulator.go(5)

        self.nodes[ROUTER2].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER2].get_state(), 'router')

        self.nodes[ROUTER2].reset()
        self._setUpRouter2()

        self.nodes[COMMISSIONER].send_mgmt_pending_set(
            pending_timestamp=20,
            active_timestamp=20,
            delay_timer=200000,
            mesh_local='fd00:0db7::',
            panid=PANID_FINAL,
        )
        self.simulator.go(5)

        self.nodes[COMMISSIONER].send_mgmt_active_set(active_timestamp=15, network_name='threadCert')
        self.simulator.go(100)

        self.nodes[ROUTER2].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER2].get_state(), 'router')

        self.assertEqual(self.nodes[COMMISSIONER].get_network_name(), NETWORK_NAME_FINAL)
        self.assertEqual(self.nodes[LEADER].get_network_name(), NETWORK_NAME_FINAL)
        self.assertEqual(self.nodes[ROUTER1].get_network_name(), NETWORK_NAME_FINAL)
        self.assertEqual(self.nodes[ROUTER2].get_network_name(), NETWORK_NAME_FINAL)

        self.simulator.go(100)

        self.assertEqual(self.nodes[COMMISSIONER].get_panid(), PANID_FINAL)
        self.assertEqual(self.nodes[LEADER].get_panid(), PANID_FINAL)
        self.assertEqual(self.nodes[ROUTER1].get_panid(), PANID_FINAL)
        self.assertEqual(self.nodes[ROUTER2].get_panid(), PANID_FINAL)

        ipaddrs = self.nodes[ROUTER2].get_addrs()
        for ipaddr in ipaddrs:
            if ipaddr[0:4] != 'fe80':
                break
        self.assertTrue(self.nodes[LEADER].ping(ipaddr))


if __name__ == '__main__':
    unittest.main()
