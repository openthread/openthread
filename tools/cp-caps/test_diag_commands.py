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
import os

from typing import List
from otci import OTCI
from device_manager import DeviceManager, Frame


class TestDiagCommands(unittest.TestCase):
    """Test whether the DUT supports all diag commands."""

    @classmethod
    def setUpClass(cls):
        cls.__device_manager = DeviceManager()
        cls.__dut = cls.__device_manager.dut
        cls.__ref = cls.__device_manager.ref
        cls.__dut.factory_reset()
        cls.__ref.factory_reset()

    def setUp(self):
        self.__dut.diag_start()
        self.__ref.diag_start()

    def test_diag_channel(self):
        """Test whether the DUT supports the `diag channel` commands."""
        channel = 20
        commands = ['diag channel', f'diag channel {channel}']

        if self.__support_commands(commands):
            self.__dut.diag_set_channel(channel)
            value = self.__dut.diag_get_channel()
            ret = value == channel
        else:
            ret = False

        self.__output_results(commands, ret)

    def test_diag_cw(self):
        """Test whether the DUT supports the `diag cw` commands."""
        commands = ['diag cw start', 'diag cw stop']

        for command in commands:
            ret = self.__dut.is_command_supported(command)
            self.__device_manager.output_test_result_bool(command, ret)

    def test_diag_echo(self):
        """Test whether the DUT supports the `diag echo` commands."""
        echo_msg = '0123456789'
        cmd_diag_echo = f'diag echo {echo_msg}'
        cmd_diag_echo_num = f'diag echo -n 10'

        if self.__dut.is_command_supported(cmd_diag_echo):
            reply = self.__dut.diag_echo(echo_msg)
            ret = reply == echo_msg
        else:
            ret = False
        self.__device_manager.output_test_result_bool(cmd_diag_echo, ret)

        if self.__dut.is_command_supported(cmd_diag_echo_num):
            reply = self.__dut.diag_echo_number(10)
            ret = reply == echo_msg
        else:
            ret = False
        self.__device_manager.output_test_result_bool(cmd_diag_echo_num, ret)

    def test_diag_frame(self):
        """Test whether the DUT supports the `diag frame` commands."""
        frame = Frame(name='diag frame 00010203040506070809', tx_frame='00010203040506070809', dst_address='-')
        ret = self.__device_manager.send_formatted_frames_retries(self.__dut, self.__ref, frame)
        self.__device_manager.output_test_result_bool(frame.name, ret)
        # The method send_formatted_frames_retries() calls the `diag stop`.
        # Here calls the `diag start` to make the 'tearDown()' happy.
        self.__dut.diag_start()
        self.__ref.diag_start()

    def test_diag_gpio_mode(self):
        """Test whether the DUT supports the `diag gpio mode` commands."""
        gpio = self.__get_dut_diag_gpio()
        commands = [f'diag gpio mode {gpio}', f'diag gpio mode {gpio} in', f'diag gpio mode {gpio} out']

        if self.__support_commands(commands):
            self.__dut.diag_set_gpio_mode(gpio, 'in')
            mode_in = self.__dut.diag_get_gpio_mode(gpio)
            self.__dut.diag_set_gpio_mode(gpio, 'out')
            mode_out = self.__dut.diag_get_gpio_mode(gpio)

            ret = mode_in == 'in' and mode_out == 'out'
        else:
            ret = False

        self.__output_results(commands, ret)

    def test_diag_gpio_value(self):
        """Test whether the DUT supports the `diag gpio get` and 'diag gpio set' commands."""
        gpio = self.__get_dut_diag_gpio()
        commands = [f'diag gpio get {gpio}', f'diag gpio set {gpio} 0', f'diag gpio set {gpio} 1']

        if self.__support_commands(commands):
            self.__dut.diag_set_gpio_value(gpio, 0)
            value_0 = self.__dut.diag_get_gpio_value(gpio)
            self.__dut.diag_set_gpio_value(gpio, 1)
            value_1 = self.__dut.diag_get_gpio_value(gpio)

            ret = value_0 == 0 and value_1 == 1
        else:
            ret = False

        self.__output_results(commands, ret)

    def test_diag_power(self):
        """Test whether the DUT supports the `diag power` commands."""
        power = self.__get_dut_diag_power()
        commands = ['diag power', f'diag power {power}']

        if self.__support_commands(commands):
            self.__dut.diag_set_power(power)
            value = self.__dut.diag_get_power()
            ret = value == power
        else:
            ret = False

        self.__output_results(commands, ret)

    def test_diag_powersettings(self):
        """Test whether the DUT supports the `diag powersettings` commands."""
        commands = ['diag powersettings', 'diag powersettings 20']

        if self.__support_commands(commands):
            powersettings = self.__dut.diag_get_powersettings()
            ret = len(powersettings) > 0
        else:
            ret = False

        self.__output_results(commands, ret)

    def test_diag_radio(self):
        """Test whether the DUT supports the `diag radio` commands."""
        commands = ['diag radio receive', 'diag radio sleep', 'diag radio state']

        if self.__support_commands(commands):
            self.__dut.diag_radio_receive()
            receive_state = self.__dut.diag_get_radio_state()
            self.__dut.wait(0.1)
            self.__dut.diag_radio_sleep()
            sleep_state = self.__dut.diag_get_radio_state()

            ret = sleep_state == 'sleep' and receive_state == 'receive'
        else:
            ret = False

        self.__output_results(commands, ret)

    def test_diag_rawpowersetting(self):
        """Test whether the DUT supports the `diag rawpowersetting` commands."""
        rawpowersetting = self.__get_dut_diag_raw_power_setting()
        commands = [
            'diag rawpowersetting enable', f'diag rawpowersetting {rawpowersetting}', 'diag rawpowersetting',
            'diag rawpowersetting disable'
        ]

        if self.__support_commands(commands):
            self.__dut.diag_enable_rawpowersetting()
            self.__dut.diag_set_rawpowersetting(rawpowersetting)
            reply = self.__dut.diag_get_rawpowersetting()
            self.__dut.diag_disable_rawpowersetting()

            ret = reply == rawpowersetting
        else:
            ret = False

        self.__output_results(commands, ret)

    def test_diag_repeat(self):
        """Test whether the DUT supports the `diag repeat` commands."""
        delay = 10
        threshold = 80
        length = 64
        channel = 20
        cmd_diag_repeat = f'diag repeat {delay} {length}'
        cmd_diag_repeat_stop = 'diag repeat stop'
        commands = [cmd_diag_repeat, 'diag repeat stop', 'diag stats', 'diag stats clear']

        if self.__support_commands(commands):
            self.__dut.diag_set_channel(channel)
            self.__ref.diag_set_channel(channel)
            self.__ref.diag_radio_receive()

            self.__dut.diag_stats_clear()
            self.__ref.diag_stats_clear()

            self.__dut.diag_repeat(delay, length)
            self.__dut.wait(1)
            self.__dut.diag_repeat_stop()
            dut_stats = self.__dut.diag_get_stats()
            ref_stats = self.__ref.diag_get_stats()

            ret = dut_stats['sent_success_packets'] > threshold and ref_stats['received_packets'] > threshold
        else:
            ret = False

        self.__device_manager.output_test_result_bool(cmd_diag_repeat, ret)
        self.__device_manager.output_test_result_bool(cmd_diag_repeat_stop, ret)

    def test_diag_send(self):
        """Test whether the DUT supports the `diag send` commands."""
        packets = 100
        threshold = 80
        length = 64
        channel = 20
        commands = [f'diag send {packets} {length}']

        if self.__support_commands(commands):
            self.__dut.wait(1)
            self.__dut.diag_set_channel(channel)
            self.__ref.diag_set_channel(channel)
            self.__ref.diag_radio_receive()

            self.__dut.diag_stats_clear()
            self.__ref.diag_stats_clear()

            self.__dut.diag_send(packets, length)
            self.__dut.wait(1)
            dut_stats = self.__dut.diag_get_stats()
            ref_stats = self.__ref.diag_get_stats()

            ret = dut_stats['sent_success_packets'] == packets and ref_stats['received_packets'] > threshold
        else:
            ret = False

        self.__output_results(commands, ret)

    def test_diag_stats(self):
        """Test whether the DUT supports the `diag stats` commands."""
        commands = ['diag stats', 'diag stats clear']

        for command in commands:
            ret = self.__dut.is_command_supported(command)
            self.__device_manager.output_test_result_bool(command, ret)

    def test_diag_stream(self):
        """Test whether the DUT supports the 'diag stream' commands."""
        commands = ['diag stream start', 'diag stream stop']

        for command in commands:
            ret = self.__dut.is_command_supported(command)
            self.__device_manager.output_test_result_bool(command, ret)

    def __get_dut_diag_power(self) -> int:
        return int(os.getenv('DUT_DIAG_POWER', '10'))

    def __get_dut_diag_gpio(self) -> int:
        return int(os.getenv('DUT_DIAG_GPIO', '0'))

    def __get_dut_diag_raw_power_setting(self) -> str:
        return os.getenv('DUT_DIAG_RAW_POWER_SETTING', '112233')

    def __support_commands(self, commands: List[str]) -> bool:
        ret = True

        for command in commands:
            if self.__dut.is_command_supported(command) is False:
                ret = False
                break

        return ret

    def __output_results(self, commands: List[str], support: bool):
        for command in commands:
            self.__device_manager.output_test_result_bool(command, support)

    def tearDown(self):
        self.__ref.diag_stop()
        self.__dut.diag_stop()

    @classmethod
    def tearDownClass(cls):
        cls.__dut.close()
        cls.__ref.close()


if __name__ == '__main__':
    unittest.main()
