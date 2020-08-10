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

KEY1 = '00112233445566778899aabbccddeeff'
KEY2 = 'ffeeddccbbaa99887766554433221100'

CHANNEL_INIT = 19
PANID_INIT = 0xface

COMMISSIONER = 1
LEADER = 2
ROUTER1 = 3
ROUTER2 = 4
ED1 = 5
SED1 = 6

MTDS = [ED1, SED1]


class Cert_9_2_18_RollBackActiveTimestamp(thread_cert.TestCase):
    SUPPORT_NCP = False

    TOPOLOGY = {
        COMMISSIONER: {
            'active_dataset': {
                'timestamp': 1,
                'panid': PANID_INIT,
                'channel': CHANNEL_INIT,
                'master_key': '00112233445566778899aabbccddeeff'
            },
            'mode': 'rsdn',
            'router_selection_jitter': 1,
            'whitelist': [LEADER]
        },
        LEADER: {
            'active_dataset': {
                'timestamp': 1,
                'panid': PANID_INIT,
                'channel': CHANNEL_INIT,
                'master_key': '00112233445566778899aabbccddeeff'
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
                'channel': CHANNEL_INIT,
                'master_key': '00112233445566778899aabbccddeeff'
            },
            'mode': 'rsdn',
            'router_selection_jitter': 1,
            'whitelist': [LEADER, ROUTER2, ED1, SED1]
        },
        ROUTER2: {
            'active_dataset': {
                'timestamp': 1,
                'panid': PANID_INIT,
                'channel': CHANNEL_INIT,
                'master_key': '00112233445566778899aabbccddeeff'
            },
            'mode': 'rsdn',
            'router_selection_jitter': 1,
            'whitelist': [ROUTER1]
        },
        ED1: {
            'channel': CHANNEL_INIT,
            'is_mtd': True,
            'masterkey': '00112233445566778899aabbccddeeff',
            'mode': 'rsn',
            'panid': PANID_INIT,
            'whitelist': [ROUTER1]
        },
        SED1: {
            'channel': CHANNEL_INIT,
            'is_mtd': True,
            'masterkey': '00112233445566778899aabbccddeeff',
            'mode': 's',
            'panid': PANID_INIT,
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

        self.nodes[COMMISSIONER].send_mgmt_active_set(active_timestamp=20000, network_name='GRL')
        self.simulator.go(5)

        self.nodes[COMMISSIONER].send_mgmt_pending_set(
            pending_timestamp=20,
            active_timestamp=20,
            delay_timer=20000,
            network_name='Shouldnotbe',
        )
        self.simulator.go(5)

        self.nodes[COMMISSIONER].send_mgmt_pending_set(
            pending_timestamp=20,
            active_timestamp=20,
            delay_timer=20000,
            network_name='MyHouse',
            master_key=KEY2,
        )
        self.simulator.go(310)

        self.assertEqual(self.nodes[COMMISSIONER].get_masterkey(), KEY2)
        self.assertEqual(self.nodes[LEADER].get_masterkey(), KEY2)
        self.assertEqual(self.nodes[ROUTER1].get_masterkey(), KEY2)
        self.assertEqual(self.nodes[ED1].get_masterkey(), KEY2)
        self.assertEqual(self.nodes[SED1].get_masterkey(), KEY2)
        self.assertEqual(self.nodes[ROUTER2].get_masterkey(), KEY1)

        self.nodes[ROUTER2].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER2].get_state(), 'leader')


if __name__ == '__main__':
    unittest.main()
