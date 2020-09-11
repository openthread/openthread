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
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        2: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        3: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        4: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        5: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        6: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        7: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        8: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        9: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        10: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        11: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        12: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        13: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        14: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        15: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        16: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        17: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        18: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        19: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        20: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        21: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        22: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        23: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        24: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        25: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        26: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        27: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        28: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        29: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        30: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        31: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
        },
        32: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_downgrade_threshold': 32,
            'router_selection_jitter': 1,
            'router_upgrade_threshold': 32
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
