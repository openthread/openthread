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
# Test description: Adding off-mesh routes (on routers and FEDs) and traffic flow to off-mesh addresses.
#
# Test topology:
#
#     r1---- r2 ---- r3 ---- r4
#     |              |
#     |              |
#    fed1           med3
#
# The off-mesh-routes are added as follows:
# - `r1`   adds fd00:1:1::/48   high
# - `r2`   adds fd00:1:1:2::64  med
# - `r3`   adds fd00:3::/64     med  & fed00:4::/32 med
# - 'r4'   adds fd00:1:1::/48   low
# - `fed1` adds fd00:4::/32     med
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
fed1 = cli.Node()
med3 = cli.Node()

nodes = [r1, r2, r3, r4, fed1, med3]

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

r1.allowlist_node(r2)
r1.allowlist_node(fed1)

r2.allowlist_node(r1)
r2.allowlist_node(r3)

r3.allowlist_node(r2)
r3.allowlist_node(r4)
r3.allowlist_node(med3)

r4.allowlist_node(r3)

fed1.allowlist_node(r1)
med3.allowlist_node(r3)

r1.form("off-mesh")
r2.join(r1)
r3.join(r2)
r4.join(r3)
fed1.join(r1, cli.JOIN_TYPE_REED)
med3.join(r3, cli.JOIN_TYPE_END_DEVICE)

verify(r1.get_state() == 'leader')
verify(r2.get_state() == 'router')
verify(r3.get_state() == 'router')
verify(r4.get_state() == 'router')
verify(fed1.get_state() == 'child')
verify(med3.get_state() == 'child')

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


verify_within(check_r1_router_table, 120)

# Add an on-mesh prefix on r1 and different off-mesh routes
# on different nodes and make sure they are seen on all nodes

r1.add_prefix('fd00:abba::/64', 'paros', 'med')
r1.add_route('fd00:1:1::/48', 's', 'high')
r1.register_netdata()

r2.add_route('fd00:1:1:2::/64', 's', 'med')
r2.register_netdata()

r3.add_route('fd00:3::/64', 's', 'med')
r3.add_route('fd00:4::/32', 'med')
r3.register_netdata()

r4.add_route('fd00:1:1::/48', 's', 'med')

fed1.add_route('fd00:4::/32', 'med')
fed1.register_netdata()


def check_netdata_on_all_nodes():
    for node in nodes:
        netdata = node.get_netdata()
        verify(len(netdata['prefixes']) == 1)
        verify(len(netdata['routes']) == 6)


verify_within(check_netdata_on_all_nodes, 5)

# Send from `med3` to an address matching `fd00:1:1:2::/64` added
# by`r2` and verify that it is received on `r2` and not `r1`, since
# it is better prefix match with `fd00:1:1:2/64` on r2.

r1_counter = r1.get_br_counter_unicast_outbound_packets()
r2_counter = r2.get_br_counter_unicast_outbound_packets()

med3.ping('fd00:1:1:2::1', verify_success=False)

verify(r1_counter == r1.get_br_counter_unicast_outbound_packets())
verify(r2_counter + 1 == r2.get_br_counter_unicast_outbound_packets())

# Send from`r3` to an address matching `fd00:1:1::/48` which is
# added by both `r1` and `r4` and verify that it is received on
# `r1` since it adds it with higher preference.

r1_counter = r1.get_br_counter_unicast_outbound_packets()
r4_counter = r4.get_br_counter_unicast_outbound_packets()

r3.ping('fd00:1:1::2', count=3, verify_success=False)

verify(r1_counter + 3 == r1.get_br_counter_unicast_outbound_packets())
verify(r4_counter == r4.get_br_counter_unicast_outbound_packets())

# TRy the same address from `r4` itself, it should again end up
# going to `r1` due to it adding it at high preference.

r1_counter = r1.get_br_counter_unicast_outbound_packets()
r4_counter = r4.get_br_counter_unicast_outbound_packets()

r4.ping('fd00:1:1::2', count=2, verify_success=False)

verify(r1_counter + 2 == r1.get_br_counter_unicast_outbound_packets())
verify(r4_counter == r4.get_br_counter_unicast_outbound_packets())

# Send from `fed1` to an address matching `fd00::3::/64` (from `r3`)
# and verify that it is received on `r3`.

r3_counter = r3.get_br_counter_unicast_outbound_packets()

fed1.ping('fd00:3::3', count=2, verify_success=False)

verify(r3_counter + 2 == r3.get_br_counter_unicast_outbound_packets())

# Send from `r1` to an address matching `fd00::4::/32` which is added
# by both `fed1` and `r3` with same preference. Verify that it is
# received on `fed1` since it is closer to `r1` and we would have a
# smaller path cost from `r1` to `fed1`.

fed1_counter = fed1.get_br_counter_unicast_outbound_packets()
r3_counter = r3.get_br_counter_unicast_outbound_packets()

r1.ping('fd00:4::4', count=1, verify_success=False)

verify(fed1_counter + 1 == fed1.get_br_counter_unicast_outbound_packets())
verify(r3_counter == r3.get_br_counter_unicast_outbound_packets())

# Try the same, but now send from `fed1` and make sure it selects
# itself as destination.

fed1_counter = fed1.get_br_counter_unicast_outbound_packets()
r3_counter = r3.get_br_counter_unicast_outbound_packets()

fed1.ping('fd00:4::5', count=2, verify_success=False)

verify(fed1_counter + 2 == fed1.get_br_counter_unicast_outbound_packets())
verify(r3_counter == r3.get_br_counter_unicast_outbound_packets())

# Try the same but now send from `r2`. Now the `r3` would be closer
# and should be selected as destination.

fed1_counter = fed1.get_br_counter_unicast_outbound_packets()
r3_counter = r3.get_br_counter_unicast_outbound_packets()

r2.ping('fd00:4::6', count=3, verify_success=False)

verify(fed1_counter == fed1.get_br_counter_unicast_outbound_packets())
verify(r3_counter + 3 == r3.get_br_counter_unicast_outbound_packets())

# Again try same but send from `med1` and make sure its parent
# `r3` receives.

fed1_counter = fed1.get_br_counter_unicast_outbound_packets()
r3_counter = r3.get_br_counter_unicast_outbound_packets()

med3.ping('fd00:4::7', count=1, verify_success=False)

verify(fed1_counter == fed1.get_br_counter_unicast_outbound_packets())
verify(r3_counter + 1 == r3.get_br_counter_unicast_outbound_packets())

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
