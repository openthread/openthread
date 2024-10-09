"""
  Copyright (c) 2024, The OpenThread Authors.
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
import os

from ble.ble_connection_constants import BBTC_SERVICE_UUID, BBTC_TX_CHAR_UUID, \
    BBTC_RX_CHAR_UUID
from ble.ble_stream import BleStream
from ble.ble_stream_secure import BleStreamSecure
from ble.udp_stream import UdpStream
from ble import ble_scanner
from cli.cli import CLI
from dataset.dataset import ThreadDataset
from cli.command import CommandResult
from utils import select_device_by_user_input, quit_with_reason

logger = logging.getLogger(__name__)


async def main():
    logging.basicConfig(level=logging.WARNING)

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
        logger.setLevel(logging.DEBUG)
        logging.getLogger('ble.ble_stream').setLevel(logging.DEBUG)
        logging.getLogger('ble.ble_stream_secure').setLevel(logging.DEBUG)
        logging.getLogger('ble.udp_stream').setLevel(logging.DEBUG)
    elif args.info:
        logger.setLevel(logging.INFO)
        logging.getLogger('ble.ble_stream').setLevel(logging.INFO)
        logging.getLogger('ble.ble_stream_secure').setLevel(logging.INFO)
        logging.getLogger('ble.udp_stream').setLevel(logging.INFO)

    device = await get_device_by_args(args)

    ble_sstream = None

    if device is not None:
        print(f'Connecting to {device}')
        ble_sstream = BleStreamSecure(device)
        ble_sstream.load_cert(
            certfile=os.path.join(args.cert_path, 'commissioner_cert.pem'),
            keyfile=os.path.join(args.cert_path, 'commissioner_key.pem'),
            cafile=os.path.join(args.cert_path, 'ca_cert.pem'),
        )
        logger.info(f"Certificates and key loaded from '{args.cert_path}'")

        print('Setting up secure TLS channel..', end='')
        try:
            await ble_sstream.do_handshake()
            print('Done')
        except Exception as e:
            print('Failed')
            logger.error(e)
            quit_with_reason('TLS handshake failure')

    ds = ThreadDataset()
    cli = CLI(ds, args, ble_sstream)
    loop = asyncio.get_running_loop()
    print('Enter \'help\' to see available commands' ' or \'exit\' to exit the application.')
    while True:
        user_input = await loop.run_in_executor(None, lambda: input('> '))
        if user_input.lower() == 'exit':
            break
        try:
            result: CommandResult = await cli.evaluate_input(user_input)
            if result:
                result.pretty_print()
        except Exception as e:
            logger.error(e)

    print('Disconnecting...')
    if ble_sstream is not None:
        await ble_sstream.close()


async def get_device_by_args(args):
    device = None
    if args.mac:
        device = await ble_scanner.find_first_by_mac(args.mac)
        device = await BleStream.create(device.address, BBTC_SERVICE_UUID, BBTC_TX_CHAR_UUID, BBTC_RX_CHAR_UUID)
    elif args.name:
        device = await ble_scanner.find_first_by_name(args.name)
        device = await BleStream.create(device.address, BBTC_SERVICE_UUID, BBTC_TX_CHAR_UUID, BBTC_RX_CHAR_UUID)
    elif args.scan:
        tcat_devices = await ble_scanner.scan_tcat_devices(adapter=args.adapter)
        device = select_device_by_user_input(tcat_devices)
        if device:
            device = await BleStream.create(device, BBTC_SERVICE_UUID, BBTC_TX_CHAR_UUID, BBTC_RX_CHAR_UUID)
    elif args.simulation:
        device = UdpStream("127.0.0.1", int(args.simulation))

    return device


if __name__ == '__main__':
    try:
        asyncio.run(main())
    except asyncio.CancelledError:
        pass  # device disconnected
