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
#  Test SRP Replication behavior when removing services and/or hosts while retaining the name. We validate that
#  partners in routine operation, get the same update (entry is correctly removed on them) and that the removed entries
#  are correctly synced with a new partner (during initial sync with it).
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

WAIT_TIME = 20

# ----------------------------------------------------------------------------------------------------------------------
# Test implementation

node1.form('srpl-remove')
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

# Register one service from `node1`, and one service from `node2`.

node1.srp_client_enable_auto_start_mode()
node1.srp_client_set_host_name('host1')
node1.srp_client_set_host_address('fd00::1')
node1.srp_client_add_service('srv1', '_test1._udp,_s11', 1234, 0, 0, ['a=01', 'x'])

node2.srp_client_enable_auto_start_mode()
node2.srp_client_set_host_name('host2')
node2.srp_client_set_host_address('fd00::2')
node2.srp_client_add_service('srv2', '_test2._udp,_s21,_s22', 2222)

node3.srp_client_enable_auto_start_mode()
node3.srp_client_set_host_name('host3')
node3.srp_client_set_host_address('fd00::3')
node3.srp_client_add_service('srv3', '_test3._udp', 3333)


def check_node1_and_node2_have_three_services():
    verify(len(node1.srp_server_get_services()) == 3)
    verify(len(node2.srp_server_get_services()) == 3)


verify_within(check_node1_and_node2_have_three_services, WAIT_TIME)

# Register a second service on `node1` and `node3`. Registering the second
# service later ensures that it is registered in a new SRP update message
# different from the first SRP update message that registered the first one.

node1.srp_client_add_service('srv4', '_test4._udp', 4444)
node2.srp_client_add_service('srv5', '_test5,_udp', 5555)


def check_node1_and_node2_have_five_services():
    verify(len(node1.srp_server_get_services()) == 5)
    verify(len(node2.srp_server_get_services()) == 5)


verify_within(check_node1_and_node2_have_five_services, WAIT_TIME)
verify_servers_have_same_services(node1, node2)

# Remove first services added on `node1` and `node2`

node1.srp_client_remove_service('srv1', '_test1._udp')
node2.srp_client_remove_service('srv2', '_test2._udp')

# Check that both services are marked as deleted on all SRPL partners.


def check_srv1_and_srv2_services_are_deleted_on_node1_and_node2():
    for node in [node1, node2]:
        for service in node.srp_server_get_services():
            if service['instance'] in ['srv1', 'srv2']:
                verify(service['deleted'] == 'true')


verify_within(check_srv1_and_srv2_services_are_deleted_on_node1_and_node2, WAIT_TIME)

# Now we remove `srv3` from `node3`. Also fully remove `host1` on `node1`
# while retaining its name (not removing SRP key). Removing `host1` will
# also remove `srv4` which is added by same host.

node3.srp_client_remove_service('srv3', '_test3._udp')
node1.srp_client_remove_host(remove_key=False)

# Check that services are marked as deleted on all SRPL partners.


def check_srv3_and_srv4_are_deleted_on_node1_and_node2():
    for node in [node1, node2]:
        for service in node.srp_server_get_services():
            if service['instance'] in ['srv1', 'srv2', 'srv3', 'srv4']:
                verify(service['deleted'] == 'true')


verify_within(check_srv3_and_srv4_are_deleted_on_node1_and_node2, WAIT_TIME)
verify(len(node1.srp_server_get_services()) == 5)
verify_servers_have_same_services(node1, node2)

# Check that `host1` is marked as deleted on all SRPL partners

for node in [node1, node2]:
    for host in node1.srp_server_get_hosts():
        if host['name'] == 'host1':
            verify(host['deleted'] == 'true')
        else:
            verify(host['deleted'] == 'false')

# Finally enable SRPL on `node3` and check that everything is properly synced
# including all the deleted (but retained) service and host names.

node3.srp_replication_enable()


def check_node3_finishes_initial_sync():
    partners = node3.srp_replication_get_partners()
    verify(len(partners) == 2)
    for partner in partners:
        verify(partner['state'] == 'RoutineOperation')


verify_within(check_node3_finishes_initial_sync, WAIT_TIME)

verify_servers_have_same_services(node1, node3)

hosts = node3.srp_server_get_hosts()
verify(len(hosts) == 3)
for host in hosts:
    if host['name'] == 'host1':
        verify(host['deleted'] == 'true')
    else:
        verify(host['deleted'] == 'false')

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
