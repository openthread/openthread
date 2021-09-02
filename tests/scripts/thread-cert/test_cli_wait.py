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

import unittest

import thread_cert

# Test description:
#
# The purpose of this test is to verify the functionality of CLI wait command.
#
# Topology:
#
#  ROUTER_1 ----- ROUTER_2
#

ROUTER_1 = 1
ROUTER_2 = 2

PORT = 12345


class TestPing(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False
    SUPPORT_NCP = False

    TOPOLOGY = {
        ROUTER_1: {
            'name': 'Router_1',
        },
        ROUTER_2: {
            'name': 'Router_2',
        },
    }

    def test(self):
        router1 = self.nodes[ROUTER_1]
        router2 = self.nodes[ROUTER_2]

        router1.start()
        self.simulator.go(5)
        self.assertEqual('leader', router1.get_state())

        router2.start()
        self.simulator.go(5)
        self.assertEqual('router', router2.get_state())

        router1.udp_start("::", PORT)
        router2.udp_start("::", PORT)

        router1_rloc = router1.get_rloc()

        # Verify that `wait` command can be used to get received UDP messages.
        router1.udp_send(ipaddr=router2.get_rloc(), port=PORT, bytes=b'AAAA')
        self.assertEqual(router2.wait(5000), [f'4 bytes from {router1_rloc} 12345 AAAA'])
        router1.udp_send(ipaddr=router2.get_rloc(), port=PORT, bytes=b'BBBB')
        self.assertEqual(router2.wait(5000), [f'4 bytes from {router1_rloc} 12345 BBBB'])

        router1.udp_send(ipaddr=router2.get_rloc(), port=PORT, bytes=b'CCCC')
        router1.udp_send(ipaddr=router2.get_rloc(), port=PORT, bytes=b'DDDD')
        self.assertEqual(router2.wait(5000), [
            f'4 bytes from {router1_rloc} 12345 CCCC',
            f'4 bytes from {router1_rloc} 12345 DDDD',
        ])


if __name__ == '__main__':
    unittest.main()
