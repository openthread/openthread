"""
  Copyright (c) 2025, The OpenThread Authors.
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


class DiagnosticTLVType:

    def __init__(self):
        self._tlv_dict = {
            'extaddr': '0',
            'macaddr': '1',
            'mode': '2',
            'timeout': '3',
            'connectivity': '4',
            'route64': '5',
            'leaderdata': '6',
            'networkdata': '7',
            'ipaddr': '8',
            'maccounters': '9',
            'batterylevel': '14',
            'supplyvoltage': '15',
            'childtable': '16',
            'channelpages': '17',
            'maxchildtimeout': '19',
            'eui64': '23',
            'version': '24',
            'vendorname': '25',
            'vendormodel': '26',
            'vendorswversion': '27',
            'threadstackversion': '28',
            'child': '29',
            'childipv6list': '30',
            'routerneighbor': '31',
            'mlecounters': '34',
            'vendorappurl': '35',
            'channeldenylist': '36'
        }

    @staticmethod
    def names_to_numbers(args):
        res = DiagnosticTLVType()
        return [x if x not in res._tlv_dict else res._tlv_dict[x] for x in args]

    @staticmethod
    def get_dict():
        res = DiagnosticTLVType()
        return res._tlv_dict
