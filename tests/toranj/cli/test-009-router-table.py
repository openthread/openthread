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
#
# Verify router table entries.
#
#     r1 ------ r2 ---- r6
#      \        /
#       \      /
#        \    /
#          r3 ------ r4 ----- r5
#
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 10
cli.Node.set_time_speedup_factor(speedup)

r1 = cli.Node()
r2 = cli.Node()
r3 = cli.Node()
r4 = cli.Node()
r5 = cli.Node()
r6 = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

r1.allowlist_node(r2)
r1.allowlist_node(r3)

r2.allowlist_node(r1)
r2.allowlist_node(r3)
r2.allowlist_node(r6)

r3.allowlist_node(r1)
r3.allowlist_node(r2)
r3.allowlist_node(r4)

r4.allowlist_node(r3)
r4.allowlist_node(r5)

r5.allowlist_node(r4)

r6.allowlist_node(r2)

r1.form("topo")
for node in [r2, r3, r4, r5, r6]:
    node.join(r1)

verify(r1.get_state() == 'leader')
for node in [r2, r3, r4, r5, r6]:
    verify(node.get_state() == 'router')

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

r1_rloc16 = int(r1.get_rloc16(), 16)
r2_rloc16 = int(r2.get_rloc16(), 16)
r3_rloc16 = int(r3.get_rloc16(), 16)
r4_rloc16 = int(r4.get_rloc16(), 16)
r5_rloc16 = int(r5.get_rloc16(), 16)
r6_rloc16 = int(r6.get_rloc16(), 16)

r1_rid = r1_rloc16 / 1024
r2_rid = r2_rloc16 / 1024
r3_rid = r3_rloc16 / 1024
r4_rid = r4_rloc16 / 1024
r5_rid = r5_rloc16 / 1024
r6_rid = r6_rloc16 / 1024

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


def check_r1_router_table():
    table = r1.get_router_table()
    verify(len(table) == 6)
    for entry in table:
        rloc16 = int(entry['RLOC16'], 0)
        link = int(entry['Link'])
        nexthop = int(entry['Next Hop'])
        cost = int(entry['Path Cost'])
        if rloc16 == r1_rloc16:
            verify(link == 0)
        elif rloc16 == r2_rloc16:
            verify(link == 1)
            verify(nexthop == r3_rid)
        elif rloc16 == r3_rloc16:
            verify(link == 1)
            verify(nexthop == r2_rid)
        elif rloc16 == r4_rloc16:
            verify(link == 0)
            verify(nexthop == r3_rid)
            verify(cost == 1)
        elif rloc16 == r5_rloc16:
            verify(link == 0)
            verify(nexthop == r3_rid)
            verify(cost == 2)
        elif rloc16 == r6_rloc16:
            verify(link == 0)
            verify(nexthop == r2_rid)
            verify(cost == 1)


verify_within(check_r1_router_table, 160)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


def check_r2_router_table():
    table = r2.get_router_table()
    verify(len(table) == 6)
    for entry in table:
        rloc16 = int(entry['RLOC16'], 0)
        link = int(entry['Link'])
        nexthop = int(entry['Next Hop'])
        cost = int(entry['Path Cost'])
        if rloc16 == r1_rloc16:
            verify(link == 1)
            verify(nexthop == r3_rid)
        elif rloc16 == r2_rloc16:
            verify(link == 0)
        elif rloc16 == r3_rloc16:
            verify(link == 1)
            verify(nexthop == r1_rid)
        elif rloc16 == r4_rloc16:
            verify(link == 0)
            verify(nexthop == r3_rid)
            verify(cost == 1)
        elif rloc16 == r5_rloc16:
            verify(link == 0)
            verify(nexthop == r3_rid)
            verify(cost == 2)
        elif rloc16 == r6_rloc16:
            verify(link == 1)


verify_within(check_r2_router_table, 160)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


def check_r4_router_table():
    table = r4.get_router_table()
    verify(len(table) == 6)
    for entry in table:
        rloc16 = int(entry['RLOC16'], 0)
        link = int(entry['Link'])
        nexthop = int(entry['Next Hop'])
        cost = int(entry['Path Cost'])
        if rloc16 == r1_rloc16:
            verify(link == 0)
            verify(nexthop == r3_rid)
            verify(cost == 1)
        elif rloc16 == r2_rloc16:
            verify(link == 0)
            verify(nexthop == r3_rid)
            verify(cost == 1)
        elif rloc16 == r3_rloc16:
            verify(link == 1)
        elif rloc16 == r4_rloc16:
            verify(link == 0)
        elif rloc16 == r5_rloc16:
            verify(link == 1)
        elif rloc16 == r6_rloc16:
            verify(link == 0)
            verify(nexthop == r3_rid)
            verify(cost == 2)


verify_within(check_r4_router_table, 160)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


def check_r5_router_table():
    table = r5.get_router_table()
    verify(len(table) == 6)
    for entry in table:
        rloc16 = int(entry['RLOC16'], 0)
        link = int(entry['Link'])
        nexthop = int(entry['Next Hop'])
        cost = int(entry['Path Cost'])
        if rloc16 == r1_rloc16:
            verify(link == 0)
            verify(nexthop == r4_rid)
            verify(cost == 2)
        elif rloc16 == r2_rloc16:
            verify(link == 0)
            verify(nexthop == r4_rid)
            verify(cost == 2)
        elif rloc16 == r3_rloc16:
            verify(link == 0)
            verify(nexthop == r4_rid)
            verify(cost == 1)
        elif rloc16 == r4_rloc16:
            verify(link == 1)
        elif rloc16 == r5_rloc16:
            verify(link == 0)
        elif rloc16 == r6_rloc16:
            verify(link == 0)
            verify(nexthop == r4_rid)
            verify(cost == 3)


verify_within(check_r5_router_table, 160)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
