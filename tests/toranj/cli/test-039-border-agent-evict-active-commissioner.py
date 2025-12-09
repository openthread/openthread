#!/usr/bin/env python3
#
#  Copyright (c) 2025, The OpenThread Authors.
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
# Test description:
# This test covers the behavior of `BorderAgent::EvictActiveCommissioner()`
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 25
cli.Node.set_time_speedup_factor(speedup)

leader = cli.Node()
commissioner = cli.Node()
agent = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Form topology
#

leader.form('evictcmmr')
commissioner.join(leader)
agent.join(leader)

verify(leader.get_state() == 'leader')
verify(commissioner.get_state() == 'router')
verify(agent.get_state() == 'router')

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

commissioner.cli('commissioner start')


def check_commissioner_state_is_active():
    verify(commissioner.cli('commissioner state')[0] == 'active')


verify_within(check_commissioner_state_is_active, 10)

# Verify Commissioning Info in Network Data and that `commissioner` is accepted and active.

data = leader.get_netdata_commissioning()
rloc16 = int(data[0].strip().split()[1], 16)
verify(rloc16 == int(commissioner.get_rloc16(), 16))

# Evict the current active commissioner.

agent.cli('ba evictcommissioner')

# Check that the Network Data Commissioning Info is cleared after eviction.


def check_netdata_commissioning_info():
    # check there is no active commissioner
    data = leader.get_netdata_commissioning()
    verify(data[0].strip().split()[1] == '-')


verify_within(check_netdata_commissioning_info, 10)

# Check that the original commissioner's state becomes disabled


def check_commissioner_state_is_disabled():
    verify(commissioner.cli('commissioner state')[0] == 'disabled')


verify_within(check_commissioner_state_is_disabled, 60 / speedup)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
