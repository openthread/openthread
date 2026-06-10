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
# Test description: This test validates the mechanism to discover and update the TREL peer info when processing
# a received TREL packet from a peer (when peer IPv6 address or port number gets changes).
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Node` instances

speedup = 10
cli.Node.set_time_speedup_factor(speedup)

r1 = cli.Node(cli.RADIO_15_4_TREL)
r2 = cli.Node(cli.RADIO_15_4_TREL)
c1 = cli.Node(cli.RADIO_15_4_TREL)

# -----------------------------------------------------------------------------------------------------------------------
# Build network topology

r1.form("trel-peer-disc")
c1.join(r1, cli.JOIN_TYPE_REED)
r2.join(r1)

verify(r1.get_state() == 'leader')
verify(r2.get_state() == 'router')
verify(c1.get_state() == 'child')

verify(r1.multiradio_get_radios() == '[15.4, TREL]')
verify(r2.multiradio_get_radios() == '[15.4, TREL]')
verify(c1.multiradio_get_radios() == '[15.4, TREL]')

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation


def check_trel_peers(node, peer_nodes):
    # Validate that `node` has discovered `peer_nodes` as its TREL peers
    # and validate that the correct TREL socket address is discovered
    # and used for each.
    peers = node.trel_get_peers()
    verify(len(peers) == len(peer_nodes))
    peers_ext_addrs = [nd.get_ext_addr() for nd in peer_nodes]
    for peer in peers:
        for peer_node in peer_nodes:
            if peer['Ext MAC Address'] == peer_node.get_ext_addr():
                verify(peer_node.trel_test_get_sock_addr() == peer['IPv6 Socket Address'])
                break
        else:
            verify(False)  # Did not find peer in `peer_nodes`


# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Check that all nodes see each other as TREL peers with the correct
# TREL socket address.

check_trel_peers(r1, [r2, c1])
check_trel_peers(r2, [r1, c1])
check_trel_peers(c1, [r1, r2])

verify(int(r1.trel_test_get_notify_addr_counter()) == 0)
verify(int(r2.trel_test_get_notify_addr_counter()) == 0)
verify(int(c1.trel_test_get_notify_addr_counter()) == 0)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Use test CLI command to force `r2` to change its TREL socket
# address (IPv6 address only).

time.sleep(10 / speedup)

old_sock_addr = r2.trel_test_get_sock_addr()
r2.trel_test_change_sock_addr()
verify(r2.trel_test_get_sock_addr() != old_sock_addr)

# Wait for longer than the Link Advertisement interval for `r2` to
# send an advertisement. Other nodes that receive and process this
# adv should then update the socket address of `r2` in their TREL
# peer table.

time.sleep(35 / speedup)

check_trel_peers(r1, [r2, c1])
check_trel_peers(r2, [r1, c1])
check_trel_peers(c1, [r1, r2])

# Validate that the platform is notified of the socket address
# discrepancy on `r1` and `c1`.

verify(int(r1.trel_test_get_notify_addr_counter()) == 1)
verify(int(r2.trel_test_get_notify_addr_counter()) == 0)
verify(int(c1.trel_test_get_notify_addr_counter()) == 1)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Use test CLI command to force `r2` to change its TREL socket
# address port.

time.sleep(10 / speedup)

old_sock_addr = r2.trel_test_get_sock_addr()
r2.trel_test_change_sock_port()
verify(r2.trel_test_get_sock_addr() != old_sock_addr)

# Wait for longer than the Link Advertisement interval for `r2` to
# send an advertisement. Other nodes that receive and process this
# adv should then update the socket address of `r2` in their TREL
# peer table.

time.sleep(35 / speedup)

check_trel_peers(r1, [r2, c1])
check_trel_peers(r2, [r1, c1])
check_trel_peers(c1, [r1, r2])

# Validate that the platform is notified of the socket address
# discrepancy on `r1` and `c1`.

verify(int(r1.trel_test_get_notify_addr_counter()) == 2)
verify(int(r2.trel_test_get_notify_addr_counter()) == 0)
verify(int(c1.trel_test_get_notify_addr_counter()) == 2)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Use test CLI command to force `c1` to change its TREL socket
# address (IPv6 address only).

old_sock_addr = c1.trel_test_get_sock_addr()
c1.trel_test_change_sock_addr()
verify(c1.trel_test_get_sock_addr() != old_sock_addr)

# Send a ping from `c1` to `r1` to trigger communication
# between `r1` and `c1`. The receipt of a secure data frame
# from `c1` should update the TREL socket address on `r1`

c1.ping(r1.get_rloc_ip_addr())

check_trel_peers(r1, [r2, c1])
verify(int(r1.trel_test_get_notify_addr_counter()) == 3)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
