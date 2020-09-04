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

LEADER = 1
DUT_ROUTER1 = 2
DUT_REED = 17

MLE_MIN_LINKS = 3


class Cert_5_2_7_REEDSynchronization(thread_cert.TestCase):
    TOPOLOGY = {
        LEADER: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1
        },
        DUT_ROUTER1: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1
        },
        3: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1
        },
        4: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1
        },
        5: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1
        },
        6: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1
        },
        7: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1
        },
        8: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1
        },
        9: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1
        },
        10: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1
        },
        11: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1
        },
        12: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1
        },
        13: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1
        },
        14: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1
        },
        15: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1
        },
        16: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1
        },
        DUT_REED: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1
        },
    }

    def test(self):
        # 1. Ensure topology is formed correctly without DUT_ROUTER1.
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        for i in range(2, 17):
            self.nodes[i].start()
        self.simulator.go(10)

        for i in range(2, 17):
            self.assertEqual(self.nodes[i].get_state(), 'router')

        # 2. DUT_REED: Attach to network. Verify it didn't send an Address Solicit Request.
        # Avoid DUT_REED attach to DUT_ROUTER1.
        self.nodes[DUT_REED].add_allowlist(self.nodes[DUT_ROUTER1].get_addr64(), config.RSSI['LINK_QULITY_1'])

        self.nodes[DUT_REED].start()
        self.simulator.go(config.MAX_ADVERTISEMENT_INTERVAL)
        self.assertEqual(self.nodes[DUT_REED].get_state(), 'child')

        # The DUT_REED must not send a coap message here.
        reed_messages = self.simulator.get_messages_sent_by(DUT_REED)
        msg = reed_messages.does_not_contain_coap_message()
        assert (msg is True), "Error: The DUT_REED sent an Address Solicit Request"

        # 3. DUT_REED: Verify sent a Link Request to at least 3 neighboring
        # Routers.
        for i in range(0, MLE_MIN_LINKS):
            msg = reed_messages.next_mle_message(mle.CommandType.LINK_REQUEST)
            command.check_link_request(
                msg,
                source_address=command.CheckType.CONTAIN,
                leader_data=command.CheckType.CONTAIN,
            )

        # 4. DUT_REED: Verify at least 3 Link Accept messages sent to DUT_REED.
        self.simulator.go(config.MAX_ADVERTISEMENT_INTERVAL)

        link_accept_count = 0
        destination_link_local = self.nodes[DUT_REED].get_ip6_address(config.ADDRESS_TYPE.LINK_LOCAL)

        for i in range(1, DUT_REED):
            dut_messages = self.simulator.get_messages_sent_by(i)

            while True:
                msg = dut_messages.next_mle_message(mle.CommandType.LINK_ACCEPT, False)
                if msg is None:
                    break
                if (ipv6.ip_address(destination_link_local) == msg.ipv6_packet.ipv6_header.destination_address):
                    command.check_link_accept(msg, self.nodes[DUT_REED])
                    link_accept_count += 1
                    break

        assert (link_accept_count >= MLE_MIN_LINKS) is True, "Error: too few Link Accept sent to DUT_REED"


if __name__ == '__main__':
    unittest.main()
