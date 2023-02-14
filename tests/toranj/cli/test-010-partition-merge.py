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
# Test description: Partition formation and merge
#
# Network Topology:
#
#      r1 ---- / ---- r2
#      |       \      |
#      |       /      |
#      c1      \      c2
#
#
# Test covers the following situations:
#
# - r2 forming its own partition when it can no longer hear r1
# - Partitions merging into one once r1 and r2 can talk again
# - Adding on-mesh prefixes on each partition and ensuring after
#   merge the info in combined.
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 25
cli.Node.set_time_speedup_factor(speedup)

r1 = cli.Node()
r2 = cli.Node()
c1 = cli.Node()
c2 = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

r1.allowlist_node(r2)
r1.allowlist_node(c1)

r2.allowlist_node(r1)
r2.allowlist_node(c2)

r1.form("partmrge")
r2.join(r1)
c1.join(r1, cli.JOIN_TYPE_END_DEVICE)
c2.join(r2, cli.JOIN_TYPE_END_DEVICE)

verify(r1.get_state() == 'leader')
verify(r2.get_state() == 'router')
verify(c1.get_state() == 'child')
verify(c2.get_state() == 'child')

nodes = [r1, r2, c1, c2]

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

# Force the two routers to form their own partition
# by removing them from each other's allowlist table

r1.un_allowlist_node(r2)
r2.un_allowlist_node(r1)

# Add a prefix before r2 realizes it can no longer talk
# to leader (r1).

r2.add_prefix('fd00:abba::/64', 'paros', 'med')
r2.register_netdata()

# Check that r2 forms its own partition


def check_r2_become_leader():
    verify(r2.get_state() == 'leader')


verify_within(check_r2_become_leader, 20)

# While we have two partition, add a prefix on r1
r1.add_prefix('fd00:1234::/64', 'paros', 'med')
r1.register_netdata()

# Update allowlist and wait for partitions to merge.
r1.allowlist_node(r2)
r2.allowlist_node(r1)


def check_partition_id_match():
    verify(r1.get_partition_id() == r2.get_partition_id())


verify_within(check_partition_id_match, 20)

# Check that partitions merged successfully


def check_r1_r2_roles():
    verify(r1.get_state() in ['leader', 'router'])
    verify(r2.get_state() in ['leader', 'router'])


verify_within(check_r1_r2_roles, 10)

# Verify all nodes see both prefixes


def check_netdata_on_all_nodes():
    for node in nodes:
        netdata = node.get_netdata()
        verify(len(netdata['prefixes']) == 2)


verify_within(check_netdata_on_all_nodes, 10)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
