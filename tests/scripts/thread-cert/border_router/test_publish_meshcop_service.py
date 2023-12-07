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
import ipaddress
import logging
import unittest

import config
import thread_cert

# Test description:
#   This test verifies that OTBR publishes the meshcop service using a proper
#   configuration.
#
# Topology:
#    ----------------(eth)-----------------------------
#           |             |                    |
#          BR1           BR2           HOST (mDNS Browser)
#
#

BR1 = 1
BR2 = 2
HOST = 3


class PublishMeshCopService(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR1: {
            'name': 'BR_1',
            'allowlist': [],
            'is_otbr': True,
            'version': '1.2',
            'network_name': 'ot-br1',
            'boot_delay': 5,
        },
        BR2: {
            'name': 'BR_2',
            'allowlist': [],
            'is_otbr': True,
            'version': '1.2',
            'network_name': 'ot-br2',
            'boot_delay': 5,
        },
        HOST: {
            'name': 'Host',
            'is_host': True
        },
    }

    def test(self):
        host = self.nodes[HOST]
        br1 = self.nodes[BR1]
        br2 = self.nodes[BR2]
        br2.disable_br()

        # Use different network names to distinguish meshcop services
        br1.set_active_dataset(updateExisting=True, network_name='ot-br1')
        br2.set_active_dataset(updateExisting=True, network_name='ot-br2')

        host.start(start_radvd=False)
        self.simulator.go(20)

        self.assertEqual(br1.get_state(), 'disabled')
        # TODO enable this line when renaming with mDNSResponder is stable
        # self.check_meshcop_service(br1, host)
        br1.start()
        self.simulator.go(config.BORDER_ROUTER_STARTUP_DELAY)
        self.assertEqual('leader', br1.get_state())
        self.check_meshcop_service(br1, host)

        br1.disable_backbone_router()
        self.simulator.go(10)
        self.check_meshcop_service(br1, host)

        br1.stop()
        br1.set_active_dataset(updateExisting=True, network_name='ot-br1-1')
        br1.start()
        self.simulator.go(config.BORDER_ROUTER_STARTUP_DELAY)
        self.simulator.go(5)  # Needs to wait extra some time to update meshcop service on state changes.
        self.check_meshcop_service(br1, host)

        # verify that there are two meshcop services
        br2.set_active_dataset(updateExisting=True, network_name='ot-br2-1')
        br2.start()
        br2.disable_backbone_router()
        br2.enable_br()
        self.simulator.go(config.BORDER_ROUTER_STARTUP_DELAY)

        service_instances = host.browse_mdns_services('_meshcop._udp')
        self.assertEqual(len(service_instances), 2)
        br1_service = self.check_meshcop_service(br1, host)
        br2_service = self.check_meshcop_service(br2, host)
        self.assertNotEqual(br1_service['host'], br2_service['host'])

        br1.stop_otbr_service()
        self.simulator.go(5)
        br2.enable_backbone_router()
        self.simulator.go(5)
        self.assertEqual(len(host.browse_mdns_services('_meshcop._udp')), 1)
        br1.start_otbr_service()
        self.simulator.go(10)
        self.assertEqual(len(host.browse_mdns_services('_meshcop._udp')), 2)
        self.check_meshcop_service(br1, host)
        self.check_meshcop_service(br2, host)

        br1.factory_reset()

        dataset = {
            'timestamp': 1,
            'channel': config.CHANNEL,
            'channel_mask': config.CHANNEL_MASK,
            'extended_panid': config.EXTENDED_PANID,
            'mesh_local_prefix': config.MESH_LOCAL_PREFIX.split('/')[0],
            'network_key': binascii.hexlify(config.DEFAULT_NETWORK_KEY).decode(),
            'network_name': 'ot-br-1-3',
            'panid': config.PANID,
            'pskc': config.PSKC,
            'security_policy': config.SECURITY_POLICY,
        }

        br1.set_active_dataset(**dataset)
        self.simulator.go(10)

        self.assertEqual(len(host.browse_mdns_services('_meshcop._udp')), 2)
        self.check_meshcop_service(br1, host)
        self.check_meshcop_service(br2, host)

    def check_meshcop_service(self, br, host):
        services = self.discover_all_meshcop_services(host)
        for service in services:
            if service['txt']['nn'] == br.get_network_name():
                self.check_meshcop_service_by_data(br, service)
                return service
        self.fail('MeshCoP service not found')

    def check_meshcop_service_by_data(self, br, service_data):
        sb_data = service_data['txt']['sb'].encode('raw_unicode_escape')
        state_bitmap = int.from_bytes(sb_data, byteorder='big')
        logging.info(bin(state_bitmap))
        self.assertEqual((state_bitmap & 7), 1)  # connection mode = PskC
        if br.get_state() == 'disabled':
            self.assertEqual((state_bitmap >> 3 & 3), 0)  # Thread is disabled
        elif br.get_state() == 'detached':
            self.assertEqual((state_bitmap >> 3 & 3), 1)  # Thread is detached
        else:
            self.assertEqual((state_bitmap >> 3 & 3), 2)  # Thread is attached
        self.assertEqual((state_bitmap >> 5 & 3), 1)  # high availability
        self.assertEqual((state_bitmap >> 7 & 1),
                         br.get_state() not in ['disabled', 'detached'] and
                         br.get_backbone_router_state() != 'Disabled')  # BBR is enabled or not
        self.assertEqual((state_bitmap >> 8 & 1),
                         br.get_state() not in ['disabled', 'detached'] and
                         br.get_backbone_router_state() == 'Primary')  # BBR is primary or not
        self.assertEqual(service_data['txt']['nn'], br.get_network_name())
        self.assertEqual(service_data['txt']['rv'], '1')
        self.assertIn(service_data['txt']['tv'], ['1.1.0', '1.1.1', '1.2.0', '1.3.0'])

    def discover_all_meshcop_services(self, host):
        instance_names = host.browse_mdns_services('_meshcop._udp')
        services = []
        for instance_name in instance_names:
            services.append(host.discover_mdns_service(instance_name, '_meshcop._udp', None))
        return services


if __name__ == '__main__':
    unittest.main()
