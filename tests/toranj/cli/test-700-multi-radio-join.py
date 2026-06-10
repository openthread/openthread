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
# Test description: This test covers joining of nodes with different radio links
#
# Parent node with trel and 15.4 with 3 children.
#
#        parent
#    (trel + 15.4)
#      /    |    \
#     /     |     \
#    /      |      \
#   c1     c2       c3
# (15.4)  (trel)  (trel+15.4)

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Node` instances

speedup = 10
cli.Node.set_time_speedup_factor(speedup)

parent = cli.Node(cli.RADIO_15_4_TREL)
c1 = cli.Node(cli.RADIO_15_4)
c2 = cli.Node(cli.RADIO_TREL)
c3 = cli.Node(cli.RADIO_15_4_TREL)

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

# Verify that each node supports the correct radio links.

verify(parent.multiradio_get_radios() == '[15.4, TREL]')
verify(c1.multiradio_get_radios() == '[15.4]')
verify(c2.multiradio_get_radios() == '[TREL]')
verify(c3.multiradio_get_radios() == '[15.4, TREL]')

parent.form("multi-radio")

c1.join(parent, cli.JOIN_TYPE_END_DEVICE)
c2.join(parent, cli.JOIN_TYPE_END_DEVICE)
c3.join(parent, cli.JOIN_TYPE_END_DEVICE)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Verify that parent correctly learns about all the children and their
# supported radio links.

c1_ext_addr = c1.get_ext_addr()
c2_ext_addr = c2.get_ext_addr()
c3_ext_addr = c3.get_ext_addr()

neighbor_radios = parent.multiradio_get_neighbor_list()
verify(len(neighbor_radios) == 3)
for entry in neighbor_radios:
    info = cli.Node.parse_multiradio_neighbor_entry(entry)
    radios = info['Radios']
    if info['ExtAddr'] == c1_ext_addr:
        verify(int(info['RLOC16'], 16) == int(c1.get_rloc16(), 16))
        verify(len(radios) == 1)
        verify('15.4' in radios)
    elif info['ExtAddr'] == c2_ext_addr:
        verify(int(info['RLOC16'], 16) == int(c2.get_rloc16(), 16))
        verify(len(radios) == 1)
        verify('TREL' in radios)
    elif info['ExtAddr'] == c3_ext_addr:
        verify(int(info['RLOC16'], 16) == int(c3.get_rloc16(), 16))
        verify(len(radios) == 2)
        verify('15.4' in radios)
        verify('TREL' in radios)
    else:
        verify(False)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Reset the parent and check that all children are attached back successfully

del parent
parent = cli.Node(cli.RADIO_15_4_TREL, index=1)
parent.interface_up()
parent.thread_start()


def check_all_children_are_attached():
    verify(len(parent.get_child_table()) == 3)


verify_within(check_all_children_are_attached, 10)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
