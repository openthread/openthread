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

from ble.ble_connection_constants import BBTC_SERVICE_UUID, BBTC_TX_CHAR_UUID, \
    BBTC_RX_CHAR_UUID, SERVER_COMMON_NAME
from ble.ble_stream import BleStream
from ble.ble_stream_secure import BleStreamSecure
from ble import ble_scanner
from tlv.tlv import TLV
from tlv.tcat_tlv import TcatTLVType
from cli.command import Command, CommandResultNone, CommandResultTLV
from dataset.dataset import ThreadDataset
from utils import select_device_by_user_input
from os import path


class HelpCommand(Command):

    def get_help_string(self) -> str:
        return 'Display help and return.'

    async def execute_default(self, args, context):
        commands = context['commands']
        for name, command in commands.items():
            print(f'{name}')
            command.print_help(indent=1)
        return CommandResultNone()


class HelloCommand(Command):

    def get_help_string(self) -> str:
        return 'Send round trip "Hello world!" message.'

    async def execute_default(self, args, context):
        bless: BleStreamSecure = context['ble_sstream']
        print('Sending hello world...')
        data = TLV(TcatTLVType.APPLICATION.value, bytes('Hello world!', 'ascii')).to_bytes()
        response = await bless.send_with_resp(data)
        if not response:
            return
        tlv_response = TLV.from_bytes(response)
        return CommandResultTLV(tlv_response)


class CommissionCommand(Command):

    def get_help_string(self) -> str:
        return 'Update the connected device with current dataset.'

    async def execute_default(self, args, context):
        bless: BleStreamSecure = context['ble_sstream']
        dataset: ThreadDataset = context['dataset']

        print('Commissioning...')
        dataset_bytes = dataset.to_bytes()
        data = TLV(TcatTLVType.ACTIVE_DATASET.value, dataset_bytes).to_bytes()
        response = await bless.send_with_resp(data)
        if not response:
            return
        tlv_response = TLV.from_bytes(response)
        return CommandResultTLV(tlv_response)


class ThreadStartCommand(Command):

    def get_help_string(self) -> str:
        return 'Enable thread interface.'

    async def execute_default(self, args, context):
        bless: BleStreamSecure = context['ble_sstream']

        print('Enabling Thread...')
        data = TLV(TcatTLVType.THREAD_START.value, bytes()).to_bytes()
        response = await bless.send_with_resp(data)
        if not response:
            return
        tlv_response = TLV.from_bytes(response)
        return CommandResultTLV(tlv_response)


class ThreadStopCommand(Command):

    def get_help_string(self) -> str:
        return 'Disable thread interface.'

    async def execute_default(self, args, context):
        bless: BleStreamSecure = context['ble_sstream']
        print('Disabling Thread...')
        data = TLV(TcatTLVType.THREAD_STOP.value, bytes()).to_bytes()
        response = await bless.send_with_resp(data)
        if not response:
            return
        tlv_response = TLV.from_bytes(response)
        return CommandResultTLV(tlv_response)


class ThreadStateCommand(Command):

    def __init__(self):
        self._subcommands = {'start': ThreadStartCommand(), 'stop': ThreadStopCommand()}

    def get_help_string(self) -> str:
        return 'Manipulate state of the Thread interface of the connected device.'

    async def execute_default(self, args, context):
        print('Invalid usage. Provide a subcommand.')
        return CommandResultNone()


class ScanCommand(Command):

    def get_help_string(self) -> str:
        return 'Perform scan for TCAT devices.'

    async def execute_default(self, args, context):
        if not (context['ble_sstream'] is None):
            del context['ble_sstream']

        tcat_devices = await ble_scanner.scan_tcat_devices()
        device = select_device_by_user_input(tcat_devices)

        if device is None:
            return CommandResultNone()

        ble_sstream = None

        print(f'Connecting to {device}')
        ble_stream = await BleStream.create(device.address, BBTC_SERVICE_UUID, BBTC_TX_CHAR_UUID, BBTC_RX_CHAR_UUID)
        ble_sstream = BleStreamSecure(ble_stream)
        ble_sstream.load_cert(
            certfile=path.join('auth', 'commissioner_cert.pem'),
            keyfile=path.join('auth', 'commissioner_key.pem'),
            cafile=path.join('auth', 'ca_cert.pem'),
        )

        print('Setting up secure channel...')
        await ble_sstream.do_handshake(hostname=SERVER_COMMON_NAME)
        print('Done')
        context['ble_sstream'] = ble_sstream
