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
import logging
import unittest

import config
import thread_cert
# Test description: Here is the test case `5.11.1 DUA-TC-04: DUA re-registration`
#
# Topology:
#    -----------(eth)----------------
#                  |     |
#     Router_1----BBR---HOST
#        \     /
#        Router_2
#
from pktverify.packet_verifier import PacketVerifier

BBR = 1
ROUTER1 = 2
ROUTER2 = 3
HOST = 4


class BBR_5_11_01(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        BBR: {
            'name': 'BBR',
            'whitelist': [ROUTER1, ROUTER2],
            'is_otbr': True,
            'version': '1.2',
            'router_selection_jitter': 1,
        },
        ROUTER1: {
            'name': 'Router_1',
            'whitelist': [ROUTER2, BBR],
            'version': '1.2',
            'router_selection_jitter': 1,
        },
        ROUTER2: {
            'name': 'Router_2',
            'whitelist': [ROUTER1, BBR],
            'version': '1.2',
            'router_selection_jitter': 1,
        },
        HOST: {
            'name': 'Host',
            'is_host': True
        },
    }

    def test(self):
        self.nodes[HOST].start()
        # P1: Router_1 is configured with leader weight of 72 in case the test is executed on a CCM network

        self.nodes[ROUTER1].start()
        self.nodes[ROUTER1].set_weight(72)

        self.simulator.go(5)
        self.assertEqual('leader', self.nodes[ROUTER1].get_state())

        self.nodes[ROUTER2].start()
        self.simulator.go(5)
        self.assertEqual('router', self.nodes[ROUTER2].get_state())

        self.nodes[BBR].start()
        self.simulator.go(5)
        self.assertEqual('router', self.nodes[BBR].get_state())
        self.nodes[BBR].enable_backbone_router()
        self.simulator.go(3)
        self.assertTrue(self.nodes[BBR].is_primary_backbone_router)
        self.nodes[BBR].add_prefix(config.DOMAIN_PREFIX, "parosD")
        self.nodes[BBR].register_netdata()

        self.simulator.go(5)
        self.assertIsNotNone(self.nodes[ROUTER2].get_ip6_address(config.ADDRESS_TYPE.DUA))

        self.simulator.go(10)  # must wait for DUA_DAD_REPEATS to complete
        logging.info("Host addresses: %r", self.nodes[HOST].get_addrs())
        self.assertGreaterEqual(len(self.nodes[HOST].get_addrs()), 2)

        self.collect_ipaddrs()
        self.collect_rloc16s()
        D = self.nodes[ROUTER2].get_ip6_address(config.ADDRESS_TYPE.DUA)
        leader = self.nodes[ROUTER1]
        self.collect_extra_vars(
            D=D,
            LEADER_RLOC=leader.get_ip6_address(config.ADDRESS_TYPE.RLOC),
            LEADER_ALOC=leader.get_ip6_address(config.ADDRESS_TYPE.ALOC)[0],
            LEADER_DUA=leader.get_ip6_address(config.ADDRESS_TYPE.GLOBAL)[0],
            LEADER_MLEID=leader.get_ip6_address(config.ADDRESS_TYPE.ML_EID),
        )

        logging.info("BBR addrs: %r", self.nodes[BBR].get_addrs())
        logging.info("Host addrs: %r", self.nodes[HOST].get_addrs())

        # BBR and Host can ping each other on the Backbone link
        self.assertTrue(self.nodes[HOST].ping(self.nodes[BBR].get_ip6_address(config.ADDRESS_TYPE.BACKBONE_GUA),
                                              backbone=True))
        self.assertTrue(self.nodes[BBR].ping(self.nodes[HOST].get_ip6_address(config.ADDRESS_TYPE.BACKBONE_GUA),
                                             backbone=True))

        # Step 23: Host sends ping packet to destination D
        self.assertFalse(self.nodes[HOST].ping(D, backbone=True))  # Must fail since ND Proxying is not implemented yet

    def verify(self, pv: PacketVerifier):
        pkts = pv.pkts
        pv.add_common_vars()
        pv.summary.show()
        pv.verify_attached('BBR')

        MM = pv.vars['MM_PORT']
        BB = pv.vars['BB_PORT']
        BBR = pv.vars['BBR']
        BBR_ETH = pv.vars['BBR_ETH']
        Host_ETH = pv.vars['Host_ETH']
        BBR_BGUA = pv.vars['BBR_BGUA']
        Host_BGUA = pv.vars['Host_BGUA']
        Dg = pv.vars['D']  # DUA of Router_2

        # Step 3: BR_1: Checks received Network Data and determines that it needs to send its BBR Dataset to the
        #               leader to become primary BBR.
        pkts.filter_wpan_src64(BBR).filter_coap_request('/a/sd', port=MM).must_next().must_verify("""
            thread_nwd.tlv.server_16 is not null
            and thread_nwd.tlv.service.s_data.seqno is not null
            and thread_nwd.tlv.service.s_data.rrdelay is not null
            and thread_nwd.tlv.service.s_data.mlrtimeout is not null
        """)

        # Step 9: BR_1: Responds to the DUA registration.
        pkts.filter_wpan_src64(BBR).filter_coap_ack('/n/dr',
                                                    port=MM).must_next().must_verify('thread_nm.tlv.status == 0')

        # Step 10: BR_1: Performs DAD on the backbone link.
        pkts.filter_eth_src(BBR_ETH).filter_coap_request('/b/bq', port=BB).must_not_next()  # TODO: DAD not implemented

        # Verify Host ping BBR
        pkts.filter_eth_src(Host_ETH).filter_ipv6_src_dst(Host_BGUA, BBR_BGUA).filter_ping_request().must_next()
        pkts.filter_eth_src(BBR_ETH).filter_ipv6_src_dst(BBR_BGUA, Host_BGUA).filter_ping_reply().must_next()

        # Verify BBR ping Host
        pkts.filter_eth_src(BBR_ETH).filter_ipv6_src_dst(BBR_BGUA, Host_BGUA).filter_ping_request().must_next()
        pkts.filter_eth_src(Host_ETH).filter_ipv6_src_dst(Host_BGUA, BBR_BGUA).filter_ping_reply().must_next()

        # Step 16: Host: Queries DUA, Dg, with ND-NS
        pkts.filter_eth_src(Host_ETH).filter_icmpv6_nd_ns(Dg).must_not_next()  # TODO: setup radvd on Host

        # Step 17: BR_1: Responds with a neighbor advertisement.
        pkts.filter_eth_src(BBR_ETH).filter_icmpv6_nd_na(Dg).must_not_next()  # TODO: implement ND proxy on BBR


if __name__ == '__main__':
    unittest.main()
