#!/usr/bin/env python3
#
#  Copyright (c) 2024, The OpenThread Authors.
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

import ipaddress
import unittest
import math

import command
import config
import thread_cert

# Test description:
#
#   This test verifies key rotation and key guard time mechanisms.
#
#
# Topology:
#
#   leader ---  router
#    |    \
#    |     \
#  child   reed
#

LEADER = 1
CHILD = 2
REED = 3
ROUTER = 4


class MleMsgKeySeqJump(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False
    SUPPORT_NCP = False

    TOPOLOGY = {
        LEADER: {
            'name': 'LEADER',
            'mode': 'rdn',
        },
        CHILD: {
            'name': 'CHILD',
            'is_mtd': True,
            'mode': 'rn',
        },
        REED: {
            'name': 'REED',
            'mode': 'rn'
        },
        ROUTER: {
            'name': 'ROUTER',
            'mode': 'rdn',
        },
    }

    def test(self):
        leader = self.nodes[LEADER]
        child = self.nodes[CHILD]
        reed = self.nodes[REED]
        router = self.nodes[ROUTER]

        nodes = [leader, child, reed, router]

        #-------------------------------------------------------------------
        # Form the network.

        for node in nodes:
            node.set_key_sequence_counter(0)

        leader.start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual(leader.get_state(), 'leader')

        child.start()
        reed.start()
        self.simulator.go(5)
        self.assertEqual(child.get_state(), 'child')
        self.assertEqual(reed.get_state(), 'child')

        router.start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual(router.get_state(), 'router')

        #-------------------------------------------------------------------
        # Validate the initial key seq counter and key switch guard time

        for node in nodes:
            self.assertEqual(node.get_key_sequence_counter(), 0)
            self.assertEqual(node.get_key_switch_guardtime(), 624)

        #-------------------------------------------------------------------
        # Change the key rotation time a bunch of times and make sure that
        # the key switch guard time is properly changed (should be set
        # to 93% of the rotation time).

        for rotation_time in [100, 1, 10, 888, 2]:
            reed.start_dataset_updater(security_policy=[rotation_time, 'onrc'])
            guardtime = math.floor(rotation_time * 93 / 100) if rotation_time >= 2 else 1
            self.simulator.go(100)
            for node in nodes:
                self.assertEqual(node.get_key_switch_guardtime(), guardtime)

        #-------------------------------------------------------------------
        # Wait for key rotation time (2 hours) and check that all nodes
        # moved to the next key seq counter

        self.simulator.go(2 * 60 * 60)
        for node in nodes:
            self.assertEqual(node.get_key_sequence_counter(), 1)

        #-------------------------------------------------------------------
        # Manually increment the key sequence counter on leader and make
        # sure other nodes are not updated due to key guard time.

        router.set_key_sequence_counter(2)

        self.simulator.go(50 * 60)

        self.assertEqual(router.get_key_sequence_counter(), 2)

        for node in [leader, reed, child]:
            self.assertEqual(node.get_key_sequence_counter(), 1)

        #-------------------------------------------------------------------
        # Make sure nodes can communicate with each other.

        self.assertTrue(leader.ping(router.get_mleid()))
        self.assertTrue(router.ping(child.get_mleid()))

        #-------------------------------------------------------------------
        # Wait for rotation time to expire. Validate that the `router`
        # has moved to key seq `3` and all other nodes also followed.

        self.simulator.go(75 * 60)

        self.assertEqual(router.get_key_sequence_counter(), 3)

        for node in nodes:
            self.assertEqual(node.get_key_sequence_counter(), 3)

        self.assertTrue(leader.ping(router.get_mleid()))
        self.assertTrue(router.ping(child.get_mleid()))


if __name__ == '__main__':
    unittest.main()
