#!/usr/bin/env python3
#
#  Copyright (c) 2021, The OpenThread Authors.
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
import cli

import sys

print(sys.version)

# -----------------------------------------------------------------------------------------------------------------------
# Test description: simple CLI get and set commands

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `Nodes` instances

node = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Test implementation

node.set_channel(21)
verify(node.get_channel() == '21')

ext_addr = '1122334455667788'
node.set_ext_addr(ext_addr)
verify(node.get_ext_addr() == ext_addr)

ext_panid = '1020031510006016'
node.set_ext_panid(ext_panid)
verify(node.get_ext_panid() == ext_panid)

key = '0123456789abcdeffecdba9876543210'
node.set_network_key(key)
verify(node.get_network_key() == key)

panid = '0xabba'
node.set_panid(panid)
verify(node.get_panid() == panid)

mode = 'rd'
node.set_mode(mode)
verify(node.get_mode() == mode)

threshold = '1'
node.set_router_upgrade_threshold(threshold)
verify(node.get_router_upgrade_threshold() == threshold)

jitter = '100'
node.set_router_selection_jitter(jitter)
verify(node.get_router_selection_jitter() == jitter)

verify(node.get_interface_state() == 'down')
verify(node.get_state() == 'disabled')

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
