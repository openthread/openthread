#!/usr/bin/env python3
#
#  Copyright (c) 2023, The OpenThread Authors.
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

# This test covers behavior of leader take over (an already attached device
# trying to form their own partition and taking over the leader role).
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 25
cli.Node.set_time_speedup_factor(speedup)

node1 = cli.Node()
node2 = cli.Node()
node3 = cli.Node()
child2 = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

node1.form('lto')
node2.join(node1)
node3.join(node1)

child2.allowlist_node(node2)
child2.join(node2, cli.JOIN_TYPE_REED)

verify(node1.get_state() == 'leader')
verify(node2.get_state() == 'router')
verify(node3.get_state() == 'router')
verify(child2.get_state() == 'child')

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

node1.set_router_selection_jitter(1)

n1_weight = int(node1.get_leader_weight())

node2.set_leader_weight(n1_weight)

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Make sure we get `NonCapable` if local leader weight same as current leader's weight

error = None
try:
    node2.cli('state leader')
except cli.CliError as e:
    error = e

verify(error.message == 'NotCapable')

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Update local leader weight and try to take over the leader role on `node2`.

node2.set_leader_weight(n1_weight + 1)

old_partition_id = int(node2.get_partition_id())

node2.cli('state leader')

new_partition_id = int(node2.get_partition_id())
verify(new_partition_id != old_partition_id)


def check_leader_switch():
    for node in [node1, node2, node3, child2]:
        verify(int(node.get_partition_id()) == new_partition_id)
    verify(node1.get_state() == 'router')
    verify(node2.get_state() == 'leader')
    verify(node3.get_state() == 'router')
    verify(child2.get_state() == 'child')


verify_within(check_leader_switch, 30)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
