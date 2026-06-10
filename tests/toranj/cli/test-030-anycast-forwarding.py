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
# Test description: ALOC address forwarding to ED and SED devices.
#
# Network topology
#
#    r1 ---- r2 ---- r3
#                    /|\
#                   / | \
#                 ed sed sed2
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Node` instances

speedup = 40
cli.Node.set_time_speedup_factor(speedup)

r1 = cli.Node()
r2 = cli.Node()
r3 = cli.Node()
ed = cli.Node()
sed = cli.Node()
sed2 = cli.Node()

nodes = [r1, r2, r3, ed, sed, sed2]

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

r1.allowlist_node(r2)

r2.allowlist_node(r1)
r2.allowlist_node(r3)

r3.allowlist_node(r2)
r3.allowlist_node(ed)
r3.allowlist_node(sed)
r3.allowlist_node(sed2)

ed.allowlist_node(r3)
sed.allowlist_node(r3)
sed2.allowlist_node(r3)

r1.form('aloc')
r2.join(r1)
r3.join(r2)
ed.join(r2, cli.JOIN_TYPE_END_DEVICE)
sed.join(r3, cli.JOIN_TYPE_SLEEPY_END_DEVICE)
sed2.join(r3, cli.JOIN_TYPE_SLEEPY_END_DEVICE)

sed.set_pollperiod(400)
sed2.set_pollperiod(500)

verify(r1.get_state() == 'leader')
verify(r2.get_state() == 'router')
verify(r3.get_state() == 'router')
verify(ed.get_state() == 'child')
verify(sed.get_state() == 'child')
verify(sed.get_state() == 'child')

sed.set_mode('n')

for child in [ed, sed, sed2]:
    verify(int(child.get_parent_info()['Rloc'], 16) == int(r3.get_rloc16(), 16))

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation


def check_netdata_services(expected_num_services):
    # Check that all nodes see the `expected_num_services` service
    # entries in network data.
    for node in nodes:
        verify(len(node.get_netdata_services()) == expected_num_services)


wait_time = 5

# Add a service on ed which is non-sleepy child.

ed.cli('service add 44970 11 00')
ed.register_netdata()

verify_within(check_netdata_services, wait_time, 1)

# Verify that the ALOC associated with a non-sleepy child's service is
# pingable from all nodes.

aloc1 = r1.get_mesh_local_prefix().split('/')[0] + 'ff:fe00:fc10'

for node in nodes:
    node.ping(aloc1)

# Add a service now from `sed` which is a sleepy child and again make sure
# its ALOC can be pinged from all other nodes.

sed.cli('service add 44970 22 00')
sed.register_netdata()

verify_within(check_netdata_services, wait_time, 2)

aloc2 = r1.get_mesh_local_prefix().split('/')[0] + 'ff:fe00:fc11'

r1.ping(aloc2)

for node in nodes:
    node.ping(aloc2)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
