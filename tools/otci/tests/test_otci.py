#!/usr/bin/env python3
#
#  Copyright (c) 2020, The OpenThread Authors.
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
#
import json
import logging
import os
import subprocess
import unittest

import otci
from otci.errors import CommandError

logging.basicConfig(level=logging.INFO)

TEST_CHANNEL = 22
TEST_NETWORK_NAME = 'OT CI'
TEST_PANID = 0xeeee
TEST_MASTERKEY = 'ffeeddccbbaa99887766554433221100'

REAL_DEVICE = int(os.getenv('REAL_DEVICE', '0'))


class TestOTCI(unittest.TestCase):

    def testCliRealDevice(self):
        if not REAL_DEVICE:
            self.skipTest('not for virtual device')

        if os.getenv('OTBR_SSH'):
            node = otci.connect_otbr_ssh(os.getenv('OTBR_SSH'))
        elif os.getenv('OT_CLI_SERIAL'):
            node = otci.connect_cli_serial(os.getenv('OT_CLI_SERIAL'))
        else:
            self.fail("Please set OT_CLI_SERIAL or OTBR_SSH to test the real device.")

        node.factory_reset()

        self._test_otci_single_node(node)

    def testCliSimRealTime(self):
        if REAL_DEVICE:
            self.skipTest('not for real device')

        subprocess.check_call('rm -rf tmp/', shell=True)
        VIRTUAL_TIME = int(os.getenv('VIRTUAL_TIME', "1"))

        import simulator

        if VIRTUAL_TIME:
            sim = simulator.VirtualTime(use_message_factory=False)
        else:
            sim = None

        if os.getenv('OT_CLI'):
            executable = os.getenv('OT_CLI')
            connector = otci.connect_cli_sim
        elif os.getenv('OT_NCP'):
            executable = os.getenv('OT_NCP')
            connector = otci.connect_ncp_sim
        else:
            self.fail("Please set OT_CLI to test virtual device")

        node1 = connector(executable, 1, simulator=sim)
        self._test_otci_single_node(node1)

        node1.factory_reset()

        node2 = connector(executable, 2, simulator=sim)
        node3 = connector(executable, 3, simulator=sim)
        node4 = connector(executable, 4, simulator=sim)

        self._test_otci_example(node1, node2)

        node1.factory_reset()
        node2.factory_reset()

        self._test_otci_multi_nodes(node1, node2, node3, node4)

    def _test_otci_single_node(self, leader):
        logging.info('leader version: %r', leader.version)
        logging.info('leader thread version: %r', leader.thread_version)
        logging.info('API version: %r', leader.api_version)
        logging.info('log level: %r', leader.get_log_level())

        leader.enable_promiscuous()
        self.assertTrue(leader.get_promiscuous())
        leader.disable_promiscuous()
        self.assertFalse(leader.get_promiscuous())
        try:
            logging.info("RCP version: %r", leader.get_rcp_version())
        except CommandError:
            pass

        self.assertTrue(leader.get_router_eligible())
        leader.disable_router_eligible()
        self.assertFalse(leader.get_router_eligible())
        leader.enable_router_eligible()

        self.assertFalse(leader.get_ifconfig_state())
        # ifconfig up
        leader.ifconfig_up()
        self.assertTrue(leader.get_ifconfig_state())

        logging.info('leader eui64 = %r', leader.get_eui64())
        logging.info('leader extpanid = %r', leader.get_extpanid())
        logging.info('leader masterkey = %r', leader.get_master_key())

        extaddr = leader.get_extaddr()
        self.assertEqual(16, len(extaddr))
        int(extaddr, 16)
        new_extaddr = 'aabbccddeeff0011'
        leader.set_extaddr(new_extaddr)
        self.assertEqual(new_extaddr, leader.get_extaddr())

        leader.set_network_name(TEST_NETWORK_NAME)

        leader.set_master_key(TEST_MASTERKEY)
        self.assertEqual(TEST_MASTERKEY, leader.get_master_key())

        leader.set_panid(TEST_PANID)
        self.assertEqual(TEST_PANID, leader.get_panid())

        leader.set_channel(TEST_CHANNEL)
        self.assertEqual(TEST_CHANNEL, leader.get_channel())

        leader.set_network_name(TEST_NETWORK_NAME)
        self.assertEqual(TEST_NETWORK_NAME, leader.get_network_name())

        self.assertEqual('rdn', leader.get_mode())
        leader.set_mode('-')
        self.assertEqual('-', leader.get_mode())
        leader.set_mode('rdn')
        self.assertEqual('rdn', leader.get_mode())

        logging.info('leader weight: %d', leader.get_leader_weight())
        leader.set_leader_weight(72)

        logging.info('domain name: %r', leader.get_domain_name())
        leader.set_domain_name("DefaultDomain2")
        self.assertEqual("DefaultDomain2", leader.get_domain_name())

        self.assertEqual(leader.get_preferred_partition_id(), 0)
        leader.set_preferred_partition_id(0xabcddead)
        self.assertEqual(leader.get_preferred_partition_id(), 0xabcddead)

        leader.thread_start()
        leader.wait(5)
        self.assertEqual('leader', leader.get_state())
        self.assertEqual(0xabcddead, leader.get_leader_data()['partition_id'])
        logging.info('leader key sequence counter = %d', leader.get_key_sequence_counter())

        rloc16 = leader.get_rloc16()
        leader_id = leader.get_router_id()
        self.assertEqual(rloc16, leader_id << 10)

        self.assertFalse(leader.get_child_list())
        self.assertEqual({}, leader.get_child_table())

        leader.enable_allowlist()
        leader.add_allowlist(leader.get_extaddr())
        leader.remove_allowlist(leader.get_extaddr())
        leader.set_allowlist([leader.get_extaddr()])
        leader.disable_allowlist()

        leader.add_ipmaddr('ff04::1')
        leader.del_ipmaddr('ff04::1')
        leader.add_ipmaddr('ff04::2')
        logging.info('leader ipmaddrs: %r', leader.get_ipmaddrs())
        self.assertFalse(leader.has_ipmaddr('ff04::1'))
        self.assertTrue(leader.has_ipmaddr('ff04::2'))
        self.assertTrue(leader.get_ipaddr_rloc())
        self.assertTrue(leader.get_ipaddr_linklocal())
        self.assertTrue(leader.get_ipaddr_mleid())

        leader.add_ipaddr('2001::1')
        leader.del_ipaddr('2001::1')
        leader.add_ipaddr('2001::2')
        logging.info('leader ipaddrs: %r', leader.get_ipaddrs())
        self.assertFalse(leader.has_ipaddr('2001::1'))
        self.assertTrue(leader.has_ipaddr('2001::2'))

        logging.info('leader bbr state: %r', leader.get_backbone_router_state())
        bbr_config = leader.get_backbone_router_config()
        logging.info('leader bbr config: %r', bbr_config)
        logging.info('leader PBBR: %r', leader.get_primary_backbone_router_info())

        leader.set_backbone_router_config(seqno=bbr_config['seqno'] + 1, delay=10, timeout=301)
        self.assertEqual({
            'seqno': bbr_config['seqno'] + 1,
            'delay': 10,
            'timeout': 301
        }, leader.get_backbone_router_config())

        leader.enable_backbone_router()
        leader.wait(3)

        logging.info('leader bbr state: %r', leader.get_backbone_router_state())
        logging.info('leader bbr config: %r', leader.get_backbone_router_config())
        logging.info('leader PBBR: %r', leader.get_primary_backbone_router_info())

        logging.info('leader bufferinfo: %r', leader.get_message_buffer_info())

        logging.info('child ipaddrs: %r', leader.get_child_ipaddrs())
        logging.info('child ipmax: %r', leader.get_child_ip_max())
        leader.set_child_ip_max(2)
        self.assertEqual(2, leader.get_child_ip_max())
        logging.info('childmax: %r', leader.get_max_children())

        logging.info('counter names: %r', leader.counter_names)
        for counter_name in leader.counter_names:
            logging.info('counter %s: %r', counter_name, leader.get_counter(counter_name))
            leader.reset_counter(counter_name)
            self.assertTrue(all(x == 0 for x in leader.get_counter(counter_name).values()))

        logging.info("CSL config: %r", leader.get_csl_config())
        leader.config_csl(channel=13, period=100, timeout=200)
        logging.info("CSL config: %r", leader.get_csl_config())

        logging.info("EID-to-RLOC cache: %r", leader.get_eidcache())

        logging.info("ipmaddr promiscuous: %r", leader.get_ipmaddr_promiscuous())
        leader.enable_ipmaddr_promiscuous()
        self.assertTrue(leader.get_ipmaddr_promiscuous())
        leader.disable_ipmaddr_promiscuous()
        self.assertFalse(leader.get_ipmaddr_promiscuous())

        logging.info("leader data: %r", leader.get_leader_data())
        logging.info("leader neighbor list: %r", leader.get_neighbor_list())
        logging.info("leader neighbor table: %r", leader.get_neighbor_table())
        logging.info("Leader external routes: %r", leader.get_routes())
        leader.add_route('2002::/64')
        leader.register_network_data()
        logging.info("Leader external routes: %r", leader.get_routes())

        self.assertEqual(1, len(leader.get_router_list()))
        self.assertEqual(1, len(leader.get_router_table()))
        logging.info("Leader router table: %r", leader.get_router_table())

        logging.info('scan: %r', leader.scan())
        logging.info('scan energy: %r', leader.scan_energy())

        leader.add_service(44970, '112233', 'aabbcc')
        leader.register_network_data()
        leader.add_service(44971, b'\x11\x22\x33', b'\xaa\xbb\xcc\xdd')

        leader.add_prefix("2001::/64")

        logging.info("network data: %r", leader.get_network_data())
        logging.info("network data raw: %r", leader.get_network_data_bytes())

        logging.info('txpower %r', leader.get_txpower())
        leader.set_txpower(-10)
        self.assertEqual(-10, leader.get_txpower())

        self.assertTrue(leader.is_singleton())

        leader.coap_start()
        leader.coap_set_test_resource_path('test')
        leader.coap_test_set_resource_content('helloworld')
        leader.coap_get(leader.get_ipaddr_rloc(), 'test')
        leader.coap_put(leader.get_ipaddr_rloc(), 'test', 'con', 'xxx')
        leader.coap_post(leader.get_ipaddr_rloc(), 'test', 'con', 'xxx')
        leader.coap_delete(leader.get_ipaddr_rloc(), 'test', 'con', 'xxx')
        leader.wait(1)
        leader.coap_stop()

        leader.udp_open()
        leader.udp_bind("::", 1234)
        leader.udp_send(leader.get_ipaddr_rloc(), 1234, text='hello')
        leader.udp_send(leader.get_ipaddr_rloc(), 1234, random_bytes=3)
        leader.udp_send(leader.get_ipaddr_rloc(), 1234, hex='112233')
        leader.wait(1)
        leader.udp_close()

        logging.info('dataset: %r', leader.get_dataset())
        logging.info('dataset active: %r', leader.get_dataset('active'))

        leader.dataset_init_buffer()
        leader.dataset_commit_buffer('pending')
        leader.dataset_init_buffer(get_active_dataset=True)
        leader.dataset_init_buffer(get_pending_dataset=True)

        logging.info('dataset: %r', leader.get_dataset())
        logging.info('dataset active: %r', leader.get_dataset('active'))
        logging.info('dataset pending: %r', leader.get_dataset('pending'))

        logging.info('dataset active -x: %r', leader.get_dataset_bytes('active'))
        logging.info('dataset pending -x: %r', leader.get_dataset_bytes('pending'))

    def _test_otci_example(self, node1, node2):
        node1.dataset_init_buffer()
        node1.dataset_set_buffer(network_name='test',
                                 master_key='00112233445566778899aabbccddeeff',
                                 panid=0xface,
                                 channel=11)
        node1.dataset_commit_buffer('active')

        node1.ifconfig_up()
        node1.thread_start()
        node1.wait(5)
        assert node1.get_state() == "leader"

        node1.commissioner_start()
        node1.wait(3)

        node1.commissioner_add_joiner("TEST123", eui64='*')

        node2.ifconfig_up()
        node2.set_router_selection_jitter(1)

        node2.joiner_start("TEST123")
        node2.wait(10, expect_line="Join success")
        node2.thread_start()
        node2.wait(5)
        assert node2.get_state() == "router"

    def _test_otci_multi_nodes(self, leader, commissioner, child1, child2):
        self.assertFalse(leader.get_ifconfig_state())

        # ifconfig up
        leader.ifconfig_up()
        self.assertTrue(leader.get_ifconfig_state())

        logging.info('leader eui64 = %r', leader.get_eui64())
        logging.info('leader extpanid = %r', leader.get_extpanid())
        logging.info('leader masterkey = %r', leader.get_master_key())

        extaddr = leader.get_extaddr()
        self.assertEqual(16, len(extaddr))
        int(extaddr, 16)
        new_extaddr = 'aabbccddeeff0011'
        leader.set_extaddr(new_extaddr)
        self.assertEqual(new_extaddr, leader.get_extaddr())

        leader.set_network_name(TEST_NETWORK_NAME)

        leader.set_master_key(TEST_MASTERKEY)
        self.assertEqual(TEST_MASTERKEY, leader.get_master_key())

        leader.set_panid(TEST_PANID)
        self.assertEqual(TEST_PANID, leader.get_panid())

        leader.set_channel(TEST_CHANNEL)
        self.assertEqual(TEST_CHANNEL, leader.get_channel())

        leader.set_network_name(TEST_NETWORK_NAME)
        self.assertEqual(TEST_NETWORK_NAME, leader.get_network_name())

        self.assertEqual('rdn', leader.get_mode())

        leader.thread_start()
        leader.wait(5)
        self.assertEqual('leader', leader.get_state())
        logging.info('leader key sequence counter = %d', leader.get_key_sequence_counter())

        rloc16 = leader.get_rloc16()
        leader_id = leader.get_router_id()
        self.assertEqual(rloc16, leader_id << 10)

        commissioner.ifconfig_up()
        commissioner.set_channel(TEST_CHANNEL)
        commissioner.set_panid(TEST_PANID)
        commissioner.set_network_name(TEST_NETWORK_NAME)
        commissioner.set_router_selection_jitter(1)
        commissioner.set_master_key(TEST_MASTERKEY)
        commissioner.thread_start()

        commissioner.wait(5)

        self.assertEqual('router', commissioner.get_state())

        for dst_ip in leader.get_ipaddrs():
            commissioner.ping(dst_ip, size=10, count=1, interval=2, hoplimit=3)

        self.assertEqual('disabled', commissioner.get_commissioiner_state())
        commissioner.commissioner_start()
        commissioner.wait(5)
        self.assertEqual('active', commissioner.get_commissioiner_state())

        logging.info('commissioner.get_network_id_timeout() = %d', commissioner.get_network_id_timeout())
        commissioner.set_network_id_timeout(60)
        self.assertEqual(60, commissioner.get_network_id_timeout())

        commissioner.commissioner_add_joiner('TEST123', eui64='*')
        commissioner.wait(3)

        child1.ifconfig_up()

        logging.info("child1 scan: %r", child1.scan())
        logging.info("child1 scan energy: %r", child1.scan_energy())

        child1.set_mode('rn')
        child1.set_router_selection_jitter(1)

        child1.joiner_start('TEST123')
        logging.info('joiner id = %r', child1.get_joiner_id())
        child1.wait(10, expect_line="Join success")

        child1.enable_allowlist()
        child1.disable_allowlist()
        child1.add_allowlist(commissioner.get_extaddr())
        child1.remove_allowlist(commissioner.get_extaddr())
        child1.set_allowlist([commissioner.get_extaddr()])

        child1.thread_start()
        child1.wait(3)
        self.assertEqual('child', child1.get_state())

        child1.thread_stop()

        child1.set_mode('n')
        child1.set_poll_period(1000)
        self.assertEqual(1000, child1.get_poll_period())

        child1.thread_start()
        child1.wait(3)
        self.assertEqual('child', child1.get_state())

        child2.ifconfig_up()
        child2.set_mode('rn')
        child2.set_router_selection_jitter(1)

        child2.joiner_start('TEST123')
        logging.info('joiner id = %r', child2.get_joiner_id())
        child2.wait(10, expect_line="Join success")

        child2.enable_allowlist()
        child2.disable_allowlist()
        child2.add_allowlist(commissioner.get_extaddr())
        child2.remove_allowlist(commissioner.get_extaddr())
        child2.set_allowlist([commissioner.get_extaddr()])

        child2.thread_start()
        child2.wait(3)
        self.assertEqual('child', child2.get_state())

        child_table = commissioner.get_child_table()
        logging.info('commissioiner child table: \n%s\n', json.dumps(child_table, indent=True))
        child_list = commissioner.get_child_list()
        logging.info('commissioiner child list: %r', child_list)
        for child_id in child_list:
            logging.info('child %s info: %r', child_id, commissioner.get_child_info(child_id))

        logging.info('child1 info: %r', commissioner.get_child_info(child1.get_rloc16()))
        logging.info('child2 info: %r', commissioner.get_child_info(child2.get_rloc16()))

        self.assertEqual(set(commissioner.get_child_list()), set(commissioner.get_child_table().keys()))

        child1.add_ipmaddr('ff04::1')
        child1.del_ipmaddr('ff04::1')
        child1.add_ipmaddr('ff04::2')
        logging.info('child1 ipmaddrs: %r', child1.get_ipmaddrs())
        self.assertFalse(child1.has_ipmaddr('ff04::1'))
        self.assertTrue(child1.has_ipmaddr('ff04::2'))

        child1.add_ipaddr('2001::1')
        child1.del_ipaddr('2001::1')
        child1.add_ipaddr('2001::2')
        logging.info('child1 ipaddrs: %r', child1.get_ipaddrs())
        self.assertFalse(child1.has_ipaddr('2001::1'))
        self.assertTrue(child1.has_ipaddr('2001::2'))

        logging.info('child ipaddrs: %r', commissioner.get_child_ipaddrs())

        logging.info("EID-to-RLOC cache: %r", leader.get_eidcache())

        logging.info("leader neighbor list: %r", leader.get_neighbor_list())
        logging.info("leader neighbor table: %r", leader.get_neighbor_table())
        logging.info("prefixes: %r", commissioner.get_prefixes())
        commissioner.add_prefix('2001::/64')
        commissioner.register_network_data()
        commissioner.wait(1)
        logging.info("prefixes: %r", commissioner.get_prefixes())

        self.assertEqual(2, len(leader.get_router_list()))
        self.assertEqual(2, len(leader.get_router_table()))

        self.assertFalse(leader.is_singleton())

        # Shutdown
        leader.thread_stop()
        logging.info("node state: %s", leader.get_state())
        leader.ifconfig_down()
        self.assertFalse(leader.get_ifconfig_state())

        leader.close()


if __name__ == '__main__':
    unittest.main()
