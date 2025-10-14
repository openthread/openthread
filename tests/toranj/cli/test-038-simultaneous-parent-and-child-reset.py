#!/usr/bin/env python3
#
#  Copyright (c) 2025, The OpenThread Authors.
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
child = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Form topology
#
#  leader --- router
#               |
#              child
#

leader.allowlist_node(router)
router.allowlist_node(leader)

child.allowlist_node(router)
router.allowlist_node(child)

leader.form('reset')
router.join(leader)
child.join(leader, cli.JOIN_TYPE_END_DEVICE)

verify(leader.get_state() == 'leader')
verify(router.get_state() == 'router')
verify(child.get_state() == 'child')

# Make sure that `child` is attached to `router` as its parent.

router_rloc = int(router.get_rloc16(), 16)
verify(int(child.get_parent_info()['Rloc'], 16) == router_rloc)

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

del router
router = cli.Node(index=2)
router.interface_up()
router.thread_start()

del child
child = cli.Node(index=3)
child.set_router_eligible('disable')
child.interface_up()
child.thread_start()

# Ensure both child and router restore their previous role.


def check_router_and_child_states():
    verify(router.get_state() == 'router')
    verify(child.get_state() == 'child')


verify_within(check_router_and_child_states, 10)

verify(int(router.get_rloc16(), 16) == router_rloc)
verify(int(child.get_parent_info()['Rloc'], 16) == router_rloc)

# Validate that the child was restored using a "Child Update" exchange and not
# through a "Parent Request/Child ID Request" sequence. We validate this by
# checking that the "Attach Attempts" in the MLE counters remains zero.

mle_counter = child.get_mle_counter()

verify(int(cli.Node.parse_list(mle_counter)['Role Detached']) == 1)
verify(int(cli.Node.parse_list(mle_counter)['Attach Attempts']) == 0)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
