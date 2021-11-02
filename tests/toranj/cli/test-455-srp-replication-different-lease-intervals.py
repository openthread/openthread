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
#  Test SRP Replication behavior when different servers are configured with different lease intervals.
#  We validate that the lease intervals that are accepted by the first SRP server that processed the client's message
#  are also used by all SRPL partners.

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
SRP_SERVER_ANYCAST_PORT = 53

# ----------------------------------------------------------------------------------------------------------------------
# Test implementation

node1.form('srpl-lease')
node2.join(node1, cli.JOIN_TYPE_ROUTER)

# Change the lease and key lease intervals on `node1` to 8 seconds while
# keeping the lease intervals long on `node2`.

node1.srp_server_set_lease(8, 8, 8, 8)
node2.srp_server_set_lease(1800, 7200, 86400, 1209600)

# Enable SRPL on `node1` and `node2`

node1.srp_replication_enable()
node2.srp_replication_enable()


def check_node1_and_node2_start_srp_server():
    verify(node1.srp_server_get_state() == 'running')
    verify(node2.srp_server_get_state() == 'running')


verify_within(check_node1_and_node2_start_srp_server, WAIT_TIME)

# Start SRP client on `node1` and `node2` and explicitly specify the server
# address so that each client uses its own server.
node1.srp_client_start(node1.get_mleid_ip_addr(), SRP_SERVER_ANYCAST_PORT)
node2.srp_client_start(node2.get_mleid_ip_addr(), SRP_SERVER_ANYCAST_PORT)

# Register one service from `node1` and one from `node2`. Each client
# will send to its own server which have different lease intervals.

node1.srp_client_set_host_name('host1')
node1.srp_client_set_host_address('fd00::1')
node1.srp_client_add_service('srv1', '_test1._udp,_s11', 1111, 0, 0, ['a=01', 'x'])

node2.srp_client_set_host_name('host2')
node2.srp_client_set_host_address('fd00::2')
node2.srp_client_add_service('srv2', '_test2._udp', 2222)


def check_node1_and_node2_have_two_services():
    verify(len(node1.srp_server_get_services()) == 2)
    verify(len(node2.srp_server_get_services()) == 2)


verify_within(check_node1_and_node2_have_two_services, WAIT_TIME)

# Clear services on `node1` and `node2` (which removes them on client side
# without notifying the server. This allows us to check for key lease timeout.

node1.srp_client_clear_service('srv1', '_test1._udp')
node2.srp_client_clear_service('srv2', '_test2._udp')

# We wait for longer than 8 seconds which is the lease interval used by `node1`
# After the wait time, we check that service added on `node1` is expired and
# removed on both SRPL peers, but service from `node2` is still present.

time.sleep(15 / speedup)

node1_services = node1.srp_server_get_services()
verify(len(node1_services) == 1)
verify(node1_services[0]['instance'] == 'srv2')

node2_services = node2.srp_server_get_services()
verify(len(node2_services) == 1)
verify(node2_services[0]['instance'] == 'srv2')

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
