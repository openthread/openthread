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

import binascii
import json
import logging
import os
import signal
import stat
import subprocess
import sys
import time
import traceback
import unittest
from typing import Optional, Callable, Union, Mapping, Any

import config
import debug
from node import Node, OtbrNode, HostNode
from pktverify import utils as pvutils

PACKET_VERIFICATION = int(os.getenv('PACKET_VERIFICATION', 0))

if PACKET_VERIFICATION:
    from pktverify.addrs import ExtAddr, EthAddr
    from pktverify.packet_verifier import PacketVerifier

PORT_OFFSET = int(os.getenv('PORT_OFFSET', "0"))

ENV_THREAD_VERSION = os.getenv('THREAD_VERSION', '1.1')

DEFAULT_PARAMS = {
    'is_mtd': False,
    'is_ftd': False,
    'is_bbr': False,
    'is_otbr': False,
    'is_host': False,
    'mode': 'rdn',
    'allowlist': None,
    'version': ENV_THREAD_VERSION,
}
"""Default configurations when creating nodes."""

FTD_DEFAULT_PARAMS = {
    'is_ftd': True,
    'router_selection_jitter': config.DEFAULT_ROUTER_SELECTION_JITTER,
}

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

    USE_MESSAGE_FACTORY = True
    TOPOLOGY = {}
    CASE_WIRESHARK_PREFS = None
    SUPPORT_THREAD_1_1 = True
    PACKET_VERIFICATION = config.PACKET_VERIFICATION_DEFAULT

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')

        self._start_time = None
        self._do_packet_verification = PACKET_VERIFICATION and hasattr(self, 'verify') \
                                       and self.PACKET_VERIFICATION == PACKET_VERIFICATION

        # store all the backbone network names that are used in the test case,
        # it keeps empty when there's no backbone traffic in the test (no otbr or host nodes)
        self._backbone_network_names = []

    def skipTest(self, reason: Any) -> None:
        self._testSkipped = True
        super(TestCase, self).skipTest(reason)

    def setUp(self):
        self._testSkipped = False

        if ENV_THREAD_VERSION == '1.1' and not self.SUPPORT_THREAD_1_1:
            self.skipTest('Thread 1.1 not supported.')

        try:
            self._setUp()
        except:
            traceback.print_exc()
            for node in list(self.nodes.values()):
                try:
                    node.destroy()
                except Exception:
                    traceback.print_exc()

            raise

    def _setUp(self):
        """Create simulator, nodes and apply configurations.
        """
        self._clean_up_tmp()

        key_manager = config.create_default_thread_key_manager()

        self.simulator = config.create_default_simulator(
            config.create_default_thread_message_factory(key_manager) if self.USE_MESSAGE_FACTORY else None)
        self.nodes = {}

        os.environ['LD_LIBRARY_PATH'] = '/tmp/thread-wireshark'

        if self._has_backbone_traffic():
            self._prepare_backbone_network()
            self._start_backbone_sniffer()

        self._initial_topology = initial_topology = {}

        for i, params in self.TOPOLOGY.items():
            params = self._parse_params(params)
            initial_topology[i] = params

            backbone_network_name = self._construct_backbone_network_name(params.get('backbone_network_id')) \
                                    if self._has_backbone_traffic() else None

            logging.info("Creating node %d: %r", i, params)
            logging.info("Backbone network: %s", backbone_network_name)

            if params['is_otbr']:
                nodeclass = OtbrNode
            elif params['is_host']:
                nodeclass = HostNode
            else:
                nodeclass = Node

            node = nodeclass(i,
                             is_mtd=params['is_mtd'],
                             simulator=self.simulator,
                             name=params.get('name'),
                             version=params['version'],
                             is_bbr=params['is_bbr'],
                             backbone_network=backbone_network_name)
            if 'boot_delay' in params:
                self.simulator.go(params['boot_delay'])

            self.nodes[i] = node

            if node.is_host:
                continue

            self.nodes[i].set_mode(params['mode'])

            if 'partition_id' in params:
                self.nodes[i].set_preferred_partition_id(params['partition_id'])

            if params['is_ftd']:
                self.nodes[i].set_router_selection_jitter(params['router_selection_jitter'])

            if 'router_upgrade_threshold' in params:
                self.nodes[i].set_router_upgrade_threshold(params['router_upgrade_threshold'])
            if 'router_downgrade_threshold' in params:
                self.nodes[i].set_router_downgrade_threshold(params['router_downgrade_threshold'])
            if 'router_eligible' in params:
                self.nodes[i].set_router_eligible(params['router_eligible'])
            if 'prefer_router_id' in params:
                self.nodes[i].prefer_router_id(params['prefer_router_id'])

            if 'timeout' in params:
                self.nodes[i].set_timeout(params['timeout'])

            self._set_up_active_dataset(self.nodes[i], params)

            if 'pending_dataset' in params:
                self.nodes[i].set_pending_dataset(params['pending_dataset']['pendingtimestamp'],
                                                  params['pending_dataset']['activetimestamp'],
                                                  panid=params['pending_dataset'].get('panid'),
                                                  channel=params['pending_dataset'].get('channel'),
                                                  delay=params['pending_dataset'].get('delay'))

            if 'key_sequence_counter' in params:
                self.nodes[i].set_key_sequence_counter(params['key_sequence_counter'])

            if 'network_id_timeout' in params:
                self.nodes[i].set_network_id_timeout(params['network_id_timeout'])

            if 'context_reuse_delay' in params:
                self.nodes[i].set_context_reuse_delay(params['context_reuse_delay'])

            if 'max_children' in params:
                self.nodes[i].set_max_children(params['max_children'])

            if 'bbr_registration_jitter' in params:
                self.nodes[i].set_bbr_registration_jitter(params['bbr_registration_jitter'])

            if 'router_id_range' in params:
                self.nodes[i].set_router_id_range(params['router_id_range'][0], params['router_id_range'][1])

        # we have to add allowlist after nodes are all created
        for i, params in initial_topology.items():
            allowlist = params['allowlist']
            if allowlist is None:
                continue

            for j in allowlist:
                rssi = None
                if isinstance(j, tuple):
                    j, rssi = j
                self.nodes[i].add_allowlist(self.nodes[j].get_addr64(), rssi=rssi)
            self.nodes[i].enable_allowlist()

        self._inspector = debug.Inspector(self)
        self._collect_test_info_after_setup()

    def _set_up_active_dataset(self, node, params):
        dataset = {
            'timestamp': 1,
            'channel': config.CHANNEL,
            'channel_mask': config.CHANNEL_MASK,
            'extended_panid': config.EXTENDED_PANID,
            'mesh_local_prefix': config.MESH_LOCAL_PREFIX.split('/')[0],
            'network_key': config.DEFAULT_NETWORK_KEY,
            'network_name': config.NETWORK_NAME,
            'panid': config.PANID,
            'pskc': config.PSKC,
            'security_policy': config.SECURITY_POLICY,
        }

        if 'channel' in params:
            dataset['channel'] = params['channel']
        if 'network_key' in params:
            dataset['network_key'] = params['network_key']
        if 'network_name' in params:
            dataset['network_name'] = params['network_name']
        if 'panid' in params:
            dataset['panid'] = params['panid']

        if 'active_dataset' in params:
            dataset.update(params['active_dataset'])

        node.set_active_dataset(**dataset)

    def inspect(self):
        self._inspector.inspect()

    def tearDown(self):
        """Destroy nodes and simulator.
        """
        if self._do_packet_verification and os.uname().sysname != "Linux":
            raise NotImplementedError(
                f'{self.test_name}: Packet Verification not available on {os.uname().sysname} (Linux only).')

        if self._do_packet_verification:
            self.simulator.go(3)

        if self._has_backbone_traffic():
            # Stop Backbone sniffer before stopping nodes so that we don't capture Codecov Uploading traffic
            self._stop_backbone_sniffer()

        for node in list(self.nodes.values()):
            try:
                node.stop()
            except:
                traceback.print_exc()
            finally:
                node.destroy()

        self.simulator.stop()

        if self._has_backbone_traffic():
            self._remove_backbone_network()

        if self._do_packet_verification:

            if self._has_backbone_traffic():
                pcap_filename = self._merge_thread_backbone_pcaps()
            else:
                pcap_filename = self._get_thread_pcap_filename()

            self._test_info['pcap'] = pcap_filename

            test_info_path = self._output_test_info()
            if not self._testSkipped:
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
        pv = PacketVerifier(test_info_path, self.CASE_WIRESHARK_PREFS)
        pv.add_common_vars()
        pv.pkts.filter_thread_unallowed_icmpv6().must_not_next()
        self.verify(pv)
        print("Packet verification passed: %s" % test_info_path, file=sys.stderr)

    @property
    def test_name(self):
        return os.getenv('TEST_NAME', 'current')

    def collect_ipaddrs(self):
        if not self._do_packet_verification:
            return

        test_info = self._test_info

        for i, node in self.nodes.items():
            ipaddrs = node.get_addrs()

            if hasattr(node, 'get_ether_addrs'):
                ipaddrs += node.get_ether_addrs()

            test_info['ipaddrs'][i] = ipaddrs
            if not node.is_host:
                mleid = node.get_mleid()
                test_info['mleids'][i] = mleid

    def collect_rloc16s(self):
        if not self._do_packet_verification:
            return

        test_info = self._test_info
        test_info['rloc16s'] = {}

        for i, node in self.nodes.items():
            if not node.is_host:
                test_info['rloc16s'][i] = '0x%04x' % node.get_addr16()

    def collect_rlocs(self):
        if not self._do_packet_verification:
            return

        test_info = self._test_info
        test_info['rlocs'] = {}

        for i, node in self.nodes.items():
            if node.is_host:
                continue

            test_info['rlocs'][i] = node.get_rloc()

    def collect_omrs(self):
        if not self._do_packet_verification:
            return

        test_info = self._test_info
        test_info['omrs'] = {}

        for i, node in self.nodes.items():
            if node.is_host:
                continue

            test_info['omrs'][i] = node.get_ip6_address(config.ADDRESS_TYPE.OMR)

    def collect_duas(self):
        if not self._do_packet_verification:
            return

        test_info = self._test_info
        test_info['duas'] = {}

        for i, node in self.nodes.items():
            if node.is_host:
                continue

            test_info['duas'][i] = node.get_ip6_address(config.ADDRESS_TYPE.DUA)

    def collect_leader_aloc(self, node):
        if not self._do_packet_verification:
            return

        test_info = self._test_info
        test_info['leader_aloc'] = self.nodes[node].get_addr_leader_aloc()

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
            'script': os.path.abspath(sys.argv[0]),
            'testcase': self.test_name,
            'start_time': time.ctime(self._start_time),
            'pcap': '',
            'extaddrs': {},
            'ethaddrs': {},
            'ipaddrs': {},
            'mleids': {},
            'topology': self._initial_topology,
            'backbone': {
                'interface': config.BACKBONE_DOCKER_NETWORK_NAME,
                'prefix': config.BACKBONE_PREFIX,
            },
            'domain_prefix': config.DOMAIN_PREFIX,
            'env': {
                'PORT_OFFSET': config.PORT_OFFSET,
            },
        }

        for i, node in self.nodes.items():
            if not node.is_host:
                extaddr = node.get_addr64()
                test_info['extaddrs'][i] = ExtAddr(extaddr).format_octets()

            if node.is_host or node.is_otbr:
                ethaddr = node.get_ether_mac()
                test_info['ethaddrs'][i] = EthAddr(ethaddr).format_octets()

    def _construct_backbone_network_name(self, backbone_network_id) -> str:
        """
        Construct the name of the backbone network based on the given backbone network id from TOPOLOGY. If the
        backbone_network_id is not defined in TOPOLOGY, use the default backbone network id.
        """
        id = backbone_network_id if backbone_network_id is not None else config.BACKBONE_DOCKER_NETWORK_DEFAULT_ID
        backbone_name = f'{config.BACKBONE_DOCKER_NETWORK_NAME}.{id}'

        assert backbone_name in self._backbone_network_names

        return backbone_name

    def _output_test_info(self):
        """
        Output test info to json file after tearDown
        """
        filename = f'{self.test_name}.json'
        with open(filename, 'wt') as ofd:
            ofd.write(json.dumps(self._test_info, indent=1, sort_keys=True))

        return filename

    def _get_thread_pcap_filename(self):
        current_pcap = self.test_name + '.pcap'
        return os.path.abspath(current_pcap)

    def assure_run_ok(self, cmd, shell=False):
        if not shell and isinstance(cmd, str):
            cmd = cmd.split()
        proc = subprocess.run(cmd, stdout=sys.stdout, stderr=sys.stderr, shell=shell)
        print(">>> %s => %d" % (cmd, proc.returncode), file=sys.stderr)
        proc.check_returncode()

    def _parse_params(self, params: Optional[dict]) -> dict:
        params = params or {}

        if params.get('is_bbr') or params.get('is_otbr'):
            # BBRs must not use thread version 1.1
            version = params.get('version', '1.4')
            assert version != '1.1', params
            params['version'] = version
            params.setdefault('bbr_registration_jitter', config.DEFAULT_BBR_REGISTRATION_JITTER)
        elif params.get('is_host'):
            # Hosts must not specify thread version
            assert params.get('version', '') == '', params
            params['version'] = ''

        # use 1.4 node for 1.2 tests
        if params.get('version') == '1.2':
            params['version'] = '1.4'

        is_ftd = (not params.get('is_mtd') and not params.get('is_host'))

        effective_params = DEFAULT_PARAMS.copy()

        if is_ftd:
            effective_params.update(FTD_DEFAULT_PARAMS)

        effective_params.update(params)

        return effective_params

    def _has_backbone_traffic(self):
        for param in self.TOPOLOGY.values():
            if param and (param.get('is_otbr') or param.get('is_host')):
                return True

        return False

    def _prepare_backbone_network(self):
        """
        Creates one or more backbone networks (Docker bridge networks) based on the TOPOLOGY definition.

        * If `backbone_network_id` is defined in the TOPOLOGY:
            * Network name:   `backbone{PORT_OFFSET}.{backbone_network_id}`      (e.g., "backbone0.0", "backbone0.1")
            * Network prefix: `backbone{PORT_OFFSET}:{backbone_network_id}::/64` (e.g., "9100:0::/64", "9100:1::/64")

        * If `backbone_network_id` is undefined:
            * Network name:   `backbone{PORT_OFFSET}.0`    (e.g., "backbone0.0")
            * Network prefix: `backbone{PORT_OFFSET}::/64` (e.g., "9100::/64")
        """
        # Create backbone_set to store all the backbone_ids by parsing TOPOLOGY.
        backbone_id_set = set()
        for node in self.TOPOLOGY:
            id = self.TOPOLOGY[node].get('backbone_network_id')
            if id is not None:
                backbone_id_set.add(id)

        # Add default backbone network id if backbone_set is empty
        if not backbone_id_set:
            backbone_id_set.add(config.BACKBONE_DOCKER_NETWORK_DEFAULT_ID)

        # Iterate over the backbone_set and create backbone network(s)
        for id in backbone_id_set:
            backbone = f'{config.BACKBONE_DOCKER_NETWORK_NAME}.{id}'
            backbone_prefix = f'{config.BACKBONE_IPV6_ADDR_START}:{id}::/64'
            self._backbone_network_names.append(backbone)
            self.assure_run_ok(
                f'docker network create --driver bridge --ipv6 --subnet {backbone_prefix} -o "com.docker.network.bridge.name"="{backbone}" {backbone} || true',
                shell=True)

    def _remove_backbone_network(self):
        for network_name in self._backbone_network_names:
            self.assure_run_ok(f'docker network rm {network_name}', shell=True)

    def _start_backbone_sniffer(self):
        assert self._backbone_network_names, 'Internal Error: self._backbone_network_names is empty'
        # TODO: support sniffer on multiple backbone networks
        sniffer_interface = self._backbone_network_names[0]

        # don't know why but I have to create the empty bbr.pcap first, otherwise tshark won't work
        # self.assure_run_ok("truncate --size 0 bbr.pcap && chmod 664 bbr.pcap", shell=True)
        pcap_file = self._get_backbone_pcap_filename()
        try:
            os.remove(pcap_file)
        except FileNotFoundError:
            pass

        dumpcap = pvutils.which_dumpcap()
        self._dumpcap_proc = subprocess.Popen([dumpcap, '-i', sniffer_interface, '-w', pcap_file],
                                              stdout=sys.stdout,
                                              stderr=sys.stderr)
        time.sleep(0.2)
        assert self._dumpcap_proc.poll() is None, 'tshark terminated unexpectedly'
        logging.info('Backbone sniffer launched successfully on interface %s, pid=%s', sniffer_interface,
                     self._dumpcap_proc.pid)
        os.chmod(pcap_file, stat.S_IWUSR | stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH)

    def _get_backbone_pcap_filename(self):
        backbone_pcap = self.test_name + '_backbone.pcap'
        return os.path.abspath(backbone_pcap)

    def _get_merged_pcap_filename(self):
        backbone_pcap = self.test_name + '_merged.pcap'
        return os.path.abspath(backbone_pcap)

    def _stop_backbone_sniffer(self):
        self._dumpcap_proc.send_signal(signal.SIGTERM)
        self._dumpcap_proc.__exit__(None, None, None)
        logging.info('Backbone sniffer terminated successfully: pid=%s' % self._dumpcap_proc.pid)

    def _merge_thread_backbone_pcaps(self):
        thread_pcap = self._get_thread_pcap_filename()
        backbone_pcap = self._get_backbone_pcap_filename()
        merged_pcap = self._get_merged_pcap_filename()

        mergecap = pvutils.which_mergecap()
        self.assure_run_ok(f'{mergecap} -w {merged_pcap} {thread_pcap} {backbone_pcap}', shell=True)
        return merged_pcap

    def wait_until(self, cond: Callable[[], bool], timeout: int, go_interval: int = 1):
        while True:
            self.simulator.go(go_interval)

            if cond():
                break

            timeout -= go_interval
            if timeout <= 0:
                raise RuntimeError(f'wait failed after {timeout} seconds')

    def wait_node_state(self, node: Union[int, Node], state: str, timeout: int):
        node = self.nodes[node] if isinstance(node, int) else node
        self.wait_until(lambda: node.get_state() == state, timeout)

    def wait_route_established(self, node1: int, node2: int, timeout=10):
        node2_addr = self.nodes[node2].get_ip6_address(config.ADDRESS_TYPE.RLOC)

        while timeout > 0:

            if self.nodes[node1].ping(node2_addr):
                break

            self.simulator.go(1)
            timeout -= 1

        else:
            raise Exception("Route between node %d and %d is not established" % (node1, node2))

    def assertDictIncludes(self, actual: Mapping[str, str], expected: Mapping[str, str]):
        """ Asserts the `actual` dict includes the `expected` dict.

        Args:
            actual: A dict for checking.
            expected: The expected items that the actual dict should contains.
        """
        for k, v in expected.items():
            if k not in actual:
                raise AssertionError(f"key {k} is not found in first dict")
            if v != actual[k]:
                raise AssertionError(f"{repr(actual[k])} != {repr(v)} for key {k}")
