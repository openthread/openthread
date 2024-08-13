#!/usr/bin/env python3
#
#  Copyright (c) 2024, The OpenThread Authors.
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
import logging
import unittest

import config
import thread_cert

# Test description:
#   This test verifies that ephemeral key can be activated and deactivated.
#
# Topology:
#    --------------
#           |
#          BR1
#

BR1 = 1


class EphemeralKeyTest(thread_cert.TestCase):
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
    }

    def test(self):
        br1 = self.nodes[BR1]

        counters = br1.get_border_agent_counters()
        print('br1 ba counters', counters)
        self.assertTrue(counters['ePSKc']['activations'] == 0)
        self.assertTrue(counters['ePSKc']['api_deactivations'] == 0)
        self.assertTrue(counters['ePSKc']['timeout_deactivations'] == 0)
        self.assertTrue(counters['ePSKc']['max_attempt_deactivations'] == 0)
        self.assertTrue(counters['ePSKc']['disconnect_deactivations'] == 0)
        self.assertTrue(counters['ePSKc']['invalid_ba_state_errors'] == 0)
        self.assertTrue(counters['ePSKc']['invalid_args_errors'] == 0)
        self.assertTrue(counters['ePSKc']['start_secure_session_errors'] == 0)
        self.assertTrue(counters['ePSKc']['secure_session_successes'] == 0)
        self.assertTrue(counters['ePSKc']['secure_session_failures'] == 0)
        self.assertTrue(counters['ePSKc']['commissioner_petitions'] == 0)
        self.assertTrue(counters['PSKc']['secure_session_successes'] == 0)
        self.assertTrue(counters['PSKc']['secure_session_failures'] == 0)
        self.assertTrue(counters['PSKc']['commissioner_petitions'] == 0)
        self.assertTrue(counters['coap']['MGMT_ACTIVE_GET'] == 0)
        self.assertTrue(counters['coap']['MGMT_PENDING_GET'] == 0)

        # activate epskc before border agent is up
        br1.set_epskc('123456789', 10)

        counters = br1.get_border_agent_counters()
        print('br1 ba counters', counters)
        self.assertTrue(counters['ePSKc']['activations'] == 0)
        self.assertTrue(counters['ePSKc']['invalid_ba_state_errors'] == 1)

        br1.set_active_dataset(updateExisting=True, network_name='ot-br1')
        br1.start()
        self.simulator.go(config.BORDER_ROUTER_STARTUP_DELAY)
        self.assertEqual('leader', br1.get_state())

        # activate epskc and let it timeout
        br1.set_epskc('123456789', 10)
        self.simulator.go(1)

        counters = br1.get_border_agent_counters()
        print('br1 ba counters', counters)
        self.assertTrue(counters['ePSKc']['activations'] == 1)
        self.assertTrue(counters['ePSKc']['api_deactivations'] == 0)
        self.assertTrue(counters['ePSKc']['timeout_deactivations'] == 1)

        # activate epskc and clear it
        br1.set_epskc('123456789', 10000)
        self.simulator.go(1)
        br1.clear_epskc()

        counters = br1.get_border_agent_counters()
        print('br1 ba counters', counters)
        self.assertTrue(counters['ePSKc']['activations'] == 2)
        self.assertTrue(counters['ePSKc']['api_deactivations'] == 1)
        self.assertTrue(counters['ePSKc']['timeout_deactivations'] == 1)

        # set epskc with invalid passcode
        br1.set_epskc('123', 1000)
        self.simulator.go(1)

        counters = br1.get_border_agent_counters()
        print('br1 ba counters', counters)
        self.assertTrue(counters['ePSKc']['activations'] == 2)
        self.assertTrue(counters['ePSKc']['api_deactivations'] == 1)
        self.assertTrue(counters['ePSKc']['timeout_deactivations'] == 1)
        self.assertTrue(counters['ePSKc']['invalid_args_errors'] == 1)


if __name__ == '__main__':
    unittest.main()
