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
# Test description: BR published route preference.
#
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Node` instances

speedup = 40
cli.Node.set_time_speedup_factor(speedup)

leader = cli.Node()
br = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

leader.set_macfilter_lqi_to_node(br, 2)
br.set_macfilter_lqi_to_node(leader, 2)

leader.form('br-route-prf')
br.join(leader)

verify(leader.get_state() == 'leader')
verify(br.get_state() == 'router')

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

verify(br.br_get_state() == 'uninitialized')

br.br_init(1, 1)

verify(br.br_get_state() == 'disabled')

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Check the default route preference while BR is disabled

verify(br.br_get_routeprf() == 'med')

br.br_set_routeprf('low')
verify(br.br_get_routeprf() == 'low')

br.br_set_routeprf('med')
verify(br.br_get_routeprf() == 'med')

br.br_set_routeprf('high')
verify(br.br_get_routeprf() == 'high')

br.br_clear_routeprf()
verify(br.br_get_routeprf() == 'med')

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Enable BR and check the published route and its preference

br.br_enable()


def check_published_route_1():
    verify(br.br_get_state() == 'running')
    routes = br.get_netdata_routes()
    verify(len(routes) == 1)
    verify(routes[0].startswith('fc00::/7 sa med'))
    verify(br.br_get_routeprf() == 'med')


verify_within(check_published_route_1, 5)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Force `br` to become an end-device. Check that the published
# route is now using `low` (since link quality to parent is
# configured as 2)

br.set_router_eligible('disable')

parent_info = br.get_parent_info()
verify(parent_info['Link Quality In'] == '2')


def check_published_route_2():
    verify(br.get_state() == 'child')
    routes = br.get_netdata_routes()
    verify(len(routes) == 1)
    verify(routes[0].startswith('fc00::/7 sa low'))
    verify(br.br_get_routeprf() == 'low')


verify_within(check_published_route_2, 5)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Manually set the route prf to 'high` and validated that network
# data gets updated. Repeat setting route prf to `med`.

br.br_set_routeprf('high')


def check_published_route_3():
    verify(br.get_state() == 'child')
    routes = br.get_netdata_routes()
    verify(len(routes) == 1)
    verify(routes[0].startswith('fc00::/7 sa high'))
    verify(br.br_get_routeprf() == 'high')


verify_within(check_published_route_3, 5)

br.br_set_routeprf('med')


def check_published_route_4():
    verify(br.get_state() == 'child')
    routes = br.get_netdata_routes()
    verify(len(routes) == 1)
    verify(routes[0].startswith('fc00::/7 sa med'))
    verify(br.br_get_routeprf() == 'med')


verify_within(check_published_route_4, 5)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Clear the manually set route prf and validate that we go back
# to `low`.

br.br_clear_routeprf()
verify(br.br_get_routeprf() == 'low')


def check_published_route_5():
    verify(br.get_state() == 'child')
    routes = br.get_netdata_routes()
    verify(len(routes) == 1)
    verify(routes[0].startswith('fc00::/7 sa low'))
    verify(br.br_get_routeprf() == 'low')


verify_within(check_published_route_5, 5)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Allow `br` to take `router` role, validate that the route
# prf of `med` is used.

br.set_router_eligible('enable')


def check_published_route_6():
    verify(br.get_state() == 'router')
    routes = br.get_netdata_routes()
    verify(len(routes) == 1)
    verify(routes[0].startswith('fc00::/7 sa med'))
    verify(br.br_get_routeprf() == 'med')


verify_within(check_published_route_6, 15)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Update RSS MAC filter to use link quality 3 between `br` and
# `leader`. Again force `br` to become `child` and validate
# that now it still continues to use `med` prf.

leader.set_macfilter_lqi_to_node(br, 3)
br.set_macfilter_lqi_to_node(leader, 3)

br.set_router_eligible('disable')


def check_published_route_7():
    verify(br.get_state() == 'child')
    routes = br.get_netdata_routes()
    verify(len(routes) == 1)
    verify(routes[0].startswith('fc00::/7 sa med'))
    verify(br.br_get_routeprf() == 'med')


verify_within(check_published_route_7, 5)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Update RSS MAC filter to use link quality 1 between `br` and
# `leader`. Validate that route prf is updated quickly to `low`.

br.set_macfilter_lqi_to_node(leader, 1)
br.ping(leader.get_mleid_ip_addr())
parent_info = br.get_parent_info()
verify(parent_info['Link Quality In'] == '1')


def check_published_route_8():
    verify(br.get_state() == 'child')
    routes = br.get_netdata_routes()
    verify(len(routes) == 1)
    verify(routes[0].startswith('fc00::/7 sa low'))
    verify(br.br_get_routeprf() == 'low')


verify_within(check_published_route_8, 15)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Update RSS MAC filter to go back to link quality 3.
# Make sure the published route prf is not immediately
# updated and only updated after ~5 minutes.

br.set_macfilter_lqi_to_node(leader, 3)
br.ping(leader.get_mleid_ip_addr())
parent_info = br.get_parent_info()
verify(parent_info['Link Quality In'] == '3')

verify(br.br_get_routeprf() == 'low')


def check_published_route_9():
    verify(br.get_state() == 'child')
    routes = br.get_netdata_routes()
    verify(len(routes) == 1)
    verify(routes[0].startswith('fc00::/7 sa med'))
    verify(br.br_get_routeprf() == 'med')


# Wait for 5 minutes

verify_within(check_published_route_9, (5 * 60 / speedup) + 5)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
