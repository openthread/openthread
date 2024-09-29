#!/usr/bin/env python3
#
#  Copyright (c) 2024, The OpenThread Authors.
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

import argparse
import logging
import os
import sys
import textwrap
import threading

from typing import List

import otci
from otci import OTCI
from otci.types import Ip6Addr

CP_CAPABILITY_VERSION = "0.1.0"

logging.basicConfig(level=logging.WARNING)


class RcpCaps(object):
    """
    This class represents an OpenThread RCP capability test instance.
    """

    def __init__(self):
        self.__dut = self.__connect_dut()
        self.__ref = self.__connect_reference_device()

    def test_diag_commands(self):
        """Test all diag commands."""
        self.__dut.factory_reset()
        self.__ref.factory_reset()

        ret = self.__dut.is_command_supported('diag start')
        if ret is False:
            print('All diag commands are not supported')
            return

        self.__dut.diag_start()
        self.__ref.diag_start()

        self.__test_diag_channel()
        self.__test_diag_power()
        self.__test_diag_radio()
        self.__test_diag_repeat()
        self.__test_diag_send()
        self.__test_diag_frame()
        self.__test_diag_echo()
        self.__test_diag_utils()
        self.__test_diag_rawpowersetting()
        self.__test_diag_powersettings()
        self.__test_diag_gpio_mode()
        self.__test_diag_gpio_value()

        self.__ref.diag_stop()
        self.__dut.diag_stop()

    def test_csl(self):
        """Test whether the DUT supports CSL transmitter."""
        self.__dataset = self.__get_default_dataset()
        self.__test_csl_transmitter()

    def test_data_poll(self):
        """Test whether the DUT supports data poll parent and child."""
        self.__dataset = self.__get_default_dataset()
        self.__test_data_poll_parent()
        self.__test_data_poll_child()

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

        self.__dut.factory_reset()
        self.__ref.factory_reset()

        dataset = self.__get_default_dataset()

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

        self.__output_format_string('Throughput', self.__bitrate_to_string(results['receiver']['bitrate']))

    def test_link_metrics(self):
        """Test whether the DUT supports Link Metrics Initiator and Subject."""
        self.__dataset = self.__get_default_dataset()

        self.__dut.factory_reset()
        self.__ref.factory_reset()

        self.__dut.join(self.__dataset)
        self.__dut.wait_for('state', 'leader')

        self.__ref.join(self.__dataset)
        self.__ref.wait_for('state', ['child', 'router'])

        test_case = 'Link Metrics Initiator'
        ref_linklocal_address = self.__ref.get_ipaddr_linklocal()
        ret = self.__run_link_metrics_test_commands(initiator=self.__dut, subject_address=ref_linklocal_address)
        self.__output_format_bool(test_case, ret)

        test_case = 'Link Metrics Subject'
        dut_linklocal_address = self.__dut.get_ipaddr_linklocal()
        ret = self.__run_link_metrics_test_commands(initiator=self.__ref, subject_address=dut_linklocal_address)
        self.__output_format_bool(test_case, ret)

        self.__ref.leave()
        self.__dut.leave()

    #
    # Private methods
    #
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

    def __ref_iperf3_server_task(self, bind_address: str, timeout: int):
        self.__ref.iperf3_server(bind_address, timeout=timeout)

    def __bitrate_to_string(self, bitrate: float):
        units = ['bits/sec', 'Kbits/sec', 'Mbits/sec', 'Gbits/sec', 'Tbits/sec']
        unit_index = 0

        while bitrate >= 1000 and unit_index < len(units) - 1:
            bitrate /= 1000
            unit_index += 1

        return f'{bitrate:.2f} {units[unit_index]}'

    def __get_default_dataset(self):
        return self.__dut.create_dataset(channel=20, network_key='00112233445566778899aabbccddcafe')

    def __test_csl_transmitter(self):
        packets = 10

        self.__dut.factory_reset()
        self.__ref.factory_reset()

        self.__dut.join(self.__dataset)
        self.__dut.wait_for('state', 'leader')

        # Set the reference device as an SSED
        self.__ref.set_mode('-')
        self.__ref.config_csl(channel=15, period=320000, timeout=100)
        self.__ref.join(self.__dataset)
        self.__ref.wait_for('state', 'child')

        child_table = self.__dut.get_child_table()
        ret = len(child_table) == 1 and child_table[1]['csl']

        if ret:
            ref_mleid = self.__ref.get_ipaddr_mleid()
            result = self.__dut.ping(ref_mleid, count=packets, interval=1)
            ret = result['transmitted_packets'] == result['received_packets'] == packets

        self.__dut.leave()
        self.__ref.leave()

        self.__output_format_bool('CSL Transmitter', ret)

    def __test_data_poll_parent(self):
        packets = 10

        self.__dut.factory_reset()
        self.__ref.factory_reset()

        self.__dut.join(self.__dataset)
        self.__dut.wait_for('state', 'leader')

        # Set the reference device as an SED
        self.__ref.set_mode('-')
        self.__ref.set_poll_period(500)
        self.__ref.join(self.__dataset)
        self.__ref.wait_for('state', 'child')

        dut_mleid = self.__dut.get_ipaddr_mleid()
        result = self.__ref.ping(dut_mleid, count=packets, interval=1)

        self.__dut.leave()
        self.__ref.leave()

        ret = result['transmitted_packets'] == result['received_packets'] == packets
        self.__output_format_bool('Data Poll Parent', ret)

    def __test_data_poll_child(self):
        packets = 10

        self.__dut.factory_reset()
        self.__ref.factory_reset()

        self.__ref.join(self.__dataset)
        self.__ref.wait_for('state', 'leader')

        # Set the DUT as an SED
        self.__dut.set_mode('-')
        self.__dut.set_poll_period(500)
        self.__dut.join(self.__dataset)
        self.__dut.wait_for('state', 'child')

        dut_mleid = self.__dut.get_ipaddr_mleid()
        result = self.__ref.ping(dut_mleid, count=packets, interval=1)

        self.__dut.leave()
        self.__ref.leave()

        ret = result['transmitted_packets'] == result['received_packets'] == packets
        self.__output_format_bool('Data Poll Child', ret)

    def __test_diag_channel(self):
        channel = 20
        commands = ['diag channel', f'diag channel {channel}']

        if self.__support_commands(commands):
            self.__dut.diag_set_channel(channel)
            value = self.__dut.diag_get_channel()
            ret = value == channel
        else:
            ret = False

        self.__output_results(commands, ret)

    def __test_diag_power(self):
        power = self.__get_dut_diag_power()
        commands = ['diag power', f'diag power {power}']

        if self.__support_commands(commands):
            self.__dut.diag_set_power(power)
            value = self.__dut.diag_get_power()
            ret = value == power
        else:
            ret = False

        self.__output_results(commands, ret)

    def __test_diag_radio(self):
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

    def __test_diag_gpio_value(self):
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

    def __test_diag_gpio_mode(self):
        gpio = self.__get_dut_diag_gpio()
        commands = [f'diag gpio mode {gpio}', f'diag gpio mode {gpio} in', f'diag gpio mode {gpio} out']

        if self.__support_commands(commands):
            self.__dut.diag_set_gpio_mode(gpio, 'in')
            mode_in = self.__dut.diag_get_gpio_mode(gpio)
            self.__dut.diag_set_gpio_value(gpio, 'out')
            mode_out = self.__dut.diag_get_gpio_mode(gpio)

            ret = mode_in == 'in' and mode_out == 'out'
        else:
            ret = False

        self.__output_results(commands, ret)

    def __test_diag_echo(self):
        echo_msg = '0123456789'
        cmd_diag_echo = f'diag echo {echo_msg}'
        cmd_diag_echo_num = f'diag echo -n 10'

        if self.__dut.is_command_supported(cmd_diag_echo):
            reply = self.__dut.diag_echo(echo_msg)
            ret = reply == echo_msg
        else:
            ret = False
        self.__output_format_bool(cmd_diag_echo, ret)

        if self.__dut.is_command_supported(cmd_diag_echo_num):
            reply = self.__dut.diag_echo_number(10)
            ret = reply == echo_msg
        else:
            ret = False
        self.__output_format_bool(cmd_diag_echo_num, ret)

    def __test_diag_utils(self):
        commands = [
            'diag cw start', 'diag cw stop', 'diag stream start', 'diag stream stop', 'diag stats', 'diag stats clear'
        ]

        for command in commands:
            ret = self.__dut.is_command_supported(command)
            self.__output_format_bool(command, ret)

    def __test_diag_rawpowersetting(self):
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

    def __test_diag_powersettings(self):
        commands = ['diag powersettings', 'diag powersettings 20']

        if self.__support_commands(commands):
            powersettings = self.__dut.diag_get_powersettings()
            ret = len(powersettings) > 0
        else:
            ret = False

        self.__output_results(commands, ret)

    def __test_diag_send(self):
        packets = 100
        threshold = 80
        length = 64
        channel = 20
        commands = [f'diag send {packets} {length}', f'diag stats', f'diag stats clear']

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

            ret = dut_stats['sent_packets'] == packets and ref_stats['received_packets'] > threshold
        else:
            ret = False

        self.__output_results(commands, ret)

    def __test_diag_repeat(self):
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

            ret = dut_stats['sent_packets'] > threshold and ref_stats['received_packets'] > threshold
        else:
            ret = False

        self.__output_format_bool(cmd_diag_repeat, ret)
        self.__output_format_bool(cmd_diag_repeat_stop, ret)

    def __test_diag_frame(self):
        packets = 100
        threshold = 80
        channel = 20
        frame = '00010203040506070809'
        cmd_diag_frame = f'diag frame {frame}'
        commands = [cmd_diag_frame, f'diag send {packets}', f'diag stats', f'diag stats clear']

        if self.__support_commands(commands):
            self.__dut.wait(1)
            self.__dut.diag_set_channel(channel)
            self.__ref.diag_set_channel(channel)
            self.__ref.diag_radio_receive()

            self.__dut.diag_stats_clear()
            self.__ref.diag_stats_clear()

            self.__ref.diag_frame(frame)
            self.__dut.diag_send(packets, None)
            self.__dut.wait(1)
            dut_stats = self.__dut.diag_get_stats()
            ref_stats = self.__ref.diag_get_stats()

            ret = dut_stats['sent_packets'] == packets and ref_stats['received_packets'] > threshold
        else:
            ret = False

        self.__output_format_bool(cmd_diag_frame, ret)

    def __support_commands(self, commands: List[str]) -> bool:
        ret = True

        for command in commands:
            if self.__dut.is_command_supported(command) is False:
                ret = False
                break

        return ret

    def __output_results(self, commands: List[str], support: bool):
        for command in commands:
            self.__output_format_bool(command, support)

    def __get_dut_diag_power(self) -> int:
        return int(os.getenv('DUT_DIAG_POWER', '10'))

    def __get_dut_diag_gpio(self) -> int:
        return int(os.getenv('DUT_DIAG_GPIO', '0'))

    def __get_dut_diag_raw_power_setting(self) -> str:
        return os.getenv('DUT_DIAG_RAW_POWER_SETTING', '112233')

    def __connect_dut(self) -> OTCI:
        if os.getenv('DUT_ADB_TCP'):
            node = otci.connect_otbr_adb_tcp(os.getenv('DUT_ADB_TCP'))
        elif os.getenv('DUT_ADB_USB'):
            node = otci.connect_otbr_adb_usb(os.getenv('DUT_ADB_USB'))
        elif os.getenv('DUT_SSH'):
            node = otci.connect_otbr_ssh(os.getenv('DUT_SSH'))
        else:
            self.__fail("Please set DUT_ADB_TCP, DUT_ADB_USB or DUT_SSH to connect to the DUT device.")

        return node

    def __connect_reference_device(self) -> OTCI:
        if os.getenv('REF_CLI_SERIAL'):
            node = otci.connect_cli_serial(os.getenv('REF_CLI_SERIAL'))
        elif os.getenv('REF_SSH'):
            node = otci.connect_otbr_ssh(os.getenv('REF_SSH'))
        elif os.getenv('REF_ADB_USB'):
            node = otci.connect_otbr_adb_usb(os.getenv('REF_ADB_USB'))
        else:
            self.__fail("Please set REF_CLI_SERIAL, REF_SSH or REF_ADB_USB to connect to the reference device.")

        return node

    def __output_format_string(self, name: str, value: str):
        prefix = '{0:-<58}'.format('{} '.format(name))
        print(f'{prefix} {value}')

    def __output_format_bool(self, name: str, value: bool):
        self.__output_format_string(name, 'OK' if value else 'NotSupported')

    def __fail(self, value: str):
        print(f'{value}')
        sys.exit(1)


def parse_arguments():
    """Parse all arguments."""
    description_msg = 'This script is used for testing RCP capabilities.'
    epilog_msg = textwrap.dedent(
        'Device Interfaces:\r\n'
        '  DUT_SSH=<device_ip>            Connect to the DUT via ssh\r\n'
        '  DUT_ADB_TCP=<device_ip>        Connect to the DUT via adb tcp\r\n'
        '  DUT_ADB_USB=<serial_number>    Connect to the DUT via adb usb\r\n'
        '  REF_CLI_SERIAL=<serial_device> Connect to the reference device via cli serial port\r\n'
        '  REF_ADB_USB=<serial_number>    Connect to the reference device via adb usb\r\n'
        '  REF_SSH=<device_ip>            Connect to the reference device via ssh\r\n'
        '\r\n'
        'Example:\r\n'
        f'  DUT_ADB_USB=1169UC2F2T0M95OR REF_CLI_SERIAL=/dev/ttyACM0 python3 {sys.argv[0]} -d\r\n')

    parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter,
                                     description=description_msg,
                                     epilog=epilog_msg)

    parser.add_argument(
        '-c',
        '--csl',
        action='store_true',
        default=False,
        help='test whether the RCP supports CSL transmitter',
    )

    parser.add_argument(
        '-l',
        '--link-metrics',
        action='store_true',
        default=False,
        help='test whether the RCP supports link metrics',
    )

    parser.add_argument(
        '-d',
        '--diag-commands',
        action='store_true',
        default=False,
        help='test whether the RCP supports all diag commands',
    )

    parser.add_argument(
        '-p',
        '--data-poll',
        action='store_true',
        default=False,
        help='test whether the RCP supports data poll',
    )

    parser.add_argument(
        '-t',
        '--throughput',
        action='store_true',
        default=False,
        help='test Thread network 1-hop throughput',
    )

    parser.add_argument(
        '-v',
        '--version',
        action='store_true',
        default=False,
        help='output version',
    )

    parser.add_argument(
        '-D',
        '--debug',
        action='store_true',
        default=False,
        help='output debug information',
    )

    return parser.parse_args()


def main():
    arguments = parse_arguments()

    if arguments.version:
        print(f'Version: {CP_CAPABILITY_VERSION}')
        exit()

    if arguments.debug:
        logger = logging.getLogger()
        logger.setLevel(logging.DEBUG)

    rcp_caps = RcpCaps()

    if arguments.diag_commands:
        rcp_caps.test_diag_commands()

    if arguments.csl:
        rcp_caps.test_csl()

    if arguments.data_poll:
        rcp_caps.test_data_poll()

    if arguments.link_metrics:
        rcp_caps.test_link_metrics()

    if arguments.throughput:
        rcp_caps.test_throughput()


if __name__ == '__main__':
    main()
