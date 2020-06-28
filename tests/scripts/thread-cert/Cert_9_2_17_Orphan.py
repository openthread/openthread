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

CHANNEL1 = 11
CHANNEL2 = 18
CHANNEL_MASK = 1 << 18
PANID_INIT = 0xface

LEADER1 = 1
LEADER2 = 2
ED1 = 3


class Cert_9_2_17_Orphan(thread_cert.TestCase):
    SUPPORT_NCP = False

    TOPOLOGY = {
        LEADER1: {
            'active_dataset': {
                'timestamp': 10,
                'panid': PANID_INIT,
                'channel': CHANNEL1,
                'channel_mask': CHANNEL_MASK
            },
            'mode': 'rsdn',
            'router_selection_jitter': 1,
            'whitelist': [ED1]
        },
        LEADER2: {
            'active_dataset': {
                'timestamp': 20,
                'panid': PANID_INIT,
                'channel': CHANNEL2,
                'channel_mask': CHANNEL_MASK
            },
            'mode': 'rsdn',
            'router_selection_jitter': 1
        },
        ED1: {
            'channel': CHANNEL1,
            'is_mtd': True,
            'mode': 'rsn',
            'panid': PANID_INIT,
            'timeout': config.DEFAULT_CHILD_TIMEOUT,
            'whitelist': [LEADER1]
        },
    }

    def test(self):
        self.nodes[LEADER1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER1].get_state(), 'leader')

        self.nodes[LEADER2].start()
        self.nodes[LEADER2].set_state('leader')
        self.assertEqual(self.nodes[LEADER2].get_state(), 'leader')

        self.nodes[ED1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ED1].get_state(), 'child')

        self.nodes[LEADER1].stop()
        self.nodes[LEADER2].add_whitelist(self.nodes[ED1].get_addr64())
        self.nodes[ED1].add_whitelist(self.nodes[LEADER2].get_addr64())
        self.simulator.go(20)

        self.assertEqual(self.nodes[ED1].get_state(), 'child')
        self.assertEqual(self.nodes[ED1].get_channel(), CHANNEL2)


if __name__ == '__main__':
    unittest.main()
