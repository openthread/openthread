#!/usr/bin/env python3
#
#  Copyright (c) 2018, The OpenThread Authors.
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

import thread_cert
from command import check_parent_request
from command import check_child_id_request
from command import check_child_update_request_from_child
from command import CheckType
import config
import mle

LEADER = 1
REED = 2
MED = 3


class Cert_6_1_2_REEDAttach_MED(thread_cert.TestCase):
    TOPOLOGY = {
        LEADER: {
            'mode': 'rsdn',
            'panid': 0xface,
            'whitelist': [REED]
        },
        REED: {
            'mode': 'rsdn',
            'panid': 0xface,
            'router_upgrade_threshold': 0,
            'whitelist': [LEADER, MED]
        },
        MED: {
            'is_mtd': True,
            'mode': 'rsn',
            'panid': 0xface,
            'timeout': config.DEFAULT_CHILD_TIMEOUT,
            'whitelist': [REED]
        },
    }

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[REED].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[REED].get_state(), 'child')

        self.nodes[MED].start()

        self.simulator.go(5)
        self.assertEqual(self.nodes[MED].get_state(), 'child')
        self.assertEqual(self.nodes[REED].get_state(), 'router')
        med_messages = self.simulator.get_messages_sent_by(MED)

        # Step 2 - DUT sends MLE Parent Request
        msg = med_messages.next_mle_message(mle.CommandType.PARENT_REQUEST)
        check_parent_request(msg, is_first_request=True)

        # Step 4 - DUT sends MLE Parent Request again
        msg = med_messages.next_mle_message(mle.CommandType.PARENT_REQUEST)
        check_parent_request(msg, is_first_request=False)

        # Step 6 - DUT sends Child ID Request
        msg = med_messages.next_mle_message(mle.CommandType.CHILD_ID_REQUEST, sent_to_node=self.nodes[REED])
        check_child_id_request(
            msg,
            address_registration=CheckType.CONTAIN,
            tlv_request=CheckType.CONTAIN,
            mle_frame_counter=CheckType.OPTIONAL,
            route64=CheckType.OPTIONAL,
        )

        # Wait additional DEFAULT_CHILD_TIMEOUT to ensure the keep-alive
        # message (child update request from MED) happens.
        self.simulator.go(config.DEFAULT_CHILD_TIMEOUT)
        med_messages = self.simulator.get_messages_sent_by(MED)

        # Step 8 - DUT sends Child Update messages
        msg = med_messages.next_mle_message(mle.CommandType.CHILD_UPDATE_REQUEST)
        check_child_update_request_from_child(
            msg,
            source_address=CheckType.CONTAIN,
            leader_data=CheckType.CONTAIN,
        )

        # Step 10 - Leader sends ICMPv6 echo request, to DUT link local address
        med_addrs = self.nodes[MED].get_addrs()
        for addr in med_addrs:
            if addr[0:4] == 'fe80':
                self.assertTrue(self.nodes[REED].ping(addr))


if __name__ == '__main__':
    unittest.main()
