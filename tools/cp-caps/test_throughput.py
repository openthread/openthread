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

import threading
import unittest

from otci import OTCI
from device_manager import DeviceManager


class TestThroughput(unittest.TestCase):
    """Test Thread network 1 hop throughput."""

    @classmethod
    def setUpClass(cls):
        cls.__device_manager = DeviceManager()
        cls.__dut = cls.__device_manager.dut
        cls.__ref = cls.__device_manager.ref

    def setUp(self):
        self.__dut.factory_reset()
        self.__ref.factory_reset()

    def test_throughput(self):
        """Test Thread network 1 hop throughput."""
        if not self.__dut.support_iperf3():
            print("The DUT doesn't support the tool iperf3")
            return

        if not self.__ref.support_iperf3():
            print("The reference device doesn't support the tool iperf3")
            return

        bitrate = 90000
        length = 1232
        transmit_time = 30
        max_wait_time = 30
        timeout = transmit_time + max_wait_time

        dataset = self.__device_manager.get_default_dataset()

        self.__dut.join(dataset)
        self.__dut.wait_for('state', 'leader')

        self.__ref.set_router_selection_jitter(1)
        self.__ref.join(dataset)
        self.__ref.wait_for('state', ['child', 'router'])

        ref_mleid = self.__ref.get_ipaddr_mleid()

        ref_iperf3_server = threading.Thread(target=self.__ref_iperf3_server_task,
                                             args=(ref_mleid, timeout),
                                             daemon=True)
        ref_iperf3_server.start()
        self.__dut.wait(1)

        results = self.__dut.iperf3_client(host=ref_mleid, bitrate=bitrate, transmit_time=transmit_time, length=length)
        ref_iperf3_server.join()

        if not results:
            print('Failed to run the iperf3')
            return

        self.__device_manager.output_test_result_string('Throughput',
                                                        self.__bitrate_to_string(results['receiver']['bitrate']))

    def __ref_iperf3_server_task(self, bind_address: str, timeout: int):
        self.__ref.iperf3_server(bind_address, timeout=timeout)

    def __bitrate_to_string(self, bitrate: float):
        units = ['bits/sec', 'Kbits/sec', 'Mbits/sec', 'Gbits/sec', 'Tbits/sec']
        unit_index = 0

        while bitrate >= 1000 and unit_index < len(units) - 1:
            bitrate /= 1000
            unit_index += 1

        return f'{bitrate:.2f} {units[unit_index]}'

    def tearDown(self):
        pass

    @classmethod
    def tearDownClass(cls):
        cls.__dut.close()
        cls.__ref.close()


if __name__ == '__main__':
    unittest.main()
