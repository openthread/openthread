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
#  Test SRPL initial sync between multiple partners.
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

node1.form('srpl-sync')
node2.join(node1, cli.JOIN_TYPE_END_DEVICE)
node3.join(node1, cli.JOIN_TYPE_END_DEVICE)

# Enable SRPL on `node1`


def check_node1_is_running():
    verify(node1.srp_server_get_state() == 'running')


node1.srp_replication_enable()
verify_within(check_node1_is_running, WAIT_TIME)

# Register two services separately from `node1`

node1.srp_client_enable_auto_start_mode()
node1.srp_client_set_host_name('host1')
node1.srp_client_set_host_address('fd00::1')

node1.srp_client_add_service('srv11', '_test1._udp', 111, 1, 2)
time.sleep(1.0 / speedup)
node1.srp_client_add_service('srv12', '_test2._udp', 122, 0, 0)

# Register a service with two subtypes from `node2`

node2.srp_client_enable_auto_start_mode()
node2.srp_client_set_host_name('host2')
node2.srp_client_set_host_address('fd00::2')

node2.srp_client_add_service('srv2', '_xyz._udp,_sub1,_sub2', 222)

# Make sure all services are present on `node1`


def check_node1_has_all_services():
    verify(len(node1.srp_server_get_services()) == 3)


verify_within(check_node1_has_all_services, WAIT_TIME)

# Enable SRPL on `node2`


def check_node2_is_running():
    verify(node2.srp_server_get_state() == 'running')


node2.srp_replication_enable()
verify_within(check_node2_is_running, WAIT_TIME)

# Verify that both servers see the same set of services

verify_servers_have_same_services(node1, node2)

# Enable SRPL on `node3` and wait till it fully syncs


def check_node3_syncs_with_both_node1_and_node2():
    partners = node3.srp_replication_get_partners()
    verify(len(partners) == 2)
    for partner in partners:
        verify(partner['state'] == 'RoutineOperation')


node3.srp_replication_enable()
verify_within(check_node3_syncs_with_both_node1_and_node2, WAIT_TIME)

verify_servers_have_same_services(node1, node3)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
