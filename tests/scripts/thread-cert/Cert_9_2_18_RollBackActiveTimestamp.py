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

KEY1 = '00112233445566778899aabbccddeeff'
KEY2 = 'ffeeddccbbaa99887766554433221100'

CHANNEL_INIT = 19
PANID_INIT = 0xface

COMMISSIONER = 1
LEADER = 2
ROUTER1 = 3
ROUTER2 = 4
ED1 = 5
SED1 = 6

class Cert_9_2_18_RollBackActiveTimestamp(unittest.TestCase):
    def setUp(self):
        self.nodes = {}
        for i in range(1,7):
            self.nodes[i] = node.Node(i)

        self.nodes[COMMISSIONER].set_active_dataset(1, channel=CHANNEL_INIT, panid=PANID_INIT, master_key=KEY1)
        self.nodes[COMMISSIONER].set_mode('rsdn')
        self.nodes[COMMISSIONER].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[COMMISSIONER].enable_whitelist()
        self.nodes[COMMISSIONER].set_router_selection_jitter(1)

        self.nodes[LEADER].set_active_dataset(1, channel=CHANNEL_INIT, panid=PANID_INIT, master_key=KEY1)
        self.nodes[LEADER].set_mode('rsdn')
        self.nodes[LEADER].set_partition_id(0xffffffff)
        self.nodes[LEADER].add_whitelist(self.nodes[COMMISSIONER].get_addr64())
        self.nodes[LEADER].add_whitelist(self.nodes[ROUTER1].get_addr64())
        self.nodes[LEADER].enable_whitelist()
        self.nodes[LEADER].set_router_selection_jitter(1)

        self.nodes[ROUTER1].set_active_dataset(1, channel=CHANNEL_INIT, panid=PANID_INIT, master_key=KEY1)
        self.nodes[ROUTER1].set_mode('rsdn')
        self.nodes[ROUTER1].add_whitelist(self.nodes[LEADER].get_addr64())
        self.nodes[ROUTER1].add_whitelist(self.nodes[ROUTER2].get_addr64())
        self.nodes[ROUTER1].add_whitelist(self.nodes[ED1].get_addr64())
        self.nodes[ROUTER1].add_whitelist(self.nodes[SED1].get_addr64())
        self.nodes[ROUTER1].enable_whitelist()
        self.nodes[ROUTER1].set_router_selection_jitter(1)

        self.nodes[ROUTER2].set_active_dataset(1, channel=CHANNEL_INIT, panid=PANID_INIT, master_key=KEY1)
        self.nodes[ROUTER2].set_mode('rsdn')
        self.nodes[ROUTER2].add_whitelist(self.nodes[ROUTER1].get_addr64())
        self.nodes[ROUTER2].enable_whitelist()
        self.nodes[ROUTER2].set_router_selection_jitter(1)

        self.nodes[ED1].set_active_dataset(1, channel=CHANNEL_INIT, panid=PANID_INIT, master_key=KEY1)
        self.nodes[ED1].set_mode('rsn')
        self.nodes[ED1].add_whitelist(self.nodes[ROUTER1].get_addr64())
        self.nodes[ED1].enable_whitelist()

        self.nodes[SED1].set_active_dataset(1, channel=CHANNEL_INIT, panid=PANID_INIT, master_key=KEY1)
        self.nodes[SED1].set_mode('s')
        self.nodes[SED1].add_whitelist(self.nodes[ROUTER1].get_addr64())
        self.nodes[SED1].enable_whitelist()
        self.nodes[SED1].set_timeout(3)

    def tearDown(self):
        for node in list(self.nodes.values()):
            node.stop()
        del self.nodes

    def test(self):
        self.nodes[LEADER].start()
        self.nodes[LEADER].set_state('leader')
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[COMMISSIONER].start()
        time.sleep(5)
        self.assertEqual(self.nodes[COMMISSIONER].get_state(), 'router')

        self.nodes[ROUTER1].start()
        time.sleep(5)
        self.assertEqual(self.nodes[ROUTER1].get_state(), 'router')

        self.nodes[ED1].start()
        time.sleep(5)
        self.assertEqual(self.nodes[ED1].get_state(), 'child')

        self.nodes[SED1].start()
        time.sleep(5)
        self.assertEqual(self.nodes[SED1].get_state(), 'child')

        self.nodes[COMMISSIONER].send_mgmt_active_set(active_timestamp=20000,
                                                      network_name='GRL')
        time.sleep(5)

        self.nodes[COMMISSIONER].send_mgmt_pending_set(pending_timestamp=20,
                                                       active_timestamp=20,
                                                       delay_timer=20000,
                                                       network_name='Shouldnotbe')
        time.sleep(5)

        self.nodes[COMMISSIONER].send_mgmt_pending_set(pending_timestamp=20,
                                                       active_timestamp=20,
                                                       delay_timer=20000,
                                                       network_name='MyHouse',
                                                       master_key=KEY2)
        time.sleep(25)

        self.assertEqual(self.nodes[COMMISSIONER].get_masterkey(), KEY2)
        self.assertEqual(self.nodes[LEADER].get_masterkey(), KEY2)
        self.assertEqual(self.nodes[ROUTER1].get_masterkey(), KEY2)
        self.assertEqual(self.nodes[ED1].get_masterkey(), KEY2)
        self.assertEqual(self.nodes[SED1].get_masterkey(), KEY2)
        self.assertEqual(self.nodes[ROUTER2].get_masterkey(), KEY1)

        self.nodes[ROUTER2].start()
        time.sleep(5)
        self.assertEqual(self.nodes[ROUTER2].get_state(), 'leader')

if __name__ == '__main__':
    unittest.main()
