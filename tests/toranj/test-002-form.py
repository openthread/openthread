#!/usr/bin/env python
#
#  Copyright (c) 2018, The OpenThread Authors.
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

from wpan import verify
import wpan
import time

#-----------------------------------------------------------------------------------------------------------------------
# Test description: forming a Thread network

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print '-' * 120
print 'Starting \'{}\''.format(test_name)

#-----------------------------------------------------------------------------------------------------------------------
# Creating `wpan.Nodes` instances

node = wpan.Node()

#-----------------------------------------------------------------------------------------------------------------------
# Init all nodes

wpan.Node.init_all_nodes()

#-----------------------------------------------------------------------------------------------------------------------
# Test implementation

# default values after reset
DEFAULT_KEY = '[00112233445566778899AABBCCDDEEFF]'
DEFAULT_NAME = '"OpenThread"'
DEFAULT_PANID = '0xFFFF'
DEFAULT_XPANID = '0xDEAD00BEEF00CAFE'

verify(node.get(wpan.WPAN_STATE) == wpan.STATE_OFFLINE)
verify(node.get(wpan.WPAN_KEY) == DEFAULT_KEY)
verify(node.get(wpan.WPAN_NAME) == DEFAULT_NAME)
verify(node.get(wpan.WPAN_PANID) == DEFAULT_PANID)
verify(node.get(wpan.WPAN_XPANID) == DEFAULT_XPANID)

# Form a network

node.form('asha')
verify(node.get(wpan.WPAN_STATE) == wpan.STATE_ASSOCIATED)
verify(node.get(wpan.WPAN_NODE_TYPE) == wpan.NODE_TYPE_LEADER)
verify(node.get(wpan.WPAN_NAME) == '"asha"')
verify(node.get(wpan.WPAN_KEY) != DEFAULT_KEY)
verify(node.get(wpan.WPAN_PANID) != DEFAULT_PANID)
verify(node.get(wpan.WPAN_XPANID) != DEFAULT_XPANID)


node.leave()
verify(node.get(wpan.WPAN_STATE) == wpan.STATE_OFFLINE)

# Form a network on a specific channel.

node.form('ahura', channel=20)
verify(node.get(wpan.WPAN_STATE) == wpan.STATE_ASSOCIATED)
verify(node.get(wpan.WPAN_NAME) == '"ahura"')
verify(node.get(wpan.WPAN_CHANNEL) == '20')
verify(node.get(wpan.WPAN_KEY) != DEFAULT_KEY)
verify(node.get(wpan.WPAN_PANID) != DEFAULT_PANID)
verify(node.get(wpan.WPAN_XPANID) != DEFAULT_XPANID)

node.leave()
verify(node.get(wpan.WPAN_STATE) == wpan.STATE_OFFLINE)

# Form a network with a specific panid, xpanid and key.

node.set(wpan.WPAN_PANID, '0x1977')
node.set(wpan.WPAN_XPANID, '1020031510006016', binary_data=True)
node.set(wpan.WPAN_KEY, '0123456789abcdeffecdba9876543210', binary_data=True)

node.form('mazda', channel=12)
verify(node.get(wpan.WPAN_STATE) == wpan.STATE_ASSOCIATED)
verify(node.get(wpan.WPAN_NAME) == '"mazda"')
verify(node.get(wpan.WPAN_CHANNEL) == '12')
verify(node.get(wpan.WPAN_KEY) == '[0123456789ABCDEFFECDBA9876543210]')
verify(node.get(wpan.WPAN_PANID) == '0x1977')
verify(node.get(wpan.WPAN_XPANID) == '0x1020031510006016')


#-----------------------------------------------------------------------------------------------------------------------
# Test finished

print '\'{}\' passed.'.format(test_name)

