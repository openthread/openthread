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

from cli import verify
from cli import verify_within
import cli
import time

# -----------------------------------------------------------------------------------------------------------------------
# Test description:
# This test covers reset of parent, router, leader and restoring children after reset
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 25
cli.Node.set_time_speedup_factor(speedup)

leader = cli.Node()
router = cli.Node()
child1 = cli.Node()
child2 = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

leader.form('reset')
child1.join(leader, cli.JOIN_TYPE_REED)
child2.join(leader, cli.JOIN_TYPE_END_DEVICE)

verify(leader.get_state() == 'leader')
verify(child1.get_state() == 'child')
verify(child2.get_state() == 'child')

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Reset the parent and verify that both children are restored on
# parent through "Child Update" exchange process and none of them got
# detached and needed to attach back.

del leader
leader = cli.Node(index=1)
leader.interface_up()
leader.thread_start()


def check_leader_state():
    verify(leader.get_state() == 'leader')


verify_within(check_leader_state, 10)

# Check that `child1` and `child2` did not detach

verify(child1.get_state() == 'child')
verify(child2.get_state() == 'child')

verify(int(cli.Node.parse_list(child1.get_mle_counter())['Role Detached']) == 1)
verify(int(cli.Node.parse_list(child2.get_mle_counter())['Role Detached']) == 1)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Reset `router` and make sure it recovers as router with same router ID.

router.join(leader)

verify(router.get_state() == 'router')
router_rloc16 = int(router.get_rloc16(), 16)

time.sleep(0.75)

del router
router = cli.Node(index=2)
router.interface_up()
router.thread_start()


def check_router_state():
    verify(router.get_state() == 'router')


verify_within(check_router_state, 10)
verify(router_rloc16 == int(router.get_rloc16(), 16))

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Reset `leader` and make sure `router` is its neighbor again

del leader
leader = cli.Node(index=1)
leader.interface_up()
leader.thread_start()


def check_leader_state():
    verify(leader.get_state() == 'leader')


verify_within(check_leader_state, 10)


def check_leader_neighbor_table():
    verify(len(leader.get_neighbor_table()) == 3)


verify_within(check_leader_neighbor_table, 10)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Reset `child` and make sure it re-attaches successfully.

del child1
child1 = cli.Node(index=3)
child1.set_router_eligible('disable')
child1.interface_up()
child1.thread_start()


def check_child1_state():
    verify(child1.get_state() == 'child')
    table = child1.get_router_table()
    verify(len(table) == 2)


verify_within(check_child1_state, 10)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Remove all nodes and restart `router` on its own

del leader
del child1
del child2

del router
router = cli.Node(index=2)
router.interface_up()
router.thread_start()


def check_router_become_leader():
    verify(router.get_state() == 'leader')


verify_within(check_router_become_leader, 10)

# Router device should attempt 3 Link Request to restore its
# previous role, before sending Parent Request (7 times)

counters = router.get_mac_counters()
print(int(counters['TxBroadcast']) >= 10)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Remove all nodes and restart `leader` on its own

del router

leader = cli.Node(index=1)
leader.interface_up()
leader.thread_start()

verify_within(check_leader_state, 10)

# Leader device should attempt 6 Link Request to restore its
# previous role, before sending Parent Request (7 times)

counters = leader.get_mac_counters()
print(int(counters['TxBroadcast']) >= 13)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
