#!/usr/bin/env python3
#
#  Copyright (c) 2020, The OpenThread Authors.
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

import pexpect
import config
import thread_cert

LEADER = 1
ROUTER = 2


class TestCoapBlockTransfer(thread_cert.TestCase):
    """
    Test suite for CoAP Block-Wise Transfers (RFC7959).
    """

    SUPPORT_NCP = False

    TOPOLOGY = {
        LEADER: {
            'mode': 'rdn',
            'whitelist': [ROUTER]
        },
        ROUTER: {
            'mode': 'rdn',
            'whitelist': [LEADER]
        },
    }

    def test(self):
        leader = self.nodes[LEADER]
        router = self.nodes[ROUTER]

        leader.start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual(leader.get_state(), 'leader')

        router.start()
        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual(router.get_state(), 'router')

        leader_mleid = leader.get_ip6_address(config.ADDRESS_TYPE.ML_EID)
        router_mleid = router.get_ip6_address(config.ADDRESS_TYPE.ML_EID)

        leader.coap_set_resource_path_block('test', 3)

        leader.coap_start()
        router.coap_start()

        for method in ['GET', 'PUT', 'POST']:
            if method == 'GET':
                response = router.coap_get_block(leader_mleid, 'test', size=32)
            elif method == 'PUT':
                response = router.coap_put_block(leader_mleid, 'test', size=256, count=2)
            elif method == 'POST':
                response = router.coap_post_block(leader_mleid, 'test', size=1024, count=2)
            else:
                raise ValueError(method)

            self.assertEqual(response['source'], leader_mleid)

            request = leader.coap_wait_request()
            self.assertEqual(request['method'], method)
            self.assertEqual(request['source'], router_mleid)

            if method == 'GET':
                self.assertIsNotNone(response['payload'])
                self.assertIsNone(request['payload'])
            else:
                self.assertIsNone(response['payload'])
                self.assertIsNotNone(request['payload'])

            self.simulator.go(10)

        router.coap_stop()
        leader.coap_stop()


if __name__ == '__main__':
    unittest.main()
