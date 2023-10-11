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
# Test description: Multicast traffic
#
# Network topology
#
#     r1 ---- r2 ---- r3 ---- r4
#             |               |
#             |               |
#            fed             sed
#
# Test covers the following multicast traffic:
#
# - r2  =>> link-local all-nodes.   Expected response from [r1, r3, fed].
# - r3  =>> mesh-local all-nodes.   Expected response from [r1, r2, r4, fed].
# - r3  =>> link-local all-routers. Expected response from [r2, r4].
# - r3  =>> mesh-local all-routers. Expected response from all routers.
# - r1  =>> link-local all-thread.  Expected response from [r1, r2].
# - fed =>> mesh-local all-thread.  Expected response from all nodes.
# - r1  =>> mesh-local all-thread (one hop). Expected response from [r2].
# - r1  =>> mesh-local all-thread (two hops). Expected response from [r2, r3, fed].
# - r1  =>> mesh-local all-thread (three hops). Expected response from [r2, r3, r4, fed].
# - r1  =>> mesh-local all-thread (four hops). Expected response from [r2, r3, r4, fed, sed].
#
# - r1  =>> realm-local scope mcast addr (on r2 and sed). Expected to receive on [r2, sed].
# - r2  =>> realm-local scope mcast addr (on r2 and sed) without `multicast-loop`. Expected to receive from [sed].
# - r2  =>> realm-local scope mcast addr (on r2 and sed) with `multicast-loop`. Expected to receive from [r2, sed].
# - sed =>> realm-local scope mcast addr (on r2 and sed) without `multicast-loop`. Expected to receive from [r2].
# - sed =>> realm-local scope mcast addr (on r2 and sed) with `multicast-loop`. Expected to receive from [r2, sed].
#
# - r3  =>> site-local mcast addr (on r1, r2, sed). Expected to receive from [r1, r2, sed].
# - r1  =>> site-local mcast addr (on r1, r2, sed) with `multicast-loop`. Expected to receive from [r1, r2, sed].
# - r1  =>> site-local mcast addr (on r1, r2, sed) without `multicast-loop`. Expected to receive from [r2, sed].
# - sed =>> site-local mcast addr (on r1, r2, sed). Expected to receive from [r1, r2].

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
fed = cli.Node()
sed = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

r1.allowlist_node(r2)

r2.allowlist_node(r1)
r2.allowlist_node(fed)
r2.allowlist_node(r3)

fed.allowlist_node(r2)

r3.allowlist_node(r2)
r3.allowlist_node(r4)

r4.allowlist_node(r3)
r4.allowlist_node(sed)

sed.allowlist_node(r4)

r1.form("multicast")
r2.join(r1)
r3.join(r2)
r4.join(r3)
fed.join(r1, cli.JOIN_TYPE_REED)

sed.join(r3, cli.JOIN_TYPE_SLEEPY_END_DEVICE)

sed.set_pollperiod(600)

verify(r1.get_state() == 'leader')
verify(r2.get_state() == 'router')
verify(r3.get_state() == 'router')
verify(r4.get_state() == 'router')
verify(fed.get_state() == 'child')
verify(sed.get_state() == 'child')

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

# r2  =>> link-local all-nodes. Expected response from [r1, r3, fed].

outputs = r2.cli('ping ff02::1')
verify(len(outputs) == 4)
for node in [r1, r3, fed]:
    ll_addr = node.get_linklocal_ip_addr()
    verify(any(ll_addr in line for line in outputs))

# r3  =>> mesh-local all-nodes. Expected response from [r1, r2, r4, fed].

outputs = r3.cli('ping ff03::1')
verify(len(outputs) == 5)
for node in [r1, r2, r4, fed]:
    ml_addr = node.get_mleid_ip_addr()
    verify(any(ml_addr in line for line in outputs))

# r3  =>> link-local all-routers. Expected response from [r2, r4].

outputs = r3.cli('ping ff02::2')
verify(len(outputs) == 3)
for node in [r2, r4]:
    ll_addr = node.get_linklocal_ip_addr()
    verify(any(ll_addr in line for line in outputs))

# r3  =>> mesh-local all-routers. Expected response from all routers.

outputs = r3.cli('ping ff03::2')
verify(len(outputs) == 5)
for node in [r1, r2, r4, fed]:
    ml_addr = node.get_mleid_ip_addr()
    verify(any(ml_addr in line for line in outputs))

# r1  =>> link-local all-thread.  Expected response from [r2].

ml_prefix = r1.get_mesh_local_prefix().strip().split('/')[0]
ll_all_thread_nodes_addr = 'ff32:40:' + ml_prefix + '1'
outputs = r1.cli('ping', ll_all_thread_nodes_addr)
verify(len(outputs) == 2)
for node in [r2]:
    ll_addr = node.get_linklocal_ip_addr()
    verify(any(ll_addr in line for line in outputs))

# fed =>> mesh-local all-thread.  Expected response from all nodes.

ml_all_thread_nodes_addr = 'ff33:40:' + ml_prefix + '1'
outputs = fed.cli('ping', ml_all_thread_nodes_addr)
verify(len(outputs) == 6)
for node in [r1, r2, r3, r4, sed]:
    ml_addr = node.get_mleid_ip_addr()
    verify(any(ml_addr in line for line in outputs))

# Repeat the same with larger ping msg requiring fragmentation

outputs = fed.cli('ping', ml_all_thread_nodes_addr, 200)
verify(len(outputs) == 6)
for node in [r1, r2, r3, r4, sed]:
    ml_addr = node.get_mleid_ip_addr()
    verify(any(ml_addr in line for line in outputs))

# r1  =>> mesh-local all-thread (one hop). Expected response from [r2].

hop_limit = 1
outputs = r1.cli('ping', ml_all_thread_nodes_addr, 10, 1, 0, hop_limit)
verify(len(outputs) == 2)
for node in [r2]:
    ml_addr = node.get_mleid_ip_addr()
    verify(any(ml_addr in line for line in outputs))

# r1  =>> mesh-local all-thread (two hops). Expected response from [r2, r3, fed].

hop_limit = 2
outputs = r1.cli('ping', ml_all_thread_nodes_addr, 10, 1, 0, hop_limit)
verify(len(outputs) == 4)
for node in [r2, r3, fed]:
    ml_addr = node.get_mleid_ip_addr()
    verify(any(ml_addr in line for line in outputs))

# r1  =>> mesh-local all-thread (three hops). Expected response from [r2, r3, r4, fed].

hop_limit = 3
outputs = r1.cli('ping', ml_all_thread_nodes_addr, 10, 1, 0, hop_limit)
verify(len(outputs) == 5)
for node in [r2, r3, r4, fed]:
    ml_addr = node.get_mleid_ip_addr()
    verify(any(ml_addr in line for line in outputs))

# r1  =>> mesh-local all-thread (four hops). Expected response from [r2, r3, r4, fed, sed].

hop_limit = 4
outputs = r1.cli('ping', ml_all_thread_nodes_addr, 10, 1, 0, hop_limit)
verify(len(outputs) == 6)
for node in [r2, r3, r4, fed, sed]:
    ml_addr = node.get_mleid_ip_addr()
    verify(any(ml_addr in line for line in outputs))

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Subscribe to a realm-local scope multicast address on r2 and sed

mcast_addr = 'ff03:0:0:0:0:0:0:114'
r2.add_ip_maddr(mcast_addr)
sed.add_ip_maddr(mcast_addr)
time.sleep(1)
maddrs = r2.get_ip_maddrs()
verify(any(mcast_addr in maddr for maddr in maddrs))
maddrs = sed.get_ip_maddrs()
verify(any(mcast_addr in maddr for maddr in maddrs))

# r1  =>> realm-local scope mcast addr (on r2 and sed). Expected to receive on [r2, sed].

outputs = r1.cli('ping', mcast_addr)
verify(len(outputs) == 3)
for node in [r2, sed]:
    ml_addr = node.get_mleid_ip_addr()
    verify(any(ml_addr in line for line in outputs))

# r2  =>> realm-local scope mcast addr (on r2 and sed) without `multicast-loop`. Expected to receive from [sed].

outputs = r2.cli('ping', mcast_addr)
verify(len(outputs) == 2)
for node in [sed]:
    ml_addr = node.get_mleid_ip_addr()
    verify(any(ml_addr in line for line in outputs))

# r2  =>> realm-local scope mcast addr (on r2 and sed) with `multicast-loop`. Expected to receive from [r2, sed].

outputs = r2.cli('ping', '-m', mcast_addr)
verify(len(outputs) == 3)
for node in [r2, sed]:
    ml_addr = node.get_mleid_ip_addr()
    verify(any(ml_addr in line for line in outputs))

# sed  =>> realm-local scope mcast addr (on r2 and sed) without `multicast-loop`. Expected to receive from [r2].

outputs = sed.cli('ping', mcast_addr)
verify(len(outputs) == 2)
for node in [r2]:
    ml_addr = node.get_mleid_ip_addr()
    verify(any(ml_addr in line for line in outputs))

# sed  =>> realm-local scope mcast addr (on r2 and sed) with `multicast-loop`. Expected to receive from [r2, sed].

outputs = sed.cli('ping', '-m', mcast_addr)
verify(len(outputs) == 3)
for node in [r2, sed]:
    ml_addr = node.get_mleid_ip_addr()
    verify(any(ml_addr in line for line in outputs))

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Subscribe to a larger than realm-local scope (site-local) multicast address on r1, r2, and sed

mcast_addr = 'ff05:0:0:0:0:0:0:abcd'
r1.add_ip_maddr(mcast_addr)
r2.add_ip_maddr(mcast_addr)
sed.add_ip_maddr(mcast_addr)
time.sleep(1)
maddrs = r1.get_ip_maddrs()
verify(any(mcast_addr in maddr for maddr in maddrs))
maddrs = r2.get_ip_maddrs()
verify(any(mcast_addr in maddr for maddr in maddrs))
maddrs = sed.get_ip_maddrs()
verify(any(mcast_addr in maddr for maddr in maddrs))

# r3  =>> site-local mcast addr (on r1, r2, sed) with `multicast-loop`. Expected to receive from [r1, r2, sed].

outputs = r3.cli('ping', '-m', mcast_addr)
verify(len(outputs) == 4)
for node in [r1, r2, sed]:
    ml_addr = node.get_mleid_ip_addr()
    verify(any(ml_addr in line for line in outputs))

# r1  =>> site-local mcast addr (on r1, r2, sed) with `multicast-loop`. Expected to receive from [r1, r2, sed].

outputs = r1.cli('ping', '-m', mcast_addr)
verify(len(outputs) == 4)
for node in [r1, r2, sed]:
    ml_addr = node.get_mleid_ip_addr()
    verify(any(ml_addr in line for line in outputs))

# r1  =>> site-local mcast addr (on r1, r2, sed) without `multicast-loop`. Expected to receive from [r2, sed].

outputs = r1.cli('ping', mcast_addr)
verify(len(outputs) == 3)
for node in [r2, sed]:
    ml_addr = node.get_mleid_ip_addr()
    verify(any(ml_addr in line for line in outputs))

# sed  =>> site-local mcast addr (on r1, r2, sed) without `multicast-loop`. Expected to receive from [r1, r2].

outputs = sed.cli('ping', mcast_addr)
verify(len(outputs) >= 3)
for node in [r1, r2]:
    ml_addr = node.get_mleid_ip_addr()
    verify(any(ml_addr in line for line in outputs))

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
