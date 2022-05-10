#!/usr/bin/env python3
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

import unittest

import config
import thread_cert
from pktverify.consts import MLE_ADVERTISEMENT, MLE_CHILD_ID_REQUEST
from pktverify.packet_verifier import PacketVerifier

LEADER = 1
ED = 2


class Cert_6_6_1_KeyIncrement(thread_cert.TestCase):
    TOPOLOGY = {
        LEADER: {
            'name': 'LEADER',
            'key_switch_guardtime': 0,
            'mode': 'rdn',
            'allowlist': [ED]
        },
        ED: {
            'name': 'ED',
            'is_mtd': True,
            'key_switch_guardtime': 0,
            'mode': 'rn',
            'allowlist': [LEADER]
        },
    }

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), "leader")

        self.nodes[ED].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[ED].get_state(), "child")

        self.collect_rloc16s()
        addrs = self.nodes[ED].get_addrs()
        for addr in addrs:
            self.assertTrue(self.nodes[LEADER].ping(addr))

        key_sequence_counter = self.nodes[LEADER].get_key_sequence_counter()
        self.nodes[LEADER].set_key_sequence_counter(key_sequence_counter + 1)

        addrs = self.nodes[ED].get_addrs()
        for addr in addrs:
            self.assertTrue(self.nodes[LEADER].ping(addr))

    def verify(self, pv):
        pkts = pv.pkts
        pv.summary.show()

        LEADER = pv.vars['LEADER']
        ED = pv.vars['ED']
        _leader_pkts = pkts.filter_wpan_src64(LEADER)
        _ed_pkts = pkts.filter_wpan_src64(ED)

        # Step 1: The DUT must start the network using
        # thrKeySequenceCounter = 0
        _leader_pkts.filter_mle_cmd(MLE_ADVERTISEMENT).must_next().must_verify(
            lambda p: p.wpan.aux_sec.key_source == 0)

        # Step 2: Verify that the topology described above is created.
        # MLE Auxiliary security header shall contain Key Source = 0,
        # KeyIndex = 1, KeyID Mode = 2
        _ed_pkts.filter_mle_cmd(
            MLE_CHILD_ID_REQUEST).must_next().must_verify(lambda p: p.wpan.aux_sec.key_index == 1 and p.wpan.aux_sec.
                                                          key_id_mode == 2 and p.wpan.aux_sec.key_source == 0)

        # Step 3: Leader send an ICMPv6 Echo Request to DUT.
        # The MAC Auxiliary security header must contain
        # KeyIndex = 1, KeyID Mode = 1
        lp = _leader_pkts.filter_ping_request().filter(
            lambda p: p.wpan.aux_sec.key_index == 1 and p.wpan.aux_sec.key_id_mode == 1 and p.wpan.dst16 == pv.vars[
                'ED_RLOC16']).must_next()

        # Step 4: DUT send an ICMPv6 Echo Reply to Leader.
        # The MAC Auxiliary security header must contain
        # KeyIndex = 1, KeyID Mode = 1
        _ed_pkts.filter_ping_reply(identifier=lp.icmpv6.echo.identifier).must_next().must_verify(
            lambda p: p.wpan.aux_sec.key_index == 1 and p.wpan.aux_sec.key_id_mode == 1)

        # Step 5: Leader increment thrKeySequenceCounter by 1 to force a key switch.
        # Step 6: Leader Send an ICMPv6 Echo Request to DUT.
        # The MAC Auxiliary security header must contain
        # KeyIndex = 2, KeyID Mode = 1
        lp = _leader_pkts.filter_ping_request().filter(
            lambda p: p.wpan.aux_sec.key_index == 2 and p.wpan.aux_sec.key_id_mode == 1 and p.wpan.dst16 == pv.vars[
                'ED_RLOC16']).must_next()

        # Step 7: DUT send an ICMPv6 Echo Reply to Leader.
        # The MAC Auxiliary security header must contain
        # KeyIndex = 2, KeyID Mode = 1
        _ed_pkts.filter_ping_reply(identifier=lp.icmpv6.echo.identifier).must_next().must_verify(
            lambda p: p.wpan.aux_sec.key_index == 2 and p.wpan.aux_sec.key_id_mode == 1)


if __name__ == '__main__':
    unittest.main()
