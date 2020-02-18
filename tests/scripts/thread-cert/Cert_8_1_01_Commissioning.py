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
import dtls
import mle
import node

from command import CheckType

COMMISSIONER = 1
JOINER = 2


class Cert_8_1_01_Commissioning(unittest.TestCase):

    def setUp(self):
        self.simulator = config.create_default_simulator()

        self.nodes = {}
        for i in range(1, 3):
            self.nodes[i] = node.Node(i, simulator=self.simulator)

        self.nodes[COMMISSIONER].set_panid(0xface)
        self.nodes[COMMISSIONER].set_mode('rsdn')
        self.nodes[COMMISSIONER].set_masterkey(
            '00112233445566778899aabbccddeeff')

        self.nodes[JOINER].set_mode('rsdn')
        self.nodes[JOINER].set_masterkey('deadbeefdeadbeefdeadbeefdeadbeef')
        self.nodes[JOINER].set_router_selection_jitter(1)

    def tearDown(self):
        for n in list(self.nodes.values()):
            n.stop()
            n.destroy()
        self.simulator.stop()

    def test(self):
        self.nodes[COMMISSIONER].interface_up()
        self.nodes[COMMISSIONER].thread_start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[COMMISSIONER].get_state(), 'leader')
        self.nodes[COMMISSIONER].commissioner_start()
        self.simulator.go(3)
        self.nodes[COMMISSIONER].commissioner_add_joiner(
            self.nodes[JOINER].get_eui64(), 'OPENTHREAD')

        self.nodes[JOINER].interface_up()
        self.nodes[JOINER].joiner_start('OPENTHREAD')
        self.simulator.go(10)
        self.simulator.read_cert_messages_in_commissioning_log(
            [COMMISSIONER, JOINER])
        self.assertEqual(
            self.nodes[JOINER].get_masterkey(),
            self.nodes[COMMISSIONER].get_masterkey(),
        )

        joiner_messages = self.simulator.get_messages_sent_by(JOINER)
        commissioner_messages = self.simulator.get_messages_sent_by(
            COMMISSIONER)

        # 2 - N/A

        # 3 - Joiner_1
        msg = joiner_messages.next_mle_message(
            mle.CommandType.DISCOVERY_REQUEST)
        command.check_discovery_request(msg)
        request_src_addr = msg.mac_header.src_address

        # 4 - Commissioner
        msg = commissioner_messages.next_mle_message(
            mle.CommandType.DISCOVERY_RESPONSE)
        command.check_discovery_response(msg,
                                         request_src_addr,
                                         steering_data=CheckType.CONTAIN)
        udp_port_set_by_commissioner = command.get_joiner_udp_port_in_discovery_response(
            msg)

        # 5.2 - Joiner_1
        msg = joiner_messages.next_dtls_message(dtls.ContentType.HANDSHAKE,
                                                dtls.HandshakeType.CLIENT_HELLO)
        self.assertEqual(msg.get_dst_udp_port(), udp_port_set_by_commissioner)

        # 5.3 - Commissioner
        msg = commissioner_messages.next_dtls_message(
            dtls.ContentType.HANDSHAKE, dtls.HandshakeType.HELLO_VERIFY_REQUEST)
        commissioner_cookie = msg.dtls.body.cookie

        # 5.4 - Joiner_1
        msg = joiner_messages.next_dtls_message(dtls.ContentType.HANDSHAKE,
                                                dtls.HandshakeType.CLIENT_HELLO)
        self.assertEqual(commissioner_cookie, msg.dtls.body.cookie)
        self.assertEqual(msg.get_dst_udp_port(), udp_port_set_by_commissioner)

        # 5.5 - Commissioner
        commissioner_messages.next_dtls_message(dtls.ContentType.HANDSHAKE,
                                                dtls.HandshakeType.SERVER_HELLO)
        commissioner_messages.next_dtls_message(
            dtls.ContentType.HANDSHAKE, dtls.HandshakeType.SERVER_KEY_EXCHANGE)
        commissioner_messages.next_dtls_message(
            dtls.ContentType.HANDSHAKE, dtls.HandshakeType.SERVER_HELLO_DONE)

        # 5.6 - Joiner_1
        msg = joiner_messages.next_dtls_message(
            dtls.ContentType.HANDSHAKE, dtls.HandshakeType.CLIENT_KEY_EXCHANGE)
        self.assertEqual(msg.get_dst_udp_port(), udp_port_set_by_commissioner)
        msg = joiner_messages.next_dtls_message(
            dtls.ContentType.CHANGE_CIPHER_SPEC)
        self.assertEqual(msg.get_dst_udp_port(), udp_port_set_by_commissioner)

        # TODO(wgtdkp): It's required to verify DTLS FINISHED message here.
        # Currently not handled as it is encrypted.

        # 5.7 - Commissioner
        commissioner_messages.next_dtls_message(
            dtls.ContentType.CHANGE_CIPHER_SPEC)

        # TODO(wgtdkp): It's required to verify DTLS FINISHED message here.
        # Currently not handled as it is encrypted.

        # 5.8,9,10,11
        # - Joiner_1
        command.check_joiner_commissioning_messages(
            joiner_messages.commissioning_messages)
        # - Commissioner
        command.check_commissioner_commissioning_messages(
            commissioner_messages.commissioning_messages)
        # As commissioner is also joiner router
        command.check_joiner_router_commissioning_messages(
            commissioner_messages.commissioning_messages)

        self.nodes[JOINER].thread_start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[JOINER].get_state(), 'router')


if __name__ == '__main__':
    unittest.main()
