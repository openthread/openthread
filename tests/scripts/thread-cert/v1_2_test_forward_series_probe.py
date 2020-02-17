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

import unittest

import mle
import thread_cert

LEADER = 1
SSED_1 = 2

SERIES_ID = 1
CSL_PERIOD = 500 * 6.25  # 500ms


class SSED_ForwardSeriesProbe(thread_cert.TestCase):

    topology = {LEADER: {'whitelist': [SSED_1],}, SSED_1: {'mode': 's',}}

    def test(self):
        self.nodes[LEADER].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[LEADER].get_state(), 'leader')

        self.nodes[SSED_1].set_csl_period(CSL_PERIOD)
        self.nodes[SSED_1].start()
        self.simulator.go(5)
        self.assertEqual(self.nodes[SSED_1].get_state(), 'child')

        leader_addr = self.nodes[LEADER].get_linklocal()
        leader_messages = self.simulator.get_messages_sent_by(LEADER)

        self.assertTrue(self.nodes[SSED_1].ping(leader_addr))
        self.simulator.go(5)

        # Forward Series Flags = 0x02:
        # - Bit 1 = 1, MLE Link Probes.
        # - Bits 0, 2-7 = 0
        # Concatenation of Link Metric Type ID Flags = 0x00:
        # - Item1: (0)(0)(000)(000) = 0x00
        # -- E = 0
        # -- L = 0
        # -- Type/Average Enum = 0(count)
        # -- Metrics Enum = 0(count)
        leader_addr = self.nodes[LEADER].get_linklocal()
        self.nodes[SSED_1].link_metrics_mgmt_req(leader_addr, 'FWD', 0x02, 0x00,
                                                 SERIES_ID)
        self.simulator.go(1)

        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        msg = leader_messages.next_mle_message(
            mle.CommandType.LINK_METRICS_MANAGEMENT_RESPONSE)

        self.nodes[SSED_1].link_metrics_send_link_probe(leader_addr, 1)
        self.simulator.go(1)

        self.nodes[SSED_1].link_metrics_send_link_probe(leader_addr, 1)
        self.simulator.go(1)

        self.nodes[SSED_1].link_metrics_send_link_probe(leader_addr, 1)
        self.simulator.go(1)

        # SSED_1 sends a MLE Data Request to retrieve aggregated Forward Series Results
        self.nodes[SSED_1].link_metrics_single_probe_forward(
            leader_addr, SERIES_ID)
        self.simulator.go(1)

        leader_messages = self.simulator.get_messages_sent_by(LEADER)
        msg = leader_messages.next_mle_message(mle.CommandType.DATA_RESPONSE)
        msg.assertMleMessageContainsTlv(mle.LinkMetricsReport)


if __name__ == '__main__':
    unittest.main()
