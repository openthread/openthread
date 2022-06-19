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
#   This test verifies DNS-SD server and Discovery Proxy can handle Instance names with whitespaces.
#
# Topology:
#    ----------------(eth)--------------------
#           |                 |         |
#          BR1 (Server)------BR2       HOST
#         /
#      CLIENT1

SERVER = BR1 = 1
CLIENT = 2
HOST = 3
BR2 = 4

DOMAIN = 'default.service.arpa.'
SERVICE = '_testsrv._udp'
SERVICE_FULL_NAME = f'{SERVICE}.{DOMAIN}'
INSTANCE_NAME = 'O T B R'
HOST_NAME = 'host1'
HOST_FULL_NAME = f'{HOST_NAME}.{DOMAIN}'


class TestDnssdInstanceNameWithSpace(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR1: {
            'name': 'SERVER',
            'is_otbr': True,
            'version': '1.2',
        },
        BR2: {
            'name': 'BR2',
            'is_otbr': True,
            'version': '1.2',
        },
        CLIENT: {
            'name': 'CLIENT1',
        },
        HOST: {
            'name': 'Host',
            'is_host': True
        },
    }

    def test(self):
        server = br1 = self.nodes[BR1]
        br2 = self.nodes[BR2]
        client = self.nodes[CLIENT]
        digger = host = self.nodes[HOST]

        host.start(start_radvd=False)
        self.simulator.go(5)

        br1.start()
        self.simulator.go(config.BORDER_ROUTER_STARTUP_DELAY)
        self.assertEqual('leader', br1.get_state())
        server.srp_server_set_enabled(True)

        br2.start()
        self.simulator.go(config.BORDER_ROUTER_STARTUP_DELAY)
        self.assertEqual('router', br2.get_state())

        client.start()

        self.simulator.go(config.ROUTER_STARTUP_DELAY)
        self.assertEqual('router', client.get_state())

        self.simulator.go(10)

        server_addr = server.get_ip6_address(config.ADDRESS_TYPE.OMR)[0]

        client1_addrs = [client.get_mleid(), client.get_ip6_address(config.ADDRESS_TYPE.OMR)[0]]
        self._config_srp_client_services(client, INSTANCE_NAME, HOST_NAME, 11111, 0, 0, client1_addrs)

        full_instance_name = f'{INSTANCE_NAME}.{SERVICE_FULL_NAME}'
        EMPTY_TXT = {}

        self._verify_service_browse_result(client.dns_browse(SERVICE_FULL_NAME, server=br1.get_rloc()))
        self._verify_service_browse_result(client.dns_browse(SERVICE_FULL_NAME, server=br2.get_rloc()))
        self._verify_service_browse_result(client.dns_browse(SERVICE_FULL_NAME.lower(), server=br2.get_rloc()))

        # check if PTR query works
        dig_result = digger.dns_dig(server_addr, SERVICE_FULL_NAME, 'PTR')

        self._assert_dig_result_matches(
            dig_result, {
                'QUESTION': [(SERVICE_FULL_NAME, 'IN', 'PTR')],
                'ANSWER': [(SERVICE_FULL_NAME, 'IN', 'PTR', f'{INSTANCE_NAME}.{SERVICE_FULL_NAME}')],
                'ADDITIONAL': [
                    (full_instance_name, 'IN', 'SRV', 0, 0, 11111, HOST_FULL_NAME),
                    (full_instance_name, 'IN', 'TXT', EMPTY_TXT),
                ],
            })

        # check if SRV query works
        dig_result = digger.dns_dig(server_addr, full_instance_name, 'SRV')
        self._assert_dig_result_matches(
            dig_result, {
                'QUESTION': [(full_instance_name, 'IN', 'SRV')],
                'ANSWER': [(full_instance_name, 'IN', 'SRV', 0, 0, 11111, HOST_FULL_NAME),],
                'ADDITIONAL': [],
            })

    def _config_srp_client_services(self, client, instancename, hostname, port, priority, weight, addrs):
        client.srp_client_enable_auto_start_mode()
        client.srp_client_set_host_name(hostname)
        client.srp_client_set_host_address(*addrs)
        client.srp_client_add_service(instancename, SERVICE, port, priority, weight)

        self.simulator.go(5)
        self.assertEqual(client.srp_client_get_host_state(), 'Registered')

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

    def _verify_service_browse_result(self, services):
        assert len(services) == 1, services
        assert INSTANCE_NAME in services, services
        service = services[INSTANCE_NAME]
        logging.info("Service Browse Result: %r", service)
        self.assertEqual(service['host'], HOST_FULL_NAME)


if __name__ == '__main__':
    unittest.main()
