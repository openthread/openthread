#!/usr/bin/env python3
#
#  Copyright (c) 2025, The OpenThread Authors.
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

import config
import unittest

import thread_cert

# Test description:
#   This test verifies that a TCAT-enabled device can successfully respond to
#   a TMF request to enable TCAT (TCAT_ENABLE.req) from another node.
#
# Topology:
#     ROUTER_1 (TCAT agent)
#       |
#       |
#     ROUTER_2 (TMF client)
#

ROUTER_1 = 1
ROUTER_2 = 2


class TcatTmfEnableReq(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False
    SUPPORT_NCP = False

    TOPOLOGY = {
        ROUTER_1: {
            'name': 'ROUTER_1_TCAT',
            'network_key': '00112233445566778899aabbccddeeff',
            'mode': 'rdn',
        },
        ROUTER_2: {
            'name': 'ROUTER_2_CLIENT',
            'network_key': '00112233445566778899aabbccddeeff',
            'mode': 'rdn',
        },
    }

    def test(self):
        router1 = self.nodes[ROUTER_1]
        router2 = self.nodes[ROUTER_2]

        #
        # 0. Start the nodes and form a network.
        #
        router1.start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual(router1.get_state(), 'leader')

        router2.start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual(router2.get_state(), 'router')

        # Allow some time for the network to stabilize and routes to propagate.
        self.simulator.go(5)

        # Enable the TCAT agent on Router 1.
        router1.tcat('start')
        router1.tcat('standby')
        self.simulator.go(1)
        print("TCAT agent started and in standby on Router 1.")

        # Router 2 sends a TMF request to Router 1.
        rloc1 = router1.get_rloc()
        router2.udp_start_client()
        print(f"Router 2 sending TCAT_ENABLE.req to Router 1 at RLOC {rloc1}...")
        # CoAP header: Ver=1, Type=CON(0), Code=POST(0.02), MID=0x1234
        # URI-Option: 'c' (0x63)
        # URI-Option: 'te' (0x7465) (TCAT_ENABLE.req)
        # Payload Marker (0xff)
        # DelayTimer TLV: type=52/0x34, len=4, val=1024ms
        tmf_payload = bytes.fromhex('40021234b163027465ff340400000400')
        router2.udp_send(0, rloc1, 61631, data_bytes=tmf_payload)

        # Allow time for the request to be sent and the response to be received.
        self.simulator.go(5)

        # Read the UDP command result (comes later).
        tmf_rsp = router2.udp_rx()
        print(f'Received CoAP TMF response: {tmf_rsp}')

        # Verify OK response
        expected_state_tlv = bytes.fromhex('100101')  # Accept status 0x01
        tmf_rsp_state_tlv = tmf_rsp[-len(expected_state_tlv):]
        self.assertEqual(tmf_rsp_state_tlv, expected_state_tlv,
                         f"Expected State TLV {expected_state_tlv} but got {tmf_rsp_state_tlv}")

        # TODO: no way yet to verify actual tcat state on Router 1 via CLI.

        # Router 2 sends invalid TCAT_ENABLE.req request.
        print(f"Router 2 sending invalid TCAT_ENABLE.req to Router 1 at RLOC {rloc1}...")
        # CoAP header: Ver=1, Type=CON(0), Code=POST(0.02), MID=0x5678
        # URI-Path: 'c/te' (TCAT_ENABLE.req)
        # Payload Marker (0xff)
        # Incomplete payload: contains only a Duration TLV (type 23/0x17) with uint16 value 240 (0xf0),
        # but misses a DelayTimer TLV.
        tmf_payload = bytes.fromhex('40025678b163027465ff170200f0')
        router2.udp_send(0, rloc1, 61631, data_bytes=tmf_payload)

        # And remaining verification steps.
        self.simulator.go(5)
        tmf_rsp2 = router2.udp_rx()
        print(f'Received CoAP TMF response: {tmf_rsp2}')
        expected_state_tlv = bytes.fromhex('1001ff')  # Reject status 0xff
        tmf_rsp_state_tlv = tmf_rsp2[-len(expected_state_tlv):]
        self.assertEqual(tmf_rsp_state_tlv, expected_state_tlv,
                         f"Expected State TLV {expected_state_tlv} but got {tmf_rsp_state_tlv}")


if __name__ == '__main__':
    unittest.main()
