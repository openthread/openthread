#!/usr/bin/env python
#
#  Copyright (c) 2018, The OpenThread Authors.
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

import time
import wpan
from wpan import verify

#-----------------------------------------------------------------------------------------------------------------------
# Test description:
#
# Adding on-mesh prefix and `config-gateway` command
#
#  - Verifies adding an on-mesh prefix using wpantund/wpanctl `config-gateway` command.
#  - Verifies prefixes with different flags/priorities added on routers and/or end-devices.
#  - Verifies `wpantund` adding SLAAC based IPv6 address based on prefix.
#  - Verified `wpantund` retaining user-added prefixes and adding them back after an NCP reset.
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print '-' * 120
print 'Starting \'{}\''.format(test_name)

#-----------------------------------------------------------------------------------------------------------------------
# Utility functions

def verify_prefix(node_list, prefix, on_mesh=True, slaac=True, def_route=False, priority='med'):
    """
    This function verifies that all the nodes in the given `node_list` contain an IPv6 address with the given prefix,
    and that the given prefix is present in the on-mesh prefixes list with the given flags (on-mesh, slaac, etc).
    """
    for node in node_list:
        all_addrs = wpan.parse_list(node.get(wpan.WPAN_IP6_ALL_ADDRESSES))
        verify(any([addr.startswith(prefix[:-1]) for addr in all_addrs]))

        prefixes = wpan.parse_on_mesh_prefix_result(node.get(wpan.WPAN_THREAD_ON_MESH_PREFIXES))
        for p in prefixes:
            if p.prefix == prefix:
                verify(p.prefix_len == '64')
                verify(p.is_stable())
                verify(p.is_on_mesh() == on_mesh)
                verify(p.is_def_route() == def_route)
                verify(p.is_slaac() == slaac)
                verify(p.is_dhcp() == False)
                verify(p.priority == priority)


#-----------------------------------------------------------------------------------------------------------------------
# Creating `wpan.Nodes` instances

speedup = 4
wpan.Node.set_time_speedup_factor(speedup)

r1 = wpan.Node()
r2 = wpan.Node()
sc1 = wpan.Node()
sc2 = wpan.Node()

all_nodes = [r1, r2, sc1, sc2]

#-----------------------------------------------------------------------------------------------------------------------
# Init all nodes

wpan.Node.init_all_nodes()

for node in all_nodes:
    node.set(wpan.WPAN_OT_LOG_LEVEL, '0')

#-----------------------------------------------------------------------------------------------------------------------
# Build network topology

r1.whitelist_node(r2)
r2.whitelist_node(r1)

r1.whitelist_node(sc1)
r2.whitelist_node(sc2)

r1.form('ipv6-address')
r2.join_node(r1, node_type=wpan.JOIN_TYPE_ROUTER)
sc1.join_node(r1, node_type=wpan.JOIN_TYPE_SLEEPY_END_DEVICE)
sc2.join_node(r2, node_type=wpan.JOIN_TYPE_SLEEPY_END_DEVICE)

sc1.set(wpan.WPAN_POLL_INTERVAL, '200')
sc2.set(wpan.WPAN_POLL_INTERVAL, '200')

#-----------------------------------------------------------------------------------------------------------------------
# Test implementation

prefix1 = 'fd00:abba:cafe::'
prefix2 = 'fd00:1234::'
prefix3 = 'fd00:deed::'

# Add on-mesh prefix1 on router r1
r1.config_gateway(prefix1)
time.sleep(0.5)

# Verify that the prefix1 and its corresponding address are present on all nodes
verify_prefix(all_nodes, prefix1, def_route=False)

# Now add prefix2 with priority `high` on router r2 and check all nodes for the new prefix/address
r2.config_gateway(prefix2, default_route=True, priority='1')
time.sleep(0.5)
verify_prefix(all_nodes, prefix2, def_route=True, priority='high')

# Add prefix3 on sleepy end-device and check for it on all nodes
sc1.config_gateway(prefix3, priority='-1')
time.sleep(0.5)
verify_prefix(all_nodes, prefix3, def_route=False, priority='low')

# Verify that prefix1 is retained by `wpantund` and pushed to NCP after a reset
r1.reset()

# Wait for r1 to recover after reset
start_time = time.time()
wait_time = 5
while not r1.is_associated():
    if time.time() - start_time > wait_time:
        print 'Took too long for node to recover after reset ({}>{} sec)'.format(time.time() - start_time, wait_time)
        exit(1)
    time.sleep(0.25)

# Wait for on-mesh prefix to be updated
time.sleep(0.5)
verify_prefix(all_nodes, prefix1, def_route=False)

#-----------------------------------------------------------------------------------------------------------------------
# Test finished

print '\'{}\' passed.'.format(test_name)
