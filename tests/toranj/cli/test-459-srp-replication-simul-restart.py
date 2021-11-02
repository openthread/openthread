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

# ----------------------------------------------------------------------------------------------------------------------
# Test description:
#
#  This test covers the behavior and recovery of SRPL on simultaneous restart of a all SRPL peers (for example can
#  happen during a power outage).
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# ----------------------------------------------------------------------------------------------------------------------
# Helper function


def verify_servers_have_same_services(first_server, second_server):
    first_services = first_server.srp_server_get_services()
    second_services = second_server.srp_server_get_services()
    verify(len(first_services) == len(second_services))
    for first_service in first_services:
        for second_service in second_services:
            if (first_service == second_service):  # full dict comparison
                break
        else:
            raise cli.VerifyError('did not find matching service on second server')


# ----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 4
cli.Node.set_time_speedup_factor(speedup)

node1 = cli.Node()
node2 = cli.Node()
node3 = cli.Node()

WAIT_TIME = 30

# ----------------------------------------------------------------------------------------------------------------------
# Test implementation

node1.form('srpl-sim-restart')
node2.join(node1)
node3.join(node1)

nodes = [node1, node2, node3]

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Enable SRPL on all three nodes and ensure all get synced.

for node in nodes:
    node.srp_replication_enable()


def all_nodes_are_synced():
    for node in [node1, node2, node3]:
        verify(node.srp_replication_get_state() == 'running')
    node1_dataset_id = node1.srp_replication_get_dataset_id()
    for node in [node2, node3]:
        verify(node.srp_replication_get_dataset_id() == node1_dataset_id)


verify_within(all_nodes_are_synced, WAIT_TIME)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Register a service from each node and ensure all nodes sync all services

node1.srp_client_enable_auto_start_mode()
node1.srp_client_set_host_name('host1')
node1.srp_client_set_host_address('fd00::1')
node1.srp_client_add_service('srv11', '_test1._udp', 111, 1, 2)

node2.srp_client_enable_auto_start_mode()
node2.srp_client_set_host_name('host2')
node2.srp_client_set_host_address('fd00::2')
node2.srp_client_add_service('srv2', '_xyz._udp,_sub1,_sub2', 222)

node3.srp_client_enable_auto_start_mode()
node3.srp_client_set_host_name('host3')
node3.srp_client_set_host_address('fd00::3')
node3.srp_client_add_service('srv3', '_test3._udp', 333)


def check_all_nodes_get_all_services():
    for node in [node1, node2, node3]:
        verify(len(node.srp_server_get_services()) == 3)


verify_within(check_all_nodes_get_all_services, WAIT_TIME)

verify_servers_have_same_services(node1, node2)
verify_servers_have_same_services(node1, node3)

dataset_id = node1.srp_replication_get_dataset_id()

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Disable all nodes and then re-enable all.

for node in nodes:
    node.srp_replication_disable()
    verify(len(node.srp_server_get_services()) == 0)

for node in nodes:
    node.srp_replication_enable()

verify_within(all_nodes_are_synced, WAIT_TIME)
verify(node1.srp_replication_get_dataset_id() != dataset_id)

verify_within(check_all_nodes_get_all_services, WAIT_TIME)
verify_servers_have_same_services(node1, node2)
verify_servers_have_same_services(node1, node3)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
