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

from abc import abstractmethod
from hashlib import sha256
import hmac
import logging
from os import path
from secrets import token_bytes
from time import time

from bleak import BLEDevice

from ble.ble_connection_constants import BBTC_SERVICE_UUID, BBTC_TX_CHAR_UUID, BBTC_RX_CHAR_UUID
from ble.ble_stream import BleStream
from ble.ble_stream_secure import BleStreamSecure
from ble import ble_scanner
from ble.udp_stream import UdpStream
from cli.command import Command, CommandResultNone, CommandResultTLV, CommandResult, CommandResultError
from dataset.dataset import ThreadDataset
from tlv.tlv import TLV
from tlv.diagnostic_tlv import DiagnosticTLVType
from tlv.tcat_tlv import TcatTLVType
from utils import select_device_by_user_input

CHALLENGE_SIZE = 8

logger = logging.getLogger(__name__)


class HelpCommand(Command):

    def get_help_string(self) -> str:
        return 'Display help and return.'

    async def execute_default(self, args, context) -> CommandResult:
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
    def prepare_data(self, args, context) -> bytes:
        pass

    async def execute_default(self, args, context) -> CommandResult:
        if context['ble_sstream'] is None or not context['ble_sstream'].is_connected:
            return CommandResultError("TCAT Device not connected.")
        bless: BleStreamSecure = context['ble_sstream']

        print(self.get_log_string())
        try:
            data = self.prepare_data(args, context)
            response = await bless.send_with_resp(data)
            if not response:
                return CommandResultNone()
            tlv_response = TLV.from_bytes(response)
            self.process_response(tlv_response, context)
            return CommandResultTLV(tlv_response)
        except DataNotPrepared as err:
            return CommandResultError(f'Command failed: {err}')

    def process_response(self, tlv_response, context):
        pass


class HelloCommand(BleCommand):

    def get_log_string(self) -> str:
        return 'Sending hello world...'

    def get_help_string(self) -> str:
        return 'Send round trip "Hello world!" message.'

    def prepare_data(self, args, context) -> bytes:
        return TLV(TcatTLVType.VENDOR_APPLICATION.value, bytes('Hello world!', 'ascii')).to_bytes()


class GetApplicationLayersCommand(BleCommand):

    def get_log_string(self) -> str:
        return 'Getting application layers....'

    def get_help_string(self) -> str:
        return 'Get supported application layer service names from device.'

    def prepare_data(self, args, context) -> bytes:
        return TLV(TcatTLVType.GET_APPLICATION_LAYERS.value, bytes()).to_bytes()

    def process_response(self, tlv_response, context):
        if tlv_response.type == TcatTLVType.RESPONSE_W_PAYLOAD.value:
            payload = tlv_response.value
            i = 0
            print('Service names:')
            while payload:
                tlv_application = TLV.from_bytes(payload)
                payload = payload[2 + len(tlv_application.value):]
                i += 1
                if (tlv_application.type == TcatTLVType.SERVICE_NAME_UDP.value):
                    print(f"\tApplication {i} is UDP service: {tlv_application.value.decode('ascii')}")
                elif (tlv_application.type == TcatTLVType.SERVICE_NAME_TCP.value):
                    print(f"\tApplication {i} is TCP service: {tlv_application.value.decode('ascii')}")
                else:
                    print('\tUnknown service type.')
        else:
            print('Dataset extraction error.')


class SendApplicationData1(BleCommand):

    def get_log_string(self) -> str:
        return 'Sending data to application layer 1....'

    def get_help_string(self) -> str:
        return 'Send hex encoded data to application layer 1.'

    def prepare_data(self, args, context) -> bytes:
        payload = bytes.fromhex(args[0])
        return TLV(TcatTLVType.APPLICATION_DATA_1.value, payload).to_bytes()


class SendApplicationData2(BleCommand):

    def get_log_string(self) -> str:
        return 'Sending data to application layer 2....'

    def get_help_string(self) -> str:
        return 'Send hex encoded data to application layer 2.'

    def prepare_data(self, args, context) -> bytes:
        payload = bytes.fromhex(args[0])
        return TLV(TcatTLVType.APPLICATION_DATA_2.value, payload).to_bytes()


class SendApplicationData3(BleCommand):

    def get_log_string(self) -> str:
        return 'Sending data to application layer 3....'

    def get_help_string(self) -> str:
        return 'Send hex encoded data to application layer 3.'

    def prepare_data(self, args, context) -> bytes:
        payload = bytes.fromhex(args[0])
        return TLV(TcatTLVType.APPLICATION_DATA_3.value, payload).to_bytes()


class SendApplicationData4(BleCommand):

    def get_log_string(self) -> str:
        return 'Sending data to application layer 4....'

    def get_help_string(self) -> str:
        return 'Send hex encoded data to application layer 4.'

    def prepare_data(self, args, context) -> bytes:
        payload = bytes.fromhex(args[0])
        return TLV(TcatTLVType.APPLICATION_DATA_4.value, payload).to_bytes()


class SendVendorData(BleCommand):

    def get_log_string(self) -> str:
        return 'Sending data to vendor specific application layer....'

    def get_help_string(self) -> str:
        return 'Send hex encoded data to vendor specific application layer.'

    def prepare_data(self, args, context) -> bytes:
        payload = bytes.fromhex(args[0])
        return TLV(TcatTLVType.VENDOR_APPLICATION.value, payload).to_bytes()


class CommissionCommand(BleCommand):

    def get_log_string(self) -> str:
        return 'Commissioning...'

    def get_help_string(self) -> str:
        return 'Update the connected device with current dataset.'

    def prepare_data(self, args, context) -> bytes:
        dataset: ThreadDataset = context['dataset']
        dataset_bytes = dataset.to_bytes()
        return TLV(TcatTLVType.ACTIVE_DATASET.value, dataset_bytes).to_bytes()


class DecommissionCommand(BleCommand):

    def get_log_string(self) -> str:
        return 'Disabling Thread and decommissioning device...'

    def get_help_string(self) -> str:
        return 'Stop Thread interface and decommission device from current network.'

    def prepare_data(self, args, context) -> bytes:
        return TLV(TcatTLVType.DECOMMISSION.value, bytes()).to_bytes()


class DisconnectCommand(Command):

    def get_help_string(self) -> str:
        return 'Disconnect client from TCAT device'

    async def execute_default(self, args, context) -> CommandResult:
        await disconnect_helper(context)
        return CommandResultNone()


class ExtractDatasetCommand(BleCommand):

    def get_log_string(self) -> str:
        return 'Getting active dataset.'

    def get_help_string(self) -> str:
        return 'Get active dataset from device.'

    def prepare_data(self, args, context) -> bytes:
        return TLV(TcatTLVType.GET_ACTIVE_DATASET.value, bytes()).to_bytes()

    def process_response(self, tlv_response, context) -> None:
        if tlv_response.type == TcatTLVType.RESPONSE_W_PAYLOAD.value:
            dataset = ThreadDataset()
            dataset.set_from_bytes(tlv_response.value)
            dataset.print_content()
        else:
            print('Dataset extraction error.')


class GetCommissionerCertificate(BleCommand):

    def get_log_string(self) -> str:
        return 'Getting commissioner certificate.'

    def get_help_string(self) -> str:
        return 'Get last-store TCAT commissioner certificate from TCAT device.'

    def prepare_data(self, args, context) -> bytes:
        return TLV(TcatTLVType.GET_COMMISSIONER_CERTIFICATE.value, bytes()).to_bytes()


class GetDeviceIdCommand(BleCommand):

    def get_log_string(self) -> str:
        return 'Retrieving device id.'

    def get_help_string(self) -> str:
        return 'Get unique identifier for the TCAT device.'

    def prepare_data(self, args, context) -> bytes:
        return TLV(TcatTLVType.GET_DEVICE_ID.value, bytes()).to_bytes()


class GetExtPanIDCommand(BleCommand):

    def get_log_string(self) -> str:
        return 'Retrieving extended PAN ID.'

    def get_help_string(self) -> str:
        return 'Get extended PAN ID that is commissioned in the active dataset.'

    def prepare_data(self, args, context) -> bytes:
        return TLV(TcatTLVType.GET_EXT_PAN_ID.value, bytes()).to_bytes()


class GetProvisioningUrlCommand(BleCommand):

    def get_log_string(self) -> str:
        return 'Retrieving provisioning url.'

    def get_help_string(self) -> str:
        return 'Get a URL for an application suited to commission the TCAT device.'

    def prepare_data(self, args, context) -> bytes:
        return TLV(TcatTLVType.GET_PROVISIONING_URL.value, bytes()).to_bytes()


class GetNetworkNameCommand(BleCommand):

    def get_log_string(self) -> str:
        return 'Retrieving network name.'

    def get_help_string(self) -> str:
        return 'Get the Thread network name that is commissioned in the active dataset.'

    def prepare_data(self, args, context) -> bytes:
        return TLV(TcatTLVType.GET_NETWORK_NAME.value, bytes()).to_bytes()


class GetPskdHash(BleCommand):

    def __init__(self):
        super().__init__()
        self.digest = None

    def get_log_string(self) -> str:
        return 'Retrieving peer PSKd hash.'

    def get_help_string(self) -> str:
        return 'Get calculated PSKd hash.'

    def prepare_data(self, args, context) -> bytes:
        bless: BleStreamSecure = context['ble_sstream']
        if bless.peer_public_key is None:
            raise DataNotPrepared("Peer certificate not present.")

        challenge = token_bytes(CHALLENGE_SIZE)
        pskd = bytes(args[0], 'utf-8')

        data = TLV(TcatTLVType.GET_PSKD_HASH.value, challenge).to_bytes()

        hash = hmac.new(pskd, digestmod=sha256)
        hash.update(challenge)
        hash.update(bless.peer_public_key)
        self.digest = hash.digest()
        return data

    def process_response(self, tlv_response, context) -> None:
        if tlv_response.value == self.digest:
            print('Requested hash is valid.')
        else:
            print('Requested hash is NOT valid.')


class GetRandomNumberChallenge(BleCommand):

    def get_log_string(self) -> str:
        return 'Retrieving random challenge.'

    def get_help_string(self) -> str:
        return 'Get the device random number challenge.'

    def prepare_data(self, args, context) -> bytes:
        return TLV(TcatTLVType.GET_RANDOM_NUMBER_CHALLENGE.value, bytes()).to_bytes()

    def process_response(self, tlv_response, context) -> None:
        bless: BleStreamSecure = context['ble_sstream']
        if tlv_response.value is not None:
            if len(tlv_response.value) == CHALLENGE_SIZE:
                bless.peer_challenge = tlv_response.value
            else:
                print('Challenge format invalid.')


class PingCommand(Command):

    def get_help_string(self) -> str:
        return 'Send echo request to TCAT device.'

    async def execute_default(self, args, context) -> CommandResult:
        bless: BleStreamSecure = context['ble_sstream']
        if bless is None or not bless.is_connected:
            return CommandResultError("TCAT Device not connected.")
        payload_size = 10
        max_payload = 512
        if len(args) > 0:
            payload_size = int(args[0])
            if payload_size > max_payload:
                return CommandResultError(f'Payload size too large. Maximum supported value is {max_payload}')
        to_send = token_bytes(payload_size)
        data = TLV(TcatTLVType.PING.value, to_send).to_bytes()
        elapsed_time = time()
        response = await bless.send_with_resp(data)
        elapsed_time = 1e3 * (time() - elapsed_time)
        if not response:
            return CommandResultNone()

        tlv_response = TLV.from_bytes(response)
        if tlv_response.value != to_send:
            print("Error: Received malformed response.")

        print(f"Roundtrip time: {elapsed_time} ms")

        return CommandResultTLV(tlv_response)


class PresentHash(BleCommand):

    def get_log_string(self) -> str:
        return 'Presenting hash.'

    def get_help_string(self) -> str:
        return 'Present calculated hash.'

    def prepare_data(self, args, context) -> bytes:
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


def _handshake_progress_bar(is_concluded: bool):
    if is_concluded:
        print('')
    else:
        print('.', end='', flush=True)


async def connect_helper(device: BLEDevice | UdpStream,
                         context: dict,
                         timeout_ble=30.0,
                         timeout_simulation=5.0) -> bool:
    """Helper function for CLI and commands to establish a new secure connection with a TCAT device.

    Handles both BLE and simulated UDP connections. Loads certificates and performs handshake
    to establish a secure channel. Connection objects are stored in the context dictionary.

    Args:
        device: BLEDevice object, or UdpStream (for simulation)
        context: Dictionary containing application context including command line arguments
        timeout_ble: Timeout in seconds for handshake with real TCAT device (default: 30.0)
        timeout_simulation: Timeout in seconds for handshake for simulated TCAT device (default: 5.0)

    Returns:
        True if connection was successful, False otherwise.

    Raises:
        Exception: If certificates cannot be loaded
    """
    is_simulation = isinstance(device, UdpStream)
    is_debug = logger.getEffectiveLevel() <= logging.DEBUG

    print(f'Connecting to {device}')
    if not is_simulation:
        ble_stream = await BleStream.create(device.address, BBTC_SERVICE_UUID, BBTC_TX_CHAR_UUID, BBTC_RX_CHAR_UUID)
        ble_sstream = BleStreamSecure(ble_stream)
    else:
        ble_stream = None
        ble_sstream = BleStreamSecure(device)
    context['ble_sstream'] = ble_sstream
    context['ble_stream'] = ble_stream

    cert_path = context['cmd_args'].cert_path if context['cmd_args'] else 'auth'
    ble_sstream.load_cert(
        certfile=path.join(cert_path, 'commissioner_cert.pem'),
        keyfile=path.join(cert_path, 'commissioner_key.pem'),
        cafile=path.join(cert_path, 'ca_cert.pem'),
    )
    logger.info(f"Certificates and key loaded from '{cert_path}'")

    print('Setting up secure channel...')
    ok = False
    try:
        cb = None
        if not is_debug:
            cb = _handshake_progress_bar
        ok = await ble_sstream.do_handshake(progress_callback=cb,
                                            timeout=timeout_simulation if is_simulation else timeout_ble)
    except Exception as e:
        logger.error(e)

    if ok:
        print('Done')
        return True
    else:
        if ble_stream is not None:
            await ble_stream.disconnect()
        context['ble_stream'] = None
        context['ble_sstream'] = None
        return False


async def disconnect_helper(context: dict) -> None:
    """Helper function for CLI and commands to disconnect from a TCAT device."""
    bless: BleStreamSecure = context['ble_sstream']
    doing_disconn = False
    if bless is not None and bless.is_connected:
        print('Disconnecting...')
        doing_disconn = True
        logger.debug('Closing TLS connection.')
        await bless.close()
    context['ble_sstream'] = None

    bles: BleStream = context['ble_stream']
    if bles is not None:
        logger.debug('Closing BLE connection.')
        await bles.disconnect()
    context['ble_stream'] = None
    if doing_disconn:
        print('Done')


class ScanCommand(Command):

    def get_help_string(self) -> str:
        return 'Perform scan for TCAT devices.'

    async def execute_default(self, args, context) -> CommandResult:
        if context['ble_sstream'] is not None and context['ble_sstream'].is_connected:
            return CommandResultError('already connected to a TCAT device. Use \'disconnect\' first.')

        print('Scanning for BLE TCAT devices...')
        tcat_devices = await ble_scanner.scan_tcat_devices()
        device = select_device_by_user_input(tcat_devices)
        if device is not None:
            await connect_helper(device, context)

        return CommandResultNone()


class SimulationCommand(Command):

    def get_help_string(self) -> str:
        return 'Connect to a simulated TCAT device over UDP.'

    async def execute_default(self, args, context) -> CommandResult:
        if len(args) != 1:
            return CommandResultError('need index number of simulated TCAT device as first argument.')
        if context['ble_sstream'] is not None and context['ble_sstream'].is_connected:
            return CommandResultError('already connected to a TCAT device. Use \'disconnect\' first.')

        device = UdpStream("127.0.0.1", int(args[0]))
        await connect_helper(device, context)
        return CommandResultNone()


class DiagnosticTlvsCommand(BleCommand):

    def get_log_string(self) -> str:
        return 'Retrieving diagnostic information.'

    def get_help_string(self) -> str:
        return 'Get diagnostic TLVs from the TCAT device.'

    def prepare_data(self, args, context) -> bytes:
        num_args = DiagnosticTLVType.names_to_numbers(args)
        try:
            if not num_args:
                raise ValueError()
            vals = [int(x) for x in num_args]
            tlvs = bytes(vals)
        except ValueError:
            print('Please provide a list of diagnostic TLV types as names or numbers')
            print('TLV Types:')
            for key, value in DiagnosticTLVType.get_dict().items():
                print(f'{key} = {value},')
            raise DataNotPrepared()

        return TLV(TcatTLVType.GET_DIAGNOSTIC_TLVS.value, tlvs).to_bytes()


class ThreadStartCommand(BleCommand):

    def get_log_string(self) -> str:
        return 'Enabling Thread...'

    def get_help_string(self) -> str:
        return 'Enable thread interface.'

    def prepare_data(self, args, context) -> bytes:
        return TLV(TcatTLVType.THREAD_START.value, bytes()).to_bytes()


class ThreadStopCommand(BleCommand):

    def get_log_string(self) -> str:
        return 'Disabling Thread...'

    def get_help_string(self) -> str:
        return 'Disable thread interface.'

    def prepare_data(self, args, context) -> bytes:
        return TLV(TcatTLVType.THREAD_STOP.value, bytes()).to_bytes()


class ThreadStateCommand(Command):

    def __init__(self):
        super().__init__()
        self._subcommands = {'start': ThreadStartCommand(), 'stop': ThreadStopCommand()}

    def get_help_string(self) -> str:
        return 'Manipulate state of the Thread interface of the connected device.'

    async def execute_default(self, args, context) -> CommandResult:
        print('Invalid usage. Provide a subcommand.')
        return CommandResultNone()
