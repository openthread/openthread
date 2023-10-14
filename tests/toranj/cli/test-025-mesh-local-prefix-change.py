#!/usr/bin/env python3
#
#  Copyright (c) 2023, The OpenThread Authors.
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
# Test description: Validate update of unicast/multicast mesh-local addresses on prefix change
#
# Network topology
#
#    r1 ---- r2
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

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

r1.form('ml-change')
r2.join(r1)

verify(r1.get_state() == 'leader')
verify(r2.get_state() == 'router')

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

r1.srp_server_enable()
r1.srp_client_enable_auto_start_mode()
r2.srp_client_enable_auto_start_mode()

time.sleep(0.5)

ml_prefix = r1.get_mesh_local_prefix()
ml_prefix = ml_prefix[:ml_prefix.index("/64") - 1]

# Validate that r1 has 4 mesh-local address: ML-EID, RLOC, leader ALOC
# and since it is acting as SRP server the service ALOC.

r1_addrs = r1.get_ip_addrs()
verify(sum([addr.startswith(ml_prefix) for addr in r1_addrs]) == 4)

# Validate that r1 has link-local and realm-local All Thread Nodes
# multicast addresses which are prefix-based and use mesh-local
# prefix.

r1_maddrs = r1.get_ip_maddrs()
verify(sum([ml_prefix in maddr for maddr in r1_maddrs]) == 2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Change the mesh-local prefix on all devices.

r2.cli('dataset clear')
r2.cli('dataset meshlocalprefix fd00:1122:3344:5566::')
r2.cli('dataset delaytimer 1100')  # `toranj` allows 1000 msec as minimum in its config
r2.cli('dataset updater start')

time.sleep(0.5)

ml_prefix = r1.get_mesh_local_prefix()
ml_prefix = ml_prefix[:ml_prefix.index("/64") - 1]

verify(ml_prefix == 'fd00:1122:3344:5566:')

# Validate that all 4 mesh-local address on r1 are updated to use the
# new mesh-local prefix.

r1_addrs = r1.get_ip_addrs()
verify(sum([addr.startswith(ml_prefix) for addr in r1_addrs]) == 4)

# Validate the network data entry for SRP server is also updated to
# used the new address.

verify(r1.srp_client_get_server_address().startswith(ml_prefix))

# Validate the r1 multicast addresses are also updated

r1_maddrs = r1.get_ip_maddrs()
verify(sum([ml_prefix in maddr for maddr in r1_maddrs]) == 2)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
