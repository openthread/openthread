#!/usr/bin/env python3
#
#  Copyright (c) 2022, The OpenThread Authors.
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

import command
import config
import thread_cert

# Test description:
#
#   This test verifies behavior of MLE related to handling of received
#   larger key sequence based on the MLE message class (authoritative,
#   or peer).
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
        # Validate the initial key seq counter on all nodes

        for node in nodes:
            self.assertEqual(node.get_key_sequence_counter(), 0)

        #-------------------------------------------------------------------
        # Manually increase the key seq on child. Then change MLE mode on
        # child which triggers a "Child Update Request" to its parent
        # (leader). The key jump noticed on parent side would trigger an
        # authoritative MLE Child Update exchange (including challenge and
        # response TLVs) and causes the parent (leader) to also adopt the
        # larger key seq.

        child.set_key_sequence_counter(5)
        self.assertEqual(child.get_key_sequence_counter(), 5)

        child.set_mode('r')
        self.simulator.go(1)

        self.assertEqual(child.get_key_sequence_counter(), 5)
        self.assertEqual(leader.get_key_sequence_counter(), 5)

        #-------------------------------------------------------------------
        # Wait long enough for MLE Advertisement to be sent. This would
        # trigger reed and router to also notice key seq jump and try to
        # re-establish link again (using authoritative exchanges). Validate
        # that all nodes are using the new key seq.

        self.simulator.go(52)
        for node in nodes:
            self.assertEqual(node.get_key_sequence_counter(), 5)

        #-------------------------------------------------------------------
        # Manually increase the key seq on leader. Wait for max time between
        # advertisements. This would trigger both reed and router
        # to notice key seq jump and try to re-establish link (link
        # request/accept exchange). Validate that they all adopt the new
        # key seq.

        leader.set_key_sequence_counter(10)
        self.assertEqual(leader.get_key_sequence_counter(), 10)

        self.simulator.go(52)

        self.assertEqual(router.get_key_sequence_counter(), 10)
        self.assertEqual(reed.get_key_sequence_counter(), 10)

        #-------------------------------------------------------------------
        # Change MLE mode on child to trigger a "Child Update Request" exchange
        # which should then update the key seq on child as well.

        child.set_mode('rn')
        self.simulator.go(5)
        self.assertEqual(child.get_key_sequence_counter(), 10)

        #-------------------------------------------------------------------
        # Stop all other nodes except for leader. Move the leader key seq
        # forward and then restart all other node. Validate that router,
        # reed and child all re-attach successfully and adopt the higher key
        # sequence.

        router.stop()
        reed.stop()
        child.stop()

        leader.set_key_sequence_counter(15)
        self.assertEqual(leader.get_key_sequence_counter(), 15)

        child.start()
        reed.start()
        router.start()
        self.simulator.go(5)

        self.assertEqual(child.get_state(), 'child')
        self.assertEqual(reed.get_state(), 'child')
        self.assertEqual(router.get_state(), 'router')

        for node in nodes:
            self.assertEqual(node.get_key_sequence_counter(), 15)

        #-------------------------------------------------------------------
        # Stop all other nodes except for leader. Move the child key seq
        # forward and then restart child. Ensure it re-attached successfully
        # to leader and that leader adopts the higher key seq counter.

        router.stop()
        reed.stop()
        child.stop()

        child.set_key_sequence_counter(20)
        self.assertEqual(child.get_key_sequence_counter(), 20)

        child.start()
        self.simulator.go(5)

        self.assertEqual(child.get_state(), 'child')
        self.assertEqual(leader.get_key_sequence_counter(), 20)

        #-------------------------------------------------------------------
        # Restart router and reed and ensure they are re-attached and get the
        # higher key seq counter.

        router.start()
        reed.start()

        self.simulator.go(5)
        self.assertEqual(router.get_state(), 'router')
        self.assertEqual(reed.get_state(), 'child')

        self.assertEqual(router.get_key_sequence_counter(), 20)
        self.assertEqual(reed.get_key_sequence_counter(), 20)

        #-------------------------------------------------------------------
        # Move forward the key seq counter by one on router. Wait for max
        # time between advertisements. Validate that leader adopts the higher
        # counter value.

        router.set_key_sequence_counter(21)
        self.assertEqual(router.get_key_sequence_counter(), 21)

        self.simulator.go(52)
        self.assertEqual(leader.get_key_sequence_counter(), 21)
        self.assertEqual(reed.get_key_sequence_counter(), 21)

        child.set_mode('r')
        self.simulator.go(2)
        self.assertEqual(child.get_key_sequence_counter(), 21)

        #-------------------------------------------------------------------
        # Force a reattachment from the child with a higher key seq counter,
        # so that the leader generated a fragmented Child Id Response. Ensure
        # the child becomes attached on first attempt while the leader adopts
        # the higher counter value.

        router.stop()
        reed.stop()

        child.factory_reset()
        self.assertEqual(child.get_state(), 'disabled')

        child.set_active_dataset(channel=leader.get_channel(),
                                 network_key=leader.get_networkkey(),
                                 panid=leader.get_panid())
        child.set_key_sequence_counter(25)
        self.assertEqual(child.get_key_sequence_counter(), 25)

        child.start()
        self.simulator.go(2)

        self.assertEqual(child.get_state(), 'child')
        self.assertEqual(leader.get_key_sequence_counter(), 25)


if __name__ == '__main__':
    unittest.main()
