#!/usr/bin/env python
#
#  Copyright (c) 2018, The OpenThread Authors.
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
import mle
import node


LEADER = 1
CHILD = 2


class TestSetDeviceMode(unittest.TestCase):
    """This test case test updating device mode.
    """

    def setUp(self):
        self.simulator = config.create_default_simulator()

        self.nodes = {}
        for i in range(1, 3):
            self.nodes[i] = node.Node(i, simulator=self.simulator)

        self.nodes[LEADER].set_panid(0xface)
        self.nodes[LEADER].set_mode('rsdn')
        self.nodes[LEADER].add_whitelist(self.nodes[CHILD].get_addr64())
        self.nodes[LEADER].enable_whitelist()

        self.nodes[CHILD].set_panid(0xface)
        self.nodes[CHILD].set_mode('rsdn')
        self.nodes[CHILD].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[CHILD].enable_whitelist()
        # prevent this node becoming router
        self.nodes[CHILD].set_router_upgrade_threshold(1)

    def tearDown(self):
        for node in list(self.nodes.values()):
            node.stop()
        del self.nodes
        del self.simulator

    def test(self):
        """This test setting mode without reset devices.

        Steps:
            1. Start node 1 as leader, node 2 as child with mode `rsdn`.
            2. Set node 2 mode to `rs`, verify node 2 will reattach.
            3. Set node 2 mode to `s`, verify node 2 will reattach.
            4. Set node 2 mode to `rs`, verify node 2 will not reattach.
            5. Set node 2 mode to `rsdn`, verify node 2 will not reattach.
            6. Set node 2 mode to `s`, verify node 2 will not reattach.
            7. Set node 2 mode to `rsdn` after reset, verify node 2 will not reattach.
            8. Set node 2 mode to `s` after reset, verify node 2 will not reattach.
        """
        leader = self.nodes[LEADER]
        leader.start()
        leader.set_state('leader')
        self.simulator.go(5)
        self.assertEqual(leader.get_state(), 'leader')

        child = self.nodes[CHILD]
        child.start()
        self.simulator.go(7)

        # clear messages
        messages = self.simulator.get_messages_sent_by(CHILD)
        assert messages.contains_mle_message(mle.CommandType.PARENT_REQUEST)

        child.set_mode('rs')
        self.simulator.go(5)
        messages = self.simulator.get_messages_sent_by(CHILD)
        assert messages.contains_mle_message(mle.CommandType.PARENT_REQUEST)

        child.set_mode('s')
        self.simulator.go(5)
        messages = self.simulator.get_messages_sent_by(CHILD)
        assert messages.contains_mle_message(mle.CommandType.PARENT_REQUEST)

        child.set_mode('rs')
        self.simulator.go(5)
        messages = self.simulator.get_messages_sent_by(CHILD)
        assert not messages.contains_mle_message(mle.CommandType.PARENT_REQUEST)

        child.set_mode('rsdn')
        self.simulator.go(5)
        messages = self.simulator.get_messages_sent_by(CHILD)
        assert not messages.contains_mle_message(mle.CommandType.PARENT_REQUEST)

        child.set_mode('s')
        self.simulator.go(5)
        messages = self.simulator.get_messages_sent_by(CHILD)
        assert not messages.contains_mle_message(mle.CommandType.PARENT_REQUEST)

        child.reset()
        child.set_mode('rsdn')
        self.simulator.go(5)
        messages = self.simulator.get_messages_sent_by(CHILD)
        assert not messages.contains_mle_message(mle.CommandType.PARENT_REQUEST)

        child.reset()
        child.set_mode('s')
        self.simulator.go(5)
        messages = self.simulator.get_messages_sent_by(CHILD)
        assert not messages.contains_mle_message(mle.CommandType.PARENT_REQUEST)

if __name__ == "__main__":
    unittest.main()
