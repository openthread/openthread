#!/usr/bin/python
#
#  Copyright (c) 2016, The OpenThread Authors.
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

import node

COMMISSIONER = 1
JOINER = 2

class Cert_8_1_01_Commissioning(unittest.TestCase):
    def setUp(self):
        self.nodes = {}
        for i in range(1,3):
            self.nodes[i] = node.Node(i)

        self.nodes[COMMISSIONER].set_panid(0xface)
        self.nodes[COMMISSIONER].set_mode('rsdn')
        self.nodes[COMMISSIONER].set_masterkey('deadbeefdeadbeefdeadbeefdeadbeef')

        self.nodes[JOINER].set_mode('rsdn')
        self.nodes[JOINER].set_masterkey('00112233445566778899aabbccddeeff')
        self.nodes[JOINER].set_router_selection_jitter(1)

    def tearDown(self):
        for node in list(self.nodes.values()):
            node.stop()
        del self.nodes

    def test(self):
        self.nodes[COMMISSIONER].interface_up()
        self.nodes[COMMISSIONER].thread_start()
        time.sleep(5)
        self.assertEqual(self.nodes[COMMISSIONER].get_state(), 'leader')
        self.nodes[COMMISSIONER].commissioner_start()
        self.nodes[COMMISSIONER].commissioner_add_joiner('*', 'openthread')

        self.nodes[JOINER].interface_up()
        self.nodes[JOINER].joiner_start('openthread')
        time.sleep(10)
        self.assertEqual(self.nodes[JOINER].get_masterkey(), self.nodes[COMMISSIONER].get_masterkey())

        self.nodes[JOINER].thread_start()
        time.sleep(5)
        self.assertEqual(self.nodes[JOINER].get_state(), 'router')

if __name__ == '__main__':
    unittest.main()
