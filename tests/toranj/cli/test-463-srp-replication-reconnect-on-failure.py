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
#  This test covers the behavior SRPL peers when a an existing session/connection disconnects.
#

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

node1.form('srpl-reconnect')
node2.join(node1)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Configure `node2` with fixed peer ID 2 and enable SRPl on it.

node2_id = 2
node2.srp_replication_test_use_fixed_id(node2_id)
node2.srp_replication_enable()


def check_node2_srpl_enters_running_state():
    verify(node2.srp_replication_get_state() == 'running')


verify_within(check_node2_srpl_enters_running_state, WAIT_TIME)
verify(int(node2.srp_replication_get_id(), 0) == node2_id)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Configure `node1` with fixed peer ID 1 and enable SRPL on it.
# Configure it to reject any connection requests. This validates
# that since `node1` is a new peer, it correctly initiates the
# connection to `node2`.

node1_id = 1
node1.srp_replication_test_use_fixed_id(node1_id)
node1.srp_replication_test_enable_reject_conn_requests()
node1.srp_replication_enable()


def check_node1_srpl_enters_running_state():
    verify(node1.srp_replication_get_state() == 'running')


verify_within(check_node1_srpl_enters_running_state, WAIT_TIME)
verify(int(node1.srp_replication_get_id(), 0) == node1_id)

partners = node1.srp_replication_get_partners()
verify(len(partners) == 1)
verify(partners[0]['state'] == 'RoutineOperation')

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Configure `node1` to trigger a disconnect. After the disconnect
# `node2` should be the one connection back (since it has a larger
# ID). We validate this configuring `node2` to reject new connection
# requests.

node1.srp_replication_test_disable_reject_conn_requests()
node2.srp_replication_test_enable_reject_conn_requests()

node1.srp_replication_test_disconnect_all_conns()


def check_node1_and_node2_synced():
    for node in [node1, node2]:
        partners = node.srp_replication_get_partners()
        verify(len(partners) == 1)
        verify(partners[0]['state'] == 'RoutineOperation')


verify_within(check_node1_and_node2_synced, WAIT_TIME)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Repeat the last step, but now `node2` disconnects instead

node2.srp_replication_test_disconnect_all_conns()
verify_within(check_node1_and_node2_synced, WAIT_TIME)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
