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
#  Validate dataset ID (and seq number) selection, on SRPL restart and sync.
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# ----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 8
cli.Node.set_time_speedup_factor(speedup)

node1 = cli.Node()
node2 = cli.Node()

WAIT_TIME = 20

# ----------------------------------------------------------------------------------------------------------------------
# Test implementation

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# `node1` as a single SRPL peer

verify(node1.srp_replication_get_dataset_id() == '(none)')
node1.srp_replication_enable()


def check_node1_srpl_enters_running_state():
    verify(node1.srp_replication_get_state() == 'running')


verify_within(check_node1_srpl_enters_running_state, WAIT_TIME)

dataset_id = node1.srp_replication_get_dataset_id()
verify(dataset_id != '(none)')
seq_num = int(node1.srp_server_get_anycast_seq_num())

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Disable and re-enable SRPL on `node1` and ensure that it picks a
# different dataset ID and seq number.

node1.srp_replication_disable()
verify(node1.srp_replication_get_dataset_id() == '(none)')

node1.srp_replication_enable()
verify_within(check_node1_srpl_enters_running_state, WAIT_TIME)

new_dataset_id = node1.srp_replication_get_dataset_id()
verify(new_dataset_id != '(none)')
verify(new_dataset_id != dataset_id)

new_seq_num = int(node1.srp_server_get_anycast_seq_num())

if seq_num == 255:
    verify(new_seq_num == 0)
else:
    verify(new_seq_num == seq_num + 1)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Enable SRPL on `node2`, ensure that it adopts the dataset ID from
# `node1`

verify(node2.srp_replication_get_dataset_id() == '(none)')
node2.srp_replication_enable()


def check_node2_srpl_enters_running_state():
    verify(node2.srp_replication_get_state() == 'running')


verify_within(check_node2_srpl_enters_running_state, WAIT_TIME)
verify(node2.srp_replication_get_dataset_id() == new_dataset_id)
verify(int(node2.srp_server_get_anycast_seq_num()) == new_seq_num)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Disable and re-enable SRPL on `node2` and ensure that it adopts `node1`
# dataset ID again.

node2.srp_replication_disable()
verify(node2.srp_replication_get_dataset_id() == '(none)')

node2.srp_replication_enable()
verify_within(check_node2_srpl_enters_running_state, WAIT_TIME)

verify(node2.srp_replication_get_dataset_id() == new_dataset_id)
verify(int(node2.srp_server_get_anycast_seq_num()) == new_seq_num)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Disable SRPL on `node1 fully. Disable and re-enable SRPL on `node2`.
# Check that `node2` picks a different dataset id and moves to next
# seq number.

seq_num = new_seq_num
dataset_id = new_dataset_id

node1.srp_replication_disable()
node2.srp_replication_disable()
verify(node2.srp_replication_get_dataset_id() == '(none)')

node2.srp_replication_enable()
verify_within(check_node2_srpl_enters_running_state, WAIT_TIME)

verify(node2.srp_replication_get_dataset_id() != dataset_id)
new_seq_num = int(node2.srp_server_get_anycast_seq_num())

if seq_num == 255:
    verify(new_seq_num == 0)
else:
    verify(new_seq_num == seq_num + 1)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
