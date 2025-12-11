"""
  Copyright (c) 2024-2025, The OpenThread Authors.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
  3. Neither the name of the copyright holder nor the
     names of its contributors may be used to endorse or promote products
     derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
"""

import asyncio
import argparse
import logging

from bleak import BLEDevice

from ble.ble_stream_secure import BleStreamSecure
from ble.udp_stream import UdpStream
from ble import ble_scanner
from cli.cli import CLI
from dataset.dataset import ThreadDataset
from cli.command import CommandResult
from tlv.tcat_tlv import TcatTLVType
from tlv.tlv import TLV
from utils import hexdump_ot, select_device_by_user_input, quit_with_reason

logger = logging.getLogger(__name__)
logged_modules = ['ble', 'cli', 'dataset', 'tlv', 'utils']


async def main():
    log_level = logging.WARNING
    logging.basicConfig(level=log_level)

    parser = argparse.ArgumentParser(description='Device parameters')
    parser.add_argument('-a', '--adapter', help='Select HCI adapter')
    parser.add_argument('--debug', help='Enable debug logs', action='store_true')
    parser.add_argument('--info', help='Enable info logs', action='store_true')
    parser.add_argument('--cert_path', help='Path to certificate chain and key', action='store', default='auth')
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--mac', type=str, help='Device MAC address', action='store')
    group.add_argument('--name', type=str, help='Device name', action='store')
    group.add_argument('--scan', help='Scan all available devices', action='store_true')
    group.add_argument('--simulation', help='Connect to simulation node id', action='store')
    args = parser.parse_args()

    if args.debug:
        log_level = logging.DEBUG
    elif args.info:
        log_level = logging.INFO
    logger.setLevel(log_level)
    for module in logged_modules:
        logging.getLogger(module).setLevel(log_level)

    device = await get_device_by_args(args)

    # create CLI and (if selected) connect to TCAT device
    ds = ThreadDataset()
    cli = CLI(ds, args)
    if device is not None:
        if not await cli.connect(device):
            quit_with_reason('Failed to connect to TCAT device: TLS handshake failed.')

    # Task 1: run a receiver that gets unsolicited event data or TLS Alerts from TLS server.
    receiver_task = asyncio.create_task(receive_loop(cli.context))

    # Task 2: run the CLI
    print('Enter \'help\' to see available commands or \'exit\' to exit the application.')
    loop = asyncio.get_running_loop()
    while True:
        user_input = await loop.run_in_executor(None, lambda: input('> '))
        if user_input.lower() == 'exit':
            break
        try:
            result: CommandResult = await cli.evaluate_input(user_input)
            result.pretty_print()
        except Exception as e:
            logger.error(e)
            logger.debug(e, exc_info=True)

    # Stop Task 1
    receiver_task.cancel()
    try:
        await receiver_task
    except asyncio.CancelledError:
        # CancelledError is expected when awaiting the canceled task - not an error.
        pass

    # Disconnect from TCAT device (if still needed)
    await cli.disconnect()


async def receive_loop(cli_context: dict):
    while True:
        bless: BleStreamSecure = cli_context['ble_sstream']
        if bless is not None:
            data = await bless.recv_unsolicited_event()
            if data:
                logger.info('Received event data from TCAT Device:\n' + hexdump_ot("Event", data))
                tlv = TLV.from_bytes(data)
                validate_unsolicited_tlv(tlv)
                continue
        await asyncio.sleep(0.100)


def validate_unsolicited_tlv(tlv: TLV):
    if tlv.type in [
            TcatTLVType.APPLICATION_DATA_1.value, TcatTLVType.APPLICATION_DATA_2.value,
            TcatTLVType.APPLICATION_DATA_3.value, TcatTLVType.APPLICATION_DATA_4.value
    ]:
        num = tlv.type - TcatTLVType.APPLICATION_DATA_1.value + 1
        logger.info(f"  - Send Application Data {num} {hex(tlv.type)}")
    elif tlv.type in [TcatTLVType.RESPONSE_EVENT.value]:
        logger.info(f"  - Response Event {hex(tlv.type)}")
    else:
        logger.error(f"Error: Illegal unsolicited TLV type sent by TCAT Device: {hex(tlv.type)}")


async def get_device_by_args(args) -> BLEDevice | UdpStream | None:
    device = None
    if args.mac:
        device = await ble_scanner.find_first_by_mac(args.mac)
    elif args.name:
        device = await ble_scanner.find_first_by_name(args.name)
    elif args.scan:
        tcat_devices = await ble_scanner.scan_tcat_devices(adapter=args.adapter)
        device = select_device_by_user_input(tcat_devices)
    elif args.simulation:
        device = UdpStream("127.0.0.1", int(args.simulation))

    return device


def handshake_progress_bar(is_concluded: bool):
    if is_concluded:
        print('')
    else:
        print('.', end='', flush=True)


if __name__ == '__main__':
    try:
        asyncio.run(main())
    except asyncio.CancelledError:
        pass  # device disconnected
