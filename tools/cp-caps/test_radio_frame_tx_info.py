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

import dataclasses
import unittest
import time

from otci import OTCI
from device_manager import DeviceManager, Frame


class TestRadioFrameTxInfo(unittest.TestCase):
    """Test whether the DUT supports otRadioFrame.mInfo.mTxInfo field."""

    MAX_SEND_RETRIES = 5
    SRC_EXT_ADDRESS = '0807060504030201'  # The source extended address of the secured test frames
    DST_EXT_ADDRESS = '8070605040302010'  # The destination extended address of the secured test frames

    @classmethod
    def setUpClass(cls):
        cls.__device_manager = DeviceManager()
        cls.__dut = cls.__device_manager.dut
        cls.__ref = cls.__device_manager.ref

    def setUp(self):
        self.__dut.factory_reset()
        self.__ref.factory_reset()

    def test_radio_frame_tx_info_csma_ca_enabled(self):
        """Test whether the DUT supports mInfo.mTxInfo.mCsmaCaEnabled field."""
        self.__dut.diag_start()
        self.__ref.diag_start()

        channel = 11
        num_sent_frames = 1
        tx_frame = '01ec00dddd01fecaefbe00adde02fecaefbe00adde000102030405060708090000'

        self.__dut.diag_set_channel(channel)
        self.__ref.diag_set_channel(channel)
        self.__ref.diag_cw_start()

        self.__dut.diag_stats_clear()
        self.__dut.diag_frame(tx_frame, csma_ca_enabled=False)
        self.__dut.diag_send(num_sent_frames, is_async=False)
        dut_stats = self.__dut.diag_get_stats()
        ret = dut_stats['sent_success_packets'] == num_sent_frames
        self.__device_manager.output_test_result_bool('mCsmaCaEnabled=0', ret)

        self.__dut.diag_stats_clear()
        self.__dut.diag_frame(tx_frame, csma_ca_enabled=True)
        self.__dut.diag_send(num_sent_frames, is_async=False)
        dut_stats = self.__dut.diag_get_stats()
        ret = dut_stats['sent_error_cca_packets'] == num_sent_frames
        self.__device_manager.output_test_result_bool('mCsmaCaEnabled=1', ret)

        self.__ref.diag_cw_stop()

    def test_radio_frame_tx_info_is_security_processed(self):
        """Test whether the DUT supports mInfo.mTxInfo.mIsSecurityProcessed field.

        The DUT is configured with the default dataset (MAC key index 1, MAC frame counter 0) for every frame. The
        frames with mIsSecurityProcessed=True are encrypted with a non-zero frame counter, so they are received as is
        only if the radio driver neither encrypts them again nor overwrites their key ID or frame counter.
        """
        active_dataset = self.__device_manager.get_default_dataset()

        frames = [
            # Key ID mode 0, frame counter 0x12345678, encrypted with the KEK ffeeddccbbaa99887766554433221100.
            Frame(name='mIsSecurityProcessed=True,KeyIdMode0',
                  tx_frame='09ec00dddd10203040506070800102030405060708057856341299c1fefeadda967e8f3606def4010000',
                  dst_address=self.DST_EXT_ADDRESS,
                  is_security_processed=True,
                  active_dataset=active_dataset,
                  src_ext_address=self.SRC_EXT_ADDRESS),
            # Key ID mode 1, key index 2, frame counter 0x12345678, encrypted with the MAC key of key sequence 1.
            Frame(name='mIsSecurityProcessed=True,KeyIdMode1',
                  tx_frame='09ec00dddd102030405060708001020304050607080d785634120264e3dcbe487e8e8721b56dfb679a0000',
                  dst_address=self.DST_EXT_ADDRESS,
                  is_security_processed=True,
                  active_dataset=active_dataset,
                  src_ext_address=self.SRC_EXT_ADDRESS),
            # Key ID mode 2, frame counter 0x12345678, encrypted with the Thread key ID mode 2 key.
            Frame(name='mIsSecurityProcessed=True,KeyIdMode2',
                  tx_frame=
                  '09ec00dddd102030405060708001020304050607081578563412fffffffffff6986efcae49ee0f27add6ebce1c0000',
                  dst_address=self.DST_EXT_ADDRESS,
                  is_security_processed=True,
                  active_dataset=active_dataset,
                  src_ext_address=self.SRC_EXT_ADDRESS),
            # The radio driver sets key index 1 and frame counter 0 and encrypts the frame.
            Frame(name='mIsSecurityProcessed=False',
                  tx_frame='09ec00dddd102030405060708001020304050607080d000000000000010203040506070809000000000000',
                  expect_rx_frame=
                  '09ec00dddd102030405060708001020304050607080d0000000001db622c220fde64408db9128c93d50000',
                  is_security_processed=False,
                  dst_address=self.DST_EXT_ADDRESS,
                  active_dataset=active_dataset,
                  src_ext_address=self.SRC_EXT_ADDRESS),
        ]

        for frame in frames:
            ret = self.__device_manager.send_formatted_frames_retries(self.__dut, self.__ref, frame)
            self.__device_manager.output_test_result_bool(frame.name, ret)

        # The method `send_formatted_frames_retries()` calls the `diag stop`.
        # Here calls the `diag start` to make the `tearDown()` happy.
        self.__ref.diag_start()
        self.__dut.diag_start()

    def test_radio_frame_tx_info_is_header_updated(self):
        """Test whether the DUT supports mInfo.mTxInfo.mIsHeaderUpdated field.

        The key ID and the frame counter of a frame with mIsHeaderUpdated=True (e.g., a retransmission) are already
        set, so the radio driver must encrypt the frame with them and must not consume its own MAC frame counter.
        """
        active_dataset = self.__device_manager.get_default_dataset()

        # Key ID mode 1, key index 1, frame counter 0x12345678, not encrypted yet.
        tx_frame = '09ec00dddd102030405060708001020304050607080d785634120100010203040506070809000000000000'

        header_not_updated_frame = Frame(
            name='mIsHeaderUpdated=False',
            tx_frame=tx_frame,
            # Encrypted with key index 1 and frame counter 0.
            expect_rx_frame='09ec00dddd102030405060708001020304050607080d0000000001db622c220fde64408db9128c93d50000',
            dst_address=self.DST_EXT_ADDRESS,
            is_security_processed=False,
            is_header_updated=False,
            active_dataset=active_dataset,
            src_ext_address=self.SRC_EXT_ADDRESS)

        header_updated_frame = Frame(
            name='mIsHeaderUpdated=True',
            tx_frame=tx_frame,
            # Encrypted with key index 1 and frame counter 0x12345678.
            expect_rx_frame='09ec00dddd102030405060708001020304050607080d785634120101024cbc3e4864fd55135e6970de0000',
            dst_address=self.DST_EXT_ADDRESS,
            is_security_processed=False,
            is_header_updated=True,
            active_dataset=active_dataset,
            src_ext_address=self.SRC_EXT_ADDRESS)

        # Sent right after the `header_updated_frame` without a factory reset, so the radio driver sets the frame
        # counter to 0 unless it consumed the MAC frame counter for the `header_updated_frame`.
        frame_counter_check_frame = dataclasses.replace(header_not_updated_frame,
                                                        active_dataset=None,
                                                        src_ext_address=None)

        header_not_updated_ret = self.__device_manager.send_formatted_frames_retries(
            self.__dut, self.__ref, header_not_updated_frame)

        # A failed send resets both devices, so retry the frame counter check together with the `header_updated_frame`.
        for _ in range(self.MAX_SEND_RETRIES):
            header_updated_ret = self.__device_manager.send_formatted_frames_retries(self.__dut,
                                                                                     self.__ref,
                                                                                     header_updated_frame,
                                                                                     max_send_retries=1)
            if header_updated_ret and header_not_updated_ret:
                header_updated_ret = self.__device_manager.send_formatted_frames_retries(self.__dut,
                                                                                         self.__ref,
                                                                                         frame_counter_check_frame,
                                                                                         max_send_retries=1)
            if header_updated_ret:
                break

        self.__device_manager.output_test_result_bool(header_updated_frame.name, header_updated_ret)
        self.__device_manager.output_test_result_bool(header_not_updated_frame.name, header_not_updated_ret)

        # The method `send_formatted_frames_retries()` calls the `diag stop`.
        # Here calls the `diag start` to make the `tearDown()` happy.
        self.__ref.diag_start()
        self.__dut.diag_start()

    def test_radio_frame_tx_info_max_csma_backoffs(self):
        """Test whether the DUT supports mInfo.mTxInfo.mMaxCsmaBackoffs field."""
        self.__dut.diag_start()
        self.__ref.diag_start()

        channel = 11
        num_sent_frames = 1
        tx_frame = '01ec00dddd01fecaefbe00adde02fecaefbe00adde000102030405060708090000'

        self.__dut.diag_set_channel(channel)
        self.__ref.diag_set_channel(channel)

        self.__ref.diag_cw_start()
        self.__dut.wait(0.05)

        # When `max_csma_backoffs` is set to 0, the radio driver should skip backoff and do CCA once.
        # The CCA time is 192 us. Theoretically, the `diag send` command should return after 192us.
        # But considering the Spinel delay and the system IO delay, here sets `max_time_cost` to 20 ms.
        max_time_cost = 20
        max_csma_backoffs = 0
        self.__dut.diag_stats_clear()
        self.__dut.diag_frame(tx_frame, csma_ca_enabled=True, max_frame_retries=0, max_csma_backoffs=max_csma_backoffs)
        start_time = time.time()
        self.__dut.diag_send(num_sent_frames, is_async=False)
        end_time = time.time()
        time_cost = int((end_time - start_time) * 1000)
        ret = 'OK' if time_cost < max_time_cost else 'NotSupported'
        self.__device_manager.output_test_result_string(f'mMaxCsmaBackoffs={max_csma_backoffs}',
                                                        f'{ret} ({time_cost} ms)')

        # Basic information for calculating the backoff time:
        #   aTurnaroundTime = 192 us
        #   aCCATime = 128 us
        #   backoffExponent = (macMinBe, macMaxBe) = (3, 5)
        #   backoffPeriod = random() % (1 << backoffExponent)
        #   backoff = backoffPeriod * aUnitBackoffPeriod = backoffPeriod * (aTurnaroundTime + aCCATime)
        #           = backoffPeriod * 320 us
        #   backoff = (random() % (1 << backoffExponent)) * 320us
        #
        # `max_csma_backoffs` is set to 100 here, `backoffExponent` will be set to 5 in most retries.
        #   backoff = (random() % 32) * 320us
        #   average_backoff = 16 * 320us = 5120 us
        #   total_backoff = average_backoff * 100 = 5120 us * 100 = 512 ms
        #
        # Here sets the `max_time_cost` to half of `total_backoff`.
        #
        max_time_cost = 256
        max_frame_retries = 0
        max_csma_backoffs = 100
        self.__dut.diag_frame(tx_frame, csma_ca_enabled=True, max_frame_retries=0, max_csma_backoffs=max_csma_backoffs)
        start_time = time.time()
        self.__dut.diag_send(num_sent_frames, is_async=False)
        end_time = time.time()
        time_cost = int((end_time - start_time) * 1000)
        ret = 'OK' if time_cost > max_time_cost else 'NotSupported'
        self.__device_manager.output_test_result_string(f'mMaxCsmaBackoffs={max_csma_backoffs}',
                                                        f'{ret} ({time_cost} ms)')

        self.__ref.diag_cw_stop()

    def test_radio_frame_tx_info_rx_channel_after_tx_done(self):
        """Test whether the DUT supports mInfo.mTxInfo.mRxChannelAfterTxDone field."""
        self.__dut.diag_start()
        self.__ref.diag_start()

        channel = 11
        num_sent_frames = 1
        rx_channel_after_tx_done = 25
        dut_address = 'dead00beefcafe01'
        dut_tx_frame = '01ec00dddd02fecaefbe00adde01fecaefbe00adde000102030405060708090000'
        ref_tx_frame = '01ec00dddd01fecaefbe00adde02fecaefbe00adde000102030405060708090000'

        self.__dut.diag_set_channel(channel)
        self.__dut.diag_set_radio_receive_filter_dest_mac_address(dut_address)
        self.__dut.diag_enable_radio_receive_filter()
        self.__dut.diag_stats_clear()

        self.__dut.diag_frame(dut_tx_frame, rx_channel_after_tx_done=rx_channel_after_tx_done)
        self.__dut.diag_send(num_sent_frames, is_async=False)
        stats = self.__dut.diag_get_stats()
        ret = stats['sent_success_packets'] == num_sent_frames

        if ret:
            self.__ref.diag_set_channel(rx_channel_after_tx_done)
            self.__ref.diag_frame(ref_tx_frame)
            self.__ref.diag_send(num_sent_frames, is_async=False)
            stats = self.__dut.diag_get_stats()
            ret = stats['received_packets'] == num_sent_frames

        self.__device_manager.output_test_result_bool('mRxChannelAfterTxDone', ret)

    def test_radio_frame_tx_info_tx_delay(self):
        """Test whether the DUT supports mInfo.mTxInfo.mTxDelayBaseTime and mInfo.mTxInfo.mTxDelay fields."""
        # Enable the IPv6 interface to force the host and the RCP to synchronize the radio time.
        self.__dut.set_dataset_bytes('active', self.__device_manager.get_default_dataset())
        self.__dut.ifconfig_up()
        self.__dut.wait(0.5)
        self.__dut.ifconfig_down()

        self.__dut.diag_start()
        self.__ref.diag_start()

        channel = 11
        packets = 1
        dut_tx_delay_sec = 0.5
        dut_tx_delay_us = int(dut_tx_delay_sec * 1000000)
        ref_rx_delay_sec = dut_tx_delay_sec / 2
        # The margin for the radio time synchronization error and the latencies of the host, the radio and the CLI.
        ref_rx_margin_sec = 0.25
        ref_address = 'dead00beefcafe01'
        dut_tx_frame = '01ec00dddd01fecaefbe00adde02fecaefbe00adde000102030405060708090000'

        self.__dut.diag_set_channel(channel)
        self.__ref.diag_set_channel(channel)
        self.__ref.diag_radio_receive()
        self.__ref.diag_set_radio_receive_filter_dest_mac_address(ref_address)
        self.__ref.diag_enable_radio_receive_filter()

        # `diag frame -d` sets mTxDelayBaseTime to the current radio time, so the frame is due to be sent
        # `dut_tx_delay_sec` after the `diag frame` command.
        self.__dut.diag_frame(dut_tx_frame, tx_delay=dut_tx_delay_us)
        self.__dut.diag_send(packets, is_async=True)

        self.__ref.wait(ref_rx_delay_sec)
        stats = self.__ref.diag_get_stats()
        ret = stats['received_packets'] == 0

        if ret is True:
            self.__ref.wait(ref_rx_delay_sec + ref_rx_margin_sec)
            stats = self.__ref.diag_get_stats()
            ret = stats['received_packets'] == 1

        self.__device_manager.output_test_result_bool(f'mTxDelayBaseTime=now,mTxDelay={dut_tx_delay_us}', ret)

    def tearDown(self):
        self.__ref.diag_stop()
        self.__dut.diag_stop()

    @classmethod
    def tearDownClass(cls):
        cls.__dut.close()
        cls.__ref.close()


if __name__ == '__main__':
    unittest.main()
