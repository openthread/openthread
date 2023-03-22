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
# Test description: Network Data Context ID assignment and reuse delay.
#
# Network topology
#
#      r1 ---- r2 ---- r3
#

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

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

r1.allowlist_node(r2)

r2.allowlist_node(r1)
r2.allowlist_node(r3)

r3.allowlist_node(r2)

r1.form('netdata')
r2.join(r1)
r3.join(r1)

verify(r1.get_state() == 'leader')
verify(r2.get_state() == 'router')
verify(r3.get_state() == 'router')

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

# Check the default reuse delay on `r1` to be 5 minutes.

verify(int(r1.get_context_reuse_delay()) == 5 * 60)

# Change the reuse delay on `r1` (leader) to 5 seconds.

r1.set_context_reuse_delay(5)
verify(int(r1.get_context_reuse_delay()) == 5)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Add an on-mesh prefix from each router `r1`, r2`, and `r3`.
# Validate that netdata is updated and Context IDs are assigned.

r1.add_prefix('fd00:1::/64', 'poas')
r1.register_netdata()

r2.add_prefix('fd00:2::/64', 'poas')
r2.register_netdata()

r3.add_prefix('fd00:3::/64', 'poas')
r3.add_route('fd00:beef::/64', 's')
r3.register_netdata()


def check_netdata_1():
    global netdata
    netdata = r1.get_netdata()
    verify(len(netdata['prefixes']) == 3)
    verify(len(netdata['routes']) == 1)


verify_within(check_netdata_1, 5)

contexts = netdata['contexts']
verify(len(contexts) == 3)
verify(any([context.startswith('fd00:1:0:0::/64') for context in contexts]))
verify(any([context.startswith('fd00:2:0:0::/64') for context in contexts]))
verify(any([context.startswith('fd00:3:0:0::/64') for context in contexts]))
for context in contexts:
    verify(context.endswith('c'))

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Remove prefix on `r3`. Validate that Context compress flag
# is cleared for the removed prefix.

r3.remove_prefix('fd00:3::/64')
r3.register_netdata()


def check_netdata_2():
    global netdata, versions
    versions = r1.get_netdata_versions()
    netdata = r1.get_netdata()
    verify(len(netdata['prefixes']) == 2)
    verify(len(netdata['routes']) == 1)


verify_within(check_netdata_2, 5)

contexts = netdata['contexts']
verify(len(contexts) == 3)
verify(any([context.startswith('fd00:1:0:0::/64') for context in contexts]))
verify(any([context.startswith('fd00:2:0:0::/64') for context in contexts]))
verify(any([context.startswith('fd00:3:0:0::/64') for context in contexts]))
for context in contexts:
    if context.startswith('fd00:1:0:0::/64') or context.startswith('fd00:2:0:0::/64'):
        verify(context.endswith('c'))
    else:
        verify(context.endswith('-'))

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Validate that the prefix context is removed within reuse delay
# time (which is set to 5 seconds on leader).


def check_netdata_3():
    global netdata
    netdata = r1.get_netdata()
    verify(len(netdata['prefixes']) == 2)
    verify(len(netdata['routes']) == 1)
    verify(len(netdata['contexts']) == 2)


verify_within(check_netdata_3, 5)
old_versions = versions
versions = r1.get_netdata_versions()
verify(versions != old_versions)

# Make sure netdata does not change afterwards

time.sleep(5 * 3 / speedup)

verify(versions == r1.get_netdata_versions())
netdata = r1.get_netdata()
verify(len(netdata['prefixes']) == 2)
verify(len(netdata['routes']) == 1)
verify(len(netdata['contexts']) == 2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Add two new on-mesh prefixes from `r3`. Validate that they get
# assigned Context IDs.

r3.add_prefix('fd00:4::/64', 'poas')
r3.register_netdata()

r3.add_prefix('fd00:3::/64', 'poas')
r3.register_netdata()


def check_netdata_4():
    global netdata
    netdata = r1.get_netdata()
    verify(len(netdata['prefixes']) == 4)
    verify(len(netdata['routes']) == 1)
    verify(len(netdata['contexts']) == 4)


verify_within(check_netdata_4, 5)

contexts = netdata['contexts']
verify(len(contexts) == 4)
verify(any([context.startswith('fd00:1:0:0::/64') for context in contexts]))
verify(any([context.startswith('fd00:2:0:0::/64') for context in contexts]))
verify(any([context.startswith('fd00:3:0:0::/64') for context in contexts]))
verify(any([context.startswith('fd00:4:0:0::/64') for context in contexts]))
for context in contexts:
    verify(context.endswith('c'))

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Remove prefixes on `r1` and `r2` and re-add them back quickly both
# from `r2`. Validate that they get the same context IDs as before.

for context in contexts:
    if context.startswith('fd00:1:0:0::/64'):
        cid1 = int(context.split()[1])
    elif context.startswith('fd00:2:0:0::/64'):
        cid2 = int(context.split()[1])

r1.remove_prefix('fd00:1:0:0::/64')
r1.register_netdata()

r2.remove_prefix('fd00:2:0:0::/64')
r2.register_netdata()


def check_netdata_5():
    global netdata
    netdata = r1.get_netdata()
    verify(len(netdata['prefixes']) == 2)
    verify(len(netdata['routes']) == 1)


verify_within(check_netdata_5, 5)

contexts = netdata['contexts']
verify(len(contexts) == 4)
verify(any([context.startswith('fd00:1:0:0::/64') for context in contexts]))
verify(any([context.startswith('fd00:2:0:0::/64') for context in contexts]))
verify(any([context.startswith('fd00:3:0:0::/64') for context in contexts]))
verify(any([context.startswith('fd00:4:0:0::/64') for context in contexts]))
for context in contexts:
    if context.startswith('fd00:1:0:0::/64') or context.startswith('fd00:2:0:0::/64'):
        verify(context.endswith('-'))
    else:
        verify(context.endswith('c'))

# Re-add both prefixes (now from `r2`) before CID remove delay time
# is expired.

r2.add_prefix('fd00:1::/64', 'poas')
r2.add_prefix('fd00:2::/64', 'poas')
r2.register_netdata()


def check_netdata_6():
    global netdata
    netdata = r1.get_netdata()
    verify(len(netdata['prefixes']) == 4)
    verify(len(netdata['routes']) == 1)


verify_within(check_netdata_6, 5)

contexts = netdata['contexts']
verify(len(contexts) == 4)
verify(any([context.startswith('fd00:1:0:0::/64') for context in contexts]))
verify(any([context.startswith('fd00:2:0:0::/64') for context in contexts]))
verify(any([context.startswith('fd00:3:0:0::/64') for context in contexts]))
verify(any([context.startswith('fd00:4:0:0::/64') for context in contexts]))
for context in contexts:
    verify(context.endswith('c'))
    if context.startswith('fd00:1:0:0::/64'):
        verify(int(context.split()[1]) == cid1)
    elif context.startswith('fd00:2:0:0::/64'):
        verify(int(context.split()[1]) == cid2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Remove two prefixes on `r3`. Add one back as an external route from
# `r1`. Also add a new prefix from `r1`. Validate that the context IDs
# for the removed on-mesh prefixes are removed after reuse delay time
# and that the new prefix gets a different Context ID.

# Remember the CID used.
for context in contexts:
    if context.startswith('fd00:3:0:0::/64'):
        cid3 = int(context.split()[1])
    elif context.startswith('fd00:4:0:0::/64'):
        cid4 = int(context.split()[1])

r3.remove_prefix('fd00:3:0:0::/64')
r3.remove_prefix('fd00:4:0:0::/64')
r3.register_netdata()


def check_netdata_7():
    global netdata
    netdata = r1.get_netdata()
    verify(len(netdata['prefixes']) == 2)
    verify(len(netdata['routes']) == 1)


verify_within(check_netdata_7, 5)

contexts = netdata['contexts']
verify(len(contexts) == 4)
verify(any([context.startswith('fd00:1:0:0::/64') for context in contexts]))
verify(any([context.startswith('fd00:2:0:0::/64') for context in contexts]))
verify(any([context.startswith('fd00:3:0:0::/64') for context in contexts]))
verify(any([context.startswith('fd00:4:0:0::/64') for context in contexts]))
for context in contexts:
    if context.startswith('fd00:3:0:0::/64') or context.startswith('fd00:4:0:0::/64'):
        verify(context.endswith('-'))
    else:
        verify(context.endswith('c'))

# Add first one removed as route and add a new prefix.

r1.add_route('fd00:3:0:0::/64', 's')
r1.add_prefix('fd00:5:0:0::/64', 'poas')
r1.register_netdata()


def check_netdata_8():
    global netdata
    netdata = r1.get_netdata()
    verify(len(netdata['prefixes']) == 3)
    verify(len(netdata['routes']) == 2)
    verify(len(netdata['contexts']) == 3)


verify_within(check_netdata_8, 5)

contexts = netdata['contexts']
verify(len(contexts) == 3)
verify(any([context.startswith('fd00:1:0:0::/64') for context in contexts]))
verify(any([context.startswith('fd00:2:0:0::/64') for context in contexts]))
verify(any([context.startswith('fd00:5:0:0::/64') for context in contexts]))

for context in contexts:
    verify(context.endswith('c'))
    if context.startswith('fd00:5:0:0::/64'):
        verify(not int(context.split()[1]) in [cid3, cid4])

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Remove a prefix on `r2`, wait one second and remove a second
# prefix. Validate the Context IDs for both are removed.

r2.remove_prefix('fd00:1::/64')
r2.register_netdata()

time.sleep(1 / speedup)

r2.remove_prefix('fd00:2::/64')
r2.register_netdata()


def check_netdata_9():
    global netdata
    netdata = r1.get_netdata()
    verify(len(netdata['prefixes']) == 1)
    verify(len(netdata['routes']) == 2)
    verify(len(netdata['contexts']) == 1)


verify_within(check_netdata_9, 5)

contexts = netdata['contexts']
verify(len(contexts) == 1)
verify(contexts[0].startswith('fd00:5:0:0::/64'))
verify(contexts[0].endswith('c'))

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
