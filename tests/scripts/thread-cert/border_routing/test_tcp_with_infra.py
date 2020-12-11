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
#   This test verifies bi-directional TCP connectivity between Thread end device
#   and infra host.
#
# Topology:
#    ----------------(eth)--------------------
#           |                 |
#          BR1 (Leader)      HOST
#           |
#        ROUTER1
#

BR1 = 1
ROUTER1 = 2
HOST = 4


class SingleBorderRouter(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR1: {
            'name': 'BR_1',
            'allowlist': [ROUTER1],
            'is_otbr': True,
            'version': '1.2',
            'router_selection_jitter': 1,
        },
        ROUTER1: {
            'name': 'Router_1',
            'allowlist': [BR1],
            'version': '1.2',
            'router_selection_jitter': 1,
        },
        HOST: {
            'name': 'Host',
            'is_host': True
        },
    }

    def _bootstrap(self):
        self.nodes[HOST].start(start_radvd=False)
        self.simulator.go(5)

        self.nodes[BR1].start()
        self.simulator.go(5)
        self.assertEqual('leader', self.nodes[BR1].get_state())

        self.nodes[ROUTER1].start()
        self.simulator.go(5)
        self.assertEqual('router', self.nodes[ROUTER1].get_state())

        self.simulator.go(10)
        self.collect_ipaddrs()

        logging.info("BR1     addrs: %r", self.nodes[BR1].get_addrs())
        logging.info("ROUTER1 addrs: %r", self.nodes[ROUTER1].get_addrs())
        logging.info("HOST    addrs: %r", self.nodes[HOST].get_addrs())

        # Router1 can ping to/from the Host on infra link.
        self.assertTrue(self.nodes[ROUTER1].ping(self.nodes[HOST].get_ip6_address(config.ADDRESS_TYPE.ONLINK_ULA)[0]))
        self.assertTrue(self.nodes[HOST].ping(self.nodes[ROUTER1].get_ip6_address(config.ADDRESS_TYPE.OMR)[0],
                                              backbone=True))

    def test(self):
        self._bootstrap()
        self._test_tcp_ot_client_vs_infra_server()
        self._test_tcp_ot_server_vs_infra_client()

    def _test_tcp_ot_server_vs_infra_client(self):
        self.nodes[ROUTER1].tcp_init()
        server_addr = self.nodes[ROUTER1].get_ip6_address(config.ADDRESS_TYPE.OMR)[0]
        self.nodes[ROUTER1].tcp_bind('::', 12345)
        self.nodes[ROUTER1].tcp_listen()
        self.nodes[ROUTER1].tcp_echo()

        self.nodes[HOST].tcp_echo_start_client(server_addr, 10240)
        self.simulator.go(3)

        self.nodes[ROUTER1].tcp_close()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER1].tcp_state(), 'CLOSED')

    def _test_tcp_ot_client_vs_infra_server(self):
        self.nodes[HOST].tcp_echo_start_server()
        server_addr = self.nodes[HOST].get_ip6_address(config.ADDRESS_TYPE.ONLINK_ULA)[0]

        self.nodes[ROUTER1].tcp_init()
        self.nodes[ROUTER1].tcp_connect(server_addr, 12345)
        self.simulator.go(4)
        self.assertEqual(self.nodes[ROUTER1].tcp_state(), 'ESTABLISHED')

        senddata = self._random_data(1024)
        recvdata = b''
        leftdata = senddata

        while recvdata != senddata:
            n = self.nodes[ROUTER1].tcp_send(leftdata)
            leftdata = leftdata[n:]

            recvdata += self.nodes[ROUTER1].tcp_recv()
            self.simulator.go(1)

        self.assertEqual(self.nodes[ROUTER1].tcp_state(), 'ESTABLISHED')

        self.nodes[ROUTER1].tcp_close()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ROUTER1].tcp_state(), 'TIME-WAIT')

        self.nodes[ROUTER1].tcp_abort()
        self.assertEqual(self.nodes[ROUTER1].tcp_state(), 'CLOSED')

    def _random_data(self, datasize: int) -> bytes:
        return bytes([ord('0') + (i % 10) for i in range(datasize)])


if __name__ == '__main__':
    unittest.main()
