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

import os
import tempfile
import time
import unittest

from typing import Optional

from otci import OTCI
from csl_analyzer import CslTxReport, analyze_csl_tx
from device_manager import DeviceManager


class TestCsl(unittest.TestCase):
    """Test whether the DUT supports the CSL feature."""

    CSL_CROSS_CHANNEL = 15
    CSL_PERIOD_US = 320000
    CSL_SYNC_TIMEOUT = 10  # seconds

    @classmethod
    def setUpClass(cls):
        cls.__device_manager = DeviceManager()
        cls.__dut = cls.__device_manager.dut
        cls.__ref = cls.__device_manager.ref

    def setUp(self):
        self.__dut.factory_reset()
        self.__ref.factory_reset()

    def test_csl_transmitter(self):
        """Test whether the DUT supports the CSL transmitter.

        The CSL channel of the reference device is first the same as the PAN channel, which tests the timing of CSL
        transmissions without channel switching, and then another channel.
        """
        same_channel_report = self.__test_csl_transmitter('CSL Transmitter (Same Channel)',
                                                          DeviceManager.DEFAULT_CHANNEL)

        self.__dut.factory_reset()
        self.__ref.factory_reset()

        self.__test_csl_transmitter('CSL Transmitter (Cross Channel)', self.CSL_CROSS_CHANNEL, same_channel_report)

    def __test_csl_transmitter(self,
                               name: str,
                               csl_channel: int,
                               same_channel_report: Optional[CslTxReport] = None) -> Optional[CslTxReport]:
        packets = 10
        dataset = self.__device_manager.get_default_dataset()
        sniffer = self.__device_manager.sniffer
        frames = None
        sniffer_error = None
        pcapng_path = None
        start_us = None
        report = None
        csl_accuracy = None
        csl_uncertainty_us = None

        self.__dut.join(dataset)
        self.__dut.wait_for('state', 'leader')

        if sniffer is not None:
            if self.__device_manager.debug_enabled:
                fd, pcapng_path = tempfile.mkstemp(prefix=f'cp-caps-csl-ch{csl_channel}-', suffix='.pcapng')
                os.close(fd)

            # Starts capturing before the reference device attaches, to capture the CSL IEs it sends to the DUT
            # when the CSL channel is the PAN channel.
            sniffer.start(csl_channel, pcapng_path)

        try:
            # Set the reference device as an SSED
            self.__ref.set_mode('-')
            self.__ref.config_csl(channel=csl_channel, period=self.CSL_PERIOD_US, timeout=100)
            self.__ref.join(dataset)
            self.__ref.wait_for('state', 'child')

            ret = self.__wait_csl_synchronized()

            if ret:
                ref_mleid = self.__ref.get_ipaddr_mleid()
                dut_addresses = self.__get_addresses(self.__dut)
                ref_addresses = self.__get_addresses(self.__ref)
                # The CSL accuracy and uncertainty that the DUT advertises to the reference device. The CSL Phase is
                # truncated to units of 160 us, so a frame timed from another CSL IE than the DUT used is off by up to
                # 160 us. They are only checked on the PAN channel, where the sniffer captures all the CSL IEs that
                # the DUT may time frames from.
                if sniffer is not None and csl_channel == DeviceManager.DEFAULT_CHANNEL:
                    csl_accuracy = self.__dut.get_csl_accuracy()
                    csl_uncertainty_us = self.__dut.get_csl_uncertainty() * 10
                start_us = time.time_ns() // 1000
                result = self.__dut.ping(ref_mleid, count=packets, interval=1)
                ret = result['transmitted_packets'] == result['received_packets'] == packets
        finally:
            if sniffer is not None:
                try:
                    frames = sniffer.stop()
                except RuntimeError as e:
                    sniffer_error = e

        if frames is not None and start_us is not None:
            report = analyze_csl_tx(frames, csl_channel, dut_addresses, ref_addresses, start_us, self.CSL_PERIOD_US)

        self.__dut.leave()
        self.__ref.leave()

        self.__device_manager.output_test_result_bool(name, ret)

        if sniffer_error is not None:
            print(f'  No CSL analysis: {sniffer_error}')
        elif report is not None:
            if not ret or self.__device_manager.debug_enabled:
                self.__output_report(report, same_channel_report)

            timing = report.timing
            if timing is not None:
                for line in timing.describe(csl_accuracy, csl_uncertainty_us):
                    print(f'  {line}')
        elif sniffer is not None:
            print('  No CSL analysis: the reference device is not a CSL synchronized child of the DUT')

        if pcapng_path is not None:
            print(f'  Capture: {pcapng_path}')

        return report

    def __wait_csl_synchronized(self) -> bool:
        # The SSED synchronizes CSL with the DUT after attaching, by sending a frame with a CSL IE.
        for _ in range(self.CSL_SYNC_TIMEOUT):
            child_table = self.__dut.get_child_table()
            if len(child_table) == 1 and child_table[1]['csl']:
                return True
            self.__dut.wait(1)

        return False

    @staticmethod
    def __get_addresses(node: OTCI):
        return {node.get_extaddr(), f'{node.get_rloc16():04x}'}

    @staticmethod
    def __output_report(report: CslTxReport, same_channel_report: Optional[CslTxReport]):
        for line in report.describe():
            print(f'  {line}')

        if report.is_untimed and same_channel_report is not None:
            print(f'  Same channel ({same_channel_report.channel}): {same_channel_report.describe()[0]}')

    def tearDown(self):
        pass

    @classmethod
    def tearDownClass(cls):
        cls.__dut.close()
        cls.__ref.close()


if __name__ == '__main__':
    unittest.main()
