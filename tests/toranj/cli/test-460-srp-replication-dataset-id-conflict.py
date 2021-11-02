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
#  Validate SRPL behavior when there is dataset ID conflict and the convergence into one dataset.
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

node1.form('srpl-did-cnflct')
node2.join(node1)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Enable SRPL on `node1`, block discovery (so it does not advertise on DNS-SD)

node1.srp_replication_test_enable_block_discovery()
node1.srp_replication_enable()


def check_node1_srpl_enters_running_state():
    verify(node1.srp_replication_get_state() == 'running')


verify_within(check_node1_srpl_enters_running_state, WAIT_TIME)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Enable SRPL on `node2`, block discovery as well.

node2.srp_replication_test_enable_block_discovery()
node2.srp_replication_enable()


def check_node2_srpl_enters_running_state():
    verify(node2.srp_replication_get_state() == 'running')


verify_within(check_node2_srpl_enters_running_state, WAIT_TIME)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Validate that they do not discover each other over DNS-SD

verify(len(node1.srp_replication_get_partners()) == 0)
verify(len(node2.srp_replication_get_partners()) == 0)

node1_did = int(node1.srp_replication_get_dataset_id(), 0)
node2_did = int(node2.srp_replication_get_dataset_id(), 0)

verify(node1_did != node2_did)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Re-enable DNS-SD discovery on both nodes and make sure they resolve
# the conflict and adopt the same dataset id

node1.srp_replication_test_disable_block_discovery()
node2.srp_replication_test_disable_block_discovery()


def check_node1_and_node2_use_same_datatset_id():
    verify(node1.srp_replication_get_dataset_id() == node2.srp_replication_get_dataset_id())


verify_within(check_node1_and_node2_use_same_datatset_id, WAIT_TIME)
verify_within(check_node1_srpl_enters_running_state, WAIT_TIME)
verify_within(check_node2_srpl_enters_running_state, WAIT_TIME)

new_did = int(node1.srp_replication_get_dataset_id(), 0)
verify((new_did == node1_did) or (new_did == node2_did))

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
