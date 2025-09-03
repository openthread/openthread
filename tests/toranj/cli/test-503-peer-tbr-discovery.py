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
# Check tracking of peer BR (connected to same Thread mesh).
#
#      ______________
#     /              \
#   br1 --- br2 --- br3
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 60
cli.Node.set_time_speedup_factor(speedup)

br1 = cli.Node()
br2 = cli.Node()
br3 = cli.Node()

IF_INDEX = 1

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

br1.form("peer-brs")
br2.join(br1)
br3.join(br2)

verify(br1.get_state() == 'leader')
verify(br2.get_state() == 'router')
verify(br3.get_state() == 'router')

# -----------------------------------------------------------------------------------------------------------------------
# Test implementation

BR_STATE_TLV = 38
BR_IF_ADDRS_TLV = 39
BR_LOCAL_OMR_PREFIX_TLV = 40
BR_DHCP6_PD_OMR_PREFIX_TLV = 41
BR_LOCAL_OL_PREFIX_TLV = 42
BR_FAVORED_OL_PREFIX_TLV = 43

# Start first border router `br1`

br1.srp_server_set_addr_mode('unicast')
br1.srp_server_auto_enable()

br1.br_init(IF_INDEX, 1)
br1.br_enable()

time.sleep(1)
verify(br1.br_get_state() == 'running')

br1_local_omr = br1.br_get_local_omrprefix()
br1_favored_omr = br1.br_get_favored_omrprefix().split()[0]
verify(br1_local_omr == br1_favored_omr)

br1_local_onlink = br1.br_get_local_onlinkprefix()
br1_favored_onlink = br1.br_get_favored_onlinkprefix().split()[0]
verify(br1_local_onlink == br1_favored_onlink)

# Start `br2` and `br3` together

br2.br_init(IF_INDEX, 1)
br2.br_enable()

br3.br_init(IF_INDEX, 1)
br3.br_enable()

time.sleep(1)
verify(br2.br_get_state() == 'running')
verify(br3.br_get_state() == 'running')

# Validate that all BRs discovered the other ones as peer BR

all_brs = [br1, br2, br3]

for br in all_brs:
    routers = br.br_get_routers()
    verify(len(routers) == 2)
    for router in routers:
        verify(router.endswith('(peer BR)'))
    verify(int(br.br_count_peers().split()[0]) == 2)
    peers = br.br_get_peer_brs()
    verify(len(peers) == 2)
    other_brs = [br1, br2, br3]
    other_brs.remove(br)
    for other_br in other_brs:
        rloc16 = other_br.get_rloc16()
        verify(any([rloc16 in peer for peer in peers]))
    ifaddrs = br.br_get_ifaddrs()
    verify(len(ifaddrs) == 1)
    verify(ifaddrs[0].startswith('fe80:'))

# Validate network diagnostic BR TLVs, query BR2 from BR1.

br2_rloc = br2.get_rloc_ip_addr()
result = br1.cli('networkdiagnostic get', br2_rloc, BR_STATE_TLV)
verify(len(result) == 2)
verify(result[1] == 'BR State: running')

result = br1.cli('networkdiagnostic get', br2_rloc, BR_IF_ADDRS_TLV)
verify(len(result) > 2)
verify(result[1] == 'BR Infra-if IP6 Address List:')
verify(result[2].startswith('    - fe80:'))

result = br1.cli('networkdiagnostic get', br2_rloc, BR_LOCAL_OMR_PREFIX_TLV)
verify(len(result) == 2)
verify(result[1].startswith('BR Local OMR Prefix: '))
verify(result[1].split(': ')[1].strip() == br2.br_get_local_omrprefix())

result = br1.cli('networkdiagnostic get', br2_rloc, BR_DHCP6_PD_OMR_PREFIX_TLV)
verify(len(result) == 2)
verify(result[1].startswith('BR DHCPv6-PD OMR Prefix:'))
verify(result[1].split(': ')[1].strip() == '0:0:0:0::/64')

result = br1.cli('networkdiagnostic get', br2_rloc, BR_LOCAL_OL_PREFIX_TLV)
verify(len(result) == 2)
verify(result[1].startswith('BR Local On-link Prefix: '))
verify(result[1].split(': ')[1].strip() == br2.br_get_local_onlinkprefix())

result = br1.cli('networkdiagnostic get', br2_rloc, BR_FAVORED_OL_PREFIX_TLV)
verify(len(result) == 2)
verify(result[1].startswith('BR Favored On-link Prefix: '))
verify(result[1].split(': ')[1].strip() == br2.br_get_favored_onlinkprefix())

# Disable BR3 and validate that BR1 and BR2 detect this.
# BR3 itself should continue to detect BR1 and BR2

br3.br_disable()

time.sleep(0.5)

for br in [br1, br2]:
    verify(len(br.br_get_peer_brs()) == 1)

verify(len(br3.br_get_peer_brs()) == 2)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
