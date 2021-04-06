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
# The purpose of this test case is to verify that a Thread device is able to
# register a multicast address with a Primary BBR and that the Primary BBR can
# notify devices on the backbone link of the multicast address. The test also
# verifies that an IPv6 multicast packet originating from the backbone is
# correctly forwarded (in the Thread Network) to the device that registered the
# multicast address.
#
# Topology:
#    ----------------(eth)------------------
#           |                  |     |
#          BR1 (Leader) ----- BR2   HOST
#           |                  |
#           |                  |
#          TD --------(when TD is router)
#

BR1 = 1
BR2 = 2
TD = 3
HOST = 4

MA1 = "ff04::1234:777a:1"

CHANNEL1 = 18


class MLRFirstUse(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR1: {
            'name': 'BR_1',
            'is_otbr': True,
            'version': '1.2',
            'channel': CHANNEL1,
            'router_selection_jitter': 2,
        },
        BR2: {
            'name': 'BR_2',
            'is_otbr': True,
            'version': '1.2',
            'channel': CHANNEL1,
            'router_selection_jitter': 2,
        },
        TD: {
            'name': 'Router_2',
            'version': '1.2',
            'channel': CHANNEL1,
            'router_selection_jitter': 2,
        },
        HOST: {
            'name': 'Host',
            'is_host': True
        },
    }

    def test(self):
        br1 = self.nodes[BR1]
        br2 = self.nodes[BR2]
        td = self.nodes[TD]
        host = self.nodes[HOST]

        br1.start()
        self.simulator.go(5)
        td.start()
        self.simulator.go(5)
        br2.start()

        self.simulator.go(10)

        #1 Registers for multicast address, MA1, at BR_1.
        td.add_ipmaddr(MA1)

        self.simulator.go(50)

        host.ping(MA1)





if __name__ == '__main__':
    unittest.main()
