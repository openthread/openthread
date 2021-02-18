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
#   This test verifies DNS-SD server works on a Duckhorn BR and is accessible from a Host.
#
# Topology:
#    ----------------(eth)--------------------
#           |                      |
#          BR1 (Leader, Server)   HOST
#         /        \
#      CLIENT1    CLIENT2

SERVER = BR1 = 1
CLIENT1, CLIENT2 = 2, 3
HOST = 4

DIGGER = HOST

DOMAIN = 'default.service.arpa.'
SERVICE = '_testsrv._udp'
SERVICE_FULL_NAME = f'{SERVICE}.{DOMAIN}'

VALID_SERVICE_NAMES = [
    '_abc._udp.default.service.arpa.',
    '_abc._tcp.default.service.arpa.',
]

WRONG_SERVICE_NAMES = [
    '_testsrv._udp.default.service.xxxx.',
    '_testsrv._txp,default.service.arpa.',
]


class TestDnssdServerOnBr(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR1: {
            'name': 'SERVER',
            'is_otbr': True,
            'version': '1.2',
            'router_selection_jitter': 1,
        },
        CLIENT1: {
            'name': 'CLIENT1',
            'router_selection_jitter': 1,
        },
        CLIENT2: {
            'name': 'CLIENT2',
            'router_selection_jitter': 1,
        },
        HOST: {
            'name': 'Host',
            'is_host': True
        },
    }

    def test(self):
        self.nodes[HOST].start(start_radvd=False)
        self.simulator.go(5)

        self.nodes[BR1].start()
        self.simulator.go(5)
        self.assertEqual('leader', self.nodes[BR1].get_state())
        self.nodes[SERVER].srp_server_set_enabled(True)

        self.nodes[CLIENT1].start()

        self.simulator.go(5)
        self.assertEqual('router', self.nodes[CLIENT1].get_state())

        self.nodes[CLIENT2].start()
        self.simulator.go(5)
        self.assertEqual('router', self.nodes[CLIENT2].get_state())

        self.simulator.go(10)

        # Router1 can ping to/from the Host on infra link.
        self.assertTrue(self.nodes[BR1].ping(self.nodes[HOST].get_ip6_address(config.ADDRESS_TYPE.ONLINK_ULA)[0],
                                             backbone=True))
        self.assertTrue(self.nodes[HOST].ping(self.nodes[BR1].get_ip6_address(config.ADDRESS_TYPE.OMR)[0],
                                              backbone=True))

        client1_addrs = [
            self.nodes[CLIENT1].get_mleid(), self.nodes[CLIENT1].get_ip6_address(config.ADDRESS_TYPE.OMR)[0]
        ]
        self._config_srp_client_services(CLIENT1, 'ins1', 'host1', 11111, 1, 1, client1_addrs)

        client2_addrs = [
            self.nodes[CLIENT2].get_mleid(), self.nodes[CLIENT2].get_ip6_address(config.ADDRESS_TYPE.OMR)[0]
        ]
        self._config_srp_client_services(CLIENT2, 'ins2', 'host2', 22222, 2, 2, client2_addrs)

        ins1_full_name = f'ins1.{SERVICE_FULL_NAME}'
        ins2_full_name = f'ins2.{SERVICE_FULL_NAME}'
        host1_full_name = f'host1.{DOMAIN}'
        host2_full_name = f'host2.{DOMAIN}'
        server_addr = self.nodes[SERVER].get_ip6_address(config.ADDRESS_TYPE.OMR)[0]

        # check if PTR query works
        dig_result = self.nodes[DIGGER].dns_dig(server_addr, SERVICE_FULL_NAME, 'PTR')

        self._assert_dig_result_matches(
            dig_result, {
                'QUESTION': [(SERVICE_FULL_NAME, 'IN', 'PTR')],
                'ANSWER': [(SERVICE_FULL_NAME, 'IN', 'PTR', f'ins1.{SERVICE_FULL_NAME}'),
                           (SERVICE_FULL_NAME, 'IN', 'PTR', f'ins2.{SERVICE_FULL_NAME}')],
                'ADDITIONAL': [
                    (ins1_full_name, 'IN', 'SRV', 1, 1, 11111, host1_full_name),
                    (ins1_full_name, 'IN', 'TXT', '""'),
                    (host1_full_name, 'IN', 'AAAA', client1_addrs[0]),
                    (host1_full_name, 'IN', 'AAAA', client1_addrs[1]),
                    (ins2_full_name, 'IN', 'SRV', 2, 2, 22222, host2_full_name),
                    (ins2_full_name, 'IN', 'TXT', '""'),
                    (host2_full_name, 'IN', 'AAAA', client2_addrs[0]),
                    (host2_full_name, 'IN', 'AAAA', client2_addrs[1]),
                ],
            })

        # check if SRV query works
        dig_result = self.nodes[DIGGER].dns_dig(server_addr, ins1_full_name, 'SRV')
        self._assert_dig_result_matches(
            dig_result, {
                'QUESTION': [(ins1_full_name, 'IN', 'SRV')],
                'ANSWER': [(ins1_full_name, 'IN', 'SRV', 1, 1, 11111, host1_full_name),],
                'ADDITIONAL': [
                    (host1_full_name, 'IN', 'AAAA', client1_addrs[0]),
                    (host1_full_name, 'IN', 'AAAA', client1_addrs[1]),
                ],
            })

        dig_result = self.nodes[DIGGER].dns_dig(server_addr, ins2_full_name, 'SRV')
        self._assert_dig_result_matches(
            dig_result, {
                'QUESTION': [(ins2_full_name, 'IN', 'SRV')],
                'ANSWER': [(ins2_full_name, 'IN', 'SRV', 2, 2, 22222, host2_full_name),],
                'ADDITIONAL': [
                    (host2_full_name, 'IN', 'AAAA', client2_addrs[0]),
                    (host2_full_name, 'IN', 'AAAA', client2_addrs[1]),
                ],
            })

        # check if TXT query works
        dig_result = self.nodes[DIGGER].dns_dig(server_addr, ins1_full_name, 'TXT')
        self._assert_dig_result_matches(dig_result, {
            'QUESTION': [(ins1_full_name, 'IN', 'TXT')],
            'ANSWER': [(ins1_full_name, 'IN', 'TXT', '""'),],
        })

        dig_result = self.nodes[DIGGER].dns_dig(server_addr, ins2_full_name, 'TXT')
        self._assert_dig_result_matches(dig_result, {
            'QUESTION': [(ins2_full_name, 'IN', 'TXT')],
            'ANSWER': [(ins2_full_name, 'IN', 'TXT', '""'),],
        })

        # check if AAAA query works
        dig_result = self.nodes[DIGGER].dns_dig(server_addr, host1_full_name, 'AAAA')
        self._assert_dig_result_matches(
            dig_result, {
                'QUESTION': [(host1_full_name, 'IN', 'AAAA'),],
                'ANSWER': [
                    (host1_full_name, 'IN', 'AAAA', client1_addrs[0]),
                    (host1_full_name, 'IN', 'AAAA', client1_addrs[1]),
                ],
            })

        dig_result = self.nodes[DIGGER].dns_dig(server_addr, host2_full_name, 'AAAA')
        self._assert_dig_result_matches(
            dig_result, {
                'QUESTION': [(host2_full_name, 'IN', 'AAAA'),],
                'ANSWER': [
                    (host2_full_name, 'IN', 'AAAA', client2_addrs[0]),
                    (host2_full_name, 'IN', 'AAAA', client2_addrs[1]),
                ],
            })

        # check some invalid queries
        for qtype in ['A', 'CNAME']:
            dig_result = self.nodes[DIGGER].dns_dig(server_addr, host1_full_name, qtype)
            self._assert_dig_result_matches(dig_result, {
                'status': 'NOTIMP',
                'QUESTION': [(host1_full_name, 'IN', qtype)],
            })

        for service_name in WRONG_SERVICE_NAMES:
            dig_result = self.nodes[DIGGER].dns_dig(server_addr, service_name, 'PTR')
            self._assert_dig_result_matches(dig_result, {
                'status': 'NXDOMAIN',
                'QUESTION': [(service_name, 'IN', 'PTR')],
            })

    def _config_srp_client_services(self, client, instancename, hostname, port, priority, weight, addrs):
        self.nodes[client].netdata_show()
        srp_server_port = self.nodes[client].get_srp_server_port()

        self.nodes[client].srp_client_start(self.nodes[SERVER].get_mleid(), srp_server_port)
        self.nodes[client].srp_client_set_host_name(hostname)
        self.nodes[client].srp_client_set_host_address(*addrs)
        self.nodes[client].srp_client_add_service(instancename, SERVICE, port, priority, weight)

        self.simulator.go(5)
        self.assertEqual(self.nodes[client].srp_client_get_host_state(), 'Registered')

    def _assert_have_question(self, dig_result, question):
        self.assertIn(question, dig_result['QUESTION'], (question, dig_result))

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

            if dig_answer == record:
                return

        self.fail((record, dig_result))

    def _assert_dig_result_matches(self, dig_result, expected_result):
        self.assertEqual(dig_result['opcode'], expected_result.get('opcode', 'QUERY'), dig_result)
        self.assertEqual(dig_result['status'], expected_result.get('status', 'NOERROR'), dig_result)

        self.assertEqual(len(dig_result['QUESTION']), len(expected_result.get('QUESTION', [])), dig_result)
        self.assertEqual(len(dig_result['ANSWER']), len(expected_result.get('ANSWER', [])), dig_result)
        self.assertEqual(len(dig_result['ADDITIONAL']), len(expected_result.get('ADDITIONAL', [])), dig_result)

        for question in expected_result.get('QUESTION', []):
            self._assert_have_question(dig_result, question)

        for record in expected_result.get('ANSWER', []):
            self._assert_have_answer(dig_result, record, additional=False)

        for record in expected_result.get('ADDITIONAL', []):
            self._assert_have_answer(dig_result, record, additional=True)

        logging.info("dig result matches:\r%s", json.dumps(dig_result, indent=True))


if __name__ == '__main__':
    unittest.main()
