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

from tlv.tlv import TLV
from tlv.tcat_tlv import TcatTLVType
from ble.ble_stream_secure import BleStreamSecure

from abc import ABC, abstractmethod


class CommandResult(ABC):

    def __init__(self, value=None):
        self.value = value

    @abstractmethod
    def pretty_print(self):
        pass


class Command(ABC):

    def __init__(self):
        self._subcommands = {}

    async def execute(self, args, context) -> CommandResult:
        if len(args) > 0 and args[0] in self._subcommands.keys():
            return await self.execute_subcommand(args, context)

        return await self.execute_default(args, context)

    async def execute_subcommand(self, args, context) -> CommandResult:
        return await self._subcommands[args[0]].execute(args[1:], context)

    @abstractmethod
    async def execute_default(self, args, context):
        pass

    @abstractmethod
    def get_help_string(self) -> str:
        pass

    def print_help(self, indent=0):
        indent_width = 4
        indentation = ' ' * indent_width * indent
        print(f'{indentation}{self.get_help_string()}')

        if 'help' in self._subcommands.keys():
            print(f'{indentation}"help" command available.')
        elif len(self._subcommands) != 0:
            print(f'{indentation}Subcommands:')
            for name, sc in self._subcommands.items():
                print(f'{indentation}{" " * indent_width}{name}\t- ', end='')
                sc.print_help()


class CommandResultTLV(CommandResult):

    def pretty_print(self):
        tlv: TLV = self.value
        tlv_type = TcatTLVType.from_value(tlv.type)
        print('Result: TLV:')
        if tlv_type is not None:
            print(f'\tTYPE:\t{TcatTLVType.from_value(tlv.type).name}')
        else:
            print(f'\tTYPE:\tunknown: {hex(tlv.type)} ({tlv.type})')
        print(f'\tLEN:\t{len(tlv.value)}')
        if tlv_type == TcatTLVType.APPLICATION:
            print(f'\tVALUE:\t{tlv.value.decode("ascii")}')
        else:
            print(f'\tVALUE:\t0x{tlv.value.hex()}')


class CommandResultNone(CommandResult):

    def pretty_print(self):
        pass
