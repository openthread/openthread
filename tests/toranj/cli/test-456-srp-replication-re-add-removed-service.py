#!/usr/bin/env python3
#
#  Copyright (c) 2021, The OpenThread Authors.
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
#  Test SRP Replication behavior when a removed service (with retained name) is re-added by the client

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# ----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 4
cli.Node.set_time_speedup_factor(speedup)

node1 = cli.Node()
node2 = cli.Node()
node3 = cli.Node()

WAIT_TIME = 20

# ----------------------------------------------------------------------------------------------------------------------
# Test implementation

node1.form('srpl-re-add')
node2.join(node1, cli.JOIN_TYPE_END_DEVICE)
node3.join(node1, cli.JOIN_TYPE_END_DEVICE)

node1.srp_server_set_lease(8, 8, 8, 8)
node2.srp_server_set_lease(1800, 7200, 86400, 1209600)

# Enable SRPL on `node1` and `node2`

node1.srp_replication_enable()
node2.srp_replication_enable()


def check_node1_and_node2_start_srp_server():
    verify(node1.srp_server_get_state() == 'running')
    verify(node2.srp_server_get_state() == 'running')


verify_within(check_node1_and_node2_start_srp_server, WAIT_TIME)

# Register one service from `node1`.

node1.srp_client_enable_auto_start_mode()
node1.srp_client_set_host_name('host1')
node1.srp_client_set_host_address('fd00::1')
node1.srp_client_add_service('srv1', '_test1._udp,_s11', 1111, 1, 2)


def check_node1_and_node2_have_one_service():
    verify(len(node1.srp_server_get_services()) == 1)
    verify(len(node2.srp_server_get_services()) == 1)


verify_within(check_node1_and_node2_have_one_service, WAIT_TIME)

# Remove the service on `node1` and validate that it is marked as deleted, on
# both `node1` and `node2`

node1.srp_client_remove_service('srv1', '_test1._udp')


def check_service_is_marked_as_deleted_on_node1_and_node2():
    for node in [node1, node2]:
        services = node.srp_server_get_services()
        verify(len(services) == 1)
        service = services[0]
        verify(service['instance'] == 'srv1')
        verify(service['deleted'] == 'true')


verify_within(check_service_is_marked_as_deleted_on_node1_and_node2, WAIT_TIME)

# Now re-add the same service instance on `node1`

node1.srp_client_add_service('srv1', '_test1._udp,_s11', 1111, 1, 2)


def check_service_is_not_deleted_on_node1_and_node2():
    for node in [node1, node2]:
        services = node.srp_server_get_services()
        verify(len(services) == 1)
        service = services[0]
        verify(service['instance'] == 'srv1')
        verify(service['deleted'] == 'false')


verify_within(check_service_is_not_deleted_on_node1_and_node2, WAIT_TIME)

# Enable SRPL on `node3` and check that the service is correctly synced
# with new SRPL peer.

node3.srp_replication_enable()


def check_node3_finishes_initial_sync():
    partners = node3.srp_replication_get_partners()
    verify(len(partners) == 2)
    for partner in partners:
        verify(partner['state'] == 'RoutineOperation')


verify_within(check_node3_finishes_initial_sync, WAIT_TIME)

services = node3.srp_server_get_services()
verify(len(services) == 1)
service = services[0]
verify(service['instance'] == 'srv1')
verify(service['deleted'] == 'false')

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
