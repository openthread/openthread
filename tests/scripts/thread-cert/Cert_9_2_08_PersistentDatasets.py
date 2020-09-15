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

COMMISSIONER = 1
LEADER = 2
ROUTER = 3
ED = 4
SED = 5

CHANNEL_INIT = 19
PANID_INIT = 0xface
LEADER_ACTIVE_TIMESTAMP = 10

COMMISSIONER_PENDING_CHANNEL = 20
COMMISSIONER_PENDING_PANID = 0xafce

MTDS = [ED, SED]


class Cert_9_2_8_PersistentDatasets(thread_cert.TestCase):
    SUPPORT_NCP = False

    TOPOLOGY = {
        COMMISSIONER: {
            'active_dataset': {
                'timestamp': LEADER_ACTIVE_TIMESTAMP,
                'panid': PANID_INIT,
                'channel': CHANNEL_INIT
            },
            'mode': 'rsdn',
            'router_selection_jitter': 1,
            'allowlist': [LEADER]
        },
        LEADER: {
            'active_dataset': {
                'timestamp': LEADER_ACTIVE_TIMESTAMP,
                'panid': PANID_INIT,
                'channel': CHANNEL_INIT
            },
            'mode': 'rsdn',
            'allowlist': [COMMISSIONER, ROUTER, ED, SED]
        },
        ROUTER: {
            'active_dataset': {
                'timestamp': LEADER_ACTIVE_TIMESTAMP,
                'panid': PANID_INIT,
                'channel': CHANNEL_INIT
            },
            'mode': 'rsdn',
            'router_selection_jitter': 1,
            'allowlist': [LEADER]
        },
        ED: {
            'channel': CHANNEL_INIT,
            'is_mtd': True,
            'mode': 'rsn',
            'panid': PANID_INIT,
            'timeout': config.DEFAULT_CHILD_TIMEOUT,
            'allowlist': [LEADER]
        },
        SED: {
            'channel': CHANNEL_INIT,
            'is_mtd': True,
            'mode': 's',
            'panid': PANID_INIT,
            'timeout': config.DEFAULT_CHILD_TIMEOUT,
            'allowlist': [LEADER]
        },
    }

    def _setUpRouter(self):
        self.nodes[ROUTER].add_allowlist(self.nodes[LEADER].get_addr64())
        self.nodes[ROUTER].enable_allowlist()
        self.nodes[ROUTER].set_router_selection_jitter(1)

    def _setUpEd(self):
        self.nodes[ED].add_allowlist(self.nodes[LEADER].get_addr64())
        self.nodes[ED].enable_allowlist()
        self.nodes[ED].set_timeout(config.DEFAULT_CHILD_TIMEOUT)

    def _setUpSed(self):
        self.nodes[SED].add_allowlist(self.nodes[LEADER].get_addr64())
        self.nodes[SED].enable_allowlist()
        self.nodes[SED].set_timeout(config.DEFAULT_CHILD_TIMEOUT)

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[COMMISSIONER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[COMMISSIONER].get_state(), 'router')

        self.nodes[ROUTER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER].get_state(), 'router')

        self.nodes[ED].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ED].get_state(), 'child')

        self.nodes[SED].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[SED].get_state(), 'child')

        self.nodes[COMMISSIONER].commissioner_start()
        self.simulator.go(3)

        self.nodes[COMMISSIONER].send_mgmt_pending_set(
            pending_timestamp=10,
            active_timestamp=70,
            delay_timer=60000,
            channel=COMMISSIONER_PENDING_CHANNEL,
            panid=COMMISSIONER_PENDING_PANID,
        )
        self.simulator.go(5)

        self.nodes[ROUTER].reset()
        self.nodes[ED].reset()
        self.nodes[SED].reset()

        self.simulator.go(60)

        self.assertEqual(self.nodes[LEADER].get_panid(), COMMISSIONER_PENDING_PANID)
        self.assertEqual(self.nodes[COMMISSIONER].get_panid(), COMMISSIONER_PENDING_PANID)

        self.assertEqual(self.nodes[LEADER].get_channel(), COMMISSIONER_PENDING_CHANNEL)
        self.assertEqual(
            self.nodes[COMMISSIONER].get_channel(),
            COMMISSIONER_PENDING_CHANNEL,
        )

        # reset the devices here again to simulate the fact that the devices
        # were disabled the entire time
        self.nodes[ROUTER].reset()
        self._setUpRouter()
        self.nodes[ROUTER].start()
        self.nodes[ED].reset()
        self._setUpEd()
        self.nodes[ED].start()
        self.nodes[SED].reset()
        self._setUpSed()
        self.nodes[SED].start()

        self.assertEqual(self.nodes[ROUTER].get_panid(), PANID_INIT)
        self.assertEqual(self.nodes[ED].get_panid(), PANID_INIT)
        self.assertEqual(self.nodes[SED].get_panid(), PANID_INIT)

        self.assertEqual(self.nodes[ROUTER].get_channel(), CHANNEL_INIT)
        self.assertEqual(self.nodes[ED].get_channel(), CHANNEL_INIT)
        self.assertEqual(self.nodes[SED].get_channel(), CHANNEL_INIT)

        self.simulator.go(10)

        self.assertEqual(self.nodes[ROUTER].get_panid(), COMMISSIONER_PENDING_PANID)
        self.assertEqual(self.nodes[ED].get_panid(), COMMISSIONER_PENDING_PANID)
        self.assertEqual(self.nodes[SED].get_panid(), COMMISSIONER_PENDING_PANID)

        self.assertEqual(self.nodes[ROUTER].get_channel(), COMMISSIONER_PENDING_CHANNEL)
        self.assertEqual(self.nodes[ED].get_channel(), COMMISSIONER_PENDING_CHANNEL)
        self.assertEqual(self.nodes[SED].get_channel(), COMMISSIONER_PENDING_CHANNEL)

        self.simulator.go(5)

        ipaddrs = self.nodes[ROUTER].get_addrs()
        for ipaddr in ipaddrs:
            if ipaddr[0:4] != 'fe80':
                self.assertTrue(self.nodes[LEADER].ping(ipaddr))

        ipaddrs = self.nodes[ED].get_addrs()
        for ipaddr in ipaddrs:
            if ipaddr[0:4] != 'fe80':
                self.assertTrue(self.nodes[LEADER].ping(ipaddr))

        ipaddrs = self.nodes[SED].get_addrs()
        for ipaddr in ipaddrs:
            if ipaddr[0:4] != 'fe80':
                self.assertTrue(self.nodes[LEADER].ping(ipaddr))


if __name__ == '__main__':
    unittest.main()
