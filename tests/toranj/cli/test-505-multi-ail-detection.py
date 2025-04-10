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
# Check multi AIL detection
#
#   -+-       _______
#    |       /       \
#   br1 --- br2 --- br3
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 60
cli.Node.set_time_speedup_factor(speedup)

one_minute = 60

br1 = cli.Node()
br2 = cli.Node()
br3 = cli.Node()

IF_INDEX_1 = 1
IF_INDEX_2 = 2

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

br1.form("multiail")
br2.join(br1)
br3.join(br2)

verify(br1.get_state() == 'leader')
verify(br2.get_state() == 'router')
verify(br3.get_state() == 'router')

# -----------------------------------------------------------------------------------------------------------------------
# Test implementation

# Start first border router `br1`

br1.srp_server_set_addr_mode('unicast')
br1.srp_server_auto_enable()

br1.br_init(IF_INDEX_1, 1)
br1.br_enable()

time.sleep(one_minute / speedup)
verify(br1.br_get_state() == 'running')
verify(br1.br_get_multiail() == 'not detected')

# Start `br2` and `br3` together on a different AIL

br2.br_init(IF_INDEX_2, 1)
br2.br_enable()

br3.br_init(IF_INDEX_2, 1)
br3.br_enable()

time.sleep(one_minute / speedup)
verify(br2.br_get_state() == 'running')
verify(br3.br_get_state() == 'running')

verify(br2.br_get_multiail() == 'not detected')
verify(br3.br_get_multiail() == 'not detected')

# wait for longer than 10 minutes and check that multi AIL is detected

time.sleep(10.01 * one_minute / speedup)

verify(br1.br_get_multiail() == 'detected')
verify(br2.br_get_multiail() == 'detected')
verify(br3.br_get_multiail() == 'detected')

# Disable `br1`

br1.br_disable()
verify(br1.br_get_state() == 'disabled')

time.sleep(9 * one_minute / speedup)

verify(br2.br_get_multiail() == 'not detected')
verify(br3.br_get_multiail() == 'not detected')

# Enable `br` again, wait for a short time (before detection) and disable it

br1.br_enable()

time.sleep(2 * one_minute / speedup)
verify(br1.br_get_state() == 'running')

br1.br_disable()
verify(br1.br_get_state() == 'disabled')

time.sleep(10 * one_minute / speedup)

verify(br2.br_get_multiail() == 'not detected')
verify(br3.br_get_multiail() == 'not detected')

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
