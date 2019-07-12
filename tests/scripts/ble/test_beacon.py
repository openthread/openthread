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
from node_ble import Node


SCANNER = 1
BEACONER = 2
NODE_COUNT = 2


class test_beacon(unittest.TestCase):
    def setUp(self):
        self.nodes = Node.setUp(NODE_COUNT)

    def tearDown(self):
        del self.nodes
        Node.tearDown()

    def test_beacon(self):
        self.nodes[SCANNER].ble_start()
        self.nodes[SCANNER].ble_scan_start()

        self.nodes[BEACONER].ble_start()
        (addr, type) = self.nodes[BEACONER].ble_get_bdaddr()
        self.nodes[BEACONER].ble_adv_data("0201060302affe")
        self.nodes[BEACONER].ble_adv_start(500)

        print r"Expect: Got BLE_ADV from %s \[%s\] - 0201060302affe" % (addr, type)
        rsp = r"Got BLE_ADV from %s \[%s\] - 0201060302affe" % (addr, type)
        self.nodes[SCANNER].pexpect.expect(rsp)


if __name__ == '__main__':
    unittest.main()
