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
# Test description:
#
# Validate changes to `IntervalMax` for MLE Advertisement Trickle Timer based on number of
# router neighbors of the device.
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Node` instances

speedup = 20
cli.Node.set_time_speedup_factor(speedup)

leader = cli.Node()
routers = []
for num in range(0, 9):
    routers.append(cli.Node())

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

leader.form('mle-adv-imax')

verify(leader.get_state() == 'leader')

# The Imax is determined as `Clamp((n + 1) * 4, 12, 32)` with `n` as
# number of router neighbors with link quality 2 or higher

verify(int(leader.get_mle_adv_imax()) == 12000)

expected_neighbor_count = 0


def check_leader_has_expected_number_of_neighbors():
    verify(len(leader.get_neighbor_table()) == expected_neighbor_count)


# Add two routers one by one and check that Imax
# remains at 12 seconds.

for num in range(0, 2):
    r = routers[num]

    r.join(leader)
    verify(r.get_state() == 'router')

    expected_neighbor_count += 1
    verify_within(check_leader_has_expected_number_of_neighbors, 10)

    verify(int(leader.get_mle_adv_imax()) == 12000)

# Adding the third router, we should see Imax increasing
# to 16 seconds.

r = routers[2]
r.join(leader)
verify(r.get_state() == 'router')

expected_neighbor_count += 1
verify_within(check_leader_has_expected_number_of_neighbors, 10)

verify(int(leader.get_mle_adv_imax()) == 16000)

# Adding a neighbor with poor link quality which should not
# count.

r_poor_lqi = routers[3]
leader.set_macfilter_lqi_to_node(r_poor_lqi, 1)

r_poor_lqi.join(leader)
verify(r_poor_lqi.get_state() == 'router')

expected_neighbor_count += 1
verify_within(check_leader_has_expected_number_of_neighbors, 10)
verify(int(leader.get_mle_adv_imax()) == 16000)

expected_imax = 16000

# Add four new routers one by one and check that Imax is
# increased by 4 second for each new router neighbor up to
# 32 seconds.

for num in range(4, 8):
    r = routers[num]

    r.join(leader)
    verify(r.get_state() == 'router')

    expected_neighbor_count += 1
    verify_within(check_leader_has_expected_number_of_neighbors, 10)
    expected_imax += 4000
    verify(int(leader.get_mle_adv_imax()) == expected_imax)

# Check that Imax does not increase beyond 32 seconds.

r = routers[8]

r.join(leader)
verify(r.get_state() == 'router')

expected_neighbor_count += 1
verify_within(check_leader_has_expected_number_of_neighbors, 10)

verify(int(leader.get_mle_adv_imax()) == 32000)

# Check that all routers see each other as neighbor and they are all also
# using 32 seconds as Imax.


def check_all_routers_have_expected_number_of_neighbors():
    for r in routers:
        verify(len(r.get_neighbor_table()) == expected_neighbor_count)


verify_within(check_all_routers_have_expected_number_of_neighbors, 10)

for r in routers:
    verify(int(r.get_mle_adv_imax()) == 32000)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
