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
# This test verifies that PBBR sends BMLR.ntf correctly when multicast addresses are registered.
#
# Topology:
#    -----------(eth)----------------
#       |
#      BBR
#        \
#        Router1
#

import unittest

import thread_cert
from pktverify.packet_verifier import PacketVerifier

PBBR = 1
SBBR = 2
ROUTER1 = 3

REREG_DELAY = 4
MLR_TIMEOUT = 300


class BBR_5_11_01(thread_cert.TestCase):
    USE_MESSAGE_FACTORY = False

    TOPOLOGY = {
        PBBR: {
            'name': 'PBBR',
            'whitelist': [SBBR, ROUTER1],
            'is_otbr': True,
            'version': '1.2',
            'router_selection_jitter': 1,
        },
        SBBR: {
            'name': 'SBBR',
            'whitelist': [PBBR, ROUTER1],
            'is_otbr': True,
            'version': '1.2',
            'router_selection_jitter': 1,
        },
        ROUTER1: {
            'name': 'ROUTER1',
            'whitelist': [PBBR, SBBR],
            'version': '1.2',
            'router_selection_jitter': 1,
        },
    }

    def test(self):
        self.nodes[PBBR].start()
        self.wait_node_state(PBBR, 'leader', 5)
        self.nodes[PBBR].set_backbone_router(reg_delay=REREG_DELAY, mlr_timeout=MLR_TIMEOUT)
        self.nodes[PBBR].enable_backbone_router()
        self.wait_until(lambda: self.nodes[PBBR].is_primary_backbone_router, 5)

        self.nodes[SBBR].start()
        self.wait_node_state(SBBR, 'router', 5)
        self.nodes[SBBR].set_backbone_router(reg_delay=REREG_DELAY, mlr_timeout=MLR_TIMEOUT)
        self.nodes[SBBR].enable_backbone_router()
        self.simulator.go(5)
        self.assertFalse(self.nodes[SBBR].is_primary_backbone_router)

        self.nodes[ROUTER1].start()
        self.wait_node_state(ROUTER1, 'router', 5)

        self.nodes[PBBR].add_ipmaddr("ff04::1")
        self.simulator.go(REREG_DELAY)
        self.nodes[ROUTER1].add_ipmaddr("ff04::2")
        self.simulator.go(REREG_DELAY)

        self.collect_ipaddrs()
        self.collect_rloc16s()

    def verify(self, pv: PacketVerifier):
        pkts = pv.pkts
        pv.add_common_vars()
        pv.summary.show()
        pv.verify_attached('ROUTER1')

        PBBR_ETH = pv.vars['PBBR_ETH']
        SBBR_ETH = pv.vars['SBBR_ETH']

        # Verify SBBR must not send `/b/bmr` during the test.
        pkts.filter_eth_src(SBBR_ETH).filter_coap_request('/b/bmr').must_not_next()

        # Verify PBBR sends `/b/bmr` on the Backbone link for ff04::1 with default timeout.
        pkts.filter_eth_src(PBBR_ETH).filter_coap_request('/b/bmr').must_next().must_verify(f"""
            thread_meshcop.tlv.ipv6_addr == 'ff04::1'
            and thread_bl.tlv.timeout == {MLR_TIMEOUT}
        """)
        # Verify PBBR sends `/b/bmr` on the Backbone link for ff04::2 with default timeout.
        pkts.filter_eth_src(PBBR_ETH).filter_coap_request('/b/bmr').must_next().must_verify(f"""
            thread_meshcop.tlv.ipv6_addr == 'ff04::2'
            and thread_bl.tlv.timeout == {MLR_TIMEOUT}
        """)


if __name__ == '__main__':
    unittest.main()
