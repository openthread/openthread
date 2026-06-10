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
# Test description: Validate mechanism to detect when Network Data gets full and related callback.
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Node` instances

speedup = 40
cli.Node.set_time_speedup_factor(speedup)

leader = cli.Node()
r1 = cli.Node()
r2 = cli.Node()
r3 = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

leader.form('netdata-full')
r1.join(leader)
r2.join(r1)
r3.join(r1)

verify(leader.get_state() == 'leader')
verify(r1.get_state() == 'router')
verify(r2.get_state() == 'router')
verify(r2.get_state() == 'router')

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

verify(leader.get_netdata_full() == 'no')
verify(r1.get_netdata_full() == 'no')
verify(r2.get_netdata_full() == 'no')
verify(r3.get_netdata_full() == 'no')

# Add 7 routes from `r1` to netdata and make sure they are all
# successfully registered with leader.

r1.add_route('fd00:1:0:1::/64', 's', 'med')
r1.add_route('fd00:1:0:2::/64', 's', 'med')
r1.add_route('fd00:1:0:3::/64', 's', 'med')
r1.add_route('fd00:1:0:4::/64', 's', 'med')
r1.add_route('fd00:1:0:5::/64', 's', 'med')
r1.add_route('fd00:1:0:6::/64', 's', 'med')
r1.add_route('fd00:1:0:7::/64', 's', 'med')
r1.register_netdata()


def check_netdata_after_r1():
    netdata = leader.get_netdata()
    verify(len(netdata['routes']) == 7)


verify_within(check_netdata_after_r1, 5)

verify(leader.get_netdata_full() == 'no')
verify(r1.get_netdata_full() == 'no')
verify(r2.get_netdata_full() == 'no')
verify(r3.get_netdata_full() == 'no')

prev_len = int(leader.get_netdata_length())

# Now add 7 new routes from `r2` to netdata and make sure they are all
# successfully registered with leader.

r2.add_route('fd00:2:0:1::/64', 's', 'med')
r2.add_route('fd00:2:0:2::/64', 's', 'med')
r2.add_route('fd00:2:0:3::/64', 's', 'med')
r2.add_route('fd00:2:0:4::/64', 's', 'med')
r2.add_route('fd00:2:0:5::/64', 's', 'med')
r2.add_route('fd00:2:0:6::/64', 's', 'med')
r2.add_route('fd00:2:0:7::/64', 's', 'med')
r2.register_netdata()


def check_netdata_after_r2():
    netdata = leader.get_netdata()
    verify(len(netdata['routes']) == 14)


verify_within(check_netdata_after_r2, 5)

verify(leader.get_netdata_full() == 'no')
verify(r1.get_netdata_full() == 'no')
verify(r2.get_netdata_full() == 'no')
verify(r3.get_netdata_full() == 'no')

verify(int(leader.get_netdata_length()) > prev_len)

# Try adding 3 routes from `r3`, this should cause netdata to go
# over its size limit. Make sure that both `r3` and `leader`
# detect that

r3.add_route('fd00:3:0:1::/64', 's', 'med')
r3.add_route('fd00:3:0:2::/64', 's', 'med')
r3.add_route('fd00:3:0:3::/64', 's', 'med')
r3.register_netdata()


def check_netdata_after_r3():
    verify(leader.get_netdata_full() == 'yes')
    verify(r1.get_netdata_full() == 'no')
    verify(r2.get_netdata_full() == 'no')
    verify(r3.get_netdata_full() == 'yes')


verify_within(check_netdata_after_r3, 5)

r3.remove_prefix('fd00:3:0:1::/64')
r3.remove_prefix('fd00:3:0:2::/64')
r3.remove_prefix('fd00:3:0:3::/64')
r3.register_netdata()

leader.reset_netdata_full()
r3.reset_netdata_full()


def check_netdata_after_remove_on_r3():
    verify(leader.get_netdata_full() == 'no')
    verify(r1.get_netdata_full() == 'no')
    verify(r2.get_netdata_full() == 'no')
    verify(r3.get_netdata_full() == 'no')


verify_within(check_netdata_after_remove_on_r3, 5)

r2.add_route('fd00:2:0:8::/64', 's', 'med')
r2.add_route('fd00:2:0:9::/64', 's', 'med')
r2.register_netdata()


def check_netdata_after_more_routes_on_r2():
    verify(leader.get_netdata_full() == 'yes')
    verify(r1.get_netdata_full() == 'no')
    verify(r2.get_netdata_full() == 'yes')
    verify(r3.get_netdata_full() == 'no')


verify_within(check_netdata_after_more_routes_on_r2, 5)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
