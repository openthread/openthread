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
import readline
import shlex
from argparse import ArgumentParser
from ble.ble_stream_secure import BleStreamSecure
from cli.base_commands import (HelpCommand, HelloCommand, CommissionCommand, DecommissionCommand, GetDeviceIdCommand,
                               GetPskdHash, GetExtPanIDCommand, GetNetworkNameCommand, GetProvisioningUrlCommand,
                               PingCommand, GetRandomNumberChallenge, ThreadStateCommand, ScanCommand, PresentHash)
from cli.dataset_commands import (DatasetCommand)
from dataset.dataset import ThreadDataset
from typing import Optional


class CLI:

    def __init__(self,
                 dataset: ThreadDataset,
                 cmd_args: Optional[ArgumentParser] = None,
                 ble_sstream: Optional[BleStreamSecure] = None):
        self._commands = {
            'help': HelpCommand(),
            'hello': HelloCommand(),
            'commission': CommissionCommand(),
            'decommission': DecommissionCommand(),
            'device_id': GetDeviceIdCommand(),
            'ext_panid': GetExtPanIDCommand(),
            'provisioning_url': GetProvisioningUrlCommand(),
            'network_name': GetNetworkNameCommand(),
            'ping': PingCommand(),
            'dataset': DatasetCommand(),
            'thread': ThreadStateCommand(),
            'scan': ScanCommand(),
            'random_challenge': GetRandomNumberChallenge(),
            'present_hash': PresentHash(),
            'peer_pskd_hash': GetPskdHash(),
        }
        self._context = {
            'ble_sstream': ble_sstream,
            'dataset': dataset,
            'commands': self._commands,
            'cmd_args': cmd_args
        }
        readline.set_completer(self.completer)
        readline.parse_and_bind('tab: complete')

    def completer(self, text, state):
        command_pool = self._commands.keys()
        full_line = readline.get_line_buffer().lstrip()
        words = full_line.split()

        should_suggest_subcommands = len(words) > 1 or (len(words) == 1 and full_line[-1].isspace())
        if should_suggest_subcommands:
            if words[0] not in self._commands.keys():
                return None

            current_command = self._commands[words[0]]
            if full_line[-1].isspace():
                subcommands = words[1:]
            else:
                subcommands = words[1:-1]
            for nextarg in subcommands:
                if nextarg in current_command._subcommands.keys():
                    current_command = current_command._subcommands[nextarg]
                else:
                    return None

            if len(current_command._subcommands) == 0:
                return None

            command_pool = current_command._subcommands.keys()

        options = [c for c in command_pool if c.startswith(text)]
        if state < len(options):
            return options[state]
        else:
            return None

    async def evaluate_input(self, user_input):
        # do not parse empty commands
        if not user_input.strip():
            return

        command_parts = shlex.split(user_input)
        command = command_parts[0]
        args = command_parts[1:]

        if command not in self._commands.keys():
            raise Exception('Invalid command: {}'.format(command))

        return await self._commands[command].execute(args, self._context)
