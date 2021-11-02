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

# ----------------------------------------------------------------------------------------------------------------------
# Test description:
#
#  Test SRP Replication behavior when the client updates an existing (previously registered) service.

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

node1.form('srpl-update')
node2.join(node1, cli.JOIN_TYPE_END_DEVICE)
node3.join(node1, cli.JOIN_TYPE_END_DEVICE)

# Enable SRPL on `node1` and `node2`

node1.srp_replication_enable()
node2.srp_replication_enable()


def check_node1_and_node2_finish_initial_sync():
    node1_partnters = node1.srp_replication_get_partners()
    verify(len(node1_partnters) == 1)
    verify(node1_partnters[0]['state'] == 'RoutineOperation')
    node2_partnters = node2.srp_replication_get_partners()
    verify(len(node2_partnters) == 1)
    verify(node2_partnters[0]['state'] == 'RoutineOperation')


verify_within(check_node1_and_node2_finish_initial_sync, WAIT_TIME)

# Register one service from `node1`

node1.srp_client_enable_auto_start_mode()
node1.srp_client_set_host_name('host1')
node1.srp_client_set_host_address('fd00::1')
node1.srp_client_add_service('srv1', '_test1._udp,_s11', 1234, 0, 0, ['a=01', 'x'])


def check_node1_and_node2_have_one_service():
    verify(len(node1.srp_server_get_services()) == 1)
    verify(len(node2.srp_server_get_services()) == 1)


verify_within(check_node1_and_node2_have_one_service, WAIT_TIME)

# Update the registred service on `node1` to use a different port
# and a different TXT record

node1.srp_client_clear_service('srv1', '_test1._udp')
node1.srp_client_add_service('srv1', '_test1._udp,_s11', 4444, 0, 0, ['y'])

# Check that the service is correctly updated on both node1 and node2


def check_service_is_updated_on_node1_and_node2():
    for node in [node1, node2]:
        services = node.srp_server_get_services()
        verify(len(services) == 1)
        service = services[0]
        verify(service['instance'] == 'srv1')
        verify(service['port'] == '4444')


verify_within(check_service_is_updated_on_node1_and_node2, WAIT_TIME)

# Enable SRPL on `node3` and check that the synced service is
# the updated version (new port number).

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
verify(service['port'] == '4444')

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
