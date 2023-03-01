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
# Validate Network Data Publisher optimizing route prefixes and using compact route.
#
# Network topology
#
#     r1 ---- r2
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

r1.form('compact-prfx')
r2.join(r1)

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

r1_rloc = r1.get_rloc16()
r2_rloc = r2.get_rloc16()

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# On `r1` publish two prefixes that can be covered by `fc00::/7` and
# one that cannot. On `r2` publish four prefixes that can compacted.
# Check that we see entries from `r1` unchanged, and entries from `r2`
# are optimized and replaced with `fc00::7`.

r1.netdata_publish_route('2600:1:2:3::/64', 's')
r1.netdata_publish_route('fd01:1::/64', 's')
r1.netdata_publish_route('fd01:2::/64', 's')

r2.netdata_publish_route('fd02:1::/64', 's')
r2.netdata_publish_route('fd02:2::/48', 's')
r2.netdata_publish_route('fd02:3::/32', 's')
r2.netdata_publish_route('fd02:4::/16', 's')


def check_netdata_step1():
    routes = r2.get_netdata_routes()
    verify(len(routes) == 4)
    verify('2600:1:2:3::/64 s med ' + r1_rloc in routes)
    verify('fd01:1:0:0::/64 s med ' + r1_rloc in routes)
    verify('fd01:2:0:0::/64 s med ' + r1_rloc in routes)
    verify('fc00::/7 sc med ' + r2_rloc in routes)


verify_within(check_netdata_step1, 2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Publish one more prefix on `r1` which can be compacted. Check that
# routes from `r1` are now replaced with `fc00:7`.

r1.netdata_publish_route('fd01:3::/64', 's')


def check_netdata_step2():
    routes = r2.get_netdata_routes()
    verify(len(routes) == 3)
    verify('2600:1:2:3::/64 s med ' + r1_rloc in routes)
    verify('fc00::/7 sc med ' + r1_rloc in routes)
    verify('fc00::/7 sc med ' + r2_rloc in routes)


verify_within(check_netdata_step2, 2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Unpublish one of the four prefixes on `r2`. Since still above the
# threshold of three, we should continue to see the compact prefix.

r2.netdata_unpublish('fd02:1::/64')

time.sleep(0.3)
check_netdata_step2()

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Unpublish another prefix on `r2` causing number of prefixes to go
# below the threshold. Check that original prefixes are published and
# compact prefix is removed.

r2.netdata_unpublish('fd02:4::/16')


def check_netdata_step4():
    routes = r2.get_netdata_routes()
    verify(len(routes) == 4)
    verify('2600:1:2:3::/64 s med ' + r1_rloc in routes)
    verify('fc00::/7 sc med ' + r1_rloc in routes)
    verify('fd02:2:0::/48 s med ' + r2_rloc in routes)
    verify('fd02:3::/32 s med ' + r2_rloc in routes)


verify_within(check_netdata_step4, 2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Publish a new prefix on `r2` with `high` preference. This causes
# the number of routes to go above threshold. Check that it again
# uses the compact prefix `fc00::/7` but now with `high` preference.

r2.netdata_publish_route('fd02:5::/16', 's', 'high')


def check_netdata_step5():
    routes = r2.get_netdata_routes()
    verify(len(routes) == 3)
    verify('2600:1:2:3::/64 s med ' + r1_rloc in routes)
    verify('fc00::/7 sc med ' + r1_rloc in routes)
    verify('fc00::/7 sc high ' + r2_rloc in routes)


verify_within(check_netdata_step5, 2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Publish a new prefix on `r1` with `high` preference. `r1` was
# already using the compact prefix. Check that the new prefix
# causes the compact prefix preference to change to `high`.

r1.netdata_publish_route('fd01:4::/64', 's', 'high')


def check_netdata_step6():
    routes = r2.get_netdata_routes()
    verify(len(routes) == 3)
    verify('2600:1:2:3::/64 s med ' + r1_rloc in routes)
    verify('fc00::/7 sc high ' + r1_rloc in routes)
    verify('fc00::/7 sc high ' + r2_rloc in routes)


verify_within(check_netdata_step6, 2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Change the preference of last route on `r1` from `high` to `low`.
# Check that `r1` continues to use the compact prefix but switches
# back to `med` preference.

r1.netdata_publish_route('fd01:4::/64', 's', 'low')


def check_netdata_step7():
    routes = r2.get_netdata_routes()
    verify(len(routes) == 3)
    verify('2600:1:2:3::/64 s med ' + r1_rloc in routes)
    verify('fc00::/7 sc med ' + r1_rloc in routes)
    verify('fc00::/7 sc high ' + r2_rloc in routes)


verify_within(check_netdata_step7, 2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Unpublish `2600:1:2:3::/64` route on `r1`. Make sure it does not
# impact the compact prefix.

r1.netdata_unpublish('2600:1:2:3::/64')


def check_netdata_step8():
    routes = r2.get_netdata_routes()
    verify(len(routes) == 2)
    verify('fc00::/7 sc med ' + r1_rloc in routes)
    verify('fc00::/7 sc high ' + r2_rloc in routes)


verify_within(check_netdata_step8, 2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Unpublish two prefixes on `r1`. This causes number of routes to go
# below the threshold. Check that all original prefixes are added
# back.

r1.netdata_unpublish('fd01:1::/64')
r1.netdata_unpublish('fd01:2::/64')


def check_netdata_step9():
    routes = r2.get_netdata_routes()
    verify(len(routes) == 3)
    verify('fd01:3:0:0::/64 s med ' + r1_rloc in routes)
    verify('fd01:4:0:0::/64 s low ' + r1_rloc in routes)
    verify('fc00::/7 sc high ' + r2_rloc in routes)


verify_within(check_netdata_step9, 2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Publish a NAT64 prefix on `r1`. Make sure it is not counted
# towards optimizable routes.

r1.netdata_publish_route('fd01:5::/96', 'sn')


def check_netdata_step10():
    routes = r2.get_netdata_routes()
    verify(len(routes) == 4)
    verify('fd01:3:0:0::/64 s med ' + r1_rloc in routes)
    verify('fd01:4:0:0::/64 s low ' + r1_rloc in routes)
    verify('fd01:5:0:0:0:0::/96 sn med ' + r1_rloc in routes)
    verify('fc00::/7 sc high ' + r2_rloc in routes)


verify_within(check_netdata_step10, 2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Publish `fc00::/7` itself on `r1`.

r1.netdata_publish_route('fc00::/7', 's')


def check_netdata_step11():
    routes = r2.get_netdata_routes()
    verify(len(routes) == 3)
    verify('fc00::/7 sc med ' + r1_rloc in routes)
    verify('fd01:5:0:0:0:0::/96 sn med ' + r1_rloc in routes)
    verify('fc00::/7 sc high ' + r2_rloc in routes)


verify_within(check_netdata_step11, 2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Unpublish one of the other prefixes on `r1` to go below
# the threshold. Check that we see the original prefixes
# including the recently added `fc00::/7`.

r1.netdata_unpublish('fd01:3::/64')


def check_netdata_step12():
    routes = r2.get_netdata_routes()
    verify(len(routes) == 4)
    verify('fc00::/7 s med ' + r1_rloc in routes)
    verify('fd01:4:0:0::/64 s low ' + r1_rloc in routes)
    verify('fd01:5:0:0:0:0::/96 sn med ' + r1_rloc in routes)
    verify('fc00::/7 sc high ' + r2_rloc in routes)


verify_within(check_netdata_step12, 2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Unpublish another prefix on `r1` (we stay below
# the threshold). Check that we still see the
# `fc00::/7` (which is published explicitly).

r1.netdata_unpublish('fd01:4::/64')


def check_netdata_step13():
    routes = r2.get_netdata_routes()
    verify(len(routes) == 3)
    verify('fc00::/7 s med ' + r1_rloc in routes)
    verify('fd01:5:0:0:0:0::/96 sn med ' + r1_rloc in routes)
    verify('fc00::/7 sc high ' + r2_rloc in routes)


verify_within(check_netdata_step13, 2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Change the NAT64 prefix so that it is no longer a NAT64
# prefix. Validate that it is updated and we still keep
# the explicitly published `fc00::/7`.

r1.netdata_publish_route('fd01:5::/96', 's')


def check_netdata_step14():
    routes = r2.get_netdata_routes()
    verify(len(routes) == 3)
    verify('fc00::/7 s med ' + r1_rloc in routes)
    verify('fd01:5:0:0:0:0::/96 s med ' + r1_rloc in routes)
    verify('fc00::/7 sc high ' + r2_rloc in routes)


verify_within(check_netdata_step14, 2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# On `r1` publish `::/0`, make sure it does not impact
# other prefixes.

r1.netdata_publish_route('::/0', 's')


def check_netdata_step15():
    routes = r2.get_netdata_routes()
    verify(len(routes) == 4)
    verify('fc00::/7 s med ' + r1_rloc in routes)
    verify('fd01:5:0:0:0:0::/96 s med ' + r1_rloc in routes)
    verify('::/0 s med ' + r1_rloc in routes)
    verify('fc00::/7 sc high ' + r2_rloc in routes)


verify_within(check_netdata_step15, 2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# On `r1` publish `fc00::/6`, again make sure it does not impact
# other prefixes and that it does not trigger the compact prefix
# to be added.

r1.netdata_publish_route('fc00::/6', 's')


def check_netdata_step16():
    routes = r2.get_netdata_routes()
    verify(len(routes) == 5)
    verify('fc00::/7 s med ' + r1_rloc in routes)
    verify('fd01:5:0:0:0:0::/96 s med ' + r1_rloc in routes)
    verify('::/0 s med ' + r1_rloc in routes)
    verify('fc00::/6 s med ' + r1_rloc in routes)
    verify('fc00::/7 sc high ' + r2_rloc in routes)


verify_within(check_netdata_step16, 2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Unpublish prefixes from `r1`.

r1.netdata_unpublish('fd01:5::/96')
r1.netdata_unpublish('::/0')
r1.netdata_unpublish('fc00::/6')


def check_netdata_step17():
    routes = r2.get_netdata_routes()
    verify(len(routes) == 2)
    verify('fc00::/7 s med ' + r1_rloc in routes)
    verify('fc00::/7 sc high ' + r2_rloc in routes)


verify_within(check_netdata_step17, 2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Publish 3 new prefixes from `r2` all containing `2000::/7`
# as a sub-prefix. Make sure we see a compact prefix for
# `2000::/7`.

r2.netdata_publish_route('2001::/32', 's')
r2.netdata_publish_route('2102::/48', 's')
r2.netdata_publish_route('2003::/64', 's')


def check_netdata_step18():
    routes = r2.get_netdata_routes()
    verify(len(routes) == 3)
    verify('fc00::/7 s med ' + r1_rloc in routes)
    verify('fc00::/7 sc high ' + r2_rloc in routes)
    verify('2000::/7 sc med ' + r2_rloc in routes)


verify_within(check_netdata_step18, 2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Unpublish `fd02:5::/16` and `2003::/64` on `r2`. This should
# cause both compact prefixes `fc00::7` and `2000::/7` to be removed
# and the original prefixes to be added back.

r2.netdata_unpublish('fd02:5::/16')
r2.netdata_unpublish('2003::/64')


def check_netdata_step19():
    routes = r2.get_netdata_routes()
    verify(len(routes) == 5)
    verify('fc00::/7 s med ' + r1_rloc in routes)
    verify('fd02:2:0::/48 s med ' + r2_rloc in routes)
    verify('fd02:3::/32 s med ' + r2_rloc in routes)
    verify('2001:0::/32 s med ' + r2_rloc in routes)
    verify('2102:0:0::/48 s med ' + r2_rloc in routes)


verify_within(check_netdata_step19, 2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Publish `2204::/16` on `r2`. This prefix does not have
# a common 7-bit prefix with the other two `2001::/32` and
# `2102::/48`. So all three should be seen.

r2.netdata_publish_route('2204::/16', 's')


def check_netdata_step20():
    routes = r2.get_netdata_routes()
    verify(len(routes) == 6)
    verify('fc00::/7 s med ' + r1_rloc in routes)
    verify('fd02:2:0::/48 s med ' + r2_rloc in routes)
    verify('fd02:3::/32 s med ' + r2_rloc in routes)
    verify('2001:0::/32 s med ' + r2_rloc in routes)
    verify('2102:0:0::/48 s med ' + r2_rloc in routes)
    verify('2204::/16 s med ' + r2_rloc in routes)


verify_within(check_netdata_step20, 2)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
