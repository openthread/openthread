#!/usr/bin/env python3
#
#  Copyright (c) 2026, The OpenThread Authors.
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

from device_manager import DeviceManager


class TestAck(unittest.TestCase):
    """Test whether the DUT reports the ACK frames of the transmitted frames correctly."""

    @classmethod
    def setUpClass(cls):
        cls.__device_manager = DeviceManager()
        cls.__dut = cls.__device_manager.dut
        cls.__ref = cls.__device_manager.ref

    def setUp(self):
        self.__dut.factory_reset()
        self.__ref.factory_reset()

    def test_imm_ack(self):
        """Test whether the DUT reports the Imm-Acks of the transmitted frames with the FCS.

        The length of an ACK frame reported in the transmit done response must include the 2-byte FCS. The host
        fails to parse an Imm-Ack without the FCS and reports the transmission as `NoAck`, unless it is built with
        `OPENTHREAD_SPINEL_CONFIG_RCP_IMM_ACK_FCS_COMPATIBILITY_WORKAROUND_ENABLE`.
        """
        packets = 10

        dataset = self.__device_manager.get_default_dataset()
        self.__ref.join(dataset)
        self.__ref.wait_for('state', 'leader')

        # Attach the DUT as an rx-on-when-idle child, which does not need to process the ACKs to attach.
        self.__dut.set_mode('rn')
        self.__dut.join(dataset)
        self.__dut.wait_for('state', 'child')

        self.__dut.reset_counter('mac')
        self.__dut.ping(self.__ref.get_ipaddr_mleid(), count=packets, interval=0.1)
        counters = self.__dut.get_counter('mac')

        self.__dut.leave()
        self.__ref.leave()

        # This is a behavioral check, as the host does not expose the length of the reported ACK frames. Each Echo
        # Request is sent in a unicast frame acknowledged by an Imm-Ack. `TxAcked` counts the ACKs reported by the RCP
        # before the MAC parses them, and the MAC counts a frame whose Imm-Ack fails to be parsed, like a lost frame,
        # in `TxDirectMaxRetryExpiry`. So every reported ACK is accepted unless the RCP reports Imm-Acks without the
        # FCS, and frame loss does not count in either.
        tx_acked = counters['TxAcked']
        ack_accepted = max(0, counters['TxAckRequested'] - counters['TxDirectMaxRetryExpiry'])
        result = 'OK' if tx_acked > 0 and ack_accepted == tx_acked else 'NotSupported'
        self.__device_manager.output_test_result_string('Imm-Ack', f'{result} ({ack_accepted}/{tx_acked} ACKs)')

    def tearDown(self):
        pass

    @classmethod
    def tearDownClass(cls):
        cls.__dut.close()
        cls.__ref.close()


if __name__ == '__main__':
    unittest.main()
