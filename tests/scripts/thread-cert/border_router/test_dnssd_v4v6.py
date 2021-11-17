#!/usr/bin/env python3
#
#  Copyright (c) 2021, The OpenThread Authors.
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
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS'
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
import ipaddress
import json
import logging
import unittest

import config
import thread_cert

# Test description:
#   This test verifies DNS-SD server works well no matter a service is registered with an IPv4 address or an IPv6
#   address.
#
# Topology:
#    ------------(eth)--------------------
#           |           |            |
#        SERVER    PUBLISHER       HOST

#

SERVER = 1
PUBLISHER = 2
DIGGER = 3

DOMAIN = 'default.service.arpa.'
SERVICE_TYPE = '_testsrv._udp'
SERVICE_FULL_NAME = f'{SERVICE_TYPE}.{DOMAIN}'
INSTANCE = 'test'
HOST = 'host'
TXT = 'a=1 b=3 abc=ddd'


class TestDnssdServerOnBr(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        SERVER: {
            'name': 'Server',
            'is_otbr': True,
            'version': '1.2',
            'allow_list': [],
        },
        PUBLISHER: {
            'name': 'Publisher',
            'is_host': True,
        },
        DIGGER: {
            'name': 'Digger',
            'is_host': True
        },
    }

    def test(self):
        server = self.nodes[SERVER]
        publisher = self.nodes[PUBLISHER]
        digger = self.nodes[DIGGER]

        server.start()
        self.simulator.go(5)
        self.assertEqual('leader', server.get_state())

        publisher.start(start_radvd=False)
        digger.start(start_radvd=False)
        self.simulator.go(5)

        server_addr = server.get_ip6_address(config.ADDRESS_TYPE.OMR)[0]
        txt = {'a': '1', 'b': '3', 'abc': 'ddd'}

        # Publish both IPv4 and IPv6 addresses. The IPv6 address is resolved.
        publisher.publish_mdns_service(instance=self._instance_name(1),
                                       service_type=SERVICE_TYPE,
                                       addresses=['1.1.1.1', '2::2'],
                                       port=5555,
                                       hostname=f'{self._host_name(1)}.local.',
                                       txt='a=1 b=3 abc=ddd',
                                       timeout=10)
        instance_full_name = self._instance_full_name(1)
        host_full_name = self._host_full_name(1)

        dig_result = digger.dns_dig(server_addr, SERVICE_FULL_NAME, 'PTR')
        self._assert_dig_result_matches(
            dig_result, {
                'QUESTION': [(SERVICE_FULL_NAME, 'IN', 'PTR')],
                'ANSWER': [(SERVICE_FULL_NAME, 'IN', 'PTR', instance_full_name)],
                'ADDITIONAL': [
                    (instance_full_name, 'IN', 'SRV', 0, 0, 5555),
                    (instance_full_name, 'IN', 'TXT', txt),
                    (host_full_name, 'IN', 'AAAA', '2::2'),
                ],
            })

        dig_result = digger.dns_dig(server_addr, instance_full_name, 'SRV')
        self._assert_dig_result_matches(
            dig_result, {
                'QUESTION': [(instance_full_name, 'IN', 'SRV')],
                'ANSWER': [(instance_full_name, 'IN', 'SRV', 0, 0, 5555, host_full_name)],
                'ADDITIONAL': [(host_full_name, 'IN', 'AAAA', '2::2'), ],
            })

        dig_result = digger.dns_dig(server_addr, instance_full_name, 'TXT')
        self._assert_dig_result_matches(dig_result, {
            'QUESTION': [(instance_full_name, 'IN', 'TXT')],
            'ANSWER': [(instance_full_name, 'IN', 'TXT', txt)],
        })

        dig_result = digger.dns_dig(server_addr, host_full_name, 'AAAA')
        self._assert_dig_result_matches(dig_result, {
            'QUESTION': [(host_full_name, 'IN', 'AAAA')],
            'ANSWER': [(host_full_name, 'IN', 'AAAA', '2::2')],
        })

        self.simulator.go(15)

        # Publish only 1 IPv6 addresses. The IPv6 address is resolved.
        publisher.publish_mdns_service(instance=self._instance_name(2),
                                       service_type=SERVICE_TYPE,
                                       addresses=['2::2'],
                                       port=5555,
                                       hostname=f'{self._host_name(2)}.local.',
                                       txt='a=1 b=3 abc=ddd',
                                       timeout=10)
        instance_full_name = self._instance_full_name(2)
        host_full_name = self._host_full_name(2)

        dig_result = digger.dns_dig(server_addr, SERVICE_FULL_NAME, 'PTR')
        self._assert_dig_result_matches(
            dig_result, {
                'QUESTION': [(SERVICE_FULL_NAME, 'IN', 'PTR')],
                'ANSWER': [(SERVICE_FULL_NAME, 'IN', 'PTR', instance_full_name)],
                'ADDITIONAL': [
                    (instance_full_name, 'IN', 'SRV', 0, 0, 5555),
                    (instance_full_name, 'IN', 'TXT', txt),
                    (host_full_name, 'IN', 'AAAA', '2::2'),
                ],
            })

        dig_result = digger.dns_dig(server_addr, instance_full_name, 'SRV')
        self._assert_dig_result_matches(
            dig_result, {
                'QUESTION': [(instance_full_name, 'IN', 'SRV')],
                'ANSWER': [(instance_full_name, 'IN', 'SRV', 0, 0, 5555, host_full_name)],
                'ADDITIONAL': [(host_full_name, 'IN', 'AAAA', '2::2'), ],
            })

        dig_result = digger.dns_dig(server_addr, instance_full_name, 'TXT')
        self._assert_dig_result_matches(dig_result, {
            'QUESTION': [(instance_full_name, 'IN', 'TXT')],
            'ANSWER': [(instance_full_name, 'IN', 'TXT', txt)],
        })

        dig_result = digger.dns_dig(server_addr, host_full_name, 'AAAA')
        self._assert_dig_result_matches(dig_result, {
            'QUESTION': [(host_full_name, 'IN', 'AAAA')],
            'ANSWER': [(host_full_name, 'IN', 'AAAA', '2::2')],
        })

        self.simulator.go(15)

        # Publish only 1 IPv4 addresses. No IPv6 address is resolved.
        publisher.publish_mdns_service(instance=self._instance_name(3),
                                       service_type=SERVICE_TYPE,
                                       addresses=['1.1.1.1'],
                                       port=5555,
                                       hostname=f'{self._host_name(3)}.local.',
                                       txt='a=1 b=3 abc=ddd',
                                       timeout=10)
        instance_full_name = self._instance_full_name(3)
        host_full_name = self._host_full_name(3)

        dig_result = digger.dns_dig(server_addr, SERVICE_FULL_NAME, 'PTR')
        self._assert_dig_result_matches(
            dig_result, {
                'QUESTION': [(SERVICE_FULL_NAME, 'IN', 'PTR')],
                'ANSWER': [(SERVICE_FULL_NAME, 'IN', 'PTR', instance_full_name)],
                'ADDITIONAL': [
                    (instance_full_name, 'IN', 'SRV', 0, 0, 5555),
                    (instance_full_name, 'IN', 'TXT', txt),
                ],
            })

        dig_result = digger.dns_dig(server_addr, instance_full_name, 'SRV')
        self._assert_dig_result_matches(
            dig_result, {
                'QUESTION': [(instance_full_name, 'IN', 'SRV')],
                'ANSWER': [(instance_full_name, 'IN', 'SRV', 0, 0, 5555, host_full_name)],
                'ADDITIONAL': [],
            })

        dig_result = digger.dns_dig(server_addr, instance_full_name, 'TXT')
        self._assert_dig_result_matches(dig_result, {
            'QUESTION': [(instance_full_name, 'IN', 'TXT')],
            'ANSWER': [(instance_full_name, 'IN', 'TXT', txt)],
        })

        dig_result = digger.dns_dig(server_addr, host_full_name, 'AAAA')
        self._assert_dig_result_matches(dig_result, {
            'QUESTION': [(host_full_name, 'IN', 'AAAA')],
            'ANSWER': [],
        })

    def _instance_name(self, id):
        return f'{INSTANCE}{id}'

    def _instance_full_name(self, id):
        return f'{INSTANCE}{id}.{SERVICE_FULL_NAME}'

    def _host_name(self, id):
        return f'{HOST}{id}'

    def _host_full_name(self, id):
        return f'{HOST}{id}.{DOMAIN}'

    def _assert_have_question(self, dig_result, question):
        for dig_question in dig_result['QUESTION']:
            if self._match_record(dig_question, question):
                return

        self.fail((dig_result, question))

    def _assert_have_answer(self, dig_result, record, additional=False):
        for dig_answer in dig_result['ANSWER' if not additional else 'ADDITIONAL']:
            dig_answer = list(dig_answer)
            dig_answer[1:2] = []  # remove TTL from answer

            record = list(record)

            # convert IPv6 addresses to `ipaddress.IPv6Address` before matching
            if dig_answer[2] == 'AAAA':
                dig_answer[3] = ipaddress.IPv6Address(dig_answer[3])

            if record[2] == 'AAAA':
                record[3] = ipaddress.IPv6Address(record[3])

            if self._match_record(dig_answer, record):
                return

            print('not match: ', dig_answer, record,
                  list(a == b or (callable(b) and b(a)) for a, b in zip(dig_answer, record)))

        self.fail((record, dig_result))

    def _match_record(self, record, match):
        assert not any(callable(elem) for elem in record), record

        if record == match:
            return True

        return all(a == b or (callable(b) and b(a)) for a, b in zip(record, match))

    def _assert_dig_result_matches(self, dig_result, expected_result):
        self.assertEqual(dig_result['opcode'], expected_result.get('opcode', 'QUERY'), dig_result)
        self.assertEqual(dig_result['status'], expected_result.get('status', 'NOERROR'), dig_result)

        if 'QUESTION' in expected_result:
            self.assertEqual(len(dig_result['QUESTION']), len(expected_result['QUESTION']), dig_result)

            for question in expected_result['QUESTION']:
                self._assert_have_question(dig_result, question)

        if 'ANSWER' in expected_result:
            self.assertEqual(len(dig_result['ANSWER']), len(expected_result['ANSWER']), dig_result)

            for record in expected_result['ANSWER']:
                self._assert_have_answer(dig_result, record, additional=False)

        if 'ADDITIONAL' in expected_result:
            self.assertGreaterEqual(len(dig_result['ADDITIONAL']), len(expected_result['ADDITIONAL']), dig_result)

            for record in expected_result['ADDITIONAL']:
                self._assert_have_answer(dig_result, record, additional=True)

        logging.info("dig result matches:\r%s", json.dumps(dig_result, indent=True))


if __name__ == '__main__':
    unittest.main()
