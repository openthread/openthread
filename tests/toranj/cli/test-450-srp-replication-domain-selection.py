#!/usr/bin/env python3
#
#  Copyright (c) 2021, The OpenThread Authors.
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
#  Validate behavior of selecting/adopting domain in SRP Replication and partner discovery.
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

node1.form('srpl-domain')
node2.join(node1)
node3.join(node1)

verify(node1.srp_replication_get_state() == 'Disabled')
verify(node1.srp_replication_get_domain() == '(none)')
verify(node1.srp_replication_get_id() == '(none)')

node1.srp_replication_set_default_domain('default.local.')
verify(node1.srp_replication_get_default_domain() == 'default.local.')

node1.srp_replication_set_domain('test.local.')
verify(node1.srp_replication_get_domain() == 'test.local.')

node1.srp_replication_clear_domain()
verify(node1.srp_replication_get_domain() == '(none)')

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# `node1` as single SRPL with no domain specified (adopt default domain)

node1.srp_replication_enable()


def check_node1_srp_server_is_running():
    verify(node1.srp_server_get_state() == 'running')


verify_within(check_node1_srp_server_is_running, WAIT_TIME)

verify(node1.srp_replication_get_state() == 'Enabled')
verify(node1.srp_replication_get_id() != '(none)')
verify(node1.srp_replication_get_domain() == 'default.local.')
verify(node1.srp_replication_get_partners() == [])

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# `node2` with no specified domain, discovering `node1` and adopting its domain

node2.srp_replication_clear_domain()
verify(node2.srp_replication_get_domain() == '(none)')
node2.srp_replication_set_default_domain('default2.local')
verify(node2.srp_replication_get_default_domain() == 'default2.local')


def check_node2_srp_server_is_running():
    verify(node2.srp_server_get_state() == 'running')


node2.srp_replication_enable()
verify_within(check_node2_srp_server_is_running, WAIT_TIME)

verify(node2.srp_replication_get_domain() == node1.srp_replication_get_domain())
verify(node2.srp_replication_get_id() != '(none)')

# Verify that node1 and node has each other in their partners list
verify_node_has_partnters(node2, [node1])
verify_node_has_partnters(node1, [node2])

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# `node3` with a specific domain (different from `node1` and `node2)

verify(node3.srp_replication_get_state() == 'Disabled')
verify(node3.srp_replication_get_domain() == '(none)')
verify(node3.srp_replication_get_id() == '(none)')

node3.srp_replication_set_domain('another.local.')
node3.srp_replication_enable()


def check_node3_srp_server_is_running():
    verify(node3.srp_server_get_state() == 'running')


verify_within(check_node3_srp_server_is_running, WAIT_TIME)

verify(node3.srp_replication_get_state() == 'Enabled')
verify(node3.srp_replication_get_id() != '(none)')
verify(node3.srp_replication_get_domain() == 'another.local.')
verify(node3.srp_replication_get_partners() == [])

# Disable SRPL on node3

node3.srp_replication_disable()

verify(node3.srp_replication_get_state() == 'Disabled')
verify(node3.srp_replication_get_id() == '(none)')
verify(node3.srp_replication_get_partners() == [])
verify(node3.srp_server_get_state() == 'disabled')

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# `node3` with a specified domain same as `node1` and `node2`.

node3.srp_replication_clear_domain()
verify(node3.srp_replication_get_domain() == '(none)')
node3.srp_replication_set_domain(node1.srp_replication_get_domain())

node3.srp_replication_enable()
verify_within(check_node3_srp_server_is_running, WAIT_TIME)

verify(node3.srp_replication_get_state() == 'Enabled')
verify(node3.srp_replication_get_id() != '(none)')
verify(node3.srp_replication_get_domain() == node1.srp_replication_get_domain())

verify_node_has_partnters(node1, [node2, node3])
verify_node_has_partnters(node2, [node1, node3])
verify_node_has_partnters(node3, [node1, node2])

# -----------------------------------------------------------------------------------------------------------------------
# Test finished

cli.Node.finalize_all_nodes()

print('\'{}\' passed.'.format(test_name))
