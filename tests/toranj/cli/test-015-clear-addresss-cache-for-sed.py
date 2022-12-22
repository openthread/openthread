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
# This test verifies that address cache entries associated with SED child
# addresses are removed from new parent node ensuring we would not have a
# routing loop.
#
#
#   r1 ---- r2 ---- r3
#                   |
#                   |
#                   c
#
# c is initially attached to r3 but it switches parent during test to r2 and then r1.

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Node` instances

speedup = 40
cli.Node.set_time_speedup_factor(speedup)

r1 = cli.Node()
r2 = cli.Node()
r3 = cli.Node()
c = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

r1.allowlist_node(r2)

r2.allowlist_node(r1)
r2.allowlist_node(r3)

r3.allowlist_node(r2)
r3.allowlist_node(c)

c.allowlist_node(r3)

r1.form('sed-cache')

prefix = 'fd00:abba::'
r1.add_prefix(prefix + '/64', 'paos', 'med')
r1.register_netdata()

r2.join(r1)
r3.join(r1)
c.join(r1, cli.JOIN_TYPE_SLEEPY_END_DEVICE)
c.set_pollperiod(400)

verify(r1.get_state() == 'leader')
verify(r2.get_state() == 'router')
verify(r3.get_state() == 'router')
verify(c.get_state() == 'child')

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

r1_rloc = int(r1.get_rloc16(), 16)
r2_rloc = int(r2.get_rloc16(), 16)
r3_rloc = int(r3.get_rloc16(), 16)
c_rloc = int(c.get_rloc16(), 16)

r1_address = next(addr for addr in r1.get_ip_addrs() if addr.startswith('fd00:abba:'))
r2_address = next(addr for addr in r2.get_ip_addrs() if addr.startswith('fd00:abba:'))
r3_address = next(addr for addr in r3.get_ip_addrs() if addr.startswith('fd00:abba:'))
c_address = next(addr for addr in c.get_ip_addrs() if addr.startswith('fd00:abba:'))

port = 4321

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Send a single UDP message from r2 to c. This adds an address cache
# entry on r2 for c pointing to r3 (the current parent of c).

r2.udp_open()
r2.udp_send(c_address, port, 'hi_from_r2_to_c')


def check_r2_cache_table():
    cache_table = r2.get_eidcache()
    for entry in cache_table:
        fields = entry.strip().split(' ')
        if (fields[0] == c_address):
            verify(int(fields[1], 16) == r3_rloc)
            verify(fields[2] == 'cache')
            break
    else:
        verify(False)


verify_within(check_r2_cache_table, 20)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Force c to switch its parent from r3 to r2
#
#   r1 ---- r2 ---- r3
#            |
#            |
#            c

r3.un_allowlist_node(c)
r2.allowlist_node(c)
c.allowlist_node(r2)
c.un_allowlist_node(r3)

c.thread_stop()
c.thread_start()


def check_c_attaches_to_r2():
    verify(c.get_state() == 'child')
    verify(int(c.get_parent_info()['Rloc'], 16) == r2_rloc)


verify_within(check_c_attaches_to_r2, 10)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Send a single UDP message from r3 to c. This adds an address cache
# entry on r3 for c pointing to r2 (the current parent of c).

r3.udp_open()
r3.udp_send(c_address, port, 'hi_from_r3_to_c')


def check_r3_cache_table():
    cache_table = r3.get_eidcache()
    for entry in cache_table:
        fields = entry.strip().split(' ')
        if (fields[0] == c_address):
            verify(int(fields[1], 16) == r2_rloc)
            verify(fields[2] == 'cache')
            break
    else:
        verify(False)


verify_within(check_r3_cache_table, 20)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Force c to switch its parent again now to r1
#
#   r1 ---- r2 ---- r3
#   |
#   |
#   c

r2.un_allowlist_node(c)
r1.allowlist_node(c)
c.allowlist_node(r1)
c.un_allowlist_node(r2)

c.thread_stop()
c.thread_start()


def check_c_attaches_to_r1():
    verify(c.get_state() == 'child')
    verify(int(c.get_parent_info()['Rloc'], 16) == r1_rloc)


verify_within(check_c_attaches_to_r1, 10)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Now ping c from r2.
#
# If r2 address cache entry is not cleared when c attached to r1, r2
# will still have an entry pointing to r3, and r3 will have an entry
# pointing to r2, thus creating a loop (the msg will not be delivered
# to r3)

r2.ping(c_address)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
