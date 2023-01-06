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
# Traffic over multi-hop in a network with chain topology
#
#       r1 ----- r2 ---- r3 ----- r4
#       /\                        /\
#      /  \                      /  \
#    fed1 sed1                sed4 fed4
#
#
# Traffic flow:
#  - From first router to last router
#  - From SED child of last router to SED child of first router
#  - From FED child of first router to FED child of last router
#
# The test verifies the following:
# - Verifies Address Query process to routers and FEDs.
# - Verifies Mesh Header frame forwarding over multiple routers.
# - Verifies forwarding of large IPv6 messages (1000 bytes) requiring lowpan fragmentation.

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
fed1 = cli.Node()
sed1 = cli.Node()
fed4 = cli.Node()
sed4 = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

r1.allowlist_node(r2)
r1.allowlist_node(fed1)
r1.allowlist_node(sed1)

r2.allowlist_node(r1)
r2.allowlist_node(r3)

r3.allowlist_node(r2)
r3.allowlist_node(r4)

r4.allowlist_node(r3)
r4.allowlist_node(sed4)
r4.allowlist_node(fed4)

fed1.allowlist_node(r1)
sed1.allowlist_node(r1)

fed4.allowlist_node(r4)
sed4.allowlist_node(r4)

r1.form("pan")
r2.join(r1)
r3.join(r2)
r4.join(r3)
fed1.join(r1, cli.JOIN_TYPE_REED)
sed1.join(r1, cli.JOIN_TYPE_SLEEPY_END_DEVICE)
fed4.join(r4, cli.JOIN_TYPE_REED)
sed4.join(r4, cli.JOIN_TYPE_SLEEPY_END_DEVICE)

verify(r1.get_state() == 'leader')
verify(r2.get_state() == 'router')
verify(r3.get_state() == 'router')
verify(r4.get_state() == 'router')
verify(fed1.get_state() == 'child')
verify(fed4.get_state() == 'child')
verify(sed1.get_state() == 'child')
verify(sed4.get_state() == 'child')

sed1.set_pollperiod(200)
sed4.set_pollperiod(200)
verify(int(sed1.get_pollperiod()) == 200)
verify(int(sed4.get_pollperiod()) == 200)

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

# Wait till first router has either established a link or
# has a valid "next hop" towards all other routers.

r1_rloc16 = int(r1.get_rloc16(), 16)


def check_r1_router_table():
    table = r1.get_router_table()
    verify(len(table) == 4)
    for entry in table:
        verify(int(entry['RLOC16'], 0) == r1_rloc16 or int(entry['Link']) == 1 or int(entry['Next Hop']) != 63)


verify_within(check_r1_router_table, 160)

sizes = [0, 80, 500, 1000]

# Traffic from the first router in the chain to the last one.

r4_mleid = r4.get_mleid_ip_addr()

for size in sizes:
    r1.ping(r4_mleid, size=size)

# Send from the SED child of the last router to the SED child of the first
# router.

sed1_mleid = sed1.get_mleid_ip_addr()

for size in sizes:
    sed4.ping(sed1_mleid, size=size)

# Send from the FED child of the first router to the FED child of the last
# router.

fed4_mleid = fed4.get_mleid_ip_addr()

for size in sizes:
    fed1.ping(fed4_mleid, size=size)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
