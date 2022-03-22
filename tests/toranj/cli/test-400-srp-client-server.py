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

# -----------------------------------------------------------------------------------------------------------------------
# Test description: joining (as router, end-device, sleepy) - two node network

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 4
cli.Node.set_time_speedup_factor(speedup)

server = cli.Node()
client = cli.Node()

WAIT_TIME = 5

# -----------------------------------------------------------------------------------------------------------------------
# Test implementation

server.form('srp-test')
verify(server.get_state() == 'leader')

client.join(server)
verify(client.get_state() == 'router')

# Check initial state of SRP client and server
verify(server.srp_server_get_state() == 'disabled')
verify(server.srp_server_get_addr_mode() == 'unicast')
verify(client.srp_client_get_state() == 'Disabled')
verify(client.srp_client_get_auto_start_mode() == 'Disabled')

# Start server and client and register single service
server.srp_server_enable()
client.srp_client_enable_auto_start_mode()

client.srp_client_set_host_name('host')
client.srp_client_set_host_address('fd00::cafe')
client.srp_client_add_service('ins', '_test._udp', 777, 2, 1)


def check_server_has_service():
    verify(len(server.srp_server_get_hosts()) > 0)


verify_within(check_server_has_service, WAIT_TIME)

# Check state and service/host info on client and server

verify(client.srp_client_get_auto_start_mode() == 'Enabled')
verify(client.srp_client_get_state() == 'Enabled')
verify(client.srp_client_get_server_address() == server.get_mleid_ip_addr())

verify(client.srp_client_get_host_state() == 'Registered')
verify(client.srp_client_get_host_name() == 'host')
addresses = client.srp_client_get_host_address()
verify(len(addresses) == 1)
verify(addresses[0] == 'fd00:0:0:0:0:0:0:cafe')

services = client.srp_client_get_services()
verify(len(services) == 1)
service = services[0]
verify(service['instance'] == 'ins')
verify(service['name'] == '_test._udp')
verify(service['state'] == 'Registered')
verify(service['port'] == '777')
verify(service['priority'] == '2')
verify(service['weight'] == '1')

verify(server.srp_server_get_state() == 'running')

hosts = server.srp_server_get_hosts()
verify(len(hosts) == 1)
host = hosts[0]
verify(host['name'] == 'host')
verify(host['deleted'] == 'false')
verify(host['addresses'] == ['fd00:0:0:0:0:0:0:cafe'])

services = server.srp_server_get_services()
verify(len(services) == 1)
service = services[0]
verify(service['instance'] == 'ins')
verify(service['name'] == '_test._udp')
verify(service['deleted'] == 'false')
verify(service['subtypes'] == '(null)')
verify(service['port'] == '777')
verify(service['priority'] == '2')
verify(service['weight'] == '1')
verify(service['host'] == 'host')
verify(service['addresses'] == ['fd00:0:0:0:0:0:0:cafe'])

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
