#!/usr/bin/env python3
#
#  Copyright (c) 2024, The OpenThread Authors.
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

# -----------------------------------------------------------------------------------------------------------------------
# Test description:
#
# Network with two BRs, BR1 acting as leader. Removing BR1 ensuring BR2 taking over.
#
#      ________________
#     /                \
#   br1 --- router --- br2
#   /        / \        \
#  c1       c2  c3      c4
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 60
cli.Node.set_time_speedup_factor(speedup)

br1 = cli.Node()
br2 = cli.Node()
router = cli.Node()
c1 = cli.Node()
c2 = cli.Node()
c3 = cli.Node()
c4 = cli.Node()

IF_INDEX = 1

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

br1.allowlist_node(router)
br1.allowlist_node(br2)
br1.allowlist_node(c1)

br2.allowlist_node(router)
br2.allowlist_node(br1)
br2.allowlist_node(c4)

router.allowlist_node(br1)
router.allowlist_node(br2)
router.allowlist_node(c2)
router.allowlist_node(c3)

c1.allowlist_node(br1)

c2.allowlist_node(router)
c3.allowlist_node(router)

c4.allowlist_node(br2)

br1.form("multi-br")
br2.join(br1)
router.join(br1)
c1.join(br1, cli.JOIN_TYPE_END_DEVICE)
c2.join(br1, cli.JOIN_TYPE_END_DEVICE)
c3.join(br1, cli.JOIN_TYPE_END_DEVICE)
c4.join(br1, cli.JOIN_TYPE_END_DEVICE)

verify(br1.get_state() == 'leader')
verify(br2.get_state() == 'router')
verify(router.get_state() == 'router')
verify(c1.get_state() == 'child')
verify(c2.get_state() == 'child')
verify(c3.get_state() == 'child')
verify(c4.get_state() == 'child')

nodes_non_br = [router, c1, c2, c3, c4]

# -----------------------------------------------------------------------------------------------------------------------
# Test implementation

# Start the first BR

br1.srp_server_set_addr_mode('unicast')
br1.srp_server_auto_enable()

br1.br_init(IF_INDEX, 1)
br1.br_enable()

time.sleep(1)
verify(br1.br_get_state() == 'running')

br1_local_omr = br1.br_get_local_omrprefix()
br1_favored_omr = br1.br_get_favored_omrprefix().split()[0]
verify(br1_local_omr == br1_favored_omr)

br1_local_onlink = br1.br_get_local_onlinkprefix()
br1_favored_onlink = br1.br_get_favored_onlinkprefix().split()[0]
verify(br1_local_onlink == br1_favored_onlink)

# Start the second BR

br2.br_init(IF_INDEX, 1)
br2.br_enable()

time.sleep(1)
verify(br2.br_get_state() == 'running')

br2_local_omr = br2.br_get_local_omrprefix()
br2_favored_omr = br2.br_get_favored_omrprefix().split()[0]
verify(br2_favored_omr == br1_favored_omr)

br2_favored_onlink = br2.br_get_favored_onlinkprefix().split()[0]
verify(br2_favored_onlink == br1_favored_onlink)

verify(br1.srp_server_get_state() == 'running')
verify(br2.srp_server_get_state() == 'disabled')

# Register SRP services on all nodes

for node in nodes_non_br:
    node.srp_client_enable_auto_start_mode()
    verify(node.srp_client_get_auto_start_mode() == 'Enabled')
    node.srp_client_set_host_name('host' + str(node.index))
    node.srp_client_enable_auto_host_address()
    node.srp_client_add_service('srv' + str(node.index), '_test._udp', 777, 0, 0)

time.sleep(1)

hosts = br1.srp_server_get_hosts()
verify(len(hosts) == len(nodes_non_br))

services = br1.srp_server_get_services()
verify(len(services) == len(nodes_non_br))

# Ensure that all registered addresses are derived from BR1 OMR.

for host in hosts:
    verify(host['addresses'][0].startswith(br1_local_omr[:-4]))

# Start SRP server on BR2

br2.srp_server_set_addr_mode('unicast')
br2.srp_server_auto_enable()

time.sleep(1)

verify(br2.srp_server_get_state() == 'running')

# De-activate BR1

br1.br_disable()
br1.thread_stop()
br1.interface_down()
del br1

c1.allowlist_node(br2)
br2.allowlist_node(c1)

# Wait long enough for BR2 to take over

time.sleep(5)

# Validate that everything is registered with BR2

hosts = br2.srp_server_get_hosts()
verify(len(hosts) == len(nodes_non_br))

services = br2.srp_server_get_services()
verify(len(services) == len(nodes_non_br))

# Ensure that all registered addresses are now derived from BR2
# OMR prefix.

for host in hosts:
    verify(host['addresses'][0].startswith(br2_local_omr[:-4]))

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
