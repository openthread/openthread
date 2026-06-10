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
# Test description: This test covers behavior of MeshHeader messages (message sent over multi-hop) when the
# underlying devices (in the path) support different radio types with different frame MTU length.
#
#   r1  --------- r2 ---------- r3
# (trel)      (15.4+trel)     (15.4)
#                  |
#                  |
#               c2 (15.4)
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Node` instances

speedup = 10
cli.Node.set_time_speedup_factor(speedup)

r1 = cli.Node(cli.RADIO_TREL)
r2 = cli.Node(cli.RADIO_15_4_TREL)
r3 = cli.Node(cli.RADIO_15_4)
c2 = cli.Node(cli.RADIO_15_4)

# -----------------------------------------------------------------------------------------------------------------------
# Build network topology

r1.allowlist_node(r2)

r2.allowlist_node(r1)
r2.allowlist_node(r3)
r2.allowlist_node(c2)

r3.allowlist_node(r2)

c2.allowlist_node(r2)

r1.form("mesh-header")

r2.join(r1)
r3.join(r2)
c2.join(r2, cli.JOIN_TYPE_END_DEVICE)

verify(r1.get_state() == 'leader')
verify(r2.get_state() == 'router')
verify(r3.get_state() == 'router')
verify(c2.get_state() == 'child')

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

verify(r1.multiradio_get_radios() == '[TREL]')
verify(r2.multiradio_get_radios() == '[15.4, TREL]')
verify(r3.multiradio_get_radios() == '[15.4]')

r1_rloc = int(r1.get_rloc16(), 16)

r1_ml_addr = r1.get_mleid_ip_addr()
r3_ml_addr = r3.get_mleid_ip_addr()

# Wait till routes are discovered.


def check_r1_router_table():
    table = r1.get_router_table()
    verify(len(table) == 3)
    for entry in table:
        verify(int(entry['RLOC16'], 0) == r1_rloc or int(entry['Link']) == 1 or int(entry['Next Hop']) != 63)


verify_within(check_r1_router_table, 120)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Ping r3 from r2 using large message size. Such a message would be sent as a
# Mesh Header message. Note that the origin r1 is a trel-only device
# whereas the final destination r3 is a 15.4-only device. The originator
# should use smaller MTU size (which then fragment the message into
# multiple frames) otherwise the 15.4 link won't be able to handle it.

r1.ping(r3_ml_addr, size=1000, count=2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Ping r1 from c2 using large message size.

c2.ping(r1_ml_addr, size=1000, count=2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Ping r1 from r3

r3.ping(r1_ml_addr, size=1000, count=2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Finally ping r1 and r3 from r2.

r2.ping(r1_ml_addr, size=1000, count=2)
r2.ping(r3_ml_addr, size=1000, count=2)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
