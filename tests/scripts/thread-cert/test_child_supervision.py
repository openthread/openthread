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

import unittest

import command
import config
import thread_cert

# Test description:
#
#   This test verifies behavior child supervision.
#
#
# Topology:
#
#  Parent (leader)
#   |
#   |
#  Child (sleepy).

PARENT = 1
CHILD = 2


class ChildSupervision(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False
    SUPPORT_NCP = False

    TOPOLOGY = {
        PARENT: {
            'name': 'PARENT',
            'mode': 'rdn',
        },
        CHILD: {
            'name': 'CHILD',
            'is_mtd': True,
            'mode': 'n',
        },
    }

    def test(self):
        parent = self.nodes[PARENT]
        child = self.nodes[CHILD]

        # Form the network.

        parent.start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual(parent.get_state(), 'leader')

        child.start()
        self.simulator.go(5)
        self.assertEqual(child.get_state(), 'child')
        child.set_pollperiod(500)

        self.assertEqual(int(child.get_child_supervision_check_failure_counter()), 0)

        # Check the parent's child table.

        table = parent.get_child_table()
        self.assertEqual(len(table), 1)
        self.assertEqual(table[1]['suprvsn'], int(child.get_child_supervision_interval()))

        # Change the supervision interval on child. This should trigger an
        # MLE Child Update exchange from child to parent so to inform parent
        # about the change. Verify that parent is notified by checking the
        # parent's child table.

        child.set_child_supervision_interval(20)

        self.simulator.go(2)

        self.assertEqual(int(child.get_child_supervision_interval()), 20)
        table = parent.get_child_table()
        self.assertEqual(len(table), 1)
        self.assertEqual(table[1]['suprvsn'], int(child.get_child_supervision_interval()))

        # Change supervision check timeout on the child.

        child.set_child_supervision_check_timeout(25)
        self.assertEqual(int(child.get_child_supervision_check_timeout()), 25)

        # Wait for multiple supervision intervals and ensure that child
        # stays attached (child supervision working as expected).

        self.simulator.go(110)

        self.assertEqual(child.get_state(), 'child')
        table = parent.get_child_table()
        self.assertEqual(len(table), 1)
        self.assertEqual(int(child.get_child_supervision_check_failure_counter()), 0)

        # Disable supervision check on child.

        child.set_child_supervision_check_timeout(0)

        # Enable allowlist on parent without adding the child. After child
        # timeout expires, the parent should remove the child from its child
        # table.

        parent.clear_allowlist()
        parent.enable_allowlist()

        table = parent.get_child_table()
        child_timeout = table[1]['timeout']

        self.simulator.go(child_timeout + 1)
        table = parent.get_child_table()
        self.assertEqual(len(table), 0)

        # Since supervision check is disabled on the child, it should
        # continue to stay attached to parent (since data polls are acked by
        # radio driver).

        self.assertEqual(child.get_state(), 'child')
        self.assertEqual(int(child.get_child_supervision_check_failure_counter()), 0)

        # Re-enable supervision check on child. After the check timeout the
        # child must try to exchange "Child Update" messages with parent and
        # then detect that parent is not responding and detach.

        child.set_child_supervision_check_timeout(25)

        self.simulator.go(35)
        self.assertEqual(child.get_state(), 'detached')
        self.assertTrue(int(child.get_child_supervision_check_failure_counter()) > 0)

        # Disable allowlist on parent. Child should be able to attach again.

        parent.disable_allowlist()
        self.simulator.go(30)
        self.assertEqual(child.get_state(), 'child')
        child.reset_child_supervision_check_failure_counter()
        self.assertEqual(int(child.get_child_supervision_check_failure_counter()), 0)

        # Set the supervision interval to zero on child (child is asking
        # parent not to supervise it anymore). This practically behaves
        # the same as if parent does not support child supervision
        # feature.

        child.set_child_supervision_interval(0)
        child.set_child_supervision_check_timeout(25)
        self.simulator.go(2)

        self.assertEqual(int(child.get_child_supervision_interval()), 0)
        self.assertEqual(int(child.get_child_supervision_check_timeout()), 25)

        table = parent.get_child_table()
        self.assertEqual(len(table), 1)
        self.assertEqual(table[2]['suprvsn'], int(child.get_child_supervision_interval()))

        # Wait for multiple check timeouts. The child should still stay
        # attached to parent.

        self.simulator.go(100)
        self.assertEqual(child.get_state(), 'child')
        self.assertEqual(len(parent.get_child_table()), 1)
        self.assertTrue(int(child.get_child_supervision_check_failure_counter()) > 0)


if __name__ == '__main__':
    unittest.main()
