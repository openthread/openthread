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
# This test verifies the behavior of `AddressResolver` and how the cache
# table is managed. In particular it verifies behavior query timeout and
# query retry and snoop optimization.
#
# Build network topology
#
#  r3 ---- r1 ---- r2
#  |               |
#  |               |
#  c3              c2
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Node` instances

speedup = 10
cli.Node.set_time_speedup_factor(speedup)

r1 = cli.Node()
r2 = cli.Node()
r3 = cli.Node()
c2 = cli.Node()
c3 = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

r1.allowlist_node(r2)
r1.allowlist_node(r3)

r2.allowlist_node(r1)
r2.allowlist_node(c2)

r3.allowlist_node(r1)
r3.allowlist_node(c3)

c2.allowlist_node(r2)
c3.allowlist_node(r3)

r1.form('addrrslvr')

prefix = 'fd00:abba::'
r1.add_prefix(prefix + '/64', 'pos', 'med')
r1.register_netdata()

r2.join(r1)
r3.join(r1)
c2.join(r1, cli.JOIN_TYPE_END_DEVICE)
c3.join(r1, cli.JOIN_TYPE_SLEEPY_END_DEVICE)
c3.set_pollperiod(400)

verify(r1.get_state() == 'leader')
verify(r2.get_state() == 'router')
verify(r3.get_state() == 'router')
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

r1_rloc = int(r1.get_rloc16(), 16)
r2_rloc = int(r2.get_rloc16(), 16)
r3_rloc = int(r3.get_rloc16(), 16)
c2_rloc = int(c2.get_rloc16(), 16)
c3_rloc = int(c3.get_rloc16(), 16)

# AddressResolver constants:

max_cache_entries = 16
max_snooped_non_evictable = 2

# Add IPv6 addresses matching the on-mesh prefix on all nodes

r1.add_ip_addr(prefix + '1')

num_addresses = 4  # Number of addresses to add on r2, r3, c2, and c3

for num in range(num_addresses):
    r2.add_ip_addr(prefix + "2:" + str(num))
    r3.add_ip_addr(prefix + "3:" + str(num))
    c2.add_ip_addr(prefix + "c2:" + str(num))
    c3.add_ip_addr(prefix + "c3:" + str(num))

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

# From r1 send msg to a group of addresses that are not provided by
# any nodes in network.

num_queries = 5
stagger_interval = 1.2
port = 1234
initial_retry_delay = 8

r1.udp_open()

for num in range(num_queries):
    r1.udp_send(prefix + '800:' + str(num), port, 'hi_nobody')
    # Wait before next tx to stagger the address queries
    # request ensuring different timeouts
    time.sleep(stagger_interval / (num_queries * speedup))

# Verify that we do see entries in cache table for all the addresses
# and all are in "query" state

cache_table = r1.get_eidcache()
verify(len(cache_table) == num_queries)
for entry in cache_table:
    fields = entry.strip().split(' ')
    verify(fields[2] == 'query')
    verify(fields[3] == 'canEvict=0')
    verify(fields[4].startswith('timeout='))
    verify(int(fields[4].split('=')[1]) > 0)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Check the retry-query behavior
#
# Wait till all the address queries time out and verify they
# enter "retry-query" state.


def check_cache_entry_switch_to_retry_state():
    cache_table = r1.get_eidcache()
    for entry in cache_table:
        fields = entry.strip().split(' ')
        verify(fields[2] == 'retry')
        verify(fields[3] == 'canEvict=1')
        verify(fields[4].startswith('timeout='))
        verify(int(fields[4].split('=')[1]) >= 0)
        verify(fields[5].startswith('retryDelay='))
        verify(int(fields[5].split('=')[1]) == initial_retry_delay)


verify_within(check_cache_entry_switch_to_retry_state, 20)

# Try sending again to same addresses which are all in "retry" state.

for num in range(num_queries):
    r1.udp_send(prefix + '800:' + str(num), port, 'hi_nobody')

# Make sure the entries stayed in retry-query state as before.

verify_within(check_cache_entry_switch_to_retry_state, 20)

# Now wait for all entries to reach zero timeout.


def check_cache_entry_in_retry_state_to_enter_rampdown():
    cache_table = r1.get_eidcache()
    for entry in cache_table:
        fields = entry.strip().split(' ')
        verify(fields[2] == 'retry')
        verify(fields[3] == 'canEvict=1')
        verify(fields[4].startswith('timeout='))
        verify(fields[5].startswith('retryDelay='))
        verify(fields[6] == 'rampDown=1')


verify_within(check_cache_entry_in_retry_state_to_enter_rampdown, 20)

# Now send again to the same addresses.

for num in range(num_queries):
    r1.udp_send(prefix + '800:' + str(num), port, 'hi_nobody')

# We expect now after the delay to see retries for same addresses.


def check_cache_entry_switch_to_query_state():
    cache_table = r1.get_eidcache()
    for entry in cache_table:
        fields = entry.strip().split(' ')
        verify(fields[2] == 'query')
        verify(fields[3] == 'canEvict=1')


verify_within(check_cache_entry_switch_to_query_state, 20)


def check_cache_entry_switch_to_retry_state_with_double_retry_delay():
    cache_table = r1.get_eidcache()
    for entry in cache_table:
        fields = entry.strip().split(' ')
        verify(fields[2] == 'retry')
        verify(fields[3] == 'canEvict=1')
        verify(fields[4].startswith('timeout='))
        verify(fields[5].startswith('retryDelay='))
        verify(int(fields[5].split('=')[1]) == 2 * initial_retry_delay)


verify_within(check_cache_entry_switch_to_retry_state_with_double_retry_delay, 40)

verify_within(check_cache_entry_in_retry_state_to_enter_rampdown, 40)


def check_cache_entry_ramp_down_to_initial_retry_delay():
    cache_table = r1.get_eidcache()
    for entry in cache_table:
        fields = entry.strip().split(' ')
        verify(fields[2] == 'retry')
        verify(fields[3] == 'canEvict=1')
        verify(fields[4].startswith('timeout='))
        verify(fields[5].startswith('retryDelay='))
        verify(int(fields[5].split('=')[1]) == initial_retry_delay)
        verify(fields[6] == 'rampDown=1')


verify_within(check_cache_entry_ramp_down_to_initial_retry_delay, 60)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Verify snoop optimization behavior.

# Send to r1 from all addresses on r2.

r2.udp_open()
for num in range(num_addresses):
    r2.udp_bind(prefix + '2:' + str(num), port)
    r2.udp_send(prefix + '1', port, 'hi_r1_from_r2_snoop_me')

# Verify that we see all addresses from r2 as snooped in cache table.
# At most two of them should be marked as non-evictable.


def check_cache_entry_contains_snooped_entries():
    cache_table = r1.get_eidcache()
    verify(len(cache_table) >= num_addresses)
    snooped_count = 0
    snooped_non_evictable = 0
    for entry in cache_table:
        fields = entry.strip().split(' ')
        if fields[2] == 'snoop':
            verify(fields[0].startswith('fd00:abba:0:0:0:0:2:'))
            verify(int(fields[1], 16) == r2_rloc)
            snooped_count = snooped_count + 1
            if fields[3] == 'canEvict=0':
                snooped_non_evictable = snooped_non_evictable + 1
    verify(snooped_count == num_addresses)
    verify(snooped_non_evictable == max_snooped_non_evictable)


verify_within(check_cache_entry_contains_snooped_entries, 20)

# Now we use the snooped entries by sending from r1 to r2 using
# all its addresses.

for num in range(num_addresses):
    r1.udp_send(prefix + '2:' + str(num), port, 'hi_back_r2_from_r1')

time.sleep(0.1)

# We expect to see the entries to be in "cached" state now.

cache_table = r1.get_eidcache()
verify(len(cache_table) >= num_addresses)
match_count = 0
for entry in cache_table:
    fields = entry.strip().split(' ')
    if fields[0].startswith('fd00:abba:0:0:0:0:2:'):
        verify(fields[2] == 'cache')
        verify(fields[3] == 'canEvict=1')
        match_count = match_count + 1
verify(match_count == num_addresses)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Check query requests and last transaction time

# Send from r1 to all addresses on r3. Check entries
# for r3 are at the top of cache table list.

for num in range(num_addresses):
    r1.udp_send(prefix + '3:' + str(num), port, 'hi_r3_from_r1')


def check_cache_entry_contains_r3_entries():
    cache_table = r1.get_eidcache()
    for num in range(num_addresses):
        entry = cache_table[num]
        fields = entry.strip().split(' ')
        verify(fields[0].startswith('fd00:abba:0:0:0:0:3:'))
        verify(int(fields[1], 16) == r3_rloc)
        verify(fields[2] == 'cache')
        verify(fields[3] == 'canEvict=1')
        verify(fields[4] == 'transTime=0')


verify_within(check_cache_entry_contains_r3_entries, 20)

# Send from r1 to all addresses of c3 (sleepy child of r3)

for num in range(num_addresses):
    r1.udp_send(prefix + 'c3:' + str(num), port, 'hi_c3_from_r1')


def check_cache_entry_contains_c3_entries():
    cache_table = r1.get_eidcache()
    for num in range(num_addresses):
        entry = cache_table[num]
        fields = entry.strip().split(' ')
        verify(fields[0].startswith('fd00:abba:0:0:0:0:c3:'))
        verify(int(fields[1], 16) == r3_rloc)
        verify(fields[2] == 'cache')
        verify(fields[3] == 'canEvict=1')
        verify(fields[4] == 'transTime=0')


verify_within(check_cache_entry_contains_c3_entries, 20)

# Send again to r2. This should cause the related cache entries to
# be moved to top of the list.

for num in range(num_addresses):
    r1.udp_send(prefix + '2:' + str(num), port, 'hi_again_r2_from_r1')


def check_cache_entry_contains_r2_entries():
    cache_table = r1.get_eidcache()
    for num in range(num_addresses):
        entry = cache_table[num]
        fields = entry.strip().split(' ')
        verify(fields[0].startswith('fd00:abba:0:0:0:0:2:'))
        verify(int(fields[1], 16) == r2_rloc)
        verify(fields[2] == 'cache')
        verify(fields[3] == 'canEvict=1')


verify_within(check_cache_entry_contains_r2_entries, 20)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Check behavior when address cache table is full.

cache_table = r1.get_eidcache()
verify(len(cache_table) == max_cache_entries)

# From r1 send to non-existing addresses.

for num in range(num_queries):
    r1.udp_send(prefix + '900:' + str(num), port, 'hi_nobody!')

cache_table = r1.get_eidcache()
verify(len(cache_table) == max_cache_entries)

# Send from c2 to r1 and verify that snoop optimization uses at most
# `max_snooped_non_evictable` entries

c2.udp_open()

for num in range(num_addresses):
    c2.udp_bind(prefix + 'c2:' + str(num), port)
    c2.udp_send(prefix + '1', port, 'hi_r1_from_c2_snoop_me')


def check_cache_entry_contains_max_allowed_snopped():
    cache_table = r1.get_eidcache()
    snooped_non_evictable = 0
    for entry in cache_table:
        fields = entry.strip().split(' ')
        if fields[2] == 'snoop':
            verify(fields[0].startswith('fd00:abba:0:0:0:0:c2:'))
            verify(fields[3] == 'canEvict=0')
            snooped_non_evictable = snooped_non_evictable + 1
    verify(snooped_non_evictable == max_snooped_non_evictable)


verify_within(check_cache_entry_contains_max_allowed_snopped, 20)

# Now send from r1 to c2, the snooped entries would be used
# some other addresses will  go through full address query.

for num in range(num_addresses):
    r1.udp_send(prefix + 'c2:' + str(num), port, 'hi_c2_from_r1')


def check_cache_entry_contains_c2_entries():
    cache_table = r1.get_eidcache()
    for num in range(num_addresses):
        entry = cache_table[num]
        fields = entry.strip().split(' ')
        verify(fields[0].startswith('fd00:abba:0:0:0:0:c2:'))
        verify(int(fields[1], 16) == r2_rloc)
        verify(fields[2] == 'cache')
        verify(fields[3] == 'canEvict=1')


verify_within(check_cache_entry_contains_c2_entries, 20)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
