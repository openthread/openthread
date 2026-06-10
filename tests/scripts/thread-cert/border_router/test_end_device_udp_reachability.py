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
import ipaddress
import json
import logging
import unittest

import config
import pktverify.verify
import thread_cert

# Test description:
#   This test verifies Thread end-devices have UDP reachability to the infrastructure link via a Thread BR.
#
# Topology:
#    ----------------(eth)--------------------
#           |                      |
#          BR1                    HOST
#         /
#       FED1
from pktverify.packet_verifier import PacketVerifier

BR1 = 1
FED1 = 2
HOST = 3

PORT = 11111
UDP_PAYLOAD_LEN = 17


class TestEndDeviceUdpReachability(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR1: {
            'name': 'BR1',
            'is_otbr': True,
            'version': '1.2',
        },
        FED1: {
            'name': 'FED1',
            'router_eligible': False,
        },
        HOST: {
            'name': 'HOST',
            'is_host': True
        },
    }

    def test(self):
        br1 = self.nodes[BR1]
        fed1 = self.nodes[FED1]
        host = self.nodes[HOST]

        host.start(start_radvd=False)
        self.simulator.go(5)

        br1.start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual('leader', br1.get_state())
        br1.udp_start("::", PORT, bind_unspecified=True)

        fed1.start()
        self.simulator.go(5)
        self.assertEqual('child', fed1.get_state())

        self.simulator.go(20)

        fed1_omr_addr = fed1.get_ip6_address(config.ADDRESS_TYPE.OMR)[0]
        host_addr = host.get_ip6_address(config.ADDRESS_TYPE.ONLINK_ULA)[0]
        print("Host address:", host_addr)
        print('FED address:', fed1_omr_addr)

        self.assertTrue(fed1.ping(host_addr))
        self.assertTrue(host.ping(fed1_omr_addr, backbone=True))

        fed1.udp_start("::", PORT)

        fed1.udp_send(UDP_PAYLOAD_LEN, host_addr, PORT)
        self.simulator.go(5)

        host.udp_send_host(data='A' * UDP_PAYLOAD_LEN, ipaddr=fed1_omr_addr, port=PORT)
        self.simulator.go(5)

        self.collect_ipaddrs()
        self.collect_extra_vars(FED1_OMR=fed1_omr_addr, HOST_ONLINK_ULA=host_addr)

    def verify(self, pv: PacketVerifier):
        pkts = pv.pkts
        pv.summary.show()

        BR1 = pv.vars['BR1']
        FED1 = pv.vars['FED1']
        BR1_ETH = pv.vars['BR1_ETH']
        HOST_ETH = pv.vars['HOST_ETH']

        udp_pkts = pkts.filter("""
            udp.srcport == {PORT} and 
            udp.dstport == {PORT} and 
            udp.length == {UDP_LENGTH}
        """,
                               PORT=PORT,
                               UDP_LENGTH=UDP_PAYLOAD_LEN + 8)

        # FED1 should send a UDP message to BR1
        udp_pkts.filter_wpan_src64(FED1).must_next()

        # BR1 should forward the UDP message to the infrastructure link
        udp_pkts.filter_eth_src(BR1_ETH).must_next()

        udp_pkts = pkts.filter("""
            udp.dstport == {PORT} and 
            udp.length == {UDP_LENGTH}
        """,
                               PORT=PORT,
                               UDP_LENGTH=UDP_PAYLOAD_LEN + 8)

        # Host should send a UDP message to BR1
        udp_pkts.filter_eth_src(HOST_ETH).must_next()

        # BR1 should forward the UDP message to FED1
        udp_pkts.filter_wpan_src64(BR1).must_next()


if __name__ == '__main__':
    unittest.main()
