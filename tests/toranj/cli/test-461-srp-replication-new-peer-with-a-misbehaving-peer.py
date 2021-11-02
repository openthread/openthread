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

# ----------------------------------------------------------------------------------------------------------------------
# Test description:
#
#  This test covers the behavior of a new SRP peer in presence of an existing peer which is discoverable
#  over DNS-SD) but does not accept connection (on in general misbehaves). This test validates that the
#  new peer correctly starts in such a situation. This scenario can potentially happen when a peer crashes or
#  reboots (without removing its DNS-SD SRPL service advertisement) and DNS-SD cache entry associated with it
# is not yet removed (or timed out).

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# ----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 4
cli.Node.set_time_speedup_factor(speedup)

node1 = cli.Node()
node2 = cli.Node()

WAIT_TIME = 20

# ----------------------------------------------------------------------------------------------------------------------
# Test implementation

node1.form('srpl-new-misbhv')
node2.join(node1)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Configure `node1` with fixed peer ID 1 and enable SRPL on it.

node1_id = 1
node1.srp_replication_test_use_fixed_id(node1_id)
node1.srp_replication_enable()


def check_node1_srpl_enters_running_state():
    verify(node1.srp_replication_get_state() == 'running')


verify_within(check_node1_srpl_enters_running_state, WAIT_TIME)
verify(int(node1.srp_replication_get_id(), 0) == node1_id)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Configure test mode on `node1` so that it does not accept any
# connection request

node1.srp_replication_test_enable_reject_conn_requests()

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Configure `node2` with fixed peer ID 2 and enable SRPL on it
# and make sure it starts successfully.

node2_id = 2
node2.srp_replication_test_use_fixed_id(node2_id)
node2.srp_replication_enable()


def check_node2_srpl_enters_running_state():
    verify(node2.srp_replication_get_state() == 'running')


verify_within(check_node2_srpl_enters_running_state, WAIT_TIME)
verify(int(node2.srp_replication_get_id(), 0) == node2_id)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Verify that `node2` did discover `node1` but that it is correctly
# marked as "Errored".

partners = node2.srp_replication_get_partners()
verify(len(partners) == 1)
verify(int(partners[0]['id'], 0) == node1_id)
verify(partners[0]['state'] == 'Errored')

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Change test mode on `node1` so that it acts normally and accepts
# connection requests from peers

node1.srp_replication_test_disable_reject_conn_requests()

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# We expect `node2` to now successfully sync with `node1` on next
# attempt.


def check_node2_is_synced_with_node1():
    partners = node2.srp_replication_get_partners()
    verify(len(partners) == 1)
    verify(partners[0]['state'] == 'RoutineOperation')


verify_within(check_node2_is_synced_with_node1, WAIT_TIME)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
