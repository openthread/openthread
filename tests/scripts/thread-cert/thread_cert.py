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

import unittest

import config
import debug
from node import Node

DEFAULT_PARAMS = {
    'is_mtd': False,
    'mode': 'rsdn',
    'panid': 0xface,
    'whitelist': None,
    'version': '1.2',
}
"""Default configurations when creating nodes."""

EXTENDED_ADDRESS_BASE = 0x166e0a0000000000
"""Extended address base to keep U/L bit 1. The value is borrowed from Thread Test Harness."""


class TestCase(unittest.TestCase):
    """The base class for all thread certification test cases.

    The `topology` member of sub-class is used to create test topology.
    """

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.simulator = config.create_default_simulator()
        self.nodes = {}

    def setUp(self):
        """Create simulator, nodes and apply configurations.
        """
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
            )
            self.nodes[i].set_panid(params['panid'])
            self.nodes[i].set_mode(params['mode'])
            self.nodes[i].set_addr64(format(EXTENDED_ADDRESS_BASE + i, '016x'))

        # we have to add whitelist after nodes are all created
        for i, params in initial_topology.items():
            whitelist = params['whitelist']
            if not whitelist:
                continue

            for j in whitelist:
                self.nodes[i].add_whitelist(self.nodes[j].get_addr64())
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
