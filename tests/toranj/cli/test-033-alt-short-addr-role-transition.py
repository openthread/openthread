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
# Validate the use alternate short address after role transition from child to router.
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 10
cli.Node.set_time_speedup_factor(speedup)

leader = cli.Node()
node = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Test implementation

INVALID_SHORT_ADDR = '0xfffe'
ALT_SHORT_ADDR_TIMEOUT = 10

verify(leader.get_mac_alt_short_addr() == INVALID_SHORT_ADDR)

# Form topology

leader.form('alt-shrt-addr')
node.join(leader, cli.JOIN_TYPE_REED)

verify(leader.get_state() == 'leader')
verify(node.get_state() == 'child')

verify(len(leader.get_child_table()) == 1)

# Check the short address and alternate short address

node_rloc16_as_child = '0x' + node.get_rloc16()

verify(node.get_mac_alt_short_addr() == INVALID_SHORT_ADDR)

# Allow `node` to transition from child to router role

node.set_router_selection_jitter(1)
node.set_router_eligible('enable')


def check_node_become_router():
    verify(node.get_state() == 'router')


verify_within(check_node_become_router, 10)

# Make sure the old short address is now being used as
# the alternate short address.

node_rloc16_as_router = '0x' + node.get_rloc16()
verify(node_rloc16_as_router != node_rloc16_as_child)
verify(node.get_mac_alt_short_addr() == node_rloc16_as_child)

# Make sure the old short address is removed after the
# timeout

time.sleep(ALT_SHORT_ADDR_TIMEOUT / speedup)

verify(node.get_mac_alt_short_addr() == INVALID_SHORT_ADDR)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
