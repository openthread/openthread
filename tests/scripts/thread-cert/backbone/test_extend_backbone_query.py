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
import unittest

import config
import thread_cert
from pktverify.packet_verifier import PacketVerifier

# Use two channels
CH1 = 11
CH2 = 22

PBBR = 1
SBBR = 2
ROUTER1 = 3
HOST = 4
PBBR2 = 5
ROUTER2 = 6

REREG_DELAY = 5  # Seconds
MLR_TIMEOUT = 300  # Seconds
WAIT_REDUNDANCE = 3


class TestExtendBackboneQuery(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    # Topology:
    #    ------(eth)----------------------
    #       |        |       |      |
    #      PBBR----SBBR    HOST   PBBR2
    #        \ CH1 /               | CH2
    #        ROUTER1             ROUTER2
    #
    # PBBR2 is in the secondary channel
    #
    TOPOLOGY = {
        PBBR: {
            'name': 'PBBR',
            'allowlist': [SBBR, ROUTER1],
            'is_otbr': True,
            'version': '1.2',
            'router_selection_jitter': 1,
            'channel': CH1,
        },
        SBBR: {
            'name': 'SBBR',
            'allowlist': [PBBR, ROUTER1],
            'is_otbr': True,
            'version': '1.2',
            'router_selection_jitter': 1,
            'channel': CH1,
        },
        ROUTER1: {
            'name': 'ROUTER1',
            'allowlist': [PBBR, SBBR],
            'version': '1.2',
            'router_selection_jitter': 1,
            'channel': CH1,
        },
        HOST: {
            'name': 'HOST',
            'is_host': True
        },
        PBBR2: {
            'name': 'PBBR2',
            'is_otbr': True,
            'version': '1.2',
            'router_selection_jitter': 1,
            'channel': CH2,
        },
        ROUTER2: {
            'name': 'ROUTER2',
            'version': '1.2',
            'router_selection_jitter': 1,
            'channel': CH2,
        },
    }

    def _bootstrap(self):
        # Bring up HOST
        self.nodes[HOST].start()

        # Bring up PBBR
        self.nodes[PBBR].start()
        self.simulator.go(5)
        self.assertEqual('leader', self.nodes[PBBR].get_state())
        self.wait_node_state(PBBR, 'leader', 5)

        self.nodes[PBBR].set_backbone_router(reg_delay=REREG_DELAY, mlr_timeout=MLR_TIMEOUT)
        self.nodes[PBBR].enable_backbone_router()
        self.nodes[PBBR].set_domain_prefix(config.DOMAIN_PREFIX, 'prosD')
        self.simulator.go(5)
        self.assertTrue(self.nodes[PBBR].is_primary_backbone_router)
        self.assertIsNotNone(self.nodes[PBBR].get_ip6_address(config.ADDRESS_TYPE.DUA))

        # Bring up SBBR
        self.nodes[SBBR].start()
        self.simulator.go(5)
        self.assertEqual('router', self.nodes[SBBR].get_state())

        self.nodes[SBBR].set_backbone_router(reg_delay=REREG_DELAY, mlr_timeout=MLR_TIMEOUT)
        self.nodes[SBBR].enable_backbone_router()
        self.simulator.go(5)
        self.assertFalse(self.nodes[SBBR].is_primary_backbone_router)
        self.assertIsNotNone(self.nodes[SBBR].get_ip6_address(config.ADDRESS_TYPE.DUA))

        # Bring up ROUTER1
        self.nodes[ROUTER1].start()
        self.simulator.go(5)
        self.assertEqual('router', self.nodes[ROUTER1].get_state())
        self.assertIsNotNone(self.nodes[ROUTER1].get_ip6_address(config.ADDRESS_TYPE.DUA))

        # Bring up PBBR2
        self.nodes[PBBR2].start()
        self.simulator.go(5)
        self.assertEqual('leader', self.nodes[PBBR2].get_state())
        self.wait_node_state(PBBR2, 'leader', 5)

        self.nodes[PBBR2].set_backbone_router(reg_delay=REREG_DELAY, mlr_timeout=MLR_TIMEOUT)
        self.nodes[PBBR2].enable_backbone_router()
        self.nodes[PBBR2].set_domain_prefix(config.DOMAIN_PREFIX, 'prosD')
        self.simulator.go(5)
        self.assertTrue(self.nodes[PBBR2].is_primary_backbone_router)
        self.assertIsNotNone(self.nodes[PBBR2].get_ip6_address(config.ADDRESS_TYPE.DUA))

        # Bring up ROUTER2
        self.nodes[ROUTER2].start()
        self.simulator.go(5)
        self.assertEqual('router', self.nodes[ROUTER2].get_state())
        self.assertIsNotNone(self.nodes[ROUTER2].get_ip6_address(config.ADDRESS_TYPE.DUA))

        self.nodes[ROUTER2].ping(self.nodes[ROUTER1].get_ip6_address(config.ADDRESS_TYPE.DUA))

    def test(self):
        self._bootstrap()

        self.collect_ipaddrs()
        self.collect_rloc16s()
        self.collect_rlocs()

    def verify(self, pv: PacketVerifier):
        pkts = pv.pkts
        pv.add_common_vars()
        pv.summary.show()

        PBBR = pv.vars['PBBR']
        PBBR2 = pv.vars['PBBR2']
        ROUTER1 = pv.vars['ROUTER1']
        ROUTER2 = pv.vars['ROUTER2']
        ROUTER1_DUA = pv.vars['ROUTER1_DUA']
        PBBR_ETH = pv.vars['PBBR_ETH']
        SBBR_ETH = pv.vars['SBBR_ETH']
        PBBR2_ETH = pv.vars['PBBR2_ETH']

        MM = pv.vars['MM_PORT']
        BB = pv.vars['BB_PORT']

        # Router1 should send /n/dr for DUA registration
        dr = pkts.filter_wpan_src64(ROUTER1).filter_coap_request('/n/dr', port=MM).filter(
            'thread_nm.tlv.target_eid == {ROUTER1_DUA}', ROUTER1_DUA=ROUTER1_DUA).must_next()

        # SBBR should not send /b/bq for Router1's DUA
        pkts.filter_backbone_query(ROUTER1_DUA, eth_src=SBBR_ETH, port=BB).must_not_next()

        # PBBR should send /b/bq for Router1's DUA (1st time)
        bq1 = pkts.filter_backbone_query(ROUTER1_DUA, eth_src=PBBR_ETH, port=BB).must_next()
        bq1_index = pkts.index

        self.assertLessEqual(bq1.sniff_timestamp - dr.sniff_timestamp, 1.0)

        # PBBR should send /b/bq for Router1's DUA (2nd time)
        bq2 = pkts.filter_backbone_query(ROUTER1_DUA, eth_src=PBBR_ETH, port=BB).must_next()

        self.assertLess(bq2.sniff_timestamp - bq1.sniff_timestamp, 1.1)
        self.assertGreater(bq2.sniff_timestamp - bq1.sniff_timestamp, 0.9)

        # PBBR should send /b/bq for Router1's DUA (3rd time)
        bq3 = pkts.filter_backbone_query(ROUTER1_DUA, eth_src=PBBR_ETH, port=BB).must_next()

        self.assertLess(bq3.sniff_timestamp - bq2.sniff_timestamp, 1.1)
        self.assertGreater(bq3.sniff_timestamp - bq2.sniff_timestamp, 0.9)

        bq3_index = pkts.index
        # Should not recv /b/ba response during this period
        pkts.range(bq1_index, bq3_index).filter_backbone_answer(ROUTER1_DUA, port=BB).must_not_next()

        #
        # Now we verify Extending ADDR.qry to BB.qry works for the Ping Request from Router2 to Router1
        #

        # Router2 should send ADDR.qry in the Thread network for Router1's DUA
        pkts.filter_wpan_src64(ROUTER2).filter_coap_request('/a/aq', port=MM).filter(
            'thread_address.tlv.target_eid == {ROUTER1_DUA}', ROUTER1_DUA=ROUTER1_DUA).must_next()
        # PBBR2 should extend ADDR.qry to BB.qry
        pkts.filter_backbone_query(ROUTER1_DUA, eth_src=PBBR2_ETH, port=BB).must_next()
        # SBBR should not answer with BB.ans
        pkts.filter_backbone_query(ROUTER1_DUA, eth_src=SBBR_ETH, port=BB).must_not_next()
        # PBBR1 should answer with BB.ans
        pkts.filter_backbone_answer(ROUTER1_DUA, eth_src=PBBR_ETH, port=BB).must_next()
        # PBBR2 should send ADDR.ntf to Router2
        pkts.filter_wpan_src64(PBBR2).filter_coap_request('/a/an', port=MM).filter(
            'thread_address.tlv.target_eid == {ROUTER1_DUA}', ROUTER1_DUA=ROUTER1_DUA).must_next()

        # Now, Router2 should send the Ping Request to PBBR2
        ping = pkts.filter_wpan_src64(ROUTER2).filter_ping_request().filter_ipv6_dst(ROUTER1_DUA).must_next()
        pkts.filter_eth_src(PBBR2_ETH).filter_ping_request().filter_ipv6_dst(ROUTER1_DUA).must_next()


if __name__ == '__main__':
    unittest.main()
