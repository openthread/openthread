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
# Orphaned node attach through MLE Announcement after channel change.

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Node` instances

speedup = 40
cli.Node.set_time_speedup_factor(speedup)

r1 = cli.Node()
r2 = cli.Node()
c1 = cli.Node()
c2 = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

r1.form('announce-tst', channel=11)

r2.join(r1, cli.JOIN_TYPE_ROUTER)
c1.join(r1, cli.JOIN_TYPE_SLEEPY_END_DEVICE)
c2.join(r1, cli.JOIN_TYPE_SLEEPY_END_DEVICE)
c1.set_pollperiod(500)
c2.set_pollperiod(500)

verify(r1.get_state() == 'leader')
verify(r2.get_state() == 'router')
verify(c1.get_state() == 'child')
verify(c2.get_state() == 'child')

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

# Stop c2 and r2

r2.thread_stop()
c2.thread_stop()

# Switch the rest of network to channel 26
r1.cli('channel manager change 26')


def check_channel_changed_to_26_on_r1_c1():
    for node in [r1, c1]:
        verify(int(node.get_channel()) == 26)


verify_within(check_channel_changed_to_26_on_r1_c1, 10)

# Now re-enable c2 and verify that it does attach to r1 and is on
# channel 26. c2 would go through the ML Announce recovery.

c2.thread_start()
verify(int(c2.get_channel()) == 11)

# wait for 20s for c2 to be attached/associated


def check_c2_is_attched():
    verify(c2.get_state() == 'child')


verify_within(check_c2_is_attched, 20)

# Check that c2 is now on channel 26.
verify(int(c2.get_channel()) == 26)

# Now re-enable r2, and verify that it switches to channel 26
# after processing announce from r1.

r2.thread_start()
verify(int(r2.get_channel()) == 11)


def check_r2_switches_to_channel_26():
    verify(r2.get_state() != 'detached')
    verify(int(r2.get_channel()) == 26)


verify_within(check_r2_switches_to_channel_26, 60)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
