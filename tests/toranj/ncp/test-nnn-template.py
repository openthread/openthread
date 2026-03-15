#!/usr/bin/env python3
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

import wpan
from wpan import verify, verify_within

# -----------------------------------------------------------------------------------------------------------------------
# Test description:
#
# This test verifies basic Thread network formation and connectivity across a
# three-node topology: one Leader, one Router, and one End Device (SED).
#
# Topology:
#
#   leader ---- router ---- sed
#
# The test checks:
#   1. All nodes form/join the same Thread network partition.
#   2. Each node reaches the expected role (Leader / Router / End Device).
#   3. The Leader's on-mesh prefix is correctly propagated to all nodes via
#      Network Data.
#   4. Basic IPv6 ping connectivity between Leader and Router.
#   5. Basic IPv6 ping connectivity between Router and SED.

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `wpan.Nodes` instances

leader = wpan.Node()
router = wpan.Node()
sed    = wpan.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Init all nodes

wpan.Node.init_all_nodes()

# -----------------------------------------------------------------------------------------------------------------------
# Build network topology

NETWORK_NAME = 'OpenThread-test'
CHANNEL      = '15'
PANID        = '0x1234'
XPANID       = '1111111122222222'

# Bring up the leader and form a new network.
leader.form(NETWORK_NAME, channel=CHANNEL, panid=PANID, xpanid=XPANID)
leader.whitelist_node(router)
leader.whitelist_node(sed)

# Router joins the leader's network.
router.whitelist_node(leader)
router.whitelist_node(sed)
router.join_node(leader, node_type=wpan.JOIN_TYPE_ROUTER)

# SED joins as a Sleepy End Device child of the router.
sed.whitelist_node(router)
sed.join_node(router, node_type=wpan.JOIN_TYPE_SLEEPY_END_DEVICE)
sed.set(wpan.SETTING_POLL_INTERVAL, '500')   # 500 ms poll interval

# -----------------------------------------------------------------------------------------------------------------------
# Test implementation

# --------------------------------------------------
# 1. Verify node roles
# --------------------------------------------------

def check_leader_role():
    verify(leader.get(wpan.SETTING_NODE_TYPE).strip() == wpan.NODE_TYPE_LEADER)

def check_router_role():
    verify(router.get(wpan.SETTING_NODE_TYPE).strip() == wpan.NODE_TYPE_ROUTER)

def check_sed_role():
    verify(sed.get(wpan.SETTING_NODE_TYPE).strip() == wpan.NODE_TYPE_SLEEPY_END_DEVICE)

verify_within(check_leader_role, timeout_in_seconds=10)
verify_within(check_router_role, timeout_in_seconds=30)
verify_within(check_sed_role,    timeout_in_seconds=30)

print('All nodes reached expected roles.')

# --------------------------------------------------
# 2. Verify all nodes share the same partition
# --------------------------------------------------

def check_same_partition():
    leader_partition = leader.get(wpan.SETTING_PARTITION_ID).strip()
    router_partition = router.get(wpan.SETTING_PARTITION_ID).strip()
    sed_partition    = sed.get(wpan.SETTING_PARTITION_ID).strip()
    verify(leader_partition == router_partition == sed_partition)

verify_within(check_same_partition, timeout_in_seconds=30)

print('All nodes are in the same partition.')

# --------------------------------------------------
# 3. Add an on-mesh prefix on the Leader and verify
#    it propagates to all nodes via Network Data
# --------------------------------------------------

ON_MESH_PREFIX = 'fd00:1234::/64'
PREFIX_FLAGS   = wpan.ON_MESH_PREFIX_FLAG_SLAAC | wpan.ON_MESH_PREFIX_FLAG_ON_MESH

leader.add_prefix(ON_MESH_PREFIX, flags=PREFIX_FLAGS, stable=True)

def check_prefix_on_leader():
    prefixes = wpan.parse_list(leader.get(wpan.SETTING_NETWORK_DATA_ON_MESH_PREFIXES))
    verify(any(ON_MESH_PREFIX in p for p in prefixes))

def check_prefix_on_router():
    prefixes = wpan.parse_list(router.get(wpan.SETTING_NETWORK_DATA_ON_MESH_PREFIXES))
    verify(any(ON_MESH_PREFIX in p for p in prefixes))

def check_prefix_on_sed():
    prefixes = wpan.parse_list(sed.get(wpan.SETTING_NETWORK_DATA_ON_MESH_PREFIXES))
    verify(any(ON_MESH_PREFIX in p for p in prefixes))

verify_within(check_prefix_on_leader, timeout_in_seconds=10)
verify_within(check_prefix_on_router, timeout_in_seconds=30)
verify_within(check_prefix_on_sed,    timeout_in_seconds=30)

print('On-mesh prefix \'{}\' propagated to all nodes.'.format(ON_MESH_PREFIX))

# --------------------------------------------------
# 4. IPv6 ping: Leader <--> Router
# --------------------------------------------------

NUM_PINGS    = 3
PING_TIMEOUT = 5   # seconds per ping

leader_ml_addr = leader.get(wpan.SETTING_IPV6_MESH_LOCAL_ADDRESS).strip().strip('"')
router_ml_addr = router.get(wpan.SETTING_IPV6_MESH_LOCAL_ADDRESS).strip().strip('"')
sed_ml_addr    = sed.get(wpan.SETTING_IPV6_MESH_LOCAL_ADDRESS).strip().strip('"')

def check_ping_leader_to_router():
    result = leader.ping(router_ml_addr, count=NUM_PINGS, timeout=PING_TIMEOUT)
    verify(result.loss_percent == 0)

def check_ping_router_to_leader():
    result = router.ping(leader_ml_addr, count=NUM_PINGS, timeout=PING_TIMEOUT)
    verify(result.loss_percent == 0)

verify_within(check_ping_leader_to_router, timeout_in_seconds=15)
verify_within(check_ping_router_to_leader, timeout_in_seconds=15)

print('Ping Leader <--> Router: OK')

# --------------------------------------------------
# 5. IPv6 ping: Router <--> SED
# --------------------------------------------------

def check_ping_router_to_sed():
    result = router.ping(sed_ml_addr, count=NUM_PINGS, timeout=PING_TIMEOUT)
    verify(result.loss_percent == 0)

def check_ping_sed_to_router():
    result = sed.ping(router_ml_addr, count=NUM_PINGS, timeout=PING_TIMEOUT)
    verify(result.loss_percent == 0)

verify_within(check_ping_router_to_sed, timeout_in_seconds=15)
verify_within(check_ping_sed_to_router, timeout_in_seconds=15)

print('Ping Router <--> SED: OK')

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

wpan.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))

#bye I'M DONE.