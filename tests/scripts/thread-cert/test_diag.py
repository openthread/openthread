#!/usr/bin/env python
#
#  Copyright (c) 2018, The OpenThread Authors.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. Neither the name of the copyright holder nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#

import unittest

import node_cli
import config

class TestDiag(unittest.TestCase):
    def setUp(self):
        self.simulator = config.create_default_simulator()
        self.node_cli = node_cli.otCli(1, False, simulator=self.simulator)

    def tearDown(self):
        self.node_cli.destroy()
        self.simulator.stop()

    def test(self):
        cases = [
            ('diag\n',
             'diagnostics mode is disabled\r\n'),
            ('diag send 10 100\n',
             'failed\r\nstatus 0xd\r\n'),
            ('diag start\n',
             'start diagnostics mode\r\nstatus 0x00\r\n'),
            ('diag',
             'diagnostics mode is enabled\r\n'),
            ('diag channel 10\n',
             'failed\r\nstatus 0x7\r\n'),
            ('diag channel 11\n',
             'set channel to 11\r\nstatus 0x00\r\n'),
            ('diag channel\n',
             'channel: 11\r\n'),
            ('diag power -10\n',
             'set tx power to -10 dBm\r\nstatus 0x00\r\n'),
            ('diag power\n',
             'tx power: -10 dBm\r\n'),
            ('diag stats\n',
             'received packets: 0\r\nsent packets: 0\r\nfirst received packet: rssi=0, lqi=0\r\n'),
            ('diag send 20 100\n',
             'sending 0x14 packet\(s\), length 0x64\r\nstatus 0x00\r\n'),
            ('  diag \t send    \t 20\t100',
             'sending 0x14 packet\(s\), length 0x64\r\nstatus 0x00\r\n'),
            ('diag repeat 100 100\n',
             'sending packets of length 0x64 at the delay of 0x64 ms\r\nstatus 0x00\r\n'),
            ('diag repeat stop\n',
             'repeated packet transmission is stopped\r\nstatus 0x00\r\n'),
            ('diag stop\n',
             'received packets: 0\r\nsent packets: ([1-9]\d*)\r\nfirst received packet: rssi=0, lqi=0\r\n\nstop diagnostics mode\r\nstatus 0x00\r\n'),
            ('diag',
             'diagnostics mode is disabled\r\n'),
            ('diag 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32',
             'Error: too many args \(max 32\)\r\n'),
        ]

        for case in cases:
            self.node_cli.send_command(case[0])
            self.simulator.go(1)
            self.node_cli._expect(case[1])

if __name__ == '__main__':
    unittest.main()
