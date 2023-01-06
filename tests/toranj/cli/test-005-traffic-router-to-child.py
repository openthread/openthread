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

# -----------------------------------------------------------------------------------------------------------------------
# Test description:
#
# Traffic between router to end-device (link-local and mesh-local IPv6 addresses).

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 10
cli.Node.set_time_speedup_factor(speedup)

node1 = cli.Node()
node2 = cli.Node()
node3 = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Test implementation

node1.form('net')
node2.join(node1, cli.JOIN_TYPE_END_DEVICE)
node3.join(node1, cli.JOIN_TYPE_SLEEPY_END_DEVICE)

verify(node1.get_state() == 'leader')
verify(node2.get_state() == 'child')
verify(node3.get_state() == 'child')

ll1 = node1.get_linklocal_ip_addr()
ll2 = node2.get_linklocal_ip_addr()
ll3 = node2.get_linklocal_ip_addr()

ml1 = node1.get_mleid_ip_addr()
ml2 = node2.get_mleid_ip_addr()
ml3 = node2.get_mleid_ip_addr()

sizes = [0, 80, 500, 1000]
count = 2

for size in sizes:
    node1.ping(ll2, size=size, count=count)
    node2.ping(ll1, size=size, count=count)
    node1.ping(ml2, size=size, count=count)
    node2.ping(ml1, size=size, count=count)

poll_periods = [10, 100, 300]

for period in poll_periods:
    node3.set_pollperiod(period)
    verify(int(node3.get_pollperiod()) == period)

    for size in sizes:
        node1.ping(ll3, size=size, count=count)
        node3.ping(ll1, size=size, count=count)
        node1.ping(ml3, size=size, count=count)
        node3.ping(ml1, size=size, count=count)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
