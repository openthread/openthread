#!/usr/bin/env python3
#
#  Copyright (c) 2016, The OpenThread Authors.
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

import command
import config
import ipv6
import mle
import thread_cert

DUT_LEADER = 1
BR = 2
MED1 = 3
MED2 = 4

MTDS = [MED1, MED2]


class Cert_5_3_8_ChildAddressSet(thread_cert.TestCase):
    TOPOLOGY = {
        DUT_LEADER: {
            'mode': 'rsdn',
            'panid': 0xface,
            'whitelist': [BR, MED1, MED2]
        },
        BR: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'whitelist': [DUT_LEADER]
        },
        MED1: {
            'is_mtd': True,
            'mode': 'rsn',
            'panid': 0xface,
            'whitelist': [DUT_LEADER]
        },
        MED2: {
            'is_mtd': True,
            'mode': 'rsn',
            'panid': 0xface,
            'whitelist': [DUT_LEADER]
        },
    }

    def test(self):
        self.nodes[DUT_LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[DUT_LEADER].get_state(), 'leader')

        self.nodes[BR].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[BR].get_state(), 'router')

        # 1 BR: Configure BR to be a DHCPv6 server
        self.nodes[BR].add_prefix('2001::/64', 'pdros')
        self.nodes[BR].add_prefix('2002::/64', 'pdros')
        self.nodes[BR].add_prefix('2003::/64', 'pdros')
        self.nodes[BR].register_netdata()

        # Set lowpan context of sniffer
        self.simulator.set_lowpan_context(1, '2001::/64')
        self.simulator.set_lowpan_context(2, '2002::/64')
        self.simulator.set_lowpan_context(3, '2003::/64')

        # 2 DUT_LEADER: Verify LEADER sent an Advertisement
        leader_messages = self.simulator.get_messages_sent_by(DUT_LEADER)
        msg = leader_messages.next_mle_message(mle.CommandType.ADVERTISEMENT)
        command.check_mle_advertisement(msg)

        # 3 MED1, MED2: MED1 and MED2 attach to DUT_LEADER
        for i in MTDS:
            self.nodes[i].start()
            self.simulator.go(5)
            self.assertEqual(self.nodes[i].get_state(), 'child')

        # 4 MED1: MED1 send an ICMPv6 Echo Request to the MED2 ML-EID
        med2_ml_eid = self.nodes[MED2].get_ip6_address(
            config.ADDRESS_TYPE.ML_EID)
        self.assertTrue(med2_ml_eid is not None)
        self.assertTrue(self.nodes[MED1].ping(med2_ml_eid))

        # Verify DUT_LEADER didn't generate an Address Query Request
        leader_messages = self.simulator.get_messages_sent_by(DUT_LEADER)
        msg = leader_messages.next_coap_message('0.02', '/a/aq', False)
        assert (
            msg is None
        ), "Error: The DUT_LEADER sent an unexpected Address Query Request"

        # Wait for sniffer got packets
        self.simulator.go(1)

        # Verify MED2 sent an ICMPv6 Echo Reply
        med2_messages = self.simulator.get_messages_sent_by(MED2)
        msg = med2_messages.get_icmp_message(ipv6.ICMP_ECHO_RESPONSE)
        assert (msg is not None
               ), "Error: The MED2 didn't send ICMPv6 Echo Reply to MED1"

        # 5 MED1: MED1 send an ICMPv6 Echo Request to the MED2 2001::GUA
        addr = self.nodes[MED2].get_addr("2001::/64")
        self.assertTrue(addr is not None)
        self.assertTrue(self.nodes[MED1].ping(addr))

        # Verify DUT_LEADER didn't generate an Address Query Request
        leader_messages = self.simulator.get_messages_sent_by(DUT_LEADER)
        msg = leader_messages.next_coap_message('0.02', '/a/aq', False)
        assert (
            msg is None
        ), "Error: The DUT_LEADER sent an unexpected Address Query Request"

        # Wait for sniffer got packets
        self.simulator.go(1)

        # Verify MED2 sent an ICMPv6 Echo Reply
        med2_messages = self.simulator.get_messages_sent_by(MED2)
        msg = med2_messages.get_icmp_message(ipv6.ICMP_ECHO_RESPONSE)
        assert (msg is not None
               ), "Error: The MED2 didn't send ICMPv6 Echo Reply to MED1"

        # 6 MED1: MED1 send an ICMPv6 Echo Request to the MED2 2002::GUA
        addr = self.nodes[MED2].get_addr("2002::/64")
        self.assertTrue(addr is not None)
        self.assertTrue(self.nodes[MED1].ping(addr))

        # Verify DUT_LEADER didn't generate an Address Query Request
        leader_messages = self.simulator.get_messages_sent_by(DUT_LEADER)
        msg = leader_messages.next_coap_message('0.02', '/a/aq', False)
        assert (
            msg is None
        ), "Error: The DUT_LEADER sent an unexpected Address Query Request"

        # Wait for sniffer got packets
        self.simulator.go(1)

        # Verify MED2 sent an ICMPv6 Echo Reply
        med2_messages = self.simulator.get_messages_sent_by(MED2)
        msg = med2_messages.get_icmp_message(ipv6.ICMP_ECHO_RESPONSE)
        assert (msg is not None
               ), "Error: The MED2 didn't send ICMPv6 Echo Reply to MED1"

        # 7 MED1: MED1 send an ICMPv6 Echo Request to the MED2 2003::GUA
        addr = self.nodes[MED2].get_addr("2003::/64")
        self.assertTrue(addr is not None)
        self.assertTrue(self.nodes[MED1].ping(addr))

        # Verify DUT_LEADER didn't generate an Address Query Request
        leader_messages = self.simulator.get_messages_sent_by(DUT_LEADER)
        msg = leader_messages.next_coap_message('0.02', '/a/aq', False)
        assert (
            msg is None
        ), "Error: The DUT_LEADER sent an unexpected Address Query Request"

        # Wait for sniffer got packets
        self.simulator.go(1)

        # Verify MED2 sent an ICMPv6 Echo Reply
        med2_messages = self.simulator.get_messages_sent_by(MED2)
        msg = med2_messages.get_icmp_message(ipv6.ICMP_ECHO_RESPONSE)
        assert (msg is not None
               ), "Error: The MED2 didn't send ICMPv6 Echo Reply to MED1"


if __name__ == '__main__':
    unittest.main()
