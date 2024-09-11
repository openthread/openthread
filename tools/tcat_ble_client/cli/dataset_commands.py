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

from cli.command import Command, CommandResultNone
from dataset.dataset import ThreadDataset, initial_dataset
from tlv.dataset_tlv import MeshcopTlvType
from copy import deepcopy


def handle_dataset_entry_command(type: MeshcopTlvType, args, context):
    ds: ThreadDataset = context['dataset']
    if len(args) == 0:
        ds.get_entry(type).print_content()
        return CommandResultNone()

    ds.set_entry(type, args)
    print('Done.')
    return CommandResultNone()


class DatasetClearCommand(Command):

    def get_help_string(self) -> str:
        return 'Clear dataset.'

    async def execute_default(self, args, context):
        ds: ThreadDataset = context['dataset']
        ds.clear()
        return CommandResultNone()


class DatasetHelpCommand(Command):

    def get_help_string(self) -> str:
        return 'Display help message and return.'

    async def execute_default(self, args, context):
        indent_width = 4
        indentation = ' ' * indent_width
        commands: ThreadDataset = context['commands']
        ds_command: Command = commands['dataset']
        print(ds_command.get_help_string())
        print('Subcommands:')
        for name, subcommand in ds_command._subcommands.items():
            print(f'{indentation}{name}')
            print(f'{indentation}{" " * indent_width}{subcommand.get_help_string()}')
        return CommandResultNone()


class DatasetHexCommand(Command):

    def get_help_string(self) -> str:
        return 'Get or set dataset as hex-encoded TLVs.'

    async def execute_default(self, args, context):
        ds: ThreadDataset = context['dataset']
        if args:
            try:
                ds_tmp = deepcopy(ds)
                tlvs_str: str = args[0]
                tlvs_bytes = bytes.fromhex(tlvs_str)
                ds_tmp.set_from_bytes(tlvs_bytes)
                ds.clear()
                ds.set_from_bytes(tlvs_bytes)
            except Exception as e:
                print(e)
        print(ds.to_bytes().hex())
        return CommandResultNone()


class ReloadDatasetCommand(Command):

    def get_help_string(self) -> str:
        return 'Reset dataset to the initial value.'

    async def execute_default(self, args, context):
        context['dataset'].set_from_bytes(initial_dataset)
        return CommandResultNone()


class ActiveTimestampCommand(Command):

    def get_help_string(self) -> str:
        return 'View and set ActiveTimestamp seconds. Arguments: [seconds (int)]'

    async def execute_default(self, args, context):
        return handle_dataset_entry_command(MeshcopTlvType.ACTIVETIMESTAMP, args, context)


class PendingTimestampCommand(Command):

    def get_help_string(self) -> str:
        return 'View and set PendingTimestamp seconds. Arguments: [seconds (int)]'

    async def execute_default(self, args, context):
        return handle_dataset_entry_command(MeshcopTlvType.PENDINGTIMESTAMP, args, context)


class NetworkKeyCommand(Command):

    def get_help_string(self) -> str:
        return 'View and set NetworkKey. Arguments: [nk (hexstring, len=32)]'

    async def execute_default(self, args, context):
        return handle_dataset_entry_command(MeshcopTlvType.NETWORKKEY, args, context)


class NetworkNameCommand(Command):

    def get_help_string(self) -> str:
        return 'View and set NetworkName. Arguments: [nn (string, maxlen=16)]'

    async def execute_default(self, args, context):
        return handle_dataset_entry_command(MeshcopTlvType.NETWORKNAME, args, context)


class ExtPanIDCommand(Command):

    def get_help_string(self) -> str:
        return 'View and set ExtPanID. Arguments: [extpanid (hexstring, len=16)]'

    async def execute_default(self, args, context):
        return handle_dataset_entry_command(MeshcopTlvType.EXTPANID, args, context)


class MeshLocalPrefixCommand(Command):

    def get_help_string(self) -> str:
        return 'View and set MeshLocalPrefix. Arguments: [mlp (hexstring, len=16)]'

    async def execute_default(self, args, context):
        return handle_dataset_entry_command(MeshcopTlvType.MESHLOCALPREFIX, args, context)


class DelayTimerCommand(Command):

    def get_help_string(self) -> str:
        return 'View and set DelayTimer delay. Arguments: [delay (int)]'

    async def execute_default(self, args, context):
        return handle_dataset_entry_command(MeshcopTlvType.DELAYTIMER, args, context)


class PanIDCommand(Command):

    def get_help_string(self) -> str:
        return 'View and set PanID. Arguments: [panid (hexstring, len=4)]'

    async def execute_default(self, args, context):
        return handle_dataset_entry_command(MeshcopTlvType.PANID, args, context)


class ChannelCommand(Command):

    def get_help_string(self) -> str:
        return 'View and set Channel. Arguments: [channel (int)]'

    async def execute_default(self, args, context):
        return handle_dataset_entry_command(MeshcopTlvType.CHANNEL, args, context)


class ChannelMaskCommand(Command):

    def get_help_string(self) -> str:
        return 'View and set ChannelMask. Arguments: [mask (hexstring)]'

    async def execute_default(self, args, context):
        return handle_dataset_entry_command(MeshcopTlvType.CHANNELMASK, args, context)


class PskcCommand(Command):

    def get_help_string(self) -> str:
        return 'View and set Pskc. Arguments: [pskc (hexstring, maxlen=32)]'

    async def execute_default(self, args, context):
        return handle_dataset_entry_command(MeshcopTlvType.PSKC, args, context)


class SecurityPolicyCommand(Command):

    def get_help_string(self) -> str:
        return 'View and set SecurityPolicy. Arguments: '\
               '[<rotation_time (int)> [flags (string)] [version_threshold (int)]]'

    async def execute_default(self, args, context):
        return handle_dataset_entry_command(MeshcopTlvType.SECURITYPOLICY, args, context)


class DatasetCommand(Command):

    def __init__(self):
        self._subcommands = {
            'clear': DatasetClearCommand(),
            'help': DatasetHelpCommand(),
            'hex': DatasetHexCommand(),
            'reload': ReloadDatasetCommand(),
            'activetimestamp': ActiveTimestampCommand(),
            'pendingtimestamp': PendingTimestampCommand(),
            'networkkey': NetworkKeyCommand(),
            'networkname': NetworkNameCommand(),
            'extpanid': ExtPanIDCommand(),
            'meshlocalprefix': MeshLocalPrefixCommand(),
            'delay': DelayTimerCommand(),
            'panid': PanIDCommand(),
            'channel': ChannelCommand(),
            'channelmask': ChannelMaskCommand(),
            'pskc': PskcCommand(),
            'securitypolicy': SecurityPolicyCommand()
        }

    def get_help_string(self) -> str:
        return 'View and manipulate current dataset. ' \
            'Call without parameters to show current dataset.'

    async def execute_default(self, args, context):
        ds: ThreadDataset = context['dataset']
        ds.print_content()
        return CommandResultNone()
