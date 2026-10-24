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

import atexit
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
from sniffer import Sniffer

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
        self.__dut = None
        self.__ref = None
        self.__sniffer = None
        atexit.register(self.close)
        try:
            self.__dut = self.__connect_dut()
            self.__ref = self.__connect_reference_device()
            self.__sniffer = self.__create_sniffer()
        except Exception:
            self.close()
            raise

        if self.__get_debug_enabled():
            logger = logging.getLogger()
            logger.setLevel(logging.DEBUG)

    def close(self):
        """Closes connected devices and tears down active proxies/processes."""
        try:
            atexit.unregister(self.close)
        except Exception:
            pass

        if getattr(self, '_DeviceManager__dut', None) is not None:
            try:
                self.__dut.close()
            except Exception:
                pass
            self.__dut = None

        if getattr(self, '_DeviceManager__ref', None) is not None:
            try:
                self.__ref.close()
            except Exception:
                pass
            self.__ref = None

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    @property
    def dut(self) -> OTCI:
        return self.__dut

    @property
    def ref(self) -> OTCI:
        return self.__ref

    @property
    def sniffer(self) -> Optional[Sniffer]:
        """The sniffer, or None if no sniffer is configured (`SNIFFER_SERIAL` or `SNIFFER_RADIO_URL`)."""
        return self.__sniffer

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

    def __get_ot_cli(self) -> str:
        if os.getenv('OT_CLI'):
            return os.getenv('OT_CLI')
        if os.getenv('OT_POSIX_CLI'):
            return os.getenv('OT_POSIX_CLI')
        if os.getenv('OT_POSIX_APPS'):
            candidate = os.path.join(os.getenv('OT_POSIX_APPS'), 'ot-cli')
            if os.path.isfile(candidate) and os.access(candidate, os.X_OK):
                return candidate
        default_posix = os.path.join(os.path.dirname(__file__), '../../build/posix/src/posix/ot-cli')
        if os.path.isfile(default_posix) and os.access(default_posix, os.X_OK):
            return os.path.abspath(default_posix)
        return 'ot-cli'

    def __is_spinel_proxy_enabled(self, prefix: str = 'DUT') -> bool:
        val = os.getenv(
            f'{prefix}_SPINEL_PROXY',
            os.getenv(f'{prefix}_USE_SPINEL_PROXY', os.getenv('SPINEL_PROXY', os.getenv('USE_SPINEL_PROXY', ''))))
        return val.lower() in ('1', 'true', 'on', 'yes')

    def __create_sniffer(self) -> Optional[Sniffer]:
        if os.getenv('SNIFFER_RADIO_URL'):
            radio_url = os.getenv('SNIFFER_RADIO_URL')
        elif os.getenv('SNIFFER_SERIAL'):
            radio_url = f'spinel+hdlc+uart://{os.getenv("SNIFFER_SERIAL")}?uart-baudrate=115200'
        else:
            return None

        return Sniffer(os.getenv('OT_EXTCAP', 'ot-extcap'), radio_url)

    def __connect_dut(self) -> OTCI:
        if os.getenv('DUT_RADIO_URL'):
            node = otci.connect_cli_posix(os.getenv('DUT_RADIO_URL'), executable=self.__get_ot_cli())
        elif os.getenv('DUT_ADB_SPINEL_PROXY'):
            node = otci.connect_cli_adb_spinel_proxy(
                device_serial=os.getenv('DUT_ADB_SPINEL_PROXY'),
                socket_path=os.getenv('DUT_SPINEL_PROXY_SOCKET'),
                executable=self.__get_ot_cli(),
                adb_key=self.__get_adb_key(),
            )
        elif os.getenv('DUT_ADB_TCP'):
            if self.__is_spinel_proxy_enabled('DUT'):
                addr = os.getenv('DUT_ADB_TCP')
                if ':' in addr:
                    host, port_str = addr.split(':', 1)
                    port = int(port_str)
                else:
                    host, port = addr, 5555
                node = otci.connect_cli_adb_spinel_proxy(
                    host=host,
                    port=port,
                    socket_path=os.getenv('DUT_SPINEL_PROXY_SOCKET'),
                    executable=self.__get_ot_cli(),
                    adb_key=self.__get_adb_key(),
                )
            else:
                node = otci.connect_otbr_adb_tcp(os.getenv('DUT_ADB_TCP'), adb_key=self.__get_adb_key())
        elif os.getenv('DUT_ADB_USB'):
            if self.__is_spinel_proxy_enabled('DUT'):
                node = otci.connect_cli_adb_spinel_proxy(
                    device_serial=os.getenv('DUT_ADB_USB'),
                    socket_path=os.getenv('DUT_SPINEL_PROXY_SOCKET'),
                    executable=self.__get_ot_cli(),
                    adb_key=self.__get_adb_key(),
                )
            else:
                node = otci.connect_otbr_adb_usb(os.getenv('DUT_ADB_USB'), adb_key=self.__get_adb_key())
        elif os.getenv('DUT_CLI_SERIAL'):
            node = otci.connect_cli_serial(os.getenv('DUT_CLI_SERIAL'))
        elif os.getenv('DUT_SSH'):
            node = otci.connect_otbr_ssh(os.getenv('DUT_SSH'))
        else:
            raise InvalidArgumentsError(
                "Please set DUT_RADIO_URL, DUT_ADB_SPINEL_PROXY, DUT_ADB_TCP, DUT_ADB_USB, DUT_CLI_SERIAL or DUT_SSH to connect to the DUT device."
            )

        return node

    def __connect_reference_device(self) -> OTCI:
        if os.getenv('REF_RADIO_URL'):
            node = otci.connect_cli_posix(os.getenv('REF_RADIO_URL'), executable=self.__get_ot_cli())
        elif os.getenv('REF_ADB_SPINEL_PROXY'):
            node = otci.connect_cli_adb_spinel_proxy(
                device_serial=os.getenv('REF_ADB_SPINEL_PROXY'),
                socket_path=os.getenv('REF_SPINEL_PROXY_SOCKET'),
                executable=self.__get_ot_cli(),
                adb_key=self.__get_adb_key(),
            )
        elif os.getenv('REF_ADB_TCP'):
            if self.__is_spinel_proxy_enabled('REF'):
                addr = os.getenv('REF_ADB_TCP')
                if ':' in addr:
                    host, port_str = addr.split(':', 1)
                    port = int(port_str)
                else:
                    host, port = addr, 5555
                node = otci.connect_cli_adb_spinel_proxy(
                    host=host,
                    port=port,
                    socket_path=os.getenv('REF_SPINEL_PROXY_SOCKET'),
                    executable=self.__get_ot_cli(),
                    adb_key=self.__get_adb_key(),
                )
            else:
                node = otci.connect_otbr_adb_tcp(os.getenv('REF_ADB_TCP'), adb_key=self.__get_adb_key())
        elif os.getenv('REF_ADB_USB'):
            if self.__is_spinel_proxy_enabled('REF'):
                node = otci.connect_cli_adb_spinel_proxy(
                    device_serial=os.getenv('REF_ADB_USB'),
                    socket_path=os.getenv('REF_SPINEL_PROXY_SOCKET'),
                    executable=self.__get_ot_cli(),
                    adb_key=self.__get_adb_key(),
                )
            else:
                node = otci.connect_otbr_adb_usb(os.getenv('REF_ADB_USB'), adb_key=self.__get_adb_key())
        elif os.getenv('REF_CLI_SERIAL'):
            node = otci.connect_cli_serial(os.getenv('REF_CLI_SERIAL'))
        elif os.getenv('REF_SSH'):
            node = otci.connect_otbr_ssh(os.getenv('REF_SSH'))
        else:
            raise InvalidArgumentsError(
                "Please set REF_RADIO_URL, REF_ADB_SPINEL_PROXY, REF_ADB_TCP, REF_ADB_USB, REF_CLI_SERIAL or REF_SSH to connect to the reference device."
            )

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
