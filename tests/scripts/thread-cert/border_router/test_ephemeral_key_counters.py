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
#   This test verifies the counters of ephemeral key activations/deactivations.
#
# Topology:
#    --------------
#           |
#          BR1
#

BR1 = 1


class EphemeralKeyCountersTest(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR1: {
            'name': 'BR_1',
            'is_otbr': True,
            'version': '1.4',
            'network_name': 'ot-br1',
        },
    }

    def test(self):
        br1 = self.nodes[BR1]

        counters = br1.get_border_agent_counters()
        self.assertEqual(counters['epskcActivation'], 0)
        self.assertEqual(counters['epskcApiDeactivation'], 0)
        self.assertEqual(counters['epskcTimeoutDeactivation'], 0)
        self.assertEqual(counters['epskcMaxAttemptDeactivation'], 0)
        self.assertEqual(counters['epskcDisconnectDeactivation'], 0)
        self.assertEqual(counters['epskcInvalidBaStateError'], 0)
        self.assertEqual(counters['epskcInvalidArgsError'], 0)
        self.assertEqual(counters['epskcStartSecureSessionError'], 0)
        self.assertEqual(counters['epskcSecureSessionSuccess'], 0)
        self.assertEqual(counters['epskcSecureSessionFailure'], 0)
        self.assertEqual(counters['epskcCommissionerPetition'], 0)
        self.assertEqual(counters['pskcSecureSessionSuccess'], 0)
        self.assertEqual(counters['pskcSecureSessionFailure'], 0)
        self.assertEqual(counters['pskcCommissionerPetition'], 0)
        self.assertEqual(counters['mgmtActiveGet'], 0)
        self.assertEqual(counters['mgmtPendingGet'], 0)

        # activate epskc before border agent is up returns an error
        br1.set_epskc('123456789')

        counters = br1.get_border_agent_counters()
        self.assertEqual(counters['epskcActivation'], 0)
        self.assertEqual(counters['epskcInvalidBaStateError'], 1)

        br1.set_active_dataset(updateExisting=True, network_name='ot-br1')
        br1.start()
        self.simulator.go(config.BORDER_ROUTER_STARTUP_DELAY)
        self.assertEqual('leader', br1.get_state())

        # activate epskc and let it timeout
        br1.set_epskc('123456789', 10)
        self.simulator.go(1)

        counters = br1.get_border_agent_counters()
        self.assertEqual(counters['epskcActivation'], 1)
        self.assertEqual(counters['epskcApiDeactivation'], 0)
        self.assertEqual(counters['epskcTimeoutDeactivation'], 1)

        # activate epskc and clear it
        br1.set_epskc('123456789', 10000)
        self.simulator.go(1)
        br1.clear_epskc()

        counters = br1.get_border_agent_counters()
        self.assertEqual(counters['epskcActivation'], 2)
        self.assertEqual(counters['epskcApiDeactivation'], 1)
        self.assertEqual(counters['epskcTimeoutDeactivation'], 1)

        # set epskc with invalid passcode
        br1.set_epskc('123')
        self.simulator.go(1)

        counters = br1.get_border_agent_counters()
        self.assertEqual(counters['epskcActivation'], 2)
        self.assertEqual(counters['epskcApiDeactivation'], 1)
        self.assertEqual(counters['epskcTimeoutDeactivation'], 1)
        self.assertEqual(counters['epskcInvalidArgsError'], 1)


if __name__ == '__main__':
    unittest.main()
