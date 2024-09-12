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

from abc import abstractmethod
from ble.ble_connection_constants import BBTC_SERVICE_UUID, BBTC_TX_CHAR_UUID, \
    BBTC_RX_CHAR_UUID
from ble.ble_stream import BleStream
from ble.ble_stream_secure import BleStreamSecure
from ble import ble_scanner
from tlv.tlv import TLV
from tlv.tcat_tlv import TcatTLVType
from cli.command import Command, CommandResultNone, CommandResultTLV
from dataset.dataset import ThreadDataset
from utils import select_device_by_user_input
from os import path
from time import time
from secrets import token_bytes
from hashlib import sha256
import hmac
import binascii


class HelpCommand(Command):

    def get_help_string(self) -> str:
        return 'Display help and return.'

    async def execute_default(self, args, context):
        commands = context['commands']
        for name, command in commands.items():
            print(f'{name}')
            command.print_help(indent=1)
        return CommandResultNone()


class DataNotPrepared(Exception):
    pass


class BleCommand(Command):

    @abstractmethod
    def get_log_string(self) -> str:
        pass

    @abstractmethod
    def prepare_data(self, args, context):
        pass

    async def execute_default(self, args, context):
        bless: BleStreamSecure = context['ble_sstream']

        print(self.get_log_string())
        try:
            data = self.prepare_data(args, context)
            response = await bless.send_with_resp(data)
            if not response:
                return
            tlv_response = TLV.from_bytes(response)
            return CommandResultTLV(tlv_response)
        except DataNotPrepared as err:
            print('Command failed', err)


class HelloCommand(BleCommand):

    def get_log_string(self) -> str:
        return 'Sending hello world...'

    def get_help_string(self) -> str:
        return 'Send round trip "Hello world!" message.'

    def prepare_data(self, args, context):
        return TLV(TcatTLVType.APPLICATION.value, bytes('Hello world!', 'ascii')).to_bytes()


class CommissionCommand(BleCommand):

    def get_log_string(self) -> str:
        return 'Commissioning...'

    def get_help_string(self) -> str:
        return 'Update the connected device with current dataset.'

    def prepare_data(self, args, context):
        dataset: ThreadDataset = context['dataset']
        dataset_bytes = dataset.to_bytes()
        return TLV(TcatTLVType.ACTIVE_DATASET.value, dataset_bytes).to_bytes()


class DecommissionCommand(BleCommand):

    def get_log_string(self) -> str:
        return 'Disabling Thread and decommissioning device...'

    def get_help_string(self) -> str:
        return 'Stop Thread interface and decommission device from current network.'

    def prepare_data(self, args, context):
        return TLV(TcatTLVType.DECOMMISSION.value, bytes()).to_bytes()


class GetDeviceIdCommand(BleCommand):

    def get_log_string(self) -> str:
        return 'Retrieving device id.'

    def get_help_string(self) -> str:
        return 'Get unique identifier for the TCAT device.'

    def prepare_data(self, args, context):
        return TLV(TcatTLVType.GET_DEVICE_ID.value, bytes()).to_bytes()


class GetExtPanIDCommand(BleCommand):

    def get_log_string(self) -> str:
        return 'Retrieving extended PAN ID.'

    def get_help_string(self) -> str:
        return 'Get extended PAN ID that is commissioned in the active dataset.'

    def prepare_data(self, args, context):
        return TLV(TcatTLVType.GET_EXT_PAN_ID.value, bytes()).to_bytes()


class GetProvisioningUrlCommand(BleCommand):

    def get_log_string(self) -> str:
        return 'Retrieving provisioning url.'

    def get_help_string(self) -> str:
        return 'Get a URL for an application suited to commission the TCAT device.'

    def prepare_data(self, args, context):
        return TLV(TcatTLVType.GET_PROVISIONING_URL.value, bytes()).to_bytes()


class GetNetworkNameCommand(BleCommand):

    def get_log_string(self) -> str:
        return 'Retrieving network name.'

    def get_help_string(self) -> str:
        return 'Get the Thread network name that is commissioned in the active dataset.'

    def prepare_data(self, args, context):
        return TLV(TcatTLVType.GET_NETWORK_NAME.value, bytes()).to_bytes()


class PresentHash(BleCommand):

    def get_log_string(self) -> str:
        return 'Presenting hash.'

    def get_help_string(self) -> str:
        return 'Present calculated hash.'

    def prepare_data(self, args, context):
        type = args[0]
        code = None
        tlv_type = None
        if type == "pskd":
            code = bytes(args[1], 'utf-8')
            tlv_type = TcatTLVType.PRESENT_PSKD_HASH.value
        elif type == "pskc":
            code = bytes.fromhex(args[1])
            tlv_type = TcatTLVType.PRESENT_PSKC_HASH.value
        elif type == "install":
            code = bytes(args[1], 'utf-8')
            tlv_type = TcatTLVType.PRESENT_INSTALL_CODE_HASH.value
        else:
            raise DataNotPrepared("Hash code name incorrect.")
        bless: BleStreamSecure = context['ble_sstream']
        if bless.peer_public_key is None:
            raise DataNotPrepared("Peer certificate not present.")

        if bless.peer_challenge is None:
            raise DataNotPrepared("Peer challenge not present.")

        hash = hmac.new(code, digestmod=sha256)
        hash.update(bless.peer_challenge)
        hash.update(bless.peer_public_key)

        data = TLV(tlv_type, hash.digest()).to_bytes()
        return data


class GetPskdHash(Command):

    def get_log_string(self) -> str:
        return 'Retrieving peer PSKd hash.'

    def get_help_string(self) -> str:
        return 'Get calculated PSKd hash.'

    async def execute_default(self, args, context):
        bless: BleStreamSecure = context['ble_sstream']

        print(self.get_log_string())
        try:
            if bless.peer_public_key is None:
                print("Peer certificate not present.")
                return
            challenge_size = 8
            challenge = token_bytes(challenge_size)
            pskd = bytes(args[0], 'utf-8')
            data = TLV(TcatTLVType.GET_PSKD_HASH.value, challenge).to_bytes()
            response = await bless.send_with_resp(data)
            if not response:
                return
            tlv_response = TLV.from_bytes(response)
            if tlv_response.value != None:
                hash = hmac.new(pskd, digestmod=sha256)
                hash.update(challenge)
                hash.update(bless.peer_public_key)
                digest = hash.digest()
                if digest == tlv_response.value:
                    print('Requested hash is valid.')
                else:
                    print('Requested hash is NOT valid.')
            return CommandResultTLV(tlv_response)
        except DataNotPrepared as err:
            print('Command failed', err)


class GetRandomNumberChallenge(Command):

    def get_log_string(self) -> str:
        return 'Retrieving random challenge.'

    def get_help_string(self) -> str:
        return 'Get the device random number challenge.'

    async def execute_default(self, args, context):
        bless: BleStreamSecure = context['ble_sstream']

        print(self.get_log_string())
        try:
            data = TLV(TcatTLVType.GET_RANDOM_NUMBER_CHALLENGE.value, bytes()).to_bytes()
            response = await bless.send_with_resp(data)
            if not response:
                return
            tlv_response = TLV.from_bytes(response)
            if tlv_response.value != None:
                if len(tlv_response.value) == 8:
                    bless.peer_challenge = tlv_response.value
                else:
                    print('Challenge format invalid.')
                    return CommandResultNone()
            return CommandResultTLV(tlv_response)
        except DataNotPrepared as err:
            print('Command failed', err)


class PingCommand(Command):

    def get_help_string(self) -> str:
        return 'Send echo request to TCAT device.'

    async def execute_default(self, args, context):
        bless: BleStreamSecure = context['ble_sstream']
        payload_size = 10
        max_payload = 512
        if len(args) > 0:
            payload_size = int(args[0])
            if payload_size > max_payload:
                print(f'Payload size too large. Maximum supported value is {max_payload}')
                return
        to_send = token_bytes(payload_size)
        data = TLV(TcatTLVType.PING.value, to_send).to_bytes()
        elapsed_time = time()
        response = await bless.send_with_resp(data)
        elapsed_time = 1e3 * (time() - elapsed_time)
        if not response:
            return

        tlv_response = TLV.from_bytes(response)
        if tlv_response.value != to_send:
            print("Received malformed response.")

        print(f"Roundtrip time: {elapsed_time} ms")

        return CommandResultTLV(tlv_response)


class ThreadStartCommand(BleCommand):

    def get_log_string(self) -> str:
        return 'Enabling Thread...'

    def get_help_string(self) -> str:
        return 'Enable thread interface.'

    def prepare_data(self, args, context):
        return TLV(TcatTLVType.THREAD_START.value, bytes()).to_bytes()


class ThreadStopCommand(BleCommand):

    def get_log_string(self) -> str:
        return 'Disabling Thread...'

    def get_help_string(self) -> str:
        return 'Disable thread interface.'

    def prepare_data(self, args, context):
        return TLV(TcatTLVType.THREAD_STOP.value, bytes()).to_bytes()


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
        cert_path = context['cmd_args'].cert_path if context['cmd_args'] else 'auth'
        ble_sstream.load_cert(
            certfile=path.join(cert_path, 'commissioner_cert.pem'),
            keyfile=path.join(cert_path, 'commissioner_key.pem'),
            cafile=path.join(cert_path, 'ca_cert.pem'),
        )
        print('Setting up secure channel...')
        if await ble_sstream.do_handshake():
            print('Done')
            context['ble_sstream'] = ble_sstream
        else:
            print('Secure channel not established.')
            await ble_stream.disconnect()
