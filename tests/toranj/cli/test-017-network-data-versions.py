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
# Test description: Network Data update and version changes (stable only vs. full version).
#
# Network topology
#
#      r1 ---- r2 ---- r3
#              |
#              |
#              sed
#
#
# sed is sleepy-end node and also configured to request stable Network Data only
#
# Test covers the following steps:
# - Adding/removing prefixes (stable or temporary) on r1
# - Verifying that Network Data is updated on all nodes
# - Ensuring correct update to version and stable version
#
# The above steps are repeated over many different situations:
# - Where the same prefixes are also added by other nodes
# - Or the same prefixes are added as off-mesh routes by other nodes

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# -----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Node` instances

speedup = 10
cli.Node.set_time_speedup_factor(speedup)

r1 = cli.Node()
r2 = cli.Node()
r3 = cli.Node()
sed = cli.Node()

# -----------------------------------------------------------------------------------------------------------------------
# Form topology

r1.allowlist_node(r2)

r2.allowlist_node(r1)
r2.allowlist_node(r3)
r2.allowlist_node(sed)

r2.allowlist_node(r2)

sed.allowlist_node(r2)

r1.form('netdata')
r2.join(r1)
r3.join(r1)
sed.join(r1, cli.JOIN_TYPE_SLEEPY_END_DEVICE)

sed.set_mode('-')
sed.set_pollperiod(400)

# -----------------------------------------------------------------------------------------------------------------------
# Test Implementation

r1_rloc = r1.get_rloc16()
r2_rloc = r2.get_rloc16()
r3_rloc = r3.get_rloc16()

versions = r1.get_netdata_versions()

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


def verify_versions_incremented():
    global versions
    new_versions = r1.get_netdata_versions()
    verify(new_versions[0] == ((versions[0] + 1) % 256))
    verify(new_versions[1] == ((versions[1] + 1) % 256))
    versions = new_versions


def verify_stabe_version_incremented():
    global versions
    new_versions = r1.get_netdata_versions()
    verify(new_versions[0] == ((versions[0] + 1) % 256))
    verify(new_versions[1] == versions[1])
    versions = new_versions


# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

# Add prefix `fd00:1::/64` on r1 as stable and validate
# that entries are updated on all nodes and versions are changed.

r1.add_prefix('fd00:1::/64', 'os')
r1.register_netdata()


def check_r1_prefix_added_on_all_nodes():
    for node in [r1, r2, r3]:
        verify('fd00:1:0:0::/64 os med ' + r1_rloc in node.get_netdata_prefixes())
    verify('fd00:1:0:0::/64 os med fffe' in sed.get_netdata_prefixes())


verify_within(check_r1_prefix_added_on_all_nodes, 2)
verify_versions_incremented()

# Add prefix `fd00:2::/64` on r2 as temporary and ensure it is seen on
# all nodes and not seen on sed.

r2.add_prefix('fd00:2::/64', 'po', 'high')
r2.register_netdata()


def check_r2_prefix_added_on_all_nodes():
    for node in [r1, r2, r3]:
        verify('fd00:2:0:0::/64 po high ' + r2_rloc in node.get_netdata_prefixes())
    verify(not 'fd00:2:0:0::/64 po high fffe' in sed.get_netdata_prefixes())


verify_within(check_r2_prefix_added_on_all_nodes, 2)
verify_stabe_version_incremented()

# Remove prefix `fd00:1::/64` from r1.

r1.remove_prefix('fd00:1::/64')
r1.register_netdata()


def check_r1_prefix_removed_on_all_nodes():
    for node in [r1, r2, r3]:
        verify(not 'fd00:1:0:0::/64 os med ' + r1_rloc in node.get_netdata_prefixes())
    verify(not 'fd00:1:0:0::/64 os med fffe' in sed.get_netdata_prefixes())


verify_within(check_r1_prefix_removed_on_all_nodes, 2)
verify_versions_incremented()

# Remove prefix `fd00:2::/64` from r2.

r2.remove_prefix('fd00:2::/64')
r2.register_netdata()


def check_r2_prefix_removed_on_all_nodes():
    for node in [r1, r2, r3]:
        verify(not 'fd00:2:0:0::/64 po high ' + r2_rloc in node.get_netdata_prefixes())
    verify(not 'fd00:2:0:0::/64 po high fffe' in sed.get_netdata_prefixes())


verify_within(check_r2_prefix_removed_on_all_nodes, 2)
verify_stabe_version_incremented()

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Repeat the same checks but now r3 also adds ``fd00:1::/64` with different
# flags.

r3.add_prefix('fd00:1::/64', 'paos')
r3.register_netdata()


def check_r3_prefix_added_on_all_nodes():
    for node in [r1, r2, r3]:
        verify('fd00:1:0:0::/64 paos med ' + r3_rloc in node.get_netdata_prefixes())
    verify('fd00:1:0:0::/64 paos med fffe' in sed.get_netdata_prefixes())


verify_within(check_r3_prefix_added_on_all_nodes, 2)
verify_versions_incremented()

r1.add_prefix('fd00:1::/64', 'os')
r1.register_netdata()
verify_within(check_r1_prefix_added_on_all_nodes, 2)
verify_versions_incremented()

r2.add_prefix('fd00:2::/64', 'po', 'high')
r2.register_netdata()
verify_within(check_r2_prefix_added_on_all_nodes, 2)
verify_stabe_version_incremented()

r1.remove_prefix('fd00:1::/64')
r1.register_netdata()
verify_within(check_r1_prefix_removed_on_all_nodes, 2)
verify_versions_incremented()

r2.remove_prefix('fd00:2::/64')
r2.register_netdata()
verify_within(check_r2_prefix_removed_on_all_nodes, 2)
verify_stabe_version_incremented()

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Repeat the same checks with r3 also adding ``fd00:2::/64`

r3.add_prefix('fd00:2::/64', 'paos')
r3.register_netdata()


def check_new_r3_prefix_added_on_all_nodes():
    for node in [r1, r2, r3]:
        verify('fd00:2:0:0::/64 paos med ' + r3_rloc in node.get_netdata_prefixes())
    verify('fd00:2:0:0::/64 paos med fffe' in sed.get_netdata_prefixes())


verify_within(check_new_r3_prefix_added_on_all_nodes, 2)
verify_versions_incremented()

r1.add_prefix('fd00:1::/64', 'os')
r1.register_netdata()
verify_within(check_r1_prefix_added_on_all_nodes, 2)
verify_versions_incremented()

r2.add_prefix('fd00:2::/64', 'po', 'high')
r2.register_netdata()
verify_within(check_r2_prefix_added_on_all_nodes, 2)
verify_stabe_version_incremented()

r1.remove_prefix('fd00:1::/64')
r1.register_netdata()
verify_within(check_r1_prefix_removed_on_all_nodes, 2)
verify_versions_incremented()

r2.remove_prefix('fd00:2::/64')
r2.register_netdata()
verify_within(check_r2_prefix_removed_on_all_nodes, 2)
verify_stabe_version_incremented()

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Repeat the same checks with r3 adding the two prefixes as temporary.

r3.remove_prefix('fd00:1::/64')
r3.remove_prefix('fd00:2::/64')
r3.add_prefix('fd00:1::/64', 'pao')
r3.add_prefix('fd00:2::/64', 'pao')
r3.register_netdata()


def check_r3_prefixes_as_temp_added_on_all_nodes():
    for node in [r1, r2, r3]:
        prefixes = node.get_netdata_prefixes()
        verify('fd00:1:0:0::/64 pao med ' + r3_rloc in prefixes)
        verify('fd00:1:0:0::/64 pao med ' + r3_rloc in prefixes)
    verify(len(sed.get_netdata_prefixes()) == 0)


verify_within(check_r3_prefixes_as_temp_added_on_all_nodes, 2)
verify_versions_incremented()

r1.add_prefix('fd00:1::/64', 'os')
r1.register_netdata()
verify_within(check_r1_prefix_added_on_all_nodes, 2)
verify_versions_incremented()

r2.add_prefix('fd00:2::/64', 'po', 'high')
r2.register_netdata()
verify_within(check_r2_prefix_added_on_all_nodes, 2)
verify_stabe_version_incremented()

r1.remove_prefix('fd00:1::/64')
r1.register_netdata()
verify_within(check_r1_prefix_removed_on_all_nodes, 2)
verify_versions_incremented()

r2.remove_prefix('fd00:2::/64')
r2.register_netdata()
verify_within(check_r2_prefix_removed_on_all_nodes, 2)
verify_stabe_version_incremented()

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Finally repeat the same checks with r3 adding the two prefixes
# as off-mesh route prefix.

r3.remove_prefix('fd00:1::/64')
r3.remove_prefix('fd00:2::/64')
r3.add_route('fd00:1::/64', 's')
r3.add_route('fd00:2::/64', '-')
r3.register_netdata()


def check_r3_routes_added_on_all_nodes():
    for node in [r1, r2, r3]:
        routes = node.get_netdata_routes()
        verify('fd00:1:0:0::/64 s med ' + r3_rloc in routes)
        verify('fd00:2:0:0::/64 med ' + r3_rloc in routes)
    verify('fd00:1:0:0::/64 s med fffe' in sed.get_netdata_routes())
    verify(not 'fd00:1:0:0::/64 med fffe' in sed.get_netdata_routes())


verify_within(check_r3_routes_added_on_all_nodes, 2)
verify_versions_incremented()

r1.add_prefix('fd00:1::/64', 'os')
r1.register_netdata()
verify_within(check_r1_prefix_added_on_all_nodes, 2)
verify_versions_incremented()

r2.add_prefix('fd00:2::/64', 'po', 'high')
r2.register_netdata()
verify_within(check_r2_prefix_added_on_all_nodes, 2)
verify_stabe_version_incremented()

r1.remove_prefix('fd00:1::/64')
r1.register_netdata()
verify_within(check_r1_prefix_removed_on_all_nodes, 2)
verify_versions_incremented()

r2.remove_prefix('fd00:2::/64')
r2.register_netdata()
verify_within(check_r2_prefix_removed_on_all_nodes, 2)
verify_stabe_version_incremented()

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
