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
# Test description: History Tracker client/server behavior
#
# Network topology
#
#      r1 ---- r2
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

r1.form('hist-srv')
r2.join(r1)

verify(r1.get_state() == 'leader')
verify(r2.get_state() == 'router')

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

r2_rlco16 = int(r2.get_rloc16(), 16)

local_table = r2.cli('history netinfo')

verify(len(local_table) == 5)
local_table = local_table[2:]

# Remove the age field from the table
for i in range(3):
    start_index = local_table[i].find(' | ')
    local_table[i] = local_table[i][start_index:]

# Check that the result of query matches the local table

table = r1.cli('history query netinfo', r2_rlco16)

verify(len(table) == 5)
table = table[2:]
for i in range(3):
    verify(table[i].endswith(local_table[i]))

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Validate different CLI command variations

# Test query with the 'list' format
result = r1.cli('history query netinfo list', r2_rlco16)
verify(len(result) == 3)

# Test specifying max entries as 2
table = r1.cli('history query netinfo', r2_rlco16, 2)
verify(len(table) == 4)

# Test specifying max entries as 10
table = r1.cli('history query netinfo', r2_rlco16, 10)
verify(len(table) == 5)

# Test specifying max entry age as zero (no age limit)
table = r1.cli('history query netinfo', r2_rlco16, 0, 0)
verify(len(table) == 5)

# Test specifying max entry age as 1 millisecond
table = r1.cli('history query netinfo', r2_rlco16, 0, 1)
verify(len(table) == 2)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
