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
            'is_otbr': True,
            'version': '1.2',
            'network_name': 'ot-br1',
            'channel': 12,
            'boot_delay': 5,
        },
        BR2: {
            'name': 'BR_2',
            'is_otbr': True,
            'version': '1.2',
            'network_name': 'ot-br2',
            'channel': 13,
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
        br1.set_network_name('ot-br1')
        br2.set_network_name('ot-br2')

        host.start(start_radvd=False)
        self.simulator.go(20)

        self.assertEqual(br1.get_state(), 'disabled')
        # TODO enable this line when renaming with mDNSResponder is stable
        # self.check_meshcop_service(br1, host)
        br1.start()
        self.simulator.go(config.BORDER_ROUTER_STARTUP_DELAY)
        self.assertEqual('leader', br1.get_state())
        br1_omr = ipaddress.IPv6Network(br1.get_br_omr_prefix()).network_address
        self.check_meshcop_service(br1, host, br1_omr)

        br1.disable_backbone_router()
        self.simulator.go(10)
        self.check_meshcop_service(br1, host, br1_omr)

        br1.stop()
        br1.set_network_name('ot-br1-1')
        br1.start()
        self.simulator.go(config.BORDER_ROUTER_STARTUP_DELAY)
        self.check_meshcop_service(br1, host, br1_omr)

        # verify that there are two meshcop services
        br2.set_network_name('ot-br2-1')
        br2.start()
        br2.disable_backbone_router()
        br2.enable_br()
        self.simulator.go(config.BORDER_ROUTER_STARTUP_DELAY)

        br2_omr = ipaddress.IPv6Network(br2.get_br_omr_prefix()).network_address
        service_instances = host.browse_mdns_services('_meshcop._udp')
        self.assertEqual(len(service_instances), 2)
        br1_service = self.check_meshcop_service(br1, host, br1_omr)
        br2_service = self.check_meshcop_service(br2, host, br2_omr)
        self.assertNotEqual(br1_service['host'], br2_service['host'])

        br1.stop_otbr_service()
        self.simulator.go(5)
        br2.enable_backbone_router()
        self.simulator.go(5)
        self.assertEqual(len(host.browse_mdns_services('_meshcop._udp')), 1)
        br1.start_otbr_service()
        self.simulator.go(10)
        self.assertEqual(len(host.browse_mdns_services('_meshcop._udp')), 2)
        self.check_meshcop_service(br1, host, br1_omr)
        self.check_meshcop_service(br2, host, br2_omr)

        # only the smallest OMR should be published in the network
        br1.stop()
        br1.set_active_dataset_hex(br2.get_active_dataset_hex())
        br1.start()
        br2.start()
        self.simulator.go(10)
        br1_omr = ipaddress.IPv6Network(br1.get_br_omr_prefix()).network_address
        br2_omr = ipaddress.IPv6Network(br2.get_br_omr_prefix()).network_address
        min_omr = min(br1_omr, br2_omr)
        self.check_meshcop_service(br1, host, min_omr)
        self.check_meshcop_service(br2, host, min_omr)

        br1.factory_reset()
        br1.set_network_name('ot-br-1-3')
        self.assertEqual(len(host.browse_mdns_services('_meshcop._udp')), 2)
        self.check_meshcop_service(br1, host, None)
        self.check_meshcop_service(br2, host, br2_omr)

    def check_meshcop_service(self, br, host, omr):
        services = host.browse_mdns_services('_meshcop._udp')
        for service in services:
            if service['txt']['xa'].hex() == br.get_addr64():
                self.check_meshcop_service_by_data(br, service, omr)
                return service
        self.fail('MeshCoP service not found')

    def check_meshcop_service_by_data(self, br, service_data, omr):
        sb_data = service_data['txt']['sb']
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
        if omr is not None:
            discovered_omr = service_data['txt']['omr']
            self.assertEqual(discovered_omr[0], 64)
            self.assertEqual(len(discovered_omr), 9)
            self.assertEqual(ipaddress.IPv6Address(discovered_omr[1:] + bytes([0] * 8)), omr)
        else:
            self.assertTrue('omr' not in service_data['txt'])
        self.assertEqual(service_data['txt']['nn'].decode('raw_unicode_escape'), br.get_network_name())
        self.assertEqual(service_data['txt']['rv'].decode('raw_unicode_escape'), '1')
        self.assertIn(service_data['txt']['tv'].decode('raw_unicode_escape'), ['1.1.0', '1.1.1', '1.2.0', '1.3.0'])


if __name__ == '__main__':
    unittest.main()
