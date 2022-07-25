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
# This test verifies that PBBR sets DUA routes correctly.
#
import ipaddress
import unittest

import config
import thread_cert
from pktverify.consts import ADDR_QRY_URI, BACKBONE_QUERY_URI, BACKBONE_ANSWER_URI, ADDR_NTF_URI
from pktverify.packet_verifier import PacketVerifier

# Use two channels
CH1 = 11
CH2 = 22

PBBR = 1
ROUTER1 = 2
MED1 = 3
HOST = 4
PBBR2 = 5
MED2 = 6

REREG_DELAY = 5  # Seconds
MLR_TIMEOUT = 300  # Seconds
WAIT_REDUNDANCE = 3


class TestDuaRoutingMed(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    # Topology:
    #    ------(eth)----------------------
    #       |        |       |
    #      PBBR     HOST   PBBR2
    #     / CH1\            | CH2
    #   MED1  ROUTER1      MED2
    #
    # PBBR2 is in the secondary channel
    #
    TOPOLOGY = {
        PBBR: {
            'name': 'PBBR',
            'allowlist': [MED1, ROUTER1],
            'is_otbr': True,
            'version': '1.2',
            'channel': CH1,
        },
        ROUTER1: {
            'name': 'ROUTER1',
            'allowlist': [PBBR],
            'version': '1.2',
            'channel': CH1,
        },
        MED1: {
            'name': 'MED1',
            'allowlist': [PBBR],
            'version': '1.2',
            'channel': CH1,
            'mode': 'rn',
        },
        HOST: {
            'name': 'HOST',
            'is_host': True
        },
        PBBR2: {
            'name': 'PBBR2',
            'is_otbr': True,
            'version': '1.2',
            'channel': CH2,
        },
        MED2: {
            'name': 'MED2',
            'version': '1.2',
            'channel': CH2,
            'mode': 'rn',
        },
    }

    def _bootstrap(self):
        # Bring up HOST
        self.nodes[HOST].start()

        # Bring up PBBR
        self.nodes[PBBR].start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual('leader', self.nodes[PBBR].get_state())
        self.wait_node_state(PBBR, 'leader', 10)

        self.nodes[PBBR].set_backbone_router(reg_delay=REREG_DELAY, mlr_timeout=MLR_TIMEOUT)
        self.nodes[PBBR].enable_backbone_router()
        self.nodes[PBBR].set_domain_prefix(config.DOMAIN_PREFIX, 'prosD')
        self.simulator.go(5)
        self.assertTrue(self.nodes[PBBR].is_primary_backbone_router)
        self.assertIsNotNone(self.nodes[PBBR].get_ip6_address(config.ADDRESS_TYPE.DUA))

        # Bring up ROUTER1
        self.nodes[ROUTER1].start()
        self.simulator.go(5)
        self.assertEqual('router', self.nodes[ROUTER1].get_state())
        self.assertIsNotNone(self.nodes[ROUTER1].get_ip6_address(config.ADDRESS_TYPE.DUA))

        # Bring up MED1
        self.nodes[MED1].start()
        self.simulator.go(5)
        self.assertEqual('child', self.nodes[MED1].get_state())
        self.assertIsNotNone(self.nodes[MED1].get_ip6_address(config.ADDRESS_TYPE.DUA))

        # Bring up PBBR2
        self.nodes[PBBR2].start()
        self.simulator.go(config.LEADER_STARTUP_DELAY)
        self.assertEqual('leader', self.nodes[PBBR2].get_state())
        self.wait_node_state(PBBR2, 'leader', 10)

        self.nodes[PBBR2].set_backbone_router(reg_delay=REREG_DELAY, mlr_timeout=MLR_TIMEOUT)
        self.nodes[PBBR2].enable_backbone_router()
        self.nodes[PBBR2].set_domain_prefix(config.DOMAIN_PREFIX, 'prosD')
        self.simulator.go(5)
        self.assertTrue(self.nodes[PBBR2].is_primary_backbone_router)
        self.assertIsNotNone(self.nodes[PBBR2].get_ip6_address(config.ADDRESS_TYPE.DUA))

        # Bring up MED2
        self.nodes[MED2].start()
        self.simulator.go(5)
        self.assertEqual('child', self.nodes[MED2].get_state())
        self.assertIsNotNone(self.nodes[MED2].get_ip6_address(config.ADDRESS_TYPE.DUA))

    def test(self):
        self._bootstrap()

        self.collect_ipaddrs()
        self.collect_rloc16s()

        self.simulator.go(10)

        # PBBR2 pings MED2
        self.assertTrue(self.nodes[PBBR2].ping(self.nodes[MED2].get_ip6_address(config.ADDRESS_TYPE.DUA)))
        self.simulator.go(WAIT_REDUNDANCE)

        # MED2 pings PBBR2
        self.assertTrue(self.nodes[MED2].ping(self.nodes[PBBR2].get_ip6_address(config.ADDRESS_TYPE.DUA)))
        self.simulator.go(WAIT_REDUNDANCE)

        # PBBR pings PBBR2
        self.nodes[PBBR].ping(self.nodes[PBBR2].get_ip6_address(config.ADDRESS_TYPE.DUA))
        self.simulator.go(WAIT_REDUNDANCE)

        # MED1 pings ROUTER1 should succeed
        self.assertTrue(self.nodes[MED1].ping(self.nodes[ROUTER1].get_ip6_address(config.ADDRESS_TYPE.DUA)))
        self.simulator.go(WAIT_REDUNDANCE)

        # MED1 pings MED2 which should succeed
        MED2_DUA = self.nodes[MED2].get_ip6_address(config.ADDRESS_TYPE.DUA)
        self.assertTrue(self.nodes[MED1].ping(MED2_DUA))
        self.simulator.go(WAIT_REDUNDANCE)

    def _get_mliid(self, nodeid: int) -> str:
        mleid = self.nodes[nodeid].get_ip6_address(config.ADDRESS_TYPE.ML_EID)
        mliid = ipaddress.IPv6Address(mleid).packed[-8:]
        return ''.join(['%02x' % b for b in mliid])

    def verify(self, pv: PacketVerifier):
        pkts = pv.pkts
        pv.add_common_vars()
        pv.summary.show()

        PBBR = pv.vars['PBBR']
        ROUTER1 = pv.vars['ROUTER1']
        PBBR2 = pv.vars['PBBR2']
        MED1 = pv.vars['MED1']
        MED2 = pv.vars['MED2']
        MED1_DUA = pv.vars['MED1_DUA']
        ROUTER1_DUA = pv.vars['ROUTER1_DUA']
        PBBR_DUA = pv.vars['PBBR_DUA']
        PBBR2_DUA = pv.vars['PBBR2_DUA']
        MED2_DUA = pv.vars['MED2_DUA']
        PBBR_ETH = pv.vars['PBBR_ETH']
        PBBR2_ETH = pv.vars['PBBR2_ETH']

        # PBBR should never send Address Query for MED1 (Child)
        pkts.filter_wpan_src64(PBBR).filter_RLARMA().filter_coap_request(ADDR_QRY_URI) \
            .filter('thread_address.tlv.target_eid == {eid}', eid=MED1_DUA) \
            .must_not_next()

        # PBBR2 should never send Address Query for MED2 (Child)
        pkts.filter_wpan_src64(PBBR2).filter_RLARMA().filter_coap_request(ADDR_QRY_URI) \
            .filter('thread_address.tlv.target_eid == {eid}', eid=MED2_DUA) \
            .must_not_next()

        # MEDs should never send Address Query
        pkts.filter_wpan_src64(MED1).filter_RLARMA().filter_coap_request(ADDR_QRY_URI).must_not_next()
        pkts.filter_wpan_src64(MED2).filter_RLARMA().filter_coap_request(ADDR_QRY_URI).must_not_next()

        # PBBR pings PBBR2 should succeed
        ping_id = pkts.filter_ipv6_src_dst(
            PBBR_DUA, PBBR2_DUA).filter_eth_src(PBBR_ETH).filter_ping_request().must_next().icmpv6.echo.identifier
        pkts.filter_ipv6_src_dst(PBBR2_DUA,
                                 PBBR_DUA).filter_eth_src(PBBR2_ETH).filter_ping_reply(identifier=ping_id).must_next()

        # MED1 pings ROUTER1
        ping_request_pkts = pkts.filter_ipv6_src_dst(MED1_DUA, ROUTER1_DUA)
        # MED1 sends the Ping Request
        ping_pkt = ping_request_pkts.filter_wpan_src64(MED1).filter_ping_request().must_next()
        ping_id = ping_pkt.icmpv6.echo.identifier
        # PBBR sends Address Query for ROUTER1_DUA
        pkts.filter_wpan_src64(PBBR).filter_RLARMA().filter_coap_request(ADDR_QRY_URI) \
            .filter('thread_address.tlv.target_eid == {eid}', eid=ROUTER1_DUA) \
            .must_next()
        # PBBR sends Backbone Query for ROUTER1_DUA
        pkts.filter_eth_src(PBBR_ETH).filter_coap_request(BACKBONE_QUERY_URI) \
            .filter('thread_bl.tlv.target_eid == {eid}', eid=ROUTER1_DUA) \
            .must_next()
        # ROUTER1 sends Address Answer for ROUTER1_DUA
        pkts.filter_wpan_src64(ROUTER1).filter_coap_request(ADDR_NTF_URI, confirmable=True) \
            .filter('thread_address.tlv.target_eid == {eid}', eid=ROUTER1_DUA) \
            .must_next()
        # PBBR forwards Ping Request to ROUTER1, decreasing TTL by 1
        ping_request_pkts.filter_wpan_src64(PBBR).filter_ping_request(identifier=ping_id).must_next().must_verify(
            'ipv6.hlim == {hlim}', hlim=ping_pkt.ipv6.hlim - 1)

        # MED1 pings MED2
        # Verify Ping Request: MED1 -> PBBR -> PBBR2 -> MED2
        ping_request_pkts = pkts.filter_ipv6_src_dst(MED1_DUA, MED2_DUA)
        # MED1 sends the Ping Request
        ping_pkt = ping_request_pkts.filter_wpan_src64(MED1).filter_ping_request().must_next()
        ping_id = ping_pkt.icmpv6.echo.identifier
        # PBBR sends Address Query for MED2_DUA
        pkts.filter_wpan_src64(PBBR).filter_RLARMA().filter_coap_request(ADDR_QRY_URI) \
            .filter('thread_address.tlv.target_eid == {eid}', eid=MED2_DUA) \
            .must_next()
        # PBBR sends Backbone Query for MED2_DUA
        pkts.filter_eth_src(PBBR_ETH).filter_coap_request(BACKBONE_QUERY_URI) \
            .filter('thread_bl.tlv.target_eid == {eid}', eid=MED2_DUA) \
            .must_next()
        # PBBR2 sends Backbone Answer for MED2_DUA
        pkts.filter_eth_src(PBBR2_ETH).filter_coap_request(BACKBONE_ANSWER_URI, confirmable=True) \
            .filter('thread_bl.tlv.target_eid == {eid}', eid=MED2_DUA) \
            .must_next()
        # PBBR forwards Ping Request to Backbone link, decreasing TTL by 1
        ping_request_pkts.filter_eth_src(PBBR_ETH).filter_ping_request(identifier=ping_id).must_next().must_verify(
            'ipv6.hlim == {hlim}', hlim=ping_pkt.ipv6.hlim - 1)
        ping_request_pkts.filter_wpan_src64(PBBR2).filter_ping_request(identifier=ping_id).must_next()
        # Verify Ping Reply: MED2 -> PBBR2 -> PBBR -> MED1
        ping_reply_pkts = pkts.filter_ipv6_src_dst(MED2_DUA, MED1_DUA).filter_ping_reply(identifier=ping_id)
        ping_reply_pkts.filter_eth_src(PBBR2_ETH).must_next()


if __name__ == '__main__':
    unittest.main()
