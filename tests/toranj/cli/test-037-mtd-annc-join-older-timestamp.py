#!/usr/bin/env python3
#
#  Copyright (c) 2025, The OpenThread Authors.
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
# This test validates that a MTD can attach to a router/parent that
# has a previous Dataset with an older active timestamp (when it
# received Announce message from the parent/router).
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 25
cli.Node.set_time_speedup_factor(speedup)

parent = cli.Node()
child = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

parent.form('mtd-old-tmstmp')
child.join(parent, cli.JOIN_TYPE_END_DEVICE)

verify(parent.get_state() == 'leader')
verify(child.get_state() == 'child')

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Disable MLE on the child.

child.thread_stop()
verify(child.get_state() == 'disabled')

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Update the Active Dataset on the child with a new channel and a
# higher active timestamp.

child.cli('dataset init active')

channel = int(child.get_channel())
channel = 11 if channel != 11 else 12
child.cli('dataset channel', channel)

timestamp = int(child.cli('dataset activetimestamp')[0])
timestamp = timestamp + 100
child.cli('dataset activetimestamp', timestamp)

child.cli('dataset commit active')

time.sleep(0.1)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Restart MLE on the child again now with the new Dataset.

child.thread_start()

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Verify that the child can successfully attach to the parent, despite
# the parent being on an older Dataset.


def check_child_state():
    verify(child.get_state() == 'child')


verify_within(check_child_state, 10)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Verify that the parent also adopts the new Dataset from the child.


def check_parent_adopts_the_new_dataset():
    verify(child.get_state() == 'child')
    verify(parent.get_state() == 'leader')
    parent.cli('dataset init active')
    verify(int(parent.cli('dataset activetimestamp')[0]) == timestamp)
    verify(int(parent.get_channel()) == channel)


verify_within(check_parent_adopts_the_new_dataset, 10)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
