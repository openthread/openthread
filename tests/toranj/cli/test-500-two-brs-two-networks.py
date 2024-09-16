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
# Two BRs on two different Thread networks.

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 60
cli.Node.set_time_speedup_factor(speedup)

br1 = cli.Node()
br2 = cli.Node()

WAIT_TIME = 5
IF_INDEX = 1

# -----------------------------------------------------------------------------------------------------------------------
# Test implementation

# Start first BR with its own Thread network

br1.form('net1')
verify(br1.get_state() == 'leader')

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

# Start second BR with its own Thread network.

br2.form('net2')
verify(br2.get_state() == 'leader')

br2.br_init(IF_INDEX, 1)
br2.br_enable()

time.sleep(1)
verify(br2.br_get_state() == 'running')

br2_local_omr = br2.br_get_local_omrprefix()
br2_favored_omr = br2.br_get_favored_omrprefix().split()[0]
verify(br2_local_omr == br2_favored_omr)

br2_local_onlink = br2.br_get_local_onlinkprefix()
br2_favored_onlink = br2.br_get_favored_onlinkprefix().split()[0]
verify(br2_local_onlink != br2_favored_onlink)

# BR2 should see and favor the on-link prefix already advertised by BR1.

verify(br1_favored_onlink == br2_favored_onlink)

# Check that the two BRs discover and track each other (not as peer BR since
# connected to different networks).

for br in [br1, br1]:
    routers = br.br_get_routers()
    verify(len(routers) > 0)
    for router in routers:
        verify('reachable:yes' in router)
        verify('S:1' in router)
        verify(not router.endswith('(peer BR)'))

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
