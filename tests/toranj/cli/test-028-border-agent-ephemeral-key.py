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
# Validate Border Agent ephemeral key APIs.

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Node` instances

speedup = 20
cli.Node.set_time_speedup_factor(speedup)

leader = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

leader.form('ba-ephemeral')

verify(leader.get_state() == 'leader')

verify(leader.ba_is_ephemeral_key_active() == 'inactive')

port = int(leader.ba_get_port())

leader.ba_set_ephemeral_key('password', 10000, 1234)

time.sleep(0.1)

verify(leader.ba_is_ephemeral_key_active() == 'active')
verify(int(leader.ba_get_port()) == 1234)

leader.ba_set_ephemeral_key('password2', 200, 45678)

time.sleep(0.100 / speedup)
verify(leader.ba_is_ephemeral_key_active() == 'active')
verify(int(leader.ba_get_port()) == 45678)

time.sleep(0.150 / speedup)
verify(leader.ba_is_ephemeral_key_active() == 'inactive')
verify(int(leader.ba_get_port()) == port)

leader.ba_set_ephemeral_key('newkey')
verify(leader.ba_is_ephemeral_key_active() == 'active')

time.sleep(0.1)
verify(leader.ba_is_ephemeral_key_active() == 'active')

leader.ba_clear_ephemeral_key()
verify(leader.ba_is_ephemeral_key_active() == 'inactive')
verify(int(leader.ba_get_port()) == port)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
