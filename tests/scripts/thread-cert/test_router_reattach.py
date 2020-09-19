#!/usr/bin/env python3
#
#  Copyright (c) 2020, The OpenThread Authors.
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


class test_router_reattach(thread_cert.TestCase):
    TOPOLOGY = {
        1: {
            'mode':
                'rsdn',
            'panid':
                0xface,
            'router_downgrade_threshold':
                32,
            'router_selection_jitter':
                1,
            'router_upgrade_threshold':
                32,
            'allowlist': [
                2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
                30, 31, 32
            ],
        },
        2: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        3: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        4: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        5: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        6: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        7: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        8: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        9: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        10: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        11: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        12: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        13: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        14: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        15: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        16: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        17: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        18: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        19: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        20: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        21: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        22: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        23: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        24: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        25: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        26: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        27: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        28: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        29: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        30: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        31: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
        32: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32,
            'allowlist': [1],
        },
    }

    def test(self):
        self.nodes[1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[1].get_state(), 'leader')

        for i in range(2, 33):
            self.nodes[i].start()
            self.simulator.go(5)
            self.assertEqual(self.nodes[i].get_state(), 'router')

        self.nodes[2].reset()
        self.nodes[2].set_router_selection_jitter(3)
        self.nodes[2].set_router_upgrade_threshold(32)
        self.nodes[2].set_router_downgrade_threshold(32)

        self.nodes[2].start()
        self.assertEqual(self.nodes[2].get_router_downgrade_threshold(), 32)
        # Verify that the node restored as Router.
        self.simulator.go(1)
        self.assertEqual(self.nodes[2].get_state(), 'router')
        # Verify that the node does not downgrade after Router Selection Jitter.
        self.simulator.go(5)
        self.assertEqual(self.nodes[2].get_state(), 'router')


if __name__ == '__main__':
    unittest.main()
