#!/usr/bin/env python3
#
#  Copyright (c) 2022, The OpenThread Authors.
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
# Verifies `ChannelManager` channel change process.
#
# Network Topology:
#
#    r1  --- r2 --- r3
#    /\      |      |
#   /  \     |      |
#  sc1 ec1  sc2     sc3
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Node` instances

speedup = 20
cli.Node.set_time_speedup_factor(speedup)

r1 = cli.Node()
r2 = cli.Node()
r3 = cli.Node()
sc1 = cli.Node()
ec1 = cli.Node()
sc2 = cli.Node()
sc3 = cli.Node()

nodes = [r1, r2, r3, sc1, ec1, sc2, sc3]

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

r1.allowlist_node(r2)
r1.allowlist_node(sc1)
r1.allowlist_node(ec1)

r2.allowlist_node(r1)
r2.allowlist_node(r3)
r2.allowlist_node(sc2)

r3.allowlist_node(r2)
r3.allowlist_node(sc3)

sc1.allowlist_node(r1)
ec1.allowlist_node(r1)
sc2.allowlist_node(r2)
sc3.allowlist_node(r3)

r1.form('chan-man', channel=11)
r2.join(r1)
r3.join(r1)
sc1.join(r1, cli.JOIN_TYPE_SLEEPY_END_DEVICE)
ec1.join(r1, cli.JOIN_TYPE_END_DEVICE)
sc2.join(r1, cli.JOIN_TYPE_SLEEPY_END_DEVICE)
sc3.join(r1, cli.JOIN_TYPE_SLEEPY_END_DEVICE)

sc1.set_pollperiod(500)
sc2.set_pollperiod(500)
sc3.set_pollperiod(500)

verify(r1.get_state() == 'leader')
verify(r2.get_state() == 'router')
verify(r3.get_state() == 'router')
verify(ec1.get_state() == 'child')
verify(sc1.get_state() == 'child')
verify(sc2.get_state() == 'child')
verify(sc3.get_state() == 'child')

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

result = cli.Node.parse_list(r1.cli('channel manager'))
verify(result['channel'] == '0')
verify(result['auto'] == '0')

r1.cli('channel manager delay 4')
verify(int(r1.cli('channel manager delay')[0]) == 4)

channel = 11


def check_channel_on_all_nodes():
    verify(all(int(node.get_channel()) == channel for node in nodes))


verify_within(check_channel_on_all_nodes, 10)

# Request a channel change to channel 13 from router r1. Verify
# that all nodes switch to the new channel.

channel = 13
r1.cli('channel manager change', channel)
verify_within(check_channel_on_all_nodes, 10)

# Request same channel change on multiple routers at the same time.

channel = 14
r1.cli('channel manager change', channel)
r2.cli('channel manager change', channel)
r3.cli('channel manager change', channel)
verify_within(check_channel_on_all_nodes, 10)

# Request different channel changes from same router back-to-back.

channel = 15
r1.cli('channel manager change', channel)
time.sleep(1 / speedup)
channel = 16
r1.cli('channel manager change', channel)
verify_within(check_channel_on_all_nodes, 10)

# Request different channels from two routers (r1 and r2).
# We increase delay on r1 to make sure r1 is in middle of
# channel when new change is requested from r2.

r1.cli('channel manager delay 20')
r1.cli('channel manager change 17')
time.sleep(5 / speedup)
verify_within(check_channel_on_all_nodes, 10)
channel = 18
r2.cli('channel manager change', channel)
verify_within(check_channel_on_all_nodes, 10)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
