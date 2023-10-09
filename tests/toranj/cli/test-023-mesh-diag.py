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
# Test description: Validate Mesh Diag module commands.
#
# Network topology
#
#    r1 ---- r2 ---- r3
#    /\              /|\
#   /  \            / | \
#  f1  s1          f2 s2 s3

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
fed1 = cli.Node()
fed2 = cli.Node()
sed1 = cli.Node()
sed2 = cli.Node()
sed3 = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

r1.allowlist_node(r2)
r1.allowlist_node(fed1)
r1.allowlist_node(sed1)

r2.allowlist_node(r1)
r2.allowlist_node(r3)

r3.allowlist_node(r2)
r3.allowlist_node(fed2)
r3.allowlist_node(sed2)
r3.allowlist_node(sed3)

fed1.allowlist_node(r1)
fed2.allowlist_node(r3)
sed1.allowlist_node(r1)
sed2.allowlist_node(r3)
sed3.allowlist_node(r3)

r1.form('mesh-diag')

r1.add_prefix('fd00:1::/64', 'poas')
r1.add_prefix('fd00:2::/64', 'poas')
r1.register_netdata()

r2.join(r1)
r3.join(r1)
fed1.join(r1, cli.JOIN_TYPE_REED)
fed2.join(r1, cli.JOIN_TYPE_REED)
sed1.join(r1, cli.JOIN_TYPE_SLEEPY_END_DEVICE)
sed2.join(r1, cli.JOIN_TYPE_SLEEPY_END_DEVICE)
sed3.join(r1, cli.JOIN_TYPE_SLEEPY_END_DEVICE)

verify(r1.get_state() == 'leader')
verify(r2.get_state() == 'router')
verify(r3.get_state() == 'router')
verify(fed1.get_state() == 'child')
verify(fed2.get_state() == 'child')
verify(sed1.get_state() == 'child')
verify(sed2.get_state() == 'child')
verify(sed3.get_state() == 'child')

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

r1_rloc = int(r1.get_rloc16(), 16)
r2_rloc = int(r2.get_rloc16(), 16)
r3_rloc = int(r3.get_rloc16(), 16)

fed1_rloc = int(fed1.get_rloc16(), 16)
fed2_rloc = int(fed2.get_rloc16(), 16)

sed1_rloc = int(sed1.get_rloc16(), 16)
sed2_rloc = int(sed2.get_rloc16(), 16)
sed3_rloc = int(sed3.get_rloc16(), 16)

topo = r1.cli('meshdiag topology')
verify(len(topo) == 6)

routers = []
for line in topo:
    if line.startswith('id:'):
        routers.append(int(line.split(' ')[1].split(':')[1], 16))

verify(r1_rloc in routers)
verify(r2_rloc in routers)
verify(r3_rloc in routers)

r2.cli('meshdiag topology ip6-addrs')
r3.cli('meshdiag topology ip6-addrs children')
r1.cli('meshdiag topology children')

childtable = r2.cli('meshdiag childtable', r1_rloc)
verify(len([line for line in childtable if line.startswith('rloc16')]) == 2)

childtable = r1.cli('meshdiag childtable', r3_rloc)
verify(len([line for line in childtable if line.startswith('rloc16')]) == 3)

childtable = r1.cli('meshdiag childtable', r2_rloc)
verify(len([line for line in childtable if line.startswith('rloc16')]) == 0)

childrenip6addrs = r2.cli('meshdiag childip6', r1_rloc)
verify(len([line for line in childrenip6addrs if line.startswith('child-rloc16')]) == 1)
verify(len([line for line in childrenip6addrs if line.startswith('   ')]) == 3)

childrenip6addrs = r1.cli('meshdiag childip6', r3_rloc)
verify(len([line for line in childrenip6addrs if line.startswith('child-rloc16')]) == 2)

neightable = r1.cli('meshdiag routerneighbortable', r2_rloc)
verify(len([line for line in neightable if line.startswith('rloc16')]) == 2)

neightable = r3.cli('meshdiag routerneighbortable', r1_rloc)
verify(len([line for line in neightable if line.startswith('rloc16')]) == 1)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
