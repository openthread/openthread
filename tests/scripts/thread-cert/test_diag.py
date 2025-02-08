#!/usr/bin/env python3
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

import time
import unittest

import thread_cert


class TestDiag(thread_cert.TestCase):
    SUPPORT_NCP = False

    TOPOLOGY = {1: None}

    def test(self):
        node = self.nodes[1]

        cases = [
            ('diag', 'diagnostics mode is disabled\r\n'),
            ('diag send 10 100', 'Error 13: InvalidState\r\n'),
            ('diag start', 'Done\r\n'),
            ('diag invalid test\n', 'diag feature \'invalid\' is not supported'),
            ('diag', 'diagnostics mode is enabled\r\n'),
            ('diag channel 10', 'Error 7: InvalidArgs\r\n'),
            ('diag channel 11', 'Done\r\n'),
            ('diag channel', '11\r\n'),
            ('diag power -10', 'Done\r\n'),
            ('diag power', '-10\r\n'),
            (
                'diag stats',
                'received packets: 0\r\n'
                'sent success packets: 0\r\n'
                'sent error cca packets: 0\r\n'
                'sent error abort packets: 0\r\n'
                'sent error invalid state packets: 0\r\n'
                'sent error others packets: 0\r\n'
                'first received packet: rssi=0, lqi=0\r\n'
                'last received packet: rssi=0, lqi=0\r\n',
            ),
            ('diag send 20 100', 'Done\r\n'),
            ('  diag \t send    \t 20\t100', 'Done\r\n'),
            ('diag repeat 100 100', 'Done\r\n'),
            ('diag repeat stop', 'Done\r\n'),
            ('diag stop', 'Done\r\n'),
            ('diag', 'diagnostics mode is disabled\r\n'),
            (
                'diag 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32',
                'Error 7: InvalidArgs\r\n',
            ),
        ]

        for case in cases:
            node.send_command(case[0])
            self.simulator.go(1)
            if type(self.simulator).__name__ == 'VirtualTime':
                time.sleep(0.1)
            node._expect(case[1])


if __name__ == '__main__':
    unittest.main()
