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

import json
import os
import subprocess
import sys
import time
import unittest

import config
import debug
from node import Node

PACKET_VERIFICATION = int(os.getenv('PACKET_VERIFICATION', 0))

if PACKET_VERIFICATION:
    from pktverify.addrs import ExtAddr
    from pktverify.packet_verifier import PacketVerifier

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


class NcpSupportMixin():
    """ The mixin to check whether a test case supports NCP.
    """

    SUPPORT_NCP = True

    def __init__(self, *args, **kwargs):
        if os.getenv('NODE_TYPE', 'sim') == 'ncp-sim' and not self.SUPPORT_NCP:
            # 77 means skip this test case in automake tests
            sys.exit(77)

        super().__init__(*args, **kwargs)


class TestCase(NcpSupportMixin, unittest.TestCase):
    """The base class for all thread certification test cases.

    The `topology` member of sub-class is used to create test topology.
    """

    TOPOLOGY = None

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._start_time = None
        self._do_packet_verification = PACKET_VERIFICATION and hasattr(self, 'verify')

    def setUp(self):
        """Create simulator, nodes and apply configurations.
        """
        self._clean_up_tmp()

        self.simulator = config.create_default_simulator()
        self.nodes = {}

        self._initial_topology = initial_topology = {}

        for i, params in self.TOPOLOGY.items():
            if params:
                params = dict(DEFAULT_PARAMS, **params)
            else:
                params = DEFAULT_PARAMS.copy()

            initial_topology[i] = params

            self.nodes[i] = Node(
                i,
                params['is_mtd'],
                simulator=self.simulator,
                name=params.get('name'),
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
                self.nodes[i].set_router_selection_jitter(params['router_selection_jitter'])
            if 'router_upgrade_threshold' in params:
                self.nodes[i].set_router_upgrade_threshold(params['router_upgrade_threshold'])
            if 'router_downgrade_threshold' in params:
                self.nodes[i].set_router_downgrade_threshold(params['router_downgrade_threshold'])

            if 'timeout' in params:
                self.nodes[i].set_timeout(params['timeout'])

            if 'active_dataset' in params:
                self.nodes[i].set_active_dataset(params['active_dataset']['timestamp'],
                                                 panid=params['active_dataset'].get('panid'),
                                                 channel=params['active_dataset'].get('channel'),
                                                 channel_mask=params['active_dataset'].get('channel_mask'),
                                                 master_key=params['active_dataset'].get('master_key'))

            if 'pending_dataset' in params:
                self.nodes[i].set_pending_dataset(params['pending_dataset']['pendingtimestamp'],
                                                  params['pending_dataset']['activetimestamp'],
                                                  panid=params['pending_dataset'].get('panid'),
                                                  channel=params['pending_dataset'].get('channel'))

            if 'key_switch_guardtime' in params:
                self.nodes[i].set_key_switch_guardtime(params['key_switch_guardtime'])
            if 'key_sequence_counter' in params:
                self.nodes[i].set_key_sequence_counter(params['key_sequence_counter'])

            if 'network_id_timeout' in params:
                self.nodes[i].set_network_id_timeout(params['network_id_timeout'])

            if 'context_reuse_delay' in params:
                self.nodes[i].set_context_reuse_delay(params['context_reuse_delay'])

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
                self.nodes[i].add_whitelist(self.nodes[j].get_addr64(), rssi=rssi)
            self.nodes[i].enable_whitelist()

        self._inspector = debug.Inspector(self)
        self._collect_test_info_after_setup()

    def inspect(self):
        self._inspector.inspect()

    def tearDown(self):
        """Destroy nodes and simulator.
        """
        if self._do_packet_verification and os.uname().sysname != "Linux":
            raise NotImplementedError(
                f'{self.testcase_name}: Packet Verification not available on {os.uname().sysname} (Linux only).')

        if self._do_packet_verification:
            time.sleep(3)

        for node in list(self.nodes.values()):
            node.stop()
            node.destroy()

        self.simulator.stop()

        if self._do_packet_verification:
            self._test_info['pcap'] = self._get_pcap_filename()

            test_info_path = self._output_test_info()
            os.environ['LD_LIBRARY_PATH'] = '/tmp/thread-wireshark'
            self._verify_packets(test_info_path)

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
        os.system(f"rm -f tmp/{PORT_OFFSET}_*.flash tmp/{PORT_OFFSET}_*.data tmp/{PORT_OFFSET}_*.swap")

    def _verify_packets(self, test_info_path: str):
        pv = PacketVerifier(test_info_path)
        pv.add_common_vars()
        self.verify(pv)
        print("Packet verification passed: %s" % test_info_path, file=sys.stderr)

    @property
    def testcase_name(self):
        return os.path.splitext(os.path.basename(sys.argv[0]))[0]

    def collect_ipaddrs(self):
        if not self._do_packet_verification:
            return

        test_info = self._test_info

        for i, node in self.nodes.items():
            ipaddrs = node.get_addrs()
            test_info['ipaddrs'][i] = ipaddrs
            mleid = node.get_mleid()
            test_info['mleids'][i] = mleid

    def collect_rloc16s(self):
        if not self._do_packet_verification:
            return

        test_info = self._test_info
        test_info['rloc16s'] = {}

        for i, node in self.nodes.items():
            test_info['rloc16s'][i] = '0x%04x' % node.get_addr16()

    def collect_extra_vars(self, **vars):
        if not self._do_packet_verification:
            return

        for k in vars.keys():
            assert isinstance(k, str), k

        test_vars = self._test_info.setdefault("extra_vars", {})
        test_vars.update(vars)

    def _collect_test_info_after_setup(self):
        """
        Collect test info after setUp
        """
        if not self._do_packet_verification:
            return

        test_info = self._test_info = {
            'testcase': self.testcase_name,
            'start_time': time.ctime(self._start_time),
            'pcap': '',
            'extaddrs': {},
            'ethaddrs': {},
            'ipaddrs': {},
            'mleids': {},
            'topology': self._initial_topology,
        }

        for i, node in self.nodes.items():
            extaddr = node.get_addr64()
            test_info['extaddrs'][i] = ExtAddr(extaddr).format_octets()

    def _output_test_info(self):
        """
        Output test info to json file after tearDown
        """
        filename = f'{self.testcase_name}.json'
        with open(filename, 'wt') as ofd:
            ofd.write(json.dumps(self._test_info, indent=1, sort_keys=True))

        return filename

    def _get_pcap_filename(self):
        current_pcap = os.getenv('TEST_NAME', 'current') + '.pcap'
        return os.path.abspath(current_pcap)

    def assure_run_ok(self, cmd, shell=False):
        if not shell and isinstance(cmd, str):
            cmd = cmd.split()
        proc = subprocess.run(cmd, stdout=sys.stdout, stderr=sys.stderr, shell=shell)
        print(">>> %s => %d" % (cmd, proc.returncode), file=sys.stderr)
        proc.check_returncode()
