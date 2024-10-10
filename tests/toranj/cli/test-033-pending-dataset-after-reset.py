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
# This test validates restoration of pending dataset after device reset
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 10
cli.Node.set_time_speedup_factor(speedup)

leader = cli.Node(index=1)
child = cli.Node(index=2)
router = cli.Node(index=3)

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

leader.form('ot-0')

child.join(leader, cli.JOIN_TYPE_REED)
router.join(leader, cli.JOIN_TYPE_ROUTER)

verify(leader.get_state() == 'leader')
verify(child.get_state() == 'child')
verify(router.get_state() == 'router')

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

time.sleep(2 / speedup)

# Repeat the same test steps twice, first iteration reboot
# the `child`, second iteration reboot `router`.

for iter in [1, 2]:
    print('-' * 40)

    # - - - - - - - - - - - - - - - - - - - - - - - - - - -
    # Prepare a pending dataset changing network name
    # with 10 seconds delay timer.

    new_name = 'ot-' + str(iter)

    leader.cli('dataset clear')
    leader.cli('dataset networkname', new_name)
    leader.cli('dataset delay 10000')
    leader.cli('dataset updater start')

    verify(leader.cli('dataset updater')[0] == 'Enabled')

    time.sleep(2.1 / speedup)

    # - - - - - - - - - - - - - - - - - - - - - - - - - - -
    # Check the pending dataset with new name and proper
    # delay.

    for node in [leader, child, router]:
        node.cli('dataset init pending')
        verify(node.cli('dataset networkname')[0] == new_name)
        verify(int(node.cli('dataset delay')[0]) > 7000)

    # - - - - - - - - - - - - - - - - - - - - - - - - - - -
    # Reboot specific node and verify that it starts with
    # pending dataset restored from non-volatile settings.

    if iter == 1:
        del child
        child = cli.Node(index=2)
        child.set_router_eligible('disable')
        node = child
        role = 'child'
    elif iter == 2:
        del router
        router = cli.Node(index=3)
        node = router
        role = 'router'
    else:
        verify(False)

    node.interface_up()
    node.thread_start()

    time.sleep(5.0 / speedup)

    # - - - - - - - - - - - - - - - - - - - - - - - - - - -
    # Check that rebooted node recovers the pending dataset
    # from other nodes and starts the delay timer properly.
    # We started with 10 seconds delay timer, and waited 2.1
    # and 5.0 so  remaining delay time should be around ~2.9
    # seconds.

    verify(node.get_state() == role)

    node.cli('dataset pending')
    node.cli('dataset init pending')
    verify(node.cli('dataset networkname')[0] == new_name)
    delay = int(node.cli('dataset delay')[0])
    verify(2000 < delay < 3500)

    # - - - - - - - - - - - - - - - - - - - - - - - - - - -
    # Check that new pending Dataset is applied after delay
    # timer expires and all nodes change to the new network
    # name.

    time.sleep(4.0 / speedup)

    for node in [leader, router, child]:
        verify(node.get_network_name() == new_name)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
