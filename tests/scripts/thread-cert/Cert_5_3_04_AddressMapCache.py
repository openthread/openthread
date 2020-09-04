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
import thread_cert

LEADER = 1
DUT_ROUTER1 = 2
SED1 = 3
ED1 = 4
ED2 = 5
ED3 = 6
ED4 = 7

MTDS = [SED1, ED1, ED2, ED3, ED4]


class Cert_5_3_4_AddressMapCache(thread_cert.TestCase):
    TOPOLOGY = {
        LEADER: {
            'mode': 'rsdn',
            'panid': 0xface,
            'allowlist': [DUT_ROUTER1, ED1, ED2, ED3, ED4]
        },
        DUT_ROUTER1: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_selection_jitter': 1,
            'allowlist': [LEADER, SED1]
        },
        SED1: {
            'is_mtd': True,
            'mode': 's',
            'panid': 0xface,
            'timeout': 5,
            'allowlist': [DUT_ROUTER1]
        },
        ED1: {
            'is_mtd': True,
            'mode': 'rsn',
            'panid': 0xface,
            'allowlist': [LEADER]
        },
        ED2: {
            'is_mtd': True,
            'mode': 'rsn',
            'panid': 0xface,
            'allowlist': [LEADER]
        },
        ED3: {
            'is_mtd': True,
            'mode': 'rsn',
            'panid': 0xface,
            'allowlist': [LEADER]
        },
        ED4: {
            'is_mtd': True,
            'mode': 'rsn',
            'panid': 0xface,
            'allowlist': [LEADER]
        },
    }

    def test(self):
        # 1
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[DUT_ROUTER1].start()
        for i in MTDS:
            self.nodes[i].start()
        self.simulator.go(5)

        self.assertEqual(self.nodes[DUT_ROUTER1].get_state(), 'router')

        for i in MTDS:
            self.assertEqual(self.nodes[i].get_state(), 'child')

        # This method flushes the message queue so calling this method again
        # will return only the newly logged messages.
        dut_messages = self.simulator.get_messages_sent_by(DUT_ROUTER1)

        # 2
        for ED in [ED1, ED2, ED3, ED4]:
            ed_mleid = self.nodes[ED].get_ip6_address(config.ADDRESS_TYPE.ML_EID)
            self.assertTrue(self.nodes[SED1].ping(ed_mleid))
            self.simulator.go(5)

            # Verify DUT_ROUTER1 generated an Address Query Request to find
            # each node's RLOC.
            dut_messages = self.simulator.get_messages_sent_by(DUT_ROUTER1)
            msg = dut_messages.next_coap_message('0.02', '/a/aq')
            command.check_address_query(
                msg,
                self.nodes[DUT_ROUTER1],
                config.REALM_LOCAL_ALL_ROUTERS_ADDRESS,
            )

        # 3 & 4
        # This method flushes the message queue so calling this method again
        # will return only the newly logged messages.
        dut_messages = self.simulator.get_messages_sent_by(DUT_ROUTER1)

        for ED in [ED1, ED2, ED3, ED4]:
            ed_mleid = self.nodes[ED].get_ip6_address(config.ADDRESS_TYPE.ML_EID)
            self.assertTrue(self.nodes[SED1].ping(ed_mleid))
            self.simulator.go(5)

            # Verify DUT_ROUTER1 didn't generate an Address Query Request.
            dut_messages = self.simulator.get_messages_sent_by(DUT_ROUTER1)
            msg = dut_messages.next_coap_message('0.02', '/a/aq', False)
            assert (msg is None), "Error: The DUT sent an unexpected Address Query Request"


if __name__ == '__main__':
    unittest.main()
