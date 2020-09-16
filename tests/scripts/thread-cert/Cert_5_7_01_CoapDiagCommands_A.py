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

import unittest

import config
import mle
import network_diag
import network_layer
import thread_cert
from network_diag import TlvType

LEADER = 1
ROUTER1 = 2
REED1 = 3
SED1 = 4
MED1 = 5
FED1 = 6

DUT = ROUTER1
MTDS = [MED1, SED1]


class Cert_5_7_01_CoapDiagCommands_A(thread_cert.TestCase):
    SUPPORT_NCP = False

    TOPOLOGY = {
        LEADER: {
            'allowlist': [ROUTER1],
        },
        ROUTER1: {
            'allowlist': [LEADER, REED1, SED1, MED1, FED1],
            'router_selection_jitter': 1
        },
        REED1: {
            'allowlist': [ROUTER1],
            'router_upgrade_threshold': 0
        },
        SED1: {
            'is_mtd': True,
            'mode': 's',
            'allowlist': [ROUTER1],
            'timeout': config.DEFAULT_CHILD_TIMEOUT
        },
        MED1: {
            'is_mtd': True,
            'mode': 'rsn',
            'allowlist': [ROUTER1]
        },
        FED1: {
            'allowlist': [ROUTER1],
            'router_upgrade_threshold': 0
        },
    }

    def test(self):
        # 1 - Form topology
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[ROUTER1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER1].get_state(), 'router')

        self.nodes[REED1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[REED1].get_state(), 'child')

        self.nodes[SED1].start()
        self.simulator.go(10)
        self.assertEqual(self.nodes[SED1].get_state(), 'child')

        self.nodes[MED1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[MED1].get_state(), 'child')

        self.nodes[FED1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[FED1].get_state(), 'child')

        dut_rloc = self.nodes[DUT].get_ip6_address(config.ADDRESS_TYPE.RLOC)

        # 2 - Leader sends DIAG_GET.req
        tlv_types = [
            TlvType.EXT_ADDRESS, TlvType.ADDRESS16, TlvType.MODE, TlvType.CONNECTIVITY, TlvType.ROUTE64,
            TlvType.LEADER_DATA, TlvType.NETWORK_DATA, TlvType.IPV6_ADDRESS_LIST, TlvType.CHANNEL_PAGES
        ]
        self.nodes[LEADER].send_network_diag_get(dut_rloc, tlv_types)
        self.simulator.go(2)

        dut_messages = self.simulator.get_messages_sent_by(DUT)

        diag_get_rsp = dut_messages.next_coap_message(code='2.04')
        diag_get_rsp.assertCoapMessageContainsTlv(network_layer.MacExtendedAddress)
        diag_get_rsp.assertCoapMessageContainsTlv(mle.Address16)
        diag_get_rsp.assertCoapMessageContainsTlv(mle.Mode)
        diag_get_rsp.assertCoapMessageContainsTlv(mle.Connectivity)
        diag_get_rsp.assertCoapMessageContainsTlv(mle.Route64)
        diag_get_rsp.assertCoapMessageContainsTlv(mle.LeaderData)
        diag_get_rsp.assertCoapMessageContainsTlv(mle.NetworkData)
        diag_get_rsp.assertCoapMessageContainsTlv(network_diag.Ipv6AddressList)
        diag_get_rsp.assertCoapMessageContainsTlv(network_diag.ChannelPages)

        # 3 - Leader sends DIAG_GET.req (MAC Counters TLV type included)
        self.nodes[LEADER].send_network_diag_get(dut_rloc, [TlvType.MAC_COUNTERS])
        self.simulator.go(2)

        dut_messages = self.simulator.get_messages_sent_by(DUT)
        diag_get_rsp = dut_messages.next_coap_message(code='2.04')
        diag_get_rsp.assertCoapMessageContainsTlv(network_diag.MacCounters)
        mac_counters = diag_get_rsp.get_coap_message_tlv(network_diag.MacCounters)

        # 4 - Leader sends DIAG_GET.req (Timeout/Polling Period TLV type included)
        self.nodes[LEADER].send_network_diag_get(dut_rloc, [TlvType.POLLING_PERIOD])
        self.simulator.go(2)

        dut_messages = self.simulator.get_messages_sent_by(DUT)
        diag_get_rsp = dut_messages.next_coap_message(code='2.04')
        diag_get_rsp.assertCoapMessageDoesNotContainTlv(mle.Timeout)

        # 5 - Leader sends DIAG_GET.req (Battery Level and Supply Voltage TLV types included)
        self.nodes[LEADER].send_network_diag_get(dut_rloc, [TlvType.BATTERY_LEVEL, TlvType.SUPPLY_VOLTAGE])
        self.simulator.go(2)

        dut_messages = self.simulator.get_messages_sent_by(DUT)
        diag_get_rsp = dut_messages.next_coap_message(code='2.04')
        diag_get_rsp.assertCoapMessageContainsOptionalTlv(network_diag.BatteryLevel)
        diag_get_rsp.assertCoapMessageContainsOptionalTlv(network_diag.SupplyVoltage)

        # 6 - Leader sends DIAG_GET.req (Child Table TLV types included)
        self.nodes[LEADER].send_network_diag_get(dut_rloc, [TlvType.CHILD_TABLE])
        self.simulator.go(2)

        dut_messages = self.simulator.get_messages_sent_by(DUT)
        diag_get_rsp = dut_messages.next_coap_message(code='2.04')
        diag_get_rsp.assertCoapMessageContainsTlv(network_diag.ChildTable)
        child_table = diag_get_rsp.get_coap_message_tlv(network_diag.ChildTable)
        self.assertEqual(len(child_table.children), 4)
        # TODO(wgtdkp): more validations

        # 7 - Leader sends DIAG_RST.ntf (MAC Counters TLV type included)
        self.nodes[LEADER].send_network_diag_reset(dut_rloc, [TlvType.MAC_COUNTERS])
        self.simulator.go(2)

        dut_messages = self.simulator.get_messages_sent_by(DUT)

        # Make sure the response is there.
        dut_messages.next_coap_message(code='2.04')

        # 8 - Leader Sends DIAG_GET.req (MAC Counters TLV type included)
        self.nodes[LEADER].send_network_diag_get(dut_rloc, [TlvType.MAC_COUNTERS])
        self.simulator.go(2)

        dut_messages = self.simulator.get_messages_sent_by(DUT)
        diag_get_rsp = dut_messages.next_coap_message(code='2.04')
        diag_get_rsp.assertCoapMessageContainsTlv(network_diag.MacCounters)
        reset_mac_counters = diag_get_rsp.get_coap_message_tlv(network_diag.MacCounters)

        self.assertEqual(len(mac_counters.counters), len(reset_mac_counters.counters))
        for old_counter, new_counter in zip(mac_counters.counters, reset_mac_counters.counters):
            self.assertTrue(new_counter == 0 or new_counter < old_counter)


if __name__ == '__main__':
    unittest.main()
