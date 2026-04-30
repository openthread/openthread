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
from otci.types import Ip6Addr
from device_manager import DeviceManager


class TestLinkMetrics(unittest.TestCase):
    """Test whether the DUT supports Link Metrics feature."""

    @classmethod
    def setUpClass(cls):
        cls.__device_manager = DeviceManager()
        cls.__dut = cls.__device_manager.dut
        cls.__ref = cls.__device_manager.ref

    def setUp(self):
        self.__dut.factory_reset()
        self.__ref.factory_reset()
        dataset = self.__device_manager.get_default_dataset()

        self.__dut.join(dataset)
        self.__dut.wait_for('state', 'leader')

        self.__ref.join(dataset)
        self.__ref.wait_for('state', ['child', 'router'])

    def test_link_metrics_initiator(self):
        """Test whether the DUT supports Link Metrics Initiator."""
        ref_linklocal_address = self.__ref.get_ipaddr_linklocal()
        ret = self.__run_link_metrics_test_commands(initiator=self.__dut, subject_address=ref_linklocal_address)
        self.__device_manager.output_test_result_bool('Link Metrics Initiator', ret)

    def test_link_metrics_subject(self):
        """Test whether the DUT supports Link Metrics Subject."""
        dut_linklocal_address = self.__dut.get_ipaddr_linklocal()
        ret = self.__run_link_metrics_test_commands(initiator=self.__ref, subject_address=dut_linklocal_address)
        self.__device_manager.output_test_result_bool('Link Metrics Subject', ret)

    def __run_link_metrics_test_commands(self, initiator: OTCI, subject_address: Ip6Addr) -> bool:
        seriesid = 1
        series_flags = 'ldra'
        link_metrics_flags = 'qr'
        probe_length = 10

        if not initiator.linkmetrics_config_enhanced_ack_register(subject_address, link_metrics_flags):
            return False

        if not initiator.linkmetrics_config_forward(subject_address, seriesid, series_flags, link_metrics_flags):
            return False

        initiator.linkmetrics_probe(subject_address, seriesid, probe_length)

        results = initiator.linkmetrics_request_single(subject_address, link_metrics_flags)
        if not ('lqi' in results.keys() and 'rssi' in results.keys()):
            return False

        results = initiator.linkmetrics_request_forward(subject_address, seriesid)
        if not ('lqi' in results.keys() and 'rssi' in results.keys()):
            return False

        if not initiator.linkmetrics_config_enhanced_ack_clear(subject_address):
            return False

        return True

    def tearDown(self):
        self.__ref.leave()
        self.__dut.leave()

    @classmethod
    def tearDownClass(cls):
        cls.__dut.close()
        cls.__ref.close()


if __name__ == '__main__':
    unittest.main()
