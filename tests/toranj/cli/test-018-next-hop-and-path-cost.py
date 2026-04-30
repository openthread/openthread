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
# Test description: Next hop and path cost calculation.
#
# Network topology
#
#     r1 ---- r2...2...r3
#    / |       \      /  \
#   /  |        \    /    \
# fed1 fed2       r4...1...r5 ---- fed3
#
# Link r2 --> r3 is configured to be at link quality of 2.
# Link r5 --> r4 is configured to be at link quality of 1.
# Link r1 --> fed2 is configured to be at link quality 1.
# Other links are at link quality 3 (best possible link quality).
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
r4 = cli.Node()
r5 = cli.Node()
fed1 = cli.Node()
fed2 = cli.Node()
fed3 = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

r1.allowlist_node(r2)
r1.allowlist_node(fed1)
r1.allowlist_node(fed2)

r2.allowlist_node(r1)
r2.allowlist_node(r3)
r2.allowlist_node(r4)

r3.allowlist_node(r2)
r3.allowlist_node(r4)
r3.allowlist_node(r5)
r3.set_macfilter_lqi_to_node(r2, 2)

r4.allowlist_node(r2)
r4.allowlist_node(r3)
r4.allowlist_node(r5)
r4.set_macfilter_lqi_to_node(r5, 1)

r5.allowlist_node(r4)
r5.allowlist_node(r3)
r5.allowlist_node(fed3)

fed1.allowlist_node(r1)
fed2.allowlist_node(r1)
fed3.allowlist_node(r5)
fed2.set_macfilter_lqi_to_node(r1, 1)

r1.form('hop-cost')
r2.join(r1)
r3.join(r1)
r4.join(r1)
r5.join(r1)
fed1.join(r1, cli.JOIN_TYPE_REED)
fed2.join(r1, cli.JOIN_TYPE_REED)
fed3.join(r1, cli.JOIN_TYPE_REED)

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

r1_rloc = int(r1.get_rloc16(), 16)
r2_rloc = int(r2.get_rloc16(), 16)
r3_rloc = int(r3.get_rloc16(), 16)
r4_rloc = int(r4.get_rloc16(), 16)
r5_rloc = int(r5.get_rloc16(), 16)

fed1_rloc = int(fed1.get_rloc16(), 16)
fed2_rloc = int(fed2.get_rloc16(), 16)
fed3_rloc = int(fed3.get_rloc16(), 16)


def parse_nexthop(line):
    # Example: "0x5000 cost:3" -> (0x5000, 3).
    items = line.strip().split(' ', 2)
    return (int(items[0], 16), int(items[1].split(':')[1]))


def check_nexthops_and_costs():
    #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    # `r1` next hops and costs
    verify(parse_nexthop(r1.get_nexthop(r1_rloc)) == (r1_rloc, 0))
    verify(parse_nexthop(r1.get_nexthop(r2_rloc)) == (r2_rloc, 1))
    verify(parse_nexthop(r1.get_nexthop(r3_rloc)) == (r2_rloc, 3))
    verify(parse_nexthop(r1.get_nexthop(r4_rloc)) == (r2_rloc, 2))
    verify(parse_nexthop(r1.get_nexthop(r5_rloc)) == (r2_rloc, 4))
    verify(parse_nexthop(r1.get_nexthop(fed3_rloc)) == (r2_rloc, 5))
    # On `r1` its children can be reached directly.
    verify(parse_nexthop(r1.get_nexthop(fed1_rloc)) == (fed1_rloc, 1))
    verify(parse_nexthop(r1.get_nexthop(fed2_rloc)) == (fed2_rloc, 1))

    #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    # `r2` next hops and costs
    verify(parse_nexthop(r2.get_nexthop(r1_rloc)) == (r1_rloc, 1))
    verify(parse_nexthop(r2.get_nexthop(r2_rloc)) == (r2_rloc, 0))
    # On `r2` the direct link to `r3` and the path through `r4` both
    # have the same cost, but the direct link should be preferred.
    verify(parse_nexthop(r2.get_nexthop(r3_rloc)) == (r3_rloc, 2))
    verify(parse_nexthop(r2.get_nexthop(r4_rloc)) == (r4_rloc, 1))
    # On 'r2' the path to `r5` can go through `r3` or `r4`
    # as both have the same cost.
    (nexthop, cost) = parse_nexthop(r2.get_nexthop(r5_rloc))
    verify(cost == 3)
    verify(nexthop in [r3_rloc, r4_rloc])
    verify(parse_nexthop(r2.get_nexthop(fed1_rloc)) == (r1_rloc, 2))

    #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    # `r3` next hops and costs
    verify(parse_nexthop(r3.get_nexthop(r3_rloc)) == (r3_rloc, 0))
    verify(parse_nexthop(r3.get_nexthop(r5_rloc)) == (r5_rloc, 1))
    verify(parse_nexthop(r3.get_nexthop(r4_rloc)) == (r4_rloc, 1))
    verify(parse_nexthop(r3.get_nexthop(r2_rloc)) == (r2_rloc, 2))
    # On `r3` the path to `r1` can go through `r2` or `r4`
    # as both have the same cost.
    (nexthop, cost) = parse_nexthop(r3.get_nexthop(r1_rloc))
    verify(cost == 3)
    verify(nexthop in [r2_rloc, r4_rloc])
    # On `r3` the path to fed1 should use the same next hop as `r1`
    verify(parse_nexthop(r3.get_nexthop(fed2_rloc)) == (nexthop, 4))

    #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    # `r4` next hops and costs
    verify(parse_nexthop(r4.get_nexthop(fed1_rloc)) == (r2_rloc, 3))
    verify(parse_nexthop(r4.get_nexthop(r1_rloc)) == (r2_rloc, 2))
    verify(parse_nexthop(r4.get_nexthop(r2_rloc)) == (r2_rloc, 1))
    verify(parse_nexthop(r4.get_nexthop(r3_rloc)) == (r3_rloc, 1))
    verify(parse_nexthop(r4.get_nexthop(r4_rloc)) == (r4_rloc, 0))
    # On `r4` even though we have a direct link to `r5`
    # the path cost through `r3` has a smaller cost over
    # the direct link cost.
    verify(parse_nexthop(r4.get_nexthop(r5_rloc)) == (r3_rloc, 2))
    verify(parse_nexthop(r4.get_nexthop(fed3_rloc)) == (r3_rloc, 3))

    #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    # `r5` next hops and costs
    verify(parse_nexthop(r5.get_nexthop(fed3_rloc)) == (fed3_rloc, 1))
    verify(parse_nexthop(r5.get_nexthop(r5_rloc)) == (r5_rloc, 0))
    verify(parse_nexthop(r5.get_nexthop(r3_rloc)) == (r3_rloc, 1))
    verify(parse_nexthop(r5.get_nexthop(r4_rloc)) == (r3_rloc, 2))
    verify(parse_nexthop(r5.get_nexthop(r2_rloc)) == (r3_rloc, 3))
    verify(parse_nexthop(r5.get_nexthop(r1_rloc)) == (r3_rloc, 4))
    verify(parse_nexthop(r5.get_nexthop(fed1_rloc)) == (r3_rloc, 5))

    #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    # `fed1` next hops and costs
    verify(parse_nexthop(fed1.get_nexthop(fed1_rloc)) == (fed1_rloc, 0))
    verify(parse_nexthop(fed1.get_nexthop(r1_rloc)) == (r1_rloc, 1))
    verify(parse_nexthop(fed1.get_nexthop(r2_rloc)) == (r1_rloc, 2))
    verify(parse_nexthop(fed1.get_nexthop(r3_rloc)) == (r1_rloc, 4))
    verify(parse_nexthop(fed1.get_nexthop(r4_rloc)) == (r1_rloc, 3))
    verify(parse_nexthop(fed1.get_nexthop(r5_rloc)) == (r1_rloc, 5))
    verify(parse_nexthop(fed1.get_nexthop(fed3_rloc)) == (r1_rloc, 6))
    # On `fed1`, path to `fed2` should go through our parent.
    verify(parse_nexthop(fed1.get_nexthop(fed2_rloc)) == (r1_rloc, 2))

    #- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    # `fed2` next hops and costs
    verify(parse_nexthop(fed2.get_nexthop(fed2_rloc)) == (fed2_rloc, 0))
    verify(parse_nexthop(fed2.get_nexthop(r1_rloc)) == (r1_rloc, 4))
    verify(parse_nexthop(fed2.get_nexthop(r2_rloc)) == (r1_rloc, 5))
    verify(parse_nexthop(fed2.get_nexthop(r3_rloc)) == (r1_rloc, 7))
    verify(parse_nexthop(fed2.get_nexthop(r4_rloc)) == (r1_rloc, 6))
    verify(parse_nexthop(fed2.get_nexthop(r5_rloc)) == (r1_rloc, 8))
    verify(parse_nexthop(fed2.get_nexthop(fed3_rloc)) == (r1_rloc, 9))
    verify(parse_nexthop(fed2.get_nexthop(fed1_rloc)) == (r1_rloc, 5))


verify_within(check_nexthops_and_costs, 5)

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Disable `r4` and check nexthop and cost on it and on other
# nodes.
#
#
#     r1 ---- r2...2...r3
#    / |                 \
#   /  |                  \
# fed1 fed2       r4       r5 ---- fed3

r4.thread_stop()
r4.interface_down()

verify(parse_nexthop(r4.get_nexthop(r2_rloc)) == (0xfffe, 16))
verify(parse_nexthop(r4.get_nexthop(r4_rloc)) == (0xfffe, 16))


def check_nexthops_and_costs_after_r4_detach():
    # Make sure we have no next hop towards `r4`.
    verify(parse_nexthop(r1.get_nexthop(r4_rloc)) == (0xfffe, 16))
    verify(parse_nexthop(r2.get_nexthop(r4_rloc)) == (0xfffe, 16))
    verify(parse_nexthop(r3.get_nexthop(r4_rloc)) == (0xfffe, 16))
    verify(parse_nexthop(r5.get_nexthop(r4_rloc)) == (0xfffe, 16))
    verify(parse_nexthop(fed3.get_nexthop(r4_rloc)) == (r5_rloc, 16))
    # Check cost and next hop on other nodes
    verify(parse_nexthop(r1.get_nexthop(r5_rloc)) == (r2_rloc, 4))
    verify(parse_nexthop(r2.get_nexthop(r3_rloc)) == (r3_rloc, 2))
    verify(parse_nexthop(r3.get_nexthop(r2_rloc)) == (r2_rloc, 2))
    verify(parse_nexthop(fed3.get_nexthop(fed1_rloc)) == (r5_rloc, 6))


verify_within(check_nexthops_and_costs_after_r4_detach, 45)
verify(r1.get_state() == 'leader')

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Disable `r1` (which was previous leader) and check
# routes on other nodes
#
#
#     r1      r2...2...r3
#    / |                 \
#   /  |                  \
# fed1 fed2       r4       r5 ---- fed3

r1.thread_stop()
r1.interface_down()
fed1.thread_stop()
fed1.interface_down()
fed2.thread_stop()
fed2.interface_down()

verify(parse_nexthop(r1.get_nexthop(r2_rloc)) == (0xfffe, 16))
verify(parse_nexthop(r1.get_nexthop(r1_rloc)) == (0xfffe, 16))
verify(parse_nexthop(r1.get_nexthop(fed1_rloc)) == (0xfffe, 16))


def check_nexthops_and_costs_after_r1_detach():
    verify(parse_nexthop(r2.get_nexthop(r1_rloc)) == (0xfffe, 16))
    verify(parse_nexthop(r3.get_nexthop(r1_rloc)) == (0xfffe, 16))
    verify(parse_nexthop(r5.get_nexthop(r1_rloc)) == (0xfffe, 16))
    verify(parse_nexthop(fed3.get_nexthop(r1_rloc)) == (r5_rloc, 16))
    verify(parse_nexthop(r2.get_nexthop(r5_rloc)) == (r3_rloc, 3))
    verify(parse_nexthop(fed3.get_nexthop(r2_rloc)) == (r5_rloc, 4))


verify_within(check_nexthops_and_costs_after_r1_detach, 30)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
