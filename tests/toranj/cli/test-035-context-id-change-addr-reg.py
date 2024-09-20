#!/usr/bin/env python3
#
#  Copyright (c) 2024, The OpenThread Authors.
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
# This test validates the behavior of MTD children regarding the
# registration of their IPv6 addresses with their parent.
# Specifically, it covers the scenario where SLAAC-based addresses
# remain unchanged, but their assigned LoWPAN Context ID for the
# corresponding on-mesh prefix in the Network Data changes. In this
# case, the MTD child should schedule an MLE Child Update Request
# exchange with its parent to re-register the addresses. This ensures
# that any earlier registration failures due to incorrect context ID
# compression are resolved.
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 25
cli.Node.set_time_speedup_factor(speedup)

leader = cli.Node()
sed = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

leader.form('cid-chng')

verify(leader.get_state() == 'leader')

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

# Set the "context ID reuse delay" to a short interval of 3 seconds.

leader.set_context_reuse_delay(3)
verify(int(leader.get_context_reuse_delay()) == 3)

# Add two on-link prefixes on `leader`.

leader.add_prefix('fd00:1::/64', 'poas')
leader.add_prefix('fd00:2::/64', 'poas')
leader.register_netdata()

# Check that context ID 2 is assigned to prefix `fd00:2::/64`.

time.sleep(0.5 / speedup)

netdata = leader.get_netdata()
contexts = netdata['contexts']
verify(len(contexts) == 2)
verify(any([context.startswith('fd00:1:0:0::/64 1 c') for context in contexts]))
verify(any([context.startswith('fd00:2:0:0::/64 2 c') for context in contexts]))

# Remove the first prefix.

leader.remove_prefix('fd00:1::/64')
leader.register_netdata()

# Wait longer than Context ID reuse delay to ensure that context ID
# associated with the removed prefix is aged and removed.

# Wait longer than the Context ID reuse delay to ensure the removed
# prefix's associated context ID is aged out and removed.

time.sleep(3.5 / speedup)

netdata = leader.get_netdata()
contexts = netdata['contexts']
verify(len(contexts) == 1)
verify(any([context.startswith('fd00:2:0:0::/64 2 c') for context in contexts]))

# Have `sed` attach as a child of `leader`.

sed.set_child_timeout(10)
sed.set_pollperiod(1000)

sed.join(leader, cli.JOIN_TYPE_SLEEPY_END_DEVICE)
verify(sed.get_state() == 'child')

verify(int(sed.get_child_timeout()) == 10)

# Check the Network Data on `sed`.

netdata = sed.get_netdata()
contexts = netdata['contexts']
verify(len(contexts) == 1)
verify(any([context.startswith('fd00:2:0:0::/64 2 c') for context in contexts]))

# Find the `sed` address associated with on-mesh prefix `fd00:2::`.

sed_ip_addrs = sed.get_ip_addrs()

for ip_addr in sed_ip_addrs:
    if ip_addr.startswith('fd00:2:0:0:'):
        sed_addr = ip_addr
        break
else:
    verify(False)

# Check the parent's child table, and that `sed` child has registered its
# IPv6 addresses with the parent.

verify(len(leader.get_child_table()) == 1)
ip_addrs = leader.get_child_ip()
verify(len(ip_addrs) == 2)

# Stop MLE operation on `sed`

sed.thread_stop()
verify(sed.get_state() == 'disabled')

# Remove the `fd00::2` prefix and wait for the child entry
# on parent to expire. Child timeout is set to 10 seconds.

leader.remove_prefix('fd00:2::/64')
leader.register_netdata()

time.sleep(10.5 / speedup)

# Validate that the Context ID associated with `fd00::2` is expired
# and removed.

netdata = leader.get_netdata()
contexts = netdata['contexts']
verify(len(contexts) == 0)

# Re-add the `fd00::2` prefix and check that it gets a different
# Context ID.

leader.add_prefix('fd00:2::/64', 'poas')
leader.register_netdata()

time.sleep(0.5 / speedup)

netdata = leader.get_netdata()
contexts = netdata['contexts']
verify(len(contexts) == 1)
verify(any([context.startswith('fd00:2:0:0::/64 1 c') for context in contexts]))

# Make sure that child is timed out and removed on parent.

verify(leader.get_child_table() == [])

# Re-enable MLE operation on `sed` for it to attach back.

sed.thread_start()

time.sleep(5.0 / speedup)

verify(sed.get_state() == 'child')

# Check the `sed` IPv6 addresses and that `sed` still has the same
# SLAAC address based on the `fd00:2::` prefix

sed_ip_addrs = sed.get_ip_addrs()
verify(any([ip_addr == sed_addr for ip_addr in sed_ip_addrs]))

# Validate that all `sed` IPv6 addresses are successfully
# registered on parent.

verify(len(leader.get_child_table()) == 1)
ip_addrs = leader.get_child_ip()
verify(len(ip_addrs) == 2)
verify(any([ip_addr.endswith(sed_addr) for ip_addr in ip_addrs]))

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
