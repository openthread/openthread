#!/usr/bin/env python3
#
#  Copyright (c) 2026, The OpenThread Authors.
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
import ipaddress
import time

# -----------------------------------------------------------------------------------------------------------------------
# Test description:
#
# Validate FTD-side behaviour when `OPENTHREAD_CONFIG_IP6_INIT_EXT_ADDR_POOL_ENABLE` is set:
#
# `ifconfig init <ucast> <mcast>` on an FTD initialises both the netif address pools and the
# shared per-child address pool (ucast + mcast slots per child) via `otChildTableInit`. A SED
# child then registers UCAST_POOL_SIZE external unicast addresses, and all of them must appear
# in the parent FTD's child-IP table, proving the runtime-allocated per-child `LinkedList` pool
# stores entries correctly.
#
# Network topology
#
#    leader (FTD) --- sed (sleepy end device)
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Node` instances

speedup = 40
cli.Node.set_time_speedup_factor(speedup)

leader = cli.Node()
sed = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

UCAST_POOL_SIZE = 6
MCAST_POOL_SIZE = 6

leader.cli('ifconfig init', UCAST_POOL_SIZE, MCAST_POOL_SIZE)
leader.form('ip6-ext-pool-ftd')
verify(leader.get_state() == 'leader')

sed.cli('ifconfig init', UCAST_POOL_SIZE, MCAST_POOL_SIZE)
sed.set_child_timeout(300)
sed.set_pollperiod(500)
sed.join(leader, cli.JOIN_TYPE_SLEEPY_END_DEVICE)
verify(sed.get_state() == 'child')
verify(len(leader.get_child_table()) == 1)

# Add UCAST_POOL_SIZE external unicast addresses on the SED using a distinct /64 prefix.
UCAST_PREFIX = 'fd00:abcd::'
ucast_addrs = ['{}{}'.format(UCAST_PREFIX, i + 1) for i in range(UCAST_POOL_SIZE)]
for addr in ucast_addrs:
    sed.add_ip_addr(addr)

# Allow time for the SED to send a Child Update Request and the leader to process it.
time.sleep(2.0 / speedup)


def _norm(addr_str):
    return str(ipaddress.ip_address(addr_str.lower()))


def check_all_child_addrs_registered():
    child_ip_entries = leader.get_child_ip()
    # Output format: "rloc16: address" per line.
    registered_addrs = [_norm(entry.split(': ', 1)[1]) for entry in child_ip_entries if ': ' in entry]
    for addr in ucast_addrs:
        verify(_norm(addr) in registered_addrs)


verify_within(check_all_child_addrs_registered, 5)

# -----------------------------------------------------------------------------------------------------------------------
print('\'{}\' passed.'.format(test_name))
