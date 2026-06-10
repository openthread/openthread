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
# BR sending ICMPv6 Unreachable error.
#
#    - - - AIL- - -
#    |            |
#   BR1          BR2
#                 |
#               router2
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 20
cli.Node.set_time_speedup_factor(speedup)

br1 = cli.Node()
br2 = cli.Node()
router2 = cli.Node()

IF_INDEX = 1

# -----------------------------------------------------------------------------------------------------------------------
# Test implementation

# Start `br1` on its own Thread network with a custom OMR
# prefix and a custom on-link prefix. Both prefixes are
# non-ULA.

br1.form('net1')
verify(br1.get_state() == 'leader')

br1.add_prefix('1000::/64', 'paors')
br1.register_netdata()

br1.br_init(IF_INDEX, 1)
br1.br_enable()

br1.br_set_test_local_onlinkprefix('2000::/64')

time.sleep(2)
verify(br1.br_get_state() == 'running')

br1_local_onlink = br1.br_get_local_onlinkprefix()
br1_favored_onlink = br1.br_get_favored_onlinkprefix().split()[0]
verify(br1_local_onlink == br1_favored_onlink)
verify(br1_local_onlink == '2000:0:0:0::/64')

br1_local_omr = br1.br_get_local_omrprefix()
br1_favored_omr = br1.br_get_favored_omrprefix().split()[0]
verify(br1_favored_omr == '1000:0:0:0::/64')

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Start `br2` on its own Thread network.

br2.form('net2')
router2.join(br2)
verify(br2.get_state() == 'leader')
verify(router2.get_state() == 'router')

br2.br_init(IF_INDEX, 1)
br2.br_enable()

time.sleep(2)
verify(br2.br_get_state() == 'running')

br2_local_omr = br2.br_get_local_omrprefix()
br2_favored_omr = br2.br_get_favored_omrprefix().split()[0]
verify(br2_local_omr == br2_favored_omr)

br2_local_onlink = br2.br_get_local_onlinkprefix()
br2_favored_onlink = br2.br_get_favored_onlinkprefix().split()[0]
verify(br2_local_onlink != br2_favored_onlink)
verify(br2_favored_onlink == '2000:0:0:0::/64')

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Validate that the `br2` published '::/0' route due to
# discovery of a non-ULA on-link prefix.

for node in [br2, router2]:
    routes = node.get_netdata_routes()
    verify(len(routes) == 1)
    verify(routes[0].startswith('::/0 s'))

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Send a ping from `router2` to an AIL address (derived from the
# advertised on-link prefix). Verify that, BR2 receives and
# forwards the message, and BR2 does not send an ICMPv6 error, as
# the destination is within the AIL.

router2.ping('2000::1', verify_success=False)

verify(br2.get_br_counter_unicast_outbound_packets() == 1)

rx_history = router2.cli('history rx list')
verify(len(rx_history) == 0)

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Send a ping from `router2` to an address on BR1 mesh network
# (derived from its OMR prefix). Verify that again it is
# received and forwarded by BR2 and again no ICMPv6 error is
# sent.

router2.ping('1000::1', verify_success=False)

verify(br2.get_br_counter_unicast_outbound_packets() == 2)

rx_history = router2.cli('history rx list')
verify(len(rx_history) == 0)

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Send a ping to an outside AIL address, make sure BR2
# does receive and forward it and now also sends an ICMPv6
# error

router2.ping('3000::', verify_success=False)

verify(br2.get_br_counter_unicast_outbound_packets() == 3)

rx_history = router2.cli('history rx list')
verify(rx_history[1].strip().startswith('type:ICMP6(Unreach)'))

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
