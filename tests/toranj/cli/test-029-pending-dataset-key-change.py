#!/usr/bin/env python3
#
#  Copyright (c) 2024, The OpenThread Authors.
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
# This test validates the rollback of the Active Timestamp when
# applying a Pending Dataset. This rollback is permitted only when
# the Network Key changes.

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Node` instances

speedup = 20
cli.Node.set_time_speedup_factor(speedup)

leader = cli.Node()
router = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

leader.form('network')
router.join(leader)

verify(leader.get_state() == 'leader')
verify(router.get_state() == 'router')

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

# Use Pending Dataset to change network name and update
# Active Timestamp to 10.

router.cli('dataset init active')
router.cli('dataset activetimestamp 10')
router.cli('dataset pendingtimestamp 10')
router.cli('dataset networkname new')
router.cli('dataset delaytimer 1000')
router.cli('dataset commit pending')

time.sleep(1.2 / speedup)

# Validate that Active Dataset is updated.

leader.cli('dataset init active')
verify(leader.get_network_name() == 'new')
verify(leader.cli('dataset activetimestamp') == ['10'])

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Use Pending Dataset with an older Active Timestamp.
# Ensure that the new dataset is not accepted.

router.cli('dataset init active')
router.cli('dataset activetimestamp 5')
router.cli('dataset pendingtimestamp 15')
router.cli('dataset networkname shouldfail')
router.cli('dataset delaytimer 1000')
router.cli('dataset commit pending')

time.sleep(1.2 / speedup)

leader.cli('dataset init active')
verify(leader.get_network_name() == 'new')
verify(leader.cli('dataset activetimestamp') == ['10'])

#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Use Pending Dataset with an older Active Timestamp but
# also change the Network Key.

router.cli('dataset init active')
router.cli('dataset activetimestamp 7')
router.cli('dataset pendingtimestamp 20')
router.cli('dataset networkkey 00112233445566778899aabbccddeeff')
router.cli('dataset delaytimer 1000')
router.cli('dataset commit pending')

time.sleep(1.2 / speedup)

# Validate that the Active Dataset is updated and
# Active Timestamp is rolled back.

leader.cli('dataset init active')
leader.cli('dataset')
verify(leader.cli('dataset activetimestamp') == ['7'])
verify(leader.get_network_key() == '00112233445566778899aabbccddeeff')

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
