#!/usr/bin/python
#
#  Copyright (c) 2016, Nest Labs, Inc.
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

import os
import sys
import time
import pexpect
import unittest

import node

class test_ncp_thread(unittest.TestCase):
    def setUp(self):
        self.node = node.Node(1)
        self.node.send_command('panid 1')
        self.node.pexpect.expect('Done')
        self.node.send_command('ifconfig up')
        self.node.pexpect.expect('Done')
        self.node.send_command('thread start')
        self.node.pexpect.expect('Done')


    def test_assisting_ports(self):
        self.node.send_command('ncp-assisting-ports 1234')
        self.node.pexpect.expect('Done')

        self.node.send_command('ncp-assisting-ports')
        self.node.pexpect.expect('1234')
        self.node.pexpect.expect('Done')

        self.node.send_command('ncp-assisting-ports remove 1234')
        self.node.pexpect.expect('Done')

        self.node.send_command('ncp-assisting-ports add 5432')
        self.node.pexpect.expect('Done')

        self.node.send_command('ncp-assisting-ports')
        self.node.pexpect.expect('5432')
        self.node.pexpect.expect('Done')

    def test_ipaddr(self):
        self.node.send_command('ipaddr')
        self.node.pexpect.expect('Done')

        self.node.send_command('ipaddr add fd00::1')
        self.node.pexpect.expect('Done')
        self.node.send_command('ipaddr')
        self.node.pexpect.expect('fd00::1')
        self.node.pexpect.expect('Done')

        #self.node.send_command('ipaddr remove fd00::1')
        #self.node.pexpect.expect('Done')

    def test_route(self):
        self.node.send_command('route add fd00::1/64')
        self.node.pexpect.expect('Done')

    def test_leader_weight(self):
        self.node.send_command('leaderweight')
        self.node.pexpect.expect('Done')

        self.node.send_command('leaderweight 4')
        self.node.pexpect.expect('Done')
        self.node.send_command('leaderweight')
        self.node.pexpect.expect('4')
        self.node.pexpect.expect('Done')


if __name__ == '__main__':
    unittest.main()
