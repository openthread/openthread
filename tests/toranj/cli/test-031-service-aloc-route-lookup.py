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
# Test description: Service ALOC destination route lookup
#
# Test topology:
#
#     r1---- r2 ---- r3 ---- r4
#     |
#     |
#    fed1
#
# The same service is added on `r4` and `fed1`.

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 40
cli.Node.set_time_speedup_factor(speedup)

r1 = cli.Node()
r2 = cli.Node()
r3 = cli.Node()
r4 = cli.Node()
fed1 = cli.Node()

nodes = [r1, r2, r3, r4, fed1]

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

r1.allowlist_node(r2)
r1.allowlist_node(fed1)

r2.allowlist_node(r1)
r2.allowlist_node(r3)

r3.allowlist_node(r2)
r3.allowlist_node(r4)

r4.allowlist_node(r3)

fed1.allowlist_node(r1)

r1.form("srv-aloc")
r2.join(r1)
r3.join(r2)
r4.join(r3)
fed1.join(r1, cli.JOIN_TYPE_REED)

verify(r1.get_state() == 'leader')
verify(r2.get_state() == 'router')
verify(r3.get_state() == 'router')
verify(r4.get_state() == 'router')
verify(fed1.get_state() == 'child')

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

# Add the same service on `r4` and `fed1`

r4.cli('service add 44970 11 00')
r4.register_netdata()

fed1.cli('service add 44970 11 00')
fed1.register_netdata()


def check_netdata_services(expected_num_services):
    # Check that all nodes see the `expected_num_services` service
    # entries in network data.
    for node in nodes:
        verify(len(node.get_netdata_services()) == expected_num_services)


verify_within(check_netdata_services, 100, 2)

# Determine the ALOC address of the added service.

aloc = r4.get_mesh_local_prefix().split('/')[0] + 'ff:fe00:fc10'

# Ping ALOC address from `r3` and verify that `r4` responds.
# `r4` should be chosen due to its shorter path cost from `r3`.

old_counters = r4.get_ip_counters()
r3.ping(aloc)
new_counters = r4.get_ip_counters()

verify(int(new_counters['RxSuccess']) > int(old_counters['RxSuccess']))
verify(int(new_counters['TxSuccess']) > int(old_counters['TxSuccess']))

# Ping ALOC address from `r1` and verify that `fed1` responds.
# `fed1` should be chosen due to its shorter path cost from `r1`.

old_counters = fed1.get_ip_counters()
r1.ping(aloc)
new_counters = fed1.get_ip_counters()

verify(int(new_counters['RxSuccess']) > int(old_counters['RxSuccess']))
verify(int(new_counters['TxSuccess']) > int(old_counters['TxSuccess']))

# Ping ALOC address from `r2` and verify that `r4` responds.
# Both `r4` and `fed1` have the same path cost, but `r4` is
# preferred because it is acting as a router.

old_counters = r4.get_ip_counters()
r2.ping(aloc)
new_counters = r4.get_ip_counters()

verify(int(new_counters['RxSuccess']) > int(old_counters['RxSuccess']))
verify(int(new_counters['TxSuccess']) > int(old_counters['TxSuccess']))

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
