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

import config
import thread_cert

CHANNEL_INIT = 19
PANID_INIT = 0xface

CHANNEL_FINAL = 16
PANID_FINAL = 0xafce

COMMISSIONER = 1
LEADER = 2
ROUTER1 = 3
ED1 = 4
SED1 = 5

MTDS = [ED1, SED1]


class Cert_9_2_10_PendingPartition(thread_cert.TestCase):
    support_ncp = False

    topology = {
        COMMISSIONER: {
            'active_dataset': {
                'timestamp': 15,
                'panid': 0xface,
                'channel': 19
            },
            'mode': 'rsdn',
            'router_selection_jitter': 1,
            'whitelist': [LEADER]
        },
        LEADER: {
            'active_dataset': {
                'timestamp': 15,
                'panid': 0xface,
                'channel': 19
            },
            'mode': 'rsdn',
            'partition_id': 0xffffffff,
            'router_selection_jitter': 1,
            'whitelist': [COMMISSIONER, ROUTER1]
        },
        ROUTER1: {
            'active_dataset': {
                'timestamp': 15,
                'panid': 0xface,
                'channel': 19
            },
            'mode': 'rsdn',
            'router_selection_jitter': 1,
            'whitelist': [LEADER, ED1, SED1]
        },
        ED1: {
            'channel': 19,
            'is_mtd': True,
            'mode': 'rsn',
            'panid': 0xface,
            'whitelist': [ROUTER1]
        },
        SED1: {
            'channel': 19,
            'is_mtd': True,
            'mode': 's',
            'panid': 0xface,
            'timeout': config.DEFAULT_CHILD_TIMEOUT,
            'whitelist': [ROUTER1]
        },
    }

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

        self.nodes[ED1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ED1].get_state(), 'child')

        self.nodes[SED1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[SED1].get_state(), 'child')

        self.nodes[COMMISSIONER].send_mgmt_pending_set(
            pending_timestamp=30,
            active_timestamp=165,
            delay_timer=150000,
            channel=CHANNEL_FINAL,
            panid=PANID_FINAL,
        )
        self.simulator.go(5)

        print(self.nodes[COMMISSIONER].get_channel())
        print(self.nodes[LEADER].get_channel())
        print(self.nodes[ROUTER1].get_channel())
        print(self.nodes[ED1].get_channel())
        print(self.nodes[SED1].get_channel())

        self.nodes[LEADER].remove_whitelist(self.nodes[ROUTER1].get_addr64())
        self.nodes[ROUTER1].remove_whitelist(self.nodes[LEADER].get_addr64())
        self.simulator.go(160)

        print(self.nodes[COMMISSIONER].get_channel())
        print(self.nodes[LEADER].get_channel())
        print(self.nodes[ROUTER1].get_channel())
        print(self.nodes[ED1].get_channel())
        print(self.nodes[SED1].get_channel())

        self.assertEqual(self.nodes[ROUTER1].get_state(), 'leader')
        self.assertEqual(self.nodes[ED1].get_state(), 'child')
        self.assertEqual(self.nodes[SED1].get_state(), 'child')

        self.assertEqual(self.nodes[ROUTER1].get_panid(), PANID_FINAL)
        self.assertEqual(self.nodes[ED1].get_panid(), PANID_FINAL)
        self.assertEqual(self.nodes[SED1].get_panid(), PANID_FINAL)

        self.assertEqual(self.nodes[ROUTER1].get_channel(), CHANNEL_FINAL)
        self.assertEqual(self.nodes[ED1].get_channel(), CHANNEL_FINAL)
        self.assertEqual(self.nodes[SED1].get_channel(), CHANNEL_FINAL)

        self.nodes[LEADER].add_whitelist(self.nodes[ROUTER1].get_addr64())
        self.nodes[ROUTER1].add_whitelist(self.nodes[LEADER].get_addr64())
        self.simulator.go(60)

        self.assertEqual(self.nodes[COMMISSIONER].get_state(), 'router')
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')
        self.assertEqual(self.nodes[ROUTER1].get_state(), 'router')
        self.assertEqual(self.nodes[ED1].get_state(), 'child')
        self.assertEqual(self.nodes[SED1].get_state(), 'child')

        ipaddrs = self.nodes[ED1].get_addrs()
        for ipaddr in ipaddrs:
            if ipaddr[0:4] != 'fe80':
                break

        self.assertTrue(self.nodes[LEADER].ping(ipaddr))


if __name__ == '__main__':
    unittest.main()
