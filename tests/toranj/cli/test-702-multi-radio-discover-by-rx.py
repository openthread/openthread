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
# Test description: This test covers behavior of device after TREL network is temporarily disabled
# and rediscovery of TREL by receiving message over that radio from the neighbor.
#
#   r1  ---------- r2
# (15.4+trel)   (15.4+trel)
#
#  On r2 we disable trel temporarily.
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

# -----------------------------------------------------------------------------------------------------------------------
# Build network topology

r1.form("discover-by-rx")
r2.join(r1)

verify(r1.get_state() == 'leader')
verify(r2.get_state() == 'router')

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

high_preference_threshold = 220
min_preference_threshold = 0

verify(r1.multiradio_get_radios() == '[15.4, TREL]')
verify(r2.multiradio_get_radios() == '[15.4, TREL]')

r1_rloc = int(r1.get_rloc16(), 16)
r2_rloc = int(r2.get_rloc16(), 16)

r1_ml_addr = r1.get_mleid_ip_addr()
r2_ml_addr = r2.get_mleid_ip_addr()

# Wait till routes are discovered.


def check_r1_router_table():
    table = r1.get_router_table()
    verify(len(table) == 2)
    for entry in table:
        verify(int(entry['RLOC16'], 0) == r1_rloc or int(entry['Link']) == 1)


verify_within(check_r1_router_table, 120)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Verify that r1 detected both TREL & 15.4 as supported radios by r2


def check_r1_sees_r2_has_two_radio_links():
    neighbor_radios = r1.multiradio_get_neighbor_list()
    verify(len(neighbor_radios) == 1)
    info = cli.Node.parse_multiradio_neighbor_entry(neighbor_radios[0])
    verify(int(info['RLOC16'], 16) == r2_rloc)
    radios = info['Radios']
    verify(len(radios) == 2)
    verify('15.4' in radios)
    verify('TREL' in radios)


cli.verify_within(check_r1_sees_r2_has_two_radio_links, 10)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Ping r2 from r1 and verify that r1 prefers trel radio link for
# sending to r2.

r1.ping(r2_ml_addr, count=5)

neighbor_radios = r1.multiradio_get_neighbor_list()
verify(len(neighbor_radios) == 1)
info = cli.Node.parse_multiradio_neighbor_entry(neighbor_radios[0])
radios = info['Radios']
verify(radios['TREL'] >= high_preference_threshold)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Now temporary filter trel link on r2 and ping again. We expect that
# r1 to quickly detect that trel is no longer supported by r2 and
# prefer 15.4 for tx to r2.

r2.cli('trel filter enable')
verify(r2.cli('trel filter')[0] == 'Enabled')

r1.udp_open()
for count in range(5):
    r1.udp_send(r2.get_mleid_ip_addr(), 12345, 'hi_r2_from_r1')


def check_r1_does_not_prefer_trel_for_r2():
    neighbor_radios = r1.multiradio_get_neighbor_list()
    verify(len(neighbor_radios) == 1)
    info = cli.Node.parse_multiradio_neighbor_entry(neighbor_radios[0])
    radios = info['Radios']
    verify(radios['TREL'] <= min_preference_threshold)


verify_within(check_r1_does_not_prefer_trel_for_r2, 10)

# Check that we can send between r1 and r2 (now all tx should use 15.4)

r1.ping(r2.get_mleid_ip_addr(), count=5, verify_success=False)
r1.ping(r2.get_mleid_ip_addr(), count=5)

neighbor_radios = r1.multiradio_get_neighbor_list()
verify(len(neighbor_radios) == 1)
info = cli.Node.parse_multiradio_neighbor_entry(neighbor_radios[0])
radios = info['Radios']
verify(radios['TREL'] <= min_preference_threshold)
verify(radios['15.4'] >= high_preference_threshold)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Enable trel back on r2, start sending traffic from r2 to r1.
# r2 would use probe mechanism to discover that trel is enabled again
# r1 should notice new rx on trel and update trel preference for r1

r2.cli('trel filter disable')
verify(r2.cli('trel filter')[0] == 'Disabled')

for count in range(80):
    r1.udp_send(r2.get_mleid_ip_addr(), 12345, 'hi_r2_from_r1_probe')


def check_r1_again_prefers_trel_for_r2():
    neighbor_radios = r1.multiradio_get_neighbor_list()
    verify(len(neighbor_radios) == 1)
    info = cli.Node.parse_multiradio_neighbor_entry(neighbor_radios[0])
    radios = info['Radios']
    verify(radios['TREL'] >= high_preference_threshold)


verify_within(check_r1_again_prefers_trel_for_r2, 10)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
