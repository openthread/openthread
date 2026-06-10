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

import logging
import os
import sys
import threading
import queue

from dataclasses import dataclass
from typing import Optional

import otci
from otci import OTCI
from otci.errors import ExpectLineTimeoutError, InvalidArgumentsError

CP_CAPABILITY_VERSION = "0.1.1-dev"

logging.basicConfig(level=logging.WARNING)


@dataclass
class Frame:
    """Represents a Thread radio frame.

    Attributes:
      name: The description of the frame.
      tx_frame: The psdu of the frame that is to be sent.
      dst_address: The destination MAC address of the `tx_frame`. It is used by the receiver to filter
          out the `tx_frame`.
      is_security_processed: The value of the `otRadioFrame.mInfo.mTxInfo.mIsSecurityProcessed` field.
          If it is set to False, the `active_dataset` and `src_ext_address` should also be set for the
          radio driver to encrypt the `tx_frame`.
      expect_rx_frame: The frame expected to be received. The expected frame should be the same as the
          `tx_frame` if the `expect_rx_frame` is set to None.
      active_dataset: The active dataset.
      src_ext_address: The source extended MAC address of the transmitter.
    """
    name: str
    tx_frame: str
    dst_address: str
    is_security_processed: Optional[bool] = True
    expect_rx_frame: Optional[str] = None
    active_dataset: Optional[str] = None
    src_ext_address: Optional[str] = None


class DeviceManager(object):
    """Provides the DUT, reference device and common methods for test cases."""

    DEFAULT_FORMAT_ALIGN_LENGTH = 58  # The default formatted string alignment length

    def __init__(self):
        self.__dut = self.__connect_dut()
        self.__ref = self.__connect_reference_device()

        if self.__get_debug_enabled():
            logger = logging.getLogger()
            logger.setLevel(logging.DEBUG)

    @property
    def dut(self) -> OTCI:
        return self.__dut

    @property
    def ref(self) -> OTCI:
        return self.__ref

    def output_test_result_string(self, name: str, value: str, align_length: int = DEFAULT_FORMAT_ALIGN_LENGTH):
        prefix = (name + ' ').ljust(align_length, '-')
        print(f'{prefix} {value}')

    def output_test_result_bool(self, name: str, value: bool, align_length: int = DEFAULT_FORMAT_ALIGN_LENGTH):
        self.output_test_result_string(name, 'OK' if value else 'NotSupported', align_length)

    def get_default_dataset(self):
        return self.__dut.create_dataset(channel=20, network_key='00112233445566778899aabbccddcafe')

    def send_formatted_frames_retries(self, sender: OTCI, receiver: OTCI, frame: Frame, max_send_retries: int = 5):
        for i in range(0, max_send_retries):
            if self.__send_formatted_frame(sender, receiver, frame):
                return True

        return False

    #
    # Private methods
    #
    def __get_debug_enabled(self) -> bool:
        enabled = os.getenv('DEBUG', None)
        return enabled == 'on'

    def __get_adb_key(self) -> Optional[str]:
        return os.getenv('ADB_KEY', None)

    def __connect_dut(self) -> OTCI:
        if os.getenv('DUT_ADB_TCP'):
            node = otci.connect_otbr_adb_tcp(os.getenv('DUT_ADB_TCP'), adb_key=self.__get_adb_key())
        elif os.getenv('DUT_ADB_USB'):
            node = otci.connect_otbr_adb_usb(os.getenv('DUT_ADB_USB'), adb_key=self.__get_adb_key())
        elif os.getenv('DUT_CLI_SERIAL'):
            node = otci.connect_cli_serial(os.getenv('DUT_CLI_SERIAL'))
        elif os.getenv('DUT_SSH'):
            node = otci.connect_otbr_ssh(os.getenv('DUT_SSH'))
        else:
            raise InvalidArgumentsError(
                "Please set DUT_ADB_TCP, DUT_ADB_USB, DUT_CLI_SERIAL or DUT_SSH to connect to the DUT device.")

        return node

    def __connect_reference_device(self) -> OTCI:
        if os.getenv('REF_CLI_SERIAL'):
            node = otci.connect_cli_serial(os.getenv('REF_CLI_SERIAL'))
        elif os.getenv('REF_SSH'):
            node = otci.connect_otbr_ssh(os.getenv('REF_SSH'))
        elif os.getenv('REF_ADB_USB'):
            node = otci.connect_otbr_adb_usb(os.getenv('REF_ADB_USB'), adb_key=self.__get_adb_key())
        else:
            raise InvalidArgumentsError(
                "Please set REF_CLI_SERIAL, REF_SSH or REF_ADB_USB to connect to the reference device.")

        return node

    def __send_formatted_frame(self, sender: OTCI, receiver: OTCI, frame: Frame):
        # When `is_security_processed` is False, the frame may need the radio driver
        # to encrypt the frame. Here sets the active dataset and the MAC source address for the
        # radio driver to encrypt the frame.
        if frame.is_security_processed is False:
            if frame.active_dataset is None or frame.src_ext_address is None:
                raise InvalidArgumentsError(
                    "When the 'is_security_processed' is 'False', the 'active_dataset' and 'src_ext_address' must be set"
                )
            # Do the factory reset to set the frame counter to 0.
            sender.factory_reset()
            receiver.factory_reset()
            sender.set_dataset_bytes('active', frame.active_dataset)
            sender.set_extaddr(frame.src_ext_address)

        sender.diag_start()
        receiver.diag_start()

        channel = 11
        num_sent_frames = 1

        sender.diag_set_channel(channel)
        receiver.diag_set_channel(channel)
        receiver.diag_radio_receive()
        receiver.diag_set_radio_receive_filter_dest_mac_address(frame.dst_address)
        receiver.diag_enable_radio_receive_filter()

        result_queue = queue.Queue()
        receive_task = threading.Thread(target=self.__radio_receive_task,
                                        args=(receiver, num_sent_frames, result_queue),
                                        daemon=True)
        receive_task.start()

        sender.wait(0.1)

        sender.diag_frame(frame.tx_frame,
                          is_security_processed=frame.is_security_processed,
                          max_csma_backoffs=4,
                          max_frame_retries=4,
                          csma_ca_enabled=True)
        sender.diag_send(num_sent_frames, is_async=False)

        receive_task.join()

        if result_queue.empty():
            ret = False  # No frame is received.
        else:
            received_frames = result_queue.get()
            if len(received_frames) != num_sent_frames:
                ret = False
            else:
                # The radio driver may not append the FCF field to the received frame. Do not check the FCF field here.
                FCF_LENGTH = 4
                expect_frame = frame.expect_rx_frame or frame.tx_frame
                ret = expect_frame[:-FCF_LENGTH] == received_frames[0]['psdu'][:-FCF_LENGTH]

        if ret:
            sender.diag_stop()
            receiver.diag_stop()
        else:
            # The command `diag radio receive <number>` may fail to receive specified number of frames in default
            # timeout time. In this case, the diag module still in `sync` mode, and it only allows users to run the
            # command `factoryreset` to terminate the test.
            sender.factory_reset()
            receiver.factory_reset()

        return ret

    def __radio_receive_task(self, receiver: OTCI, number: int, result_queue: queue):
        try:
            receiver.set_execute_command_retry(0)
            result = receiver.diag_radio_receive_number(number)
        except ExpectLineTimeoutError:
            pass
        else:
            result_queue.put(result)
        finally:
            receiver.set_execute_command_retry(OTCI.DEFAULT_EXEC_COMMAND_RETRY)
