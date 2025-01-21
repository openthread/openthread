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

import unittest

from otci import OTCI
from device_manager import DeviceManager


class TestCsl(unittest.TestCase):
    """Test whether the DUT supports the CSL feature."""

    @classmethod
    def setUpClass(cls):
        cls.__device_manager = DeviceManager()
        cls.__dut = cls.__device_manager.dut
        cls.__ref = cls.__device_manager.ref

    def setUp(self):
        self.__dut.factory_reset()
        self.__ref.factory_reset()

    def test_csl_transmitter(self):
        """Test whether the DUT supports the CSL transmitter."""
        packets = 10
        dataset = self.__device_manager.get_default_dataset()

        self.__dut.join(dataset)
        self.__dut.wait_for('state', 'leader')

        # Set the reference device as an SSED
        self.__ref.set_mode('-')
        self.__ref.config_csl(channel=15, period=320000, timeout=100)
        self.__ref.join(dataset)
        self.__ref.wait_for('state', 'child')

        child_table = self.__dut.get_child_table()
        ret = len(child_table) == 1 and child_table[1]['csl']

        if ret:
            ref_mleid = self.__ref.get_ipaddr_mleid()
            result = self.__dut.ping(ref_mleid, count=packets, interval=1)
            ret = result['transmitted_packets'] == result['received_packets'] == packets

        self.__dut.leave()
        self.__ref.leave()

        self.__device_manager.output_test_result_bool('CSL Transmitter', ret)

    def tearDown(self):
        pass

    @classmethod
    def tearDownClass(cls):
        cls.__dut.close()
        cls.__ref.close()


if __name__ == '__main__':
    unittest.main()
