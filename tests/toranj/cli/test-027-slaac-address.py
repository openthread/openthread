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
# Test description: Validate SLAAC module, addition/deprecation/removal of SLAAC addresses.
#
# Network topology
#
#    r1 ---- r2
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Node` instances

speedup = 40
cli.Node.set_time_speedup_factor(speedup)

r1 = cli.Node()
r2 = cli.Node()
c2 = cli.Node()

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  - - - - - - - - - - - - - - - - - - - - - - -


def verify_slaac_address_for_prefix(prefix, preferred, nodes=[r1, r2], origin='slaac'):
    # Verify that both nodes have SLAAC address based on `prefix`.
    for node in nodes:
        ip_addrs = node.get_ip_addrs('-v')
        matched_addrs = [addr for addr in ip_addrs if addr.startswith(prefix)]
        verify(len(matched_addrs) == 1)
        addr = matched_addrs[0]
        verify('origin:' + origin in addr)
        verify('plen:64' in addr)
        verify('valid:1' in addr)
        if (preferred):
            verify('preferred:1' in addr)
        else:
            verify('preferred:0' in addr)


def verify_no_slaac_address_for_prefix(prefix):
    for node in [r1, r2]:
        ip_addrs = node.get_ip_addrs('-v')
        matched_addrs = [addr for addr in ip_addrs if addr.startswith(prefix)]
        verify(len(matched_addrs) == 0)


def check_num_netdata_prefixes(num_prefixes):
    for node in [r1, r2]:
        verify(len(node.get_netdata_prefixes()) == num_prefixes)


def check_num_netdata_routes(num_routes):
    for node in [r1, r2]:
        verify(len(node.get_netdata_routes()) == num_routes)


# -----------------------------------------------------------------------------------------------------------------------
# Form topology

r1.allowlist_node(r2)
r2.allowlist_node(r1)
r2.allowlist_node(c2)
c2.allowlist_node(r2)

r1.form('slaac')
r2.join(r1)
c2.join(r2, cli.JOIN_TYPE_END_DEVICE)

verify(r1.get_state() == 'leader')
verify(r2.get_state() == 'router')
verify(c2.get_state() == 'child')

# Ensure `c2` is attached to `r2` as its parent
verify(int(c2.get_parent_info()['Rloc'], 16) == int(r2.get_rloc16(), 16))

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Add first prefix `fd00:11::/64` and check that SLAAC
# addressed are added based on it on all nodes.

r1.add_prefix("fd00:11:0:0::/64", "paos")
r1.register_netdata()
verify_within(check_num_netdata_prefixes, 100, 1)

verify_slaac_address_for_prefix('fd00:11:0:0:', preferred=True)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Add `fd00:22::/64` and check its related SLAAC addresses.

r2.add_prefix("fd00:22:0:0::/64", "paos")
r2.register_netdata()

verify_within(check_num_netdata_prefixes, 100, 2)

verify_slaac_address_for_prefix('fd00:11:0:0:', preferred=True)
verify_slaac_address_for_prefix('fd00:22:0:0:', preferred=True)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Add prefix `fd00:11::/64` now from `r2`, and since this was
# previously added by `r1`, there should not change to SLAAC
# addresses.

r2.add_prefix("fd00:11:0:0::/64", "paos")
r2.register_netdata()

verify_within(check_num_netdata_prefixes, 100, 3)

verify_slaac_address_for_prefix('fd00:11:0:0:', preferred=True)
verify_slaac_address_for_prefix('fd00:22:0:0:', preferred=True)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Remove prefix `fd00:11::/64` from `r2`, and since this is
# also added by `r1`, there should not change to SLAAC addresses.

r2.remove_prefix("fd00:11:0:0::/64")
r2.register_netdata()

verify_within(check_num_netdata_prefixes, 100, 2)

verify_slaac_address_for_prefix('fd00:11:0:0:', preferred=True)
verify_slaac_address_for_prefix('fd00:22:0:0:', preferred=True)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Remove all prefixes and validate that all SLAAC addresses
# start deprecating.

r1.remove_prefix("fd00:11:0:0::/64")
r1.register_netdata()
r2.remove_prefix("fd00:22:0:0::/64")
r2.register_netdata()

verify_within(check_num_netdata_prefixes, 100, 0)

verify_slaac_address_for_prefix('fd00:11:0:0:', preferred=False)
verify_slaac_address_for_prefix('fd00:22:0:0:', preferred=False)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Validate that the deprecating address can still be used
# and proper route look up happens when it is used.

# Add an off-mesh route `::/0` (default route) on `r1`.

r1.add_route('::/0', 's', 'med')
r1.register_netdata()

verify_within(check_num_netdata_routes, 10, 1)

# Now we ping from `r2` an off-mesh address, `r2` should pick one of
# its deprecating addresses as the source address for this message.
# Since the SLAAC module tracks the associated Domain ID from the
# original Prefix TLV, the route lookup for this message should
# successfully match to `r1` off-mesh route. We validate that the
# message is indeed delivered to `r1` and passed to host.

r1_counter = r1.get_br_counter_unicast_outbound_packets()

r2.ping('fd00:ee::1', verify_success=False)

verify(r1_counter + 1 == r1.get_br_counter_unicast_outbound_packets())

# Repeat the same process but now ping from `c2` which is an
# MTD child. `c2` would send the message to its parent `r2` which
# should be able to successfully do route look up for the deprecating
# prefix. The message should be delivered to `r1` and passed to host.

r1_counter = r1.get_br_counter_unicast_outbound_packets()

c2.ping('fd00:ff::1', verify_success=False)

verify(r1_counter + 1 == r1.get_br_counter_unicast_outbound_packets())

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Before the address is expired, re-add ``fd00:11::/64` prefix
# and check that the address is added back as preferred.

r2.add_prefix("fd00:11:0:0::/64", "paors")
r2.register_netdata()

verify_within(check_num_netdata_prefixes, 100, 1)

verify_slaac_address_for_prefix('fd00:11:0:0:', preferred=True)
verify_slaac_address_for_prefix('fd00:22:0:0:', preferred=False)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Make sure prefix 2 is expired and removed


def check_prefix_2_addr_expire():
    verify_no_slaac_address_for_prefix('fd00:22:0:0:')


verify_within(check_prefix_2_addr_expire, 100)

verify_slaac_address_for_prefix('fd00:11:0:0:', preferred=True)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Manually add an address for prefix `fd00:33::/64` on `r1`
# and `r2` before adding  a related on-mesh prefix. Validate that
# the SLAAC module does not add addresses.

r1.add_ip_addr('fd00:33::1')
r2.add_ip_addr('fd00:33::2')

r1.add_prefix("fd00:33:0:0::/64", "paos")
r1.register_netdata()

verify_within(check_num_netdata_prefixes, 100, 2)

verify_slaac_address_for_prefix('fd00:11:0:0:', preferred=True)
verify_slaac_address_for_prefix('fd00:33:0:0:', preferred=True, origin='manual')

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Remove the manually added address on `r2` which should trigger
# SLAAC module to add its own SLAAC address based on the prefix.

r2.remove_ip_addr('fd00:33::2')
time.sleep(0.1)

verify_slaac_address_for_prefix('fd00:11:0:0:', preferred=True)
verify_slaac_address_for_prefix('fd00:33:0:0:', preferred=True, nodes=[r2], origin='slaac')
verify_slaac_address_for_prefix('fd00:33:0:0:', preferred=True, nodes=[r1], origin='manual')

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Remove the prefix and make sure the manually added address
# is not impacted, but the one added by SLAAC on `r2` should
# be deprecating.

r1.remove_prefix("fd00:33:0:0::/64")
r1.register_netdata()

verify_within(check_num_netdata_prefixes, 100, 1)

verify_slaac_address_for_prefix('fd00:11:0:0:', preferred=True)
verify_slaac_address_for_prefix('fd00:33:0:0:', preferred=False, nodes=[r2], origin='slaac')
verify_slaac_address_for_prefix('fd00:33:0:0:', preferred=True, nodes=[r1], origin='manual')

r1.remove_ip_addr('fd00:33::1')

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Toranj config sets the max number of SLAAC addresses to 4
# Check that the max limit is applied by adding 5 prefixes.

r1.add_prefix("fd00:22:0:0::/64", "paos")
r1.add_prefix("fd00:33:0:0::/64", "paos")
r1.add_prefix("fd00:44:0:0::/64", "paos")
r1.register_netdata()

verify_within(check_num_netdata_prefixes, 100, 4)

verify_slaac_address_for_prefix('fd00:11:0:0:', preferred=True)
verify_slaac_address_for_prefix('fd00:22:0:0:', preferred=True)
verify_slaac_address_for_prefix('fd00:33:0:0:', preferred=True)
verify_slaac_address_for_prefix('fd00:44:0:0:', preferred=True)

r1.add_prefix("fd00:55:0:0::/64", "paos")
r1.register_netdata()

verify_within(check_num_netdata_prefixes, 100, 5)

verify_slaac_address_for_prefix('fd00:11:0:0:', preferred=True)
verify_slaac_address_for_prefix('fd00:22:0:0:', preferred=True)
verify_slaac_address_for_prefix('fd00:33:0:0:', preferred=True)
verify_slaac_address_for_prefix('fd00:44:0:0:', preferred=True)
verify_no_slaac_address_for_prefix('fd00:55:0:0::/64')

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Remove one of the prefixes which should deprecate an
# existing SLAAC address which should then be evicted to
# add address for the new `fd00:55::/64` prefix.

r1.remove_prefix("fd00:44:0:0::/64")
r1.register_netdata()

verify_within(check_num_netdata_prefixes, 100, 4)

verify_slaac_address_for_prefix('fd00:11:0:0:', preferred=True)
verify_slaac_address_for_prefix('fd00:22:0:0:', preferred=True)
verify_slaac_address_for_prefix('fd00:33:0:0:', preferred=True)
verify_slaac_address_for_prefix('fd00:55:0:0:', preferred=True)
verify_no_slaac_address_for_prefix('fd00:44:0:0::/64')

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Validate oldest entry is evicted when multiple addresses are deprecating
# and new prefix is added.

r1.remove_prefix("fd00:33:0:0::/64")
r1.register_netdata()
time.sleep(0.05)
r1.remove_prefix("fd00:22:0:0::/64")
r1.register_netdata()
time.sleep(0.05)
r1.remove_prefix("fd00:55:0:0::/64")
r1.register_netdata()
time.sleep(0.05)

verify_within(check_num_netdata_prefixes, 100, 1)

verify_slaac_address_for_prefix('fd00:11:0:0:', preferred=True)
verify_slaac_address_for_prefix('fd00:22:0:0:', preferred=False)
verify_slaac_address_for_prefix('fd00:33:0:0:', preferred=False)
verify_slaac_address_for_prefix('fd00:55:0:0:', preferred=False)

# Now add a new prefix `fd00:66::/64`, the `fd00:33::` address
# which was removed first should be evicted to make room for
# the SLAAC address for the new prefix.

r1.add_prefix("fd00:66:0:0::/64", "paos")
r1.register_netdata()

verify_within(check_num_netdata_prefixes, 100, 2)

verify_slaac_address_for_prefix('fd00:11:0:0:', preferred=True)
verify_slaac_address_for_prefix('fd00:22:0:0:', preferred=False)
verify_no_slaac_address_for_prefix('fd00:33:0:0:')
verify_slaac_address_for_prefix('fd00:55:0:0:', preferred=False)
verify_slaac_address_for_prefix('fd00:66:0:0:', preferred=True)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Validate re-adding of a prefix before its address is deprecated.

r1.add_prefix("fd00:22:0:0::/64", "paos")
r1.register_netdata()

verify_within(check_num_netdata_prefixes, 100, 3)

verify_slaac_address_for_prefix('fd00:11:0:0:', preferred=True)
verify_slaac_address_for_prefix('fd00:22:0:0:', preferred=True)
verify_no_slaac_address_for_prefix('fd00:33:0:0:')
verify_slaac_address_for_prefix('fd00:55:0:0:', preferred=False)
verify_slaac_address_for_prefix('fd00:66:0:0:', preferred=True)


def check_prefix_5_addr_expire():
    verify_no_slaac_address_for_prefix('fd00:55:0:0:')


verify_within(check_prefix_5_addr_expire, 100)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
