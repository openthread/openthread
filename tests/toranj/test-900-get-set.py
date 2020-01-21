#!/usr/bin/env python3
#
#  Copyright (c) 2020, The OpenThread Authors.
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

from wpan import verify
import wpan

# -----------------------------------------------------------------------------------------------------------------------
# Test description: joining (as router, end-device, sleepy) - two node network

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `wpan.Nodes` instances

speedup = 4
wpan.Node.set_time_speedup_factor(speedup)

node1 = wpan.Node()
node2 = wpan.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Init all nodes

wpan.Node.init_all_nodes()

# -----------------------------------------------------------------------------------------------------------------------
# Test implementation

# Form a network on node1
node1.form('PAN-aB71n')
verify(node1.get(wpan.WPAN_STATE) == wpan.STATE_ASSOCIATED)
verify(node1.get(wpan.WPAN_NODE_TYPE) == wpan.NODE_TYPE_LEADER)

# Join from node2 as an end-device
node2.join_node(node1, node_type=wpan.JOIN_TYPE_END_DEVICE)
verify(node2.get(wpan.WPAN_STATE) == wpan.STATE_ASSOCIATED)
verify(node2.get(wpan.WPAN_NAME) == node1.get(wpan.WPAN_NAME))
verify(node2.get(wpan.WPAN_PANID) == node1.get(wpan.WPAN_PANID))
verify(node2.get(wpan.WPAN_XPANID) == node1.get(wpan.WPAN_XPANID))
verify(node2.get(wpan.WPAN_KEY) == node1.get(wpan.WPAN_KEY))
node2.get(wpan.WPAN_THREAD_PARENT)

node2.leave()

# Join from node2 as a router
node2.join_node(node1, node_type=wpan.JOIN_TYPE_ROUTER)
verify(node2.get(wpan.WPAN_STATE) == wpan.STATE_ASSOCIATED)
verify(node2.get(wpan.WPAN_NAME) == node1.get(wpan.WPAN_NAME))
verify(node2.get(wpan.WPAN_PANID) == node1.get(wpan.WPAN_PANID))
verify(node2.get(wpan.WPAN_XPANID) == node1.get(wpan.WPAN_XPANID))
verify(node2.get(wpan.WPAN_KEY) == node1.get(wpan.WPAN_KEY))

# Cover setting detached role
node2.set(wpan.WPAN_ROLE, '0')
time.sleep(1)

# Cover setting child role
node2.set(wpan.WPAN_ROLE, '1')
time.sleep(1)

# Cover setting router role
node2.set(wpan.WPAN_ROLE, '2')
time.sleep(1)

# Cover setting leader role
node2.set(wpan.WPAN_ROLE, '3')
time.sleep(1)

# Wait for the network become stable
time.sleep(5)

# Ensure `get` is successful with all gettable properties
for prop in wpan.ALL_GETTABLE_PROPS:
    node2.get(prop)

node2.leave()

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

wpan.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
