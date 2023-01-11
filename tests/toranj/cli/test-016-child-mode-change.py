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
# Verify device mode change on children.
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Node` instances

speedup = 10
cli.Node.set_time_speedup_factor(speedup)

parent = cli.Node()
child1 = cli.Node()
child2 = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

parent.form('modechange')
child1.join(parent, cli.JOIN_TYPE_END_DEVICE)
child2.join(parent, cli.JOIN_TYPE_SLEEPY_END_DEVICE)

verify(parent.get_state() == 'leader')
verify(child1.get_state() == 'child')
verify(child2.get_state() == 'child')

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

child1_rloc = int(child1.get_rloc16(), 16)
child2_rloc = int(child2.get_rloc16(), 16)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Check the mode on children and also on parent child table

verify(parent.get_mode() == 'rdn')
verify(child1.get_mode() == 'rn')
verify(child2.get_mode() == '-')

child_table = parent.get_child_table()
verify(len(child_table) == 2)
for entry in child_table:
    if int(entry['RLOC16'], 16) == child1_rloc:
        verify(entry['R'] == '1')
        verify(entry['D'] == '0')
        verify(entry['N'] == '1')
    elif int(entry['RLOC16'], 16) == child2_rloc:
        verify(entry['R'] == '0')
        verify(entry['D'] == '0')
        verify(entry['N'] == '0')
    else:
        verify(False)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Change the network data flag on child 2 (sleepy) and verify that it
# gets changed on parent's child table.

child2.set_mode('n')


def check_child2_n_flag_change():
    verify(child2.get_mode() == 'n')
    child_table = parent.get_child_table()
    verify(len(child_table) == 2)
    for entry in child_table:
        if int(entry['RLOC16'], 16) == child1_rloc:
            verify(entry['R'] == '1')
            verify(entry['D'] == '0')
            verify(entry['N'] == '1')
        elif int(entry['RLOC16'], 16) == child2_rloc:
            verify(entry['R'] == '0')
            verify(entry['D'] == '0')
            verify(entry['N'] == '1')
        else:
            verify(False)


verify_within(check_child2_n_flag_change, 5)

# Verify that mode change did not cause child2 to detach and re-attach

verify(int(cli.Node.parse_list(child2.get_mle_counter())['Role Detached']) == 1)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Change child1 from rx-on to sleepy and verify the change on
# parent's child table. This mode change should require child
# to detach and attach again.

child1.set_mode('n')


def check_child1_become_sleepy():
    verify(child1.get_mode() == 'n')
    child_table = parent.get_child_table()
    verify(len(child_table) == 2)
    for entry in child_table:
        if int(entry['RLOC16'], 16) == child1_rloc:
            verify(entry['R'] == '0')
            verify(entry['D'] == '0')
            verify(entry['N'] == '1')
        elif int(entry['RLOC16'], 16) == child2_rloc:
            verify(entry['R'] == '0')
            verify(entry['D'] == '0')
            verify(entry['N'] == '1')
        else:
            verify(False)


verify_within(check_child1_become_sleepy, 5)

verify(int(cli.Node.parse_list(child1.get_mle_counter())['Role Detached']) == 2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Change child2 from sleepy to rx-on and verify the change on
# parent's child table. Verify that child2 did not detach and
# used MLE "Child Update" exchange.

child2.set_mode('rn')


def check_child2_become_rx_on():
    verify(child2.get_mode() == 'rn')
    child_table = parent.get_child_table()
    verify(len(child_table) == 2)
    for entry in child_table:
        if int(entry['RLOC16'], 16) == child1_rloc:
            verify(entry['R'] == '0')
            verify(entry['D'] == '0')
            verify(entry['N'] == '1')
        elif int(entry['RLOC16'], 16) == child2_rloc:
            verify(entry['R'] == '1')
            verify(entry['D'] == '0')
            verify(entry['N'] == '1')
        else:
            verify(False)


verify_within(check_child2_become_rx_on, 5)

verify(int(cli.Node.parse_list(child2.get_mle_counter())['Role Detached']) == 1)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Now change child2 to become sleepy again. Since it was originally
# attached as sleepy it should not detach and attach again and
# can do this using MlE "Child Update" exchange with parent.

child2.set_mode('n')


def check_child2_become_sleepy_again():
    verify(child2.get_mode() == 'n')
    child_table = parent.get_child_table()
    verify(len(child_table) == 2)
    for entry in child_table:
        if int(entry['RLOC16'], 16) == child1_rloc:
            verify(entry['R'] == '0')
            verify(entry['D'] == '0')
            verify(entry['N'] == '1')
        elif int(entry['RLOC16'], 16) == child2_rloc:
            verify(entry['R'] == '0')
            verify(entry['D'] == '0')
            verify(entry['N'] == '1')
        else:
            verify(False)


verify_within(check_child2_become_sleepy_again, 5)

verify(int(cli.Node.parse_list(child2.get_mle_counter())['Role Detached']) == 1)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
