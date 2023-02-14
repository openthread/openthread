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
# Test description: Address Cache Table
#
# This test verifies the behavior of `AddressResolver` module and entries in
# address cache table. It also tests the behavior of nodes when there are
# topology changes in the network (e.g., a child switches parent). In
# particular, the test covers the address cache update through snooping, i.e.,
# the logic which inspects forwarded frames to update address cache table if
# source RLOC16 on a received frame differs from an existing entry in the
# address cache table.
#
# Network topology:
#
#     r1 ---- r2 ---- r3
#     |       |       |
#     |       |       |
#     c1      c2(s)   c3
#
# c1 and c3 are FED children, c2 is an SED which is first attached to r2 and
# then forced to switch to r3.

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 25
cli.Node.set_time_speedup_factor(speedup)

r1 = cli.Node()
r2 = cli.Node()
r3 = cli.Node()
c1 = cli.Node()
c2 = cli.Node()
c3 = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

r1.allowlist_node(r2)
r1.allowlist_node(c1)

r2.allowlist_node(r1)
r2.allowlist_node(r3)
r2.allowlist_node(c2)

r3.allowlist_node(r2)
r3.allowlist_node(c3)

c1.allowlist_node(r1)
c2.allowlist_node(r2)
c3.allowlist_node(r3)

r1.form('addrrslvr')
r2.join(r1)
r3.join(r1)
c1.join(r1, cli.JOIN_TYPE_REED)
c2.join(r1, cli.JOIN_TYPE_SLEEPY_END_DEVICE)
c3.join(r1, cli.JOIN_TYPE_REED)
c2.set_pollperiod(400)

verify(r1.get_state() == 'leader')
verify(r2.get_state() == 'router')
verify(r3.get_state() == 'router')
verify(c1.get_state() == 'child')
verify(c2.get_state() == 'child')
verify(c3.get_state() == 'child')

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

# Wait till first router has either established a link or
# has a valid "next hop" towards all other routers.

r1_rloc16 = int(r1.get_rloc16(), 16)


def check_r1_router_table():
    table = r1.get_router_table()
    verify(len(table) == 3)
    for entry in table:
        verify(int(entry['RLOC16'], 0) == r1_rloc16 or int(entry['Link']) == 1 or int(entry['Next Hop']) != 63)


verify_within(check_r1_router_table, 120)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

r1_address = r1.get_mleid_ip_addr()
c1_address = c1.get_mleid_ip_addr()
c2_address = c2.get_mleid_ip_addr()
c3_address = c3.get_mleid_ip_addr()

r1_rloc16 = int(r1.get_rloc16(), 16)
r2_rloc16 = int(r2.get_rloc16(), 16)
r3_rloc16 = int(r3.get_rloc16(), 16)
c1_rloc16 = int(c1.get_rloc16(), 16)
c3_rloc16 = int(c3.get_rloc16(), 16)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#  From r1 ping c2 and c3

r1.ping(c2_address)
r1.ping(c3_address)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Verify that address cache table contains both c2 and c3 addresses c2
# address should match its parent r2 (since c2 is sleepy), and c1
# address should match to itself (since c3 is fed).

cache_table = r1.get_eidcache()
verify(len(cache_table) >= 2)

for entry in cache_table:
    fields = entry.strip().split(' ')
    verify(fields[2] == 'cache')
    if fields[0] == c2_address:
        verify(int(fields[1], 16) == r2_rloc16)
    elif fields[0] == c3_address:
        verify(int(fields[1], 16) == c3_rloc16)
    else:
        verify(False)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Force c2 to switch its parent from r2 to r3
#
# New network topology
#
#     r1 ---- r2 ---- r3
#     |               /\
#     |              /  \
#     c1           c2(s) c3

r2.un_allowlist_node(c2)
r3.allowlist_node(c2)
c2.allowlist_node(r3)

c2.thread_stop()
c2.thread_start()


def check_c2_attaches_to_r3():
    verify(c2.get_state() == 'child')
    verify(int(c2.get_parent_info()['Rloc'], 16) == r3_rloc16)


verify_within(check_c2_attaches_to_r3, 10)


def check_r2_child_table_is_empty():
    verify(len(r2.get_child_table()) == 0)


verify_within(check_r2_child_table_is_empty, 10)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Note that r1 still has r2 as the destination for c2's address in its
# address cache table. But since r2 is aware that c2 is no longer its
# child, when it receives the IPv6 message with c2's address, r2
# itself would do an address query for the address and forward the
# IPv6 message.

cache_table = r1.get_eidcache()
for entry in cache_table:
    fields = entry.strip().split(' ')
    if fields[0] == c2_address:
        verify(int(fields[1], 16) == r2_rloc16)
        break
else:
    verify(False)

r1.ping(c2_address)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Ping c1 from c2. This will go through c1's parent r1. r1 upon
# receiving and forwarding the message should update its address
# cache table for c2 (address cache update through snooping).

c2.ping(c1_address)

cache_table = r1.get_eidcache()

for entry in cache_table:
    fields = entry.strip().split(' ')
    if fields[0] == c2_address:
        verify(int(fields[1], 16) == r3_rloc16)
        verify(fields[2] == 'snoop')
        break
else:
    verify(False)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
