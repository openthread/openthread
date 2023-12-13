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
# Validate changes to `IntervalMax` for MLE Advertisement Trickle Timer based on number of
# router neighbors of the device.
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Node` instances

speedup = 40
cli.Node.set_time_speedup_factor(speedup)

fed = cli.Node()
r1 = cli.Node()
r2 = cli.Node()
c2 = cli.Node()
r3 = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

r1.set_macfilter_lqi_to_node(fed, 1)
fed.set_macfilter_lqi_to_node(r1, 1)

r1.allowlist_node(fed)
r1.allowlist_node(r2)
r1.allowlist_node(r3)

r2.allowlist_node(fed)
r2.allowlist_node(r1)
r2.allowlist_node(c2)
r2.allowlist_node(r3)

c2.allowlist_node(r2)

# `r2` is allowed to have one child only (`c2`)
r2.set_child_max(1)
verify(int(r2.get_child_max()) == 1)

# Form the network

r1.form('fed-prnt-srch')
fed.join(r1, cli.JOIN_TYPE_REED)
r2.join(r1)
c2.join(r2, cli.JOIN_TYPE_END_DEVICE)

verify(r1.get_state() == 'leader')
verify(fed.get_state() == 'child')
verify(r2.get_state() == 'router')
verify(c2.get_state() == 'child')

# Check that `c2` is attached to `r2`.

info = c2.get_parent_info()
verify(int(info['Rloc'], 16) == int(r2.get_rloc16(), 16))

# Check that `fed` is attached to `r1` and has poor link quality.

info = fed.get_parent_info()
verify(int(info['Rloc'], 16) == int(r1.get_rloc16(), 16))
verify(int(info['Link Quality In']) == 1)
verify(int(info['Link Quality Out']) == 1)

verify(int(cli.Node.parse_list(fed.get_mle_counter())['Better Parent Attach Attempts']) == 0)

# The config sets `PARENT_SEARCH_CHECK_INTERVAL` to 120 seconds.

time.sleep(130 / speedup)

# Verify that parent search mechanism did trigger an attach attempt on `fed`.

verify(int(cli.Node.parse_list(fed.get_mle_counter())['Better Parent Attach Attempts']) == 1)

# Since `r2` can have one child only and already has `c2`, the attach
# attempt should fail and `fed` should still have `r1` as its parent.

info = fed.get_parent_info()
verify(int(info['Rloc'], 16) == int(r1.get_rloc16(), 16))

# Make sure no other attach attempts are triggered on `fed` as the
# only option is `r2` which should be in "reselect timeout", not
# allowing `r2` to be selected again.

time.sleep(150 / speedup)

verify(int(cli.Node.parse_list(fed.get_mle_counter())['Better Parent Attach Attempts']) == 1)

# Add `r3` as a new router to network

r3.join(r1)
verify(r3.get_state() == 'router')

# Wait for `PARENT_SEARCH_CHECK_INTERVAL`.

time.sleep(130 / speedup)

# Parent search triggered on `fed` should have now selected
# the new `r3` and `fed` should switch to it as its new parent.

counters = cli.Node.parse_list(fed.get_mle_counter())
verify(int(counters['Better Parent Attach Attempts']) == 2)
verify(int(counters['Parent Changes']) == 1)

info = fed.get_parent_info()
verify(int(info['Rloc'], 16) == int(r3.get_rloc16(), 16))
verify(int(info['Link Quality In']) == 3)
verify(int(info['Link Quality Out']) == 3)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
