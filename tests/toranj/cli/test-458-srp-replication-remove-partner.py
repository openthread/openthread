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

# ----------------------------------------------------------------------------------------------------------------------
# Test description:
#
#  Validate behavior when removing and re-adding SRPL partners
#

test_name = __file__[:-3] if __file__.endswith('.py') else __file__
print('-' * 120)
print('Starting \'{}\''.format(test_name))

# ----------------------------------------------------------------------------------------------------------------------
# Helper function


def verify_node_has_partnters(node, partner_nodes):
    partners = node.srp_replication_get_partners()
    verify(len(partners) == len(partner_nodes))

    for partner_node in partner_nodes:
        parter_id = partner_node.srp_replication_get_id()
        for partner in partners:
            if partner['id'] == parter_id:
                verify(partner['state'] == 'RoutineOperation')
                verify(partner['sockaddr'] == f'[{partner_node.get_mleid_ip_addr()}]:{SRPL_PORT}')
                break
        else:
            raise cli.VerifyError('did not find matching parter_id in node\'s partners list')


# ----------------------------------------------------------------------------------------------------------------------
# Creating `cli.Nodes` instances

speedup = 4
cli.Node.set_time_speedup_factor(speedup)

node1 = cli.Node()
node2 = cli.Node()
node3 = cli.Node()

WAIT_TIME = 20
SRPL_PORT = 853

# ----------------------------------------------------------------------------------------------------------------------
# Test implementation

node1.form('srpl-rm-partner')
node2.join(node1)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Enable SRPL on `node1`

node1.srp_replication_enable()


def check_node1_srpl_enters_running_state():
    verify(node1.srp_replication_get_state() == 'running')


verify_within(check_node1_srpl_enters_running_state, WAIT_TIME)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Enable SRPL on `node2` and `node3` and wait for all of them to sync

node2.srp_replication_enable()
node3.srp_replication_enable()


def check_node1_syncs_with_node2_and_node3():
    verify_node_has_partnters(node1, [node2, node3])


verify_within(check_node1_syncs_with_node2_and_node3, WAIT_TIME)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Disable SRPL on `node2` and make sure `node1` has the entry as "Errored"

node2_id = node2.srp_replication_get_id()
node2.srp_replication_disable()

partners = node1.srp_replication_get_partners()
verify(len(partners) == 2)

for partner in partners:
    if partner['id'] == node2_id:
        verify(partner['state'] == 'Errored')
        break
else:
    raise cli.VerifyError('did not find node2 in node1 partner list')

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Re-enable SRPL on `node2` quickly and check that it is added back
# successfully.

node2.srp_replication_enable()

verify_within(check_node1_syncs_with_node2_and_node3, WAIT_TIME)

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Disable SRPL on `node2` and wait till entry times out and removed

node2.srp_replication_disable()


def check_node1_has_only_node3_as_partner():
    verify_node_has_partnters(node1, [node3])


verify_within(check_node1_has_only_node3_as_partner, WAIT_TIME)

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
