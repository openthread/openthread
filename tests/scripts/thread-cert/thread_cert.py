#!/usr/bin/env python3
#
#  Copyright (c) 2019, The OpenThread Authors.
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
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS'
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
#

import os
import unittest

import config
import debug
from node import Node

PORT_OFFSET = int(os.getenv('PORT_OFFSET', "0"))

DEFAULT_PARAMS = {
    'is_mtd': False,
    'is_bbr': False,
    'mode': 'rsdn',
    'panid': 0xface,
    'whitelist': None,
    'version': '1.1',
}
"""Default configurations when creating nodes."""

EXTENDED_ADDRESS_BASE = 0x166e0a0000000000
"""Extended address base to keep U/L bit 1. The value is borrowed from Thread Test Harness."""


class TestCase(unittest.TestCase):
    """The base class for all thread certification test cases.

    The `topology` member of sub-class is used to create test topology.
    """

    def setUp(self):
        """Create simulator, nodes and apply configurations.
        """
        self._clean_up_tmp()

        self.simulator = config.create_default_simulator()
        self.nodes = {}

        initial_topology = {}
        for i, params in self.topology.items():
            if params:
                params = dict(DEFAULT_PARAMS, **params)
            else:
                params = DEFAULT_PARAMS.copy()
            initial_topology[i] = params

            self.nodes[i] = Node(
                i,
                params['is_mtd'],
                simulator=self.simulator,
                version=params['version'],
                is_bbr=params['is_bbr'],
            )
            self.nodes[i].set_panid(params['panid'])
            self.nodes[i].set_mode(params['mode'])

            if 'partition_id' in params:
                self.nodes[i].set_partition_id(params['partition_id'])
            if 'channel' in params:
                self.nodes[i].set_channel(params['channel'])
            if 'masterkey' in params:
                self.nodes[i].set_masterkey(params['masterkey'])
            if 'network_name' in params:
                self.nodes[i].set_network_name(params['network_name'])

            if 'router_selection_jitter' in params:
                self.nodes[i].set_router_selection_jitter(
                    params['router_selection_jitter'])
            if 'router_upgrade_threshold' in params:
                self.nodes[i].set_router_upgrade_threshold(
                    params['router_upgrade_threshold'])
            if 'router_downgrade_threshold' in params:
                self.nodes[i].set_router_downgrade_threshold(
                    params['router_downgrade_threshold'])

            if 'timeout' in params:
                self.nodes[i].set_timeout(params['timeout'])

            if 'active_dataset' in params:
                self.nodes[i].set_active_dataset(
                    params['active_dataset']['timestamp'],
                    panid=params['active_dataset'].get('panid'),
                    channel=params['active_dataset'].get('channel'),
                    channel_mask=params['active_dataset'].get('channel_mask'),
                    master_key=params['active_dataset'].get('master_key'))

            if 'pending_dataset' in params:
                self.nodes[i].set_pending_dataset(
                    params['pending_dataset']['pendingtimestamp'],
                    params['pending_dataset']['activetimestamp'],
                    panid=params['pending_dataset'].get('panid'),
                    channel=params['pending_dataset'].get('channel'))

            if 'key_switch_guardtime' in params:
                self.nodes[i].set_key_switch_guardtime(
                    params['key_switch_guardtime'])
            if 'key_sequence_counter' in params:
                self.nodes[i].set_key_sequence_counter(
                    params['key_sequence_counter'])

            if 'network_id_timeout' in params:
                self.nodes[i].set_network_id_timeout(
                    params['network_id_timeout'])

            if 'context_reuse_delay' in params:
                self.nodes[i].set_context_reuse_delay(
                    params['context_reuse_delay'])

            if 'max_children' in params:
                self.nodes[i].set_max_children(params['max_children'])

        # we have to add whitelist after nodes are all created
        for i, params in initial_topology.items():
            whitelist = params['whitelist']
            if not whitelist:
                continue

            for j in whitelist:
                rssi = None
                if isinstance(j, tuple):
                    j, rssi = j
                self.nodes[i].add_whitelist(self.nodes[j].get_addr64(),
                                            rssi=rssi)
            self.nodes[i].enable_whitelist()

        self._inspector = debug.Inspector(self)

    def inspect(self):
        self._inspector.inspect()

    def tearDown(self):
        """Destroy nodes and simulator.
        """
        for node in list(self.nodes.values()):
            node.stop()
            node.destroy()

        self.simulator.stop()
        del self.nodes
        del self.simulator

    def flush_all(self):
        """Flush away all captured messages of all nodes.
        """
        for i in list(self.nodes.keys()):
            self.simulator.get_messages_sent_by(i)

    def flush_nodes(self, nodes):
        """Flush away all captured messages of specified nodes.

        Args:
            nodes (list): nodes whose messages to flush.

        """
        for i in nodes:
            if i in list(self.nodes.keys()):
                self.simulator.get_messages_sent_by(i)

    def _clean_up_tmp(self):
        """
        Clean up node files in tmp directory
        """
        os.system(
            f"rm -f tmp/{PORT_OFFSET}_*.flash tmp/{PORT_OFFSET}_*.data tmp/{PORT_OFFSET}_*.swap"
        )
