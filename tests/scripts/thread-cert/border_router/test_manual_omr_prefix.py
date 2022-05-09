#!/usr/bin/env python3
#
#  Copyright (c) 2022, The OpenThread Authors.
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

import unittest
import thread_cert

# Test description:
# The purpose of this test case is to verify that BR can handle manually added OMR prefixes properly.
#
# Topology:
#     BR_1 (Leader)
#       |
#       |
#     BR_2
#

BR_1 = 1
BR_2 = 2


class ManualOmrsPrefix(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR_1: {
            'name': 'BR_1',
            'is_otbr': True,
            'version': '1.2',
        },
        BR_2: {
            'name': 'BR_2',
            'is_otbr': True,
            'version': '1.2',
        },
    }

    def test(self):
        br1 = self.nodes[BR_1]
        br2 = self.nodes[BR_2]

        br1.start()
        self.simulator.go(10)
        self.assertEqual('leader', br1.get_state())

        self.simulator.go(10)
        self.assertEqual(br1.get_netdata_omr_prefixes(), [br1.get_br_omr_prefix()])

        br2.disable_br()
        br2.start()
        self.simulator.go(15)
        self.assertEqual('router', br2.get_state())
        self.assertEqual(br1.get_netdata_omr_prefixes(), [br1.get_br_omr_prefix()])

        # Add a smaller OMR prefix. Verify BR_1 withdraws its OMR prefix.
        self._test_manual_omr_prefix('2001::/64', 'paros', expect_withdraw=True)
        # Add a smaller but invalid OMR prefix (P_on_mesh = 0). Verify BR_1 does not withdraw its OMR prefix.
        self._test_manual_omr_prefix('2002::/64', 'pars', expect_withdraw=False)
        # Add a smaller but invalid OMR prefix (P_stable = 0). Verify BR_1 does not withdraw its OMR prefix.
        self._test_manual_omr_prefix('2003::/64', 'paro', expect_withdraw=False)

    def _test_manual_omr_prefix(self, prefix, flags, expect_withdraw):
        br1 = self.nodes[BR_1]
        br2 = self.nodes[BR_2]

        br2.add_prefix(prefix, flags)
        br2.register_netdata()
        self.simulator.go(10)

        if expect_withdraw:
            self.assertEqual(br1.get_netdata_omr_prefixes(), [prefix])
        else:
            self.assertEqual(br1.get_netdata_omr_prefixes(), [br1.get_br_omr_prefix()])

        br2.remove_prefix(prefix)
        br2.register_netdata()
        self.simulator.go(10)
        self.assertEqual(br1.get_netdata_omr_prefixes(), [br1.get_br_omr_prefix()])
        self.simulator.go(3)


if __name__ == '__main__':
    unittest.main()
