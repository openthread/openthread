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
# Test description: Network Data (on-mesh prefix) timeout and entry removal
#
# Network topology
#
#   r1 ----- r2
#            |
#            |
#            c2 (sleepy)
#
#
# Test covers the following steps:
#
# - Every node adds a unique on-mesh prefix.
# - Every node also adds a common on-mesh prefix (with different flags).
# - Verify that all the unique and common prefixes are present on all nodes are associated with correct RLOC16.
# - Remove `r2` from network (which removes `c2` as well) from Thread partition created by `r1`.
# - Verify that all on-mesh prefixes added by `r2` or `c2` (unique and common) are removed on `r1`.
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
c2 = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

r1.allowlist_node(r2)

r2.allowlist_node(r1)
r2.allowlist_node(c2)

c2.allowlist_node(r2)

r1.form("netdatatmout")
r2.join(r1)
c2.join(r1, cli.JOIN_TYPE_SLEEPY_END_DEVICE)
c2.set_pollperiod(500)

verify(r1.get_state() == 'leader')
verify(r2.get_state() == 'router')
verify(c2.get_state() == 'child')

nodes = [r1, r2, c2]

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

# Each node adds its own prefix.
r1.add_prefix('fd00:1::/64', 'paros', 'med')
r2.add_prefix('fd00:2::/64', 'paros', 'med')
c2.add_prefix('fd00:3::/64', 'paros', 'med')

# a common prefix is added by all three nodes (with different preference)
r1.add_prefix('fd00:abba::/64', 'paros', 'high')
r2.add_prefix('fd00:abba::/64', 'paros', 'med')
c2.add_prefix('fd00:abba::/64', 'paros', 'low')

r1.add_route('fd00:cafe::/64', 's', 'med')
r2.add_route('fd00:cafe::/64', 's', 'med')

r1.register_netdata()
r2.register_netdata()
c2.register_netdata()


def check_netdata_on_all_nodes():
    for node in nodes:
        netdata = node.get_netdata()
        verify(len(netdata['prefixes']) == 6)
        verify(len(netdata['routes']) == 2)


verify_within(check_netdata_on_all_nodes, 10)

# Check netdata filtering for r1 entries only.

r1_rloc = int(r1.get_rloc16(), 16)
netdata = r1.get_netdata(r1_rloc)
verify(len(netdata['prefixes']) == 2)
verify(len(netdata['routes']) == 1)

# Remove `r2`. This should trigger all the prefixes added by it or its
# child to timeout and be removed.

r2.thread_stop()
r2.interface_down()


def check_netdata_on_r1():
    netdata = r1.get_netdata()
    verify(len(netdata['prefixes']) == 2)
    verify(len(netdata['routes']) == 1)
    netdata = r1.get_netdata(r1_rloc)
    verify(len(netdata['prefixes']) == 2)
    verify(len(netdata['routes']) == 1)


verify_within(check_netdata_on_r1, 120)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
