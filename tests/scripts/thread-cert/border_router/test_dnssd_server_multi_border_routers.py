#!/usr/bin/env python3
#
#  Copyright (c) 2022, The OpenThread Authors.
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
#   This test verifies DNS-SD server can work well in a multiple border router scenario. The DNS-SD server can answer
#   queries from the Host.
#   BR1 is the SRP server and BR2 is the DNS-SD server.
# Topology:
#    ----------------(eth)-----------------------
#           |              |         |
#          BR1 ---------- BR2      HOST
#           |
#        +--+----+
#        |       |
#    CLIENT1  CLIENT2
#

BR1 = 1
BR2 = 2
ED1, ED2 = 3, 4
HOST = 5

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


class TestDnssdServerOnMultiBr(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BR1: {
            'name': 'BR1',
            'is_otbr': True,
            'version': '1.2',
            'allowlist': [BR2, ED1, ED2],
        },
        BR2: {
            'name': 'BR2',
            'is_otbr': True,
            'version': '1.2',
            'allowlist': [BR1],
        },
        ED1: {
            'name': 'ED1',
            'mode': 'rn',
            'allowlist': [BR1]
        },
        ED2: {
            'name': 'ED2',
            'mode': 'rn',
            'allowlist': [BR1],
        },
        HOST: {
            'name': 'Host',
            'is_host': True
        },
    }

    def test(self):
        br1 = self.nodes[BR1]
        br2 = self.nodes[BR2]
        ed1 = self.nodes[ED1]
        ed2 = self.nodes[ED2]
        host = self.nodes[HOST]

        host.start(start_radvd=False)
        self.simulator.go(5)

        br1.start()
        self.simulator.go(config.BORDER_ROUTER_STARTUP_DELAY)
        self.assertEqual('leader', br1.get_state())
        br1.srp_server_set_enabled(True)

        br2.stop_mdns_service()
        br2.stop_otbr_service()

        ed1.start()
        self.simulator.go(5)
        self.assertEqual('child', ed1.get_state())

        ed2.start()
        self.simulator.go(5)
        self.assertEqual('child', ed2.get_state())

        self.simulator.go(10)

        client1_addrs = [ed1.get_ip6_address(config.ADDRESS_TYPE.OMR)[0], ed1.get_mleid()]
        self._config_srp_client_services(ed1, 'ins1', 'host1', 11111, 1, 1, client1_addrs)

        client2_addrs = [ed2.get_ip6_address(config.ADDRESS_TYPE.OMR)[0], ed2.get_mleid()]
        self._config_srp_client_services(ed2, 'ins2', 'host2', 22222, 2, 2, client2_addrs)
        self.simulator.go(5)

        # start BR2 late to ensure it doesn't have mDNS cache
        br2.start_mdns_service()
        br2.start_otbr_service()
        br2.start()

        self.simulator.go(config.BORDER_ROUTER_STARTUP_DELAY)

        br2_addr = br2.get_ip6_address(config.ADDRESS_TYPE.OMR)[0]

        ins1_full_name = f'ins1.{SERVICE_FULL_NAME}'
        ins2_full_name = f'ins2.{SERVICE_FULL_NAME}'
        host1_full_name = f'host1.{DOMAIN}'
        host2_full_name = f'host2.{DOMAIN}'
        EMPTY_TXT = {}

        def is_int(x):
            return isinstance(x, int)

        # check if AAAA query works
        dig_result = host.dns_dig(br2_addr, host1_full_name, 'AAAA')
        self._assert_dig_result_matches(dig_result, {
            'QUESTION': [(host1_full_name, 'IN', 'AAAA')],
            'ANSWER': [(host1_full_name, 'IN', 'AAAA', client1_addrs[0]),],
        })

        dig_result = host.dns_dig(br2_addr, host2_full_name, 'AAAA')
        self._assert_dig_result_matches(dig_result, {
            'QUESTION': [(host2_full_name, 'IN', 'AAAA')],
            'ANSWER': [(host2_full_name, 'IN', 'AAAA', client2_addrs[0]),],
        })

        # check if SRV query works
        dig_result = host.dns_dig(br2_addr, ins1_full_name, 'SRV')
        self._assert_dig_result_matches(
            dig_result, {
                'QUESTION': [(ins1_full_name, 'IN', 'SRV')],
                'ANSWER': [(ins1_full_name, 'IN', 'SRV', is_int, is_int, 11111, host1_full_name),],
                'ADDITIONAL': [(host1_full_name, 'IN', 'AAAA', client1_addrs[0]),],
            })

        dig_result = host.dns_dig(br2_addr, ins2_full_name, 'SRV')
        self._assert_dig_result_matches(
            dig_result, {
                'QUESTION': [(ins2_full_name, 'IN', 'SRV')],
                'ANSWER': [(ins2_full_name, 'IN', 'SRV', is_int, is_int, 22222, host2_full_name),],
                'ADDITIONAL': [(host2_full_name, 'IN', 'AAAA', client2_addrs[0]),],
            })

        # check if TXT query works
        dig_result = host.dns_dig(br2_addr, ins1_full_name, 'TXT')
        self._assert_dig_result_matches(dig_result, {
            'QUESTION': [(ins1_full_name, 'IN', 'TXT')],
            'ANSWER': [(ins1_full_name, 'IN', 'TXT', EMPTY_TXT)],
        })

        dig_result = host.dns_dig(br2_addr, ins2_full_name, 'TXT')
        self._assert_dig_result_matches(dig_result, {
            'QUESTION': [(ins2_full_name, 'IN', 'TXT')],
            'ANSWER': [(ins2_full_name, 'IN', 'TXT', EMPTY_TXT)],
        })

        # check if PTR query works
        dig_result = host.dns_dig(br2_addr, SERVICE_FULL_NAME, 'PTR')

        self._assert_dig_result_matches_any(dig_result, [{
            'QUESTION': [(SERVICE_FULL_NAME, 'IN', 'PTR')],
            'ANSWER': [(SERVICE_FULL_NAME, 'IN', 'PTR', f'ins1.{SERVICE_FULL_NAME}')],
            'ADDITIONAL': [
                (ins1_full_name, 'IN', 'SRV', is_int, is_int, 11111, host1_full_name),
                (ins1_full_name, 'IN', 'TXT', EMPTY_TXT),
                (host1_full_name, 'IN', 'AAAA', client1_addrs[0]),
            ],
        }, {
            'QUESTION': [(SERVICE_FULL_NAME, 'IN', 'PTR')],
            'ANSWER': [(SERVICE_FULL_NAME, 'IN', 'PTR', f'ins2.{SERVICE_FULL_NAME}')],
            'ADDITIONAL': [
                (ins2_full_name, 'IN', 'SRV', is_int, is_int, 22222, host2_full_name),
                (ins2_full_name, 'IN', 'TXT', EMPTY_TXT),
                (host2_full_name, 'IN', 'AAAA', client2_addrs[0]),
            ],
        }])

        # check some invalid queries
        for qtype in ['A', 'CNAME']:
            dig_result = host.dns_dig(br2_addr, host1_full_name, qtype)
            self._assert_dig_result_matches(dig_result, {
                'status': 'NOTIMP',
            })

        for service_name in WRONG_SERVICE_NAMES:
            dig_result = host.dns_dig(br2_addr, service_name, 'PTR')
            self._assert_dig_result_matches(dig_result, {
                'status': 'NXDOMAIN',
            })

        # verify Discovery Proxy works for _meshcop._udp
        self._verify_discovery_proxy_meshcop(br2_addr, br2.get_network_name(), host)

    def _verify_discovery_proxy_meshcop(self, server_addr, network_name, digger):
        dp_service_name = '_meshcop._udp.default.service.arpa.'
        dp_hostname = lambda x: x.endswith('.default.service.arpa.')

        def check_border_agent_port(port):
            return 0 < port <= 65535

        dig_result = digger.dns_dig(server_addr, dp_service_name, 'PTR')
        for answer in dig_result['ANSWER']:
            if len(answer) >= 2 and answer[-2] == 'PTR':
                dp_instance_name = answer[-1]
                break
        self._assert_dig_result_matches(
            dig_result, {
                'QUESTION': [(dp_service_name, 'IN', 'PTR'),],
                'ANSWER': [(dp_service_name, 'IN', 'PTR', dp_instance_name),],
                'ADDITIONAL': [
                    (dp_instance_name, 'IN', 'SRV', 0, 0, check_border_agent_port, dp_hostname),
                    (dp_instance_name, 'IN', 'TXT', lambda txt: (isinstance(txt, dict) and txt.get(
                        'nn') == network_name and 'xp' in txt and 'tv' in txt and 'xa' in txt)),
                ],
            })

        # Find the actual host name and IPv6 address
        dp_ip6_address = None
        for rr in dig_result['ADDITIONAL']:
            if rr[3] == 'SRV':
                dp_hostname = rr[7]
            elif rr[3] == 'AAAA':
                dp_ip6_address = rr[4]

        assert isinstance(dp_hostname, str), dig_result

        dig_result = digger.dns_dig(server_addr, dp_instance_name, 'SRV')
        self._assert_dig_result_matches(
            dig_result, {
                'QUESTION': [(dp_instance_name, 'IN', 'SRV'),],
                'ANSWER': [(dp_instance_name, 'IN', 'SRV', 0, 0, check_border_agent_port, dp_hostname),],
                'ADDITIONAL': [(dp_instance_name, 'IN', 'TXT', lambda txt: (isinstance(txt, dict) and txt.get(
                    'nn') == network_name and 'xp' in txt and 'tv' in txt and 'xa' in txt)),],
            })

        dig_result = digger.dns_dig(server_addr, dp_instance_name, 'TXT')
        self._assert_dig_result_matches(
            dig_result, {
                'QUESTION': [(dp_instance_name, 'IN', 'TXT'),],
                'ANSWER': [(dp_instance_name, 'IN', 'TXT', lambda txt: (isinstance(txt, dict) and txt.get(
                    'nn') == network_name and 'xp' in txt and 'tv' in txt and 'xa' in txt)),],
                'ADDITIONAL': [(dp_instance_name, 'IN', 'SRV', 0, 0, check_border_agent_port, dp_hostname),],
            })

        if dp_ip6_address is not None:
            dig_result = digger.dns_dig(server_addr, dp_hostname, 'AAAA')

            self._assert_dig_result_matches(dig_result, {
                'QUESTION': [(dp_hostname, 'IN', 'AAAA'),],
                'ANSWER': [(dp_hostname, 'IN', 'AAAA', dp_ip6_address),],
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

    def _assert_dig_result_matches_any(self, dig_result, expected_results):
        logging.info(f'dig_result = {dig_result}')
        for expected_result in expected_results:
            try:
                self._assert_dig_result_matches(dig_result, expected_result)
            except Exception:
                continue
            else:
                return

        self.fail("failed to find any matched result")


if __name__ == '__main__':
    unittest.main()
