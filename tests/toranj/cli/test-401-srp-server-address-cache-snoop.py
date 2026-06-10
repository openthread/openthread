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
# Validate registered host addresses are properly added in address cache table
# on SRP server.
#
#    r1 (leader) ----- r2 ------ r3
#     /  |                        \
#    /   |                         \
#   fed1  sed1                     sed2
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

#-----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 10
cli.Node.set_time_speedup_factor(speedup)

r1 = cli.Node()
r2 = cli.Node()
r3 = cli.Node()
fed1 = cli.Node()
sed1 = cli.Node()
sed2 = cli.Node()

WAIT_TIME = 5

#-----------------------------------------------------------------------------------------------------------------------
# Form topology

r1.allowlist_node(r2)
r1.allowlist_node(fed1)
r1.allowlist_node(sed1)

r2.allowlist_node(r1)
r2.allowlist_node(r3)
r2.allowlist_node(sed2)

r3.allowlist_node(r2)
r3.allowlist_node(sed2)

fed1.allowlist_node(r1)
sed1.allowlist_node(r1)

sed2.allowlist_node(r3)

r1.form('srp-snoop')
r2.join(r1)
r3.join(r2)
fed1.join(r1, cli.JOIN_TYPE_REED)
sed1.join(r1, cli.JOIN_TYPE_SLEEPY_END_DEVICE)
sed2.join(r1, cli.JOIN_TYPE_SLEEPY_END_DEVICE)
sed1.set_pollperiod(500)
sed2.set_pollperiod(500)

verify(r1.get_state() == 'leader')
verify(r2.get_state() == 'router')
verify(r2.get_state() == 'router')
verify(sed1.get_state() == 'child')
verify(sed2.get_state() == 'child')
verify(fed1.get_state() == 'child')

r2_rloc = int(r2.get_rloc16(), 16)
r3_rloc = int(r3.get_rloc16(), 16)
fed1_rloc = int(fed1.get_rloc16(), 16)

# Start server and client and register single service
r1.srp_server_enable()

r2.srp_client_enable_auto_start_mode()
r2.srp_client_set_host_name('r2')
r2.srp_client_set_host_address('fd00::2')
r2.srp_client_add_service('srv2', '_test._udp', 222, 0, 0)


def check_server_has_host(num_hosts):
    verify(len(r1.srp_server_get_hosts()) >= num_hosts)


verify_within(check_server_has_host, WAIT_TIME, arg=1)

cache_table = r1.get_eidcache()

for entry in cache_table:
    fields = entry.strip().split(' ')
    if (fields[0] == 'fd00:0:0:0:0:0:0:2'):
        verify(int(fields[1], 16) == r2_rloc)
        verify(fields[2] == 'snoop')
        break
else:
    verify(False)  # did not find cache entry

# Register from r3 which one hop away from r1 server.

r3.srp_client_enable_auto_start_mode()
r3.srp_client_set_host_name('r3')
r3.srp_client_set_host_address('fd00::3')
r3.srp_client_add_service('srv3', '_test._udp', 333, 0, 0)

verify_within(check_server_has_host, WAIT_TIME, arg=2)

cache_table = r1.get_eidcache()

for entry in cache_table:
    fields = entry.strip().split(' ')
    if (fields[0] == 'fd00:0:0:0:0:0:0:3'):
        verify(int(fields[1], 16) == r3_rloc)
        verify(fields[2] == 'snoop')
        break
else:
    verify(False)  # did not find cache entry

# Register from sed2 which child of r3. The cache table should
# use the `r3` as the parent of sed2.

sed2.srp_client_enable_auto_start_mode()
sed2.srp_client_set_host_name('sed2')
sed2.srp_client_set_host_address('fd00::1:3')
sed2.srp_client_add_service('srv4', '_test._udp', 333, 0, 0)

verify_within(check_server_has_host, WAIT_TIME, arg=3)

cache_table = r1.get_eidcache()

for entry in cache_table:
    fields = entry.strip().split(' ')
    if (fields[0] == 'fd00:0:0:0:0:0:1:3'):
        verify(int(fields[1], 16) == r3_rloc)
        verify(fields[2] == 'snoop')
        break
else:
    verify(False)  # did not find cache entry

# Register from fed1 which child of server (r1) itself. The cache table should
# be properly updated

fed1.srp_client_enable_auto_start_mode()
fed1.srp_client_set_host_name('fed1')
fed1.srp_client_set_host_address('fd00::2:3')
fed1.srp_client_add_service('srv5', '_test._udp', 555, 0, 0)

verify_within(check_server_has_host, WAIT_TIME, arg=4)

cache_table = r1.get_eidcache()

for entry in cache_table:
    fields = entry.strip().split(' ')
    if (fields[0] == 'fd00:0:0:0:0:0:2:3'):
        verify(int(fields[1], 16) == fed1_rloc)
        verify(fields[2] == 'snoop')
        break
else:
    verify(False)  # did not find cache entry

# Register from sed1 which is a sleepy child of server (r1).
# The cache table should not be updated for sleepy child.

sed1.srp_client_enable_auto_start_mode()
sed1.srp_client_set_host_name('sed1')
sed1.srp_client_set_host_address('fd00::3:4')
sed1.srp_client_add_service('srv5', '_test._udp', 555, 0, 0)

verify_within(check_server_has_host, WAIT_TIME, arg=4)

cache_table = r1.get_eidcache()

for entry in cache_table:
    fields = entry.strip().split(' ')
    verify(fields[0] != 'fd00:0:0:0:0:0:3:4')

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
