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

from .base_commands import BleCommand, CommandResultNone, Command
from tlv.tlv import TLV
from tlv.tcat_tlv import TcatTLVType


class TlvCommandList(Command):

    def get_help_string(self) -> str:
        return 'List available TLV types to use in \'tlv send\'.'

    async def execute_default(self, args, context):
        list_tlv = "\n".join([f"{tlv.value:#x}\t{tlv.name}" for tlv in TcatTLVType])
        print(f"\n{list_tlv}")
        return CommandResultNone()


class TlvCommandSend(BleCommand):

    def get_log_string(self) -> str:
        pass

    def get_help_string(self) -> str:
        return 'Send TLV with arbitrary payload: \'tlv send <TLV_TYPE> <TLV_PAYLOAD>\'.'

    def prepare_data(self, args, context):
        tlv_type = TcatTLVType(int(args[0], 16))
        tlv_value = bytes()
        try:
            tlv_value = bytes.fromhex(args[1])
        except IndexError:
            pass
        data = TLV(tlv_type.value, tlv_value).to_bytes()
        return data


class TlvCommand(Command):

    def __init__(self):
        self._subcommands = {'list': TlvCommandList(), 'send': TlvCommandSend()}

    def get_help_string(self) -> str:
        return 'Send TLV with arbitrary payload.'

    async def execute_default(self, args, context):
        self.print_help()
        return CommandResultNone()
