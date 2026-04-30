#!/usr/bin/env python3
#
#  Copyright (c) 2026, The OpenThread Authors.
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

import sys
import os

import verify_utils
from pktverify import consts


def verify(pv):
    # 5.2.7 REED Synchronization
    #
    # 5.2.7.1 Topology
    # - Topology A
    # - Topology B
    # - Build a topology that has a total of 16 active routers, including the Leader, with no communication
    #   constraints.
    #
    # 5.2.7.2 Purpose & Description
    # The purpose of this test case is to validate the REED’s Synchronization procedure after attaching to a network
    #   with multiple Routers. A REED MUST process incoming Advertisements and perform a one-way frame-counter
    #   synchronization with at least 3 neighboring Routers. When Router receives unicast MLE Link Request from REED,
    #   it replies with MLE Link Accept.
    #
    # Spec Reference                     | V1.1 Section | V1.3.0 Section
    # -----------------------------------|--------------|---------------
    # REED and FED Synchronization       | 4.7.7.4      | 4.7.1.4

    pkts = pv.pkts
    pv.summary.show()

    REED_1 = pv.vars['REED_1']
    K_MIN_NEIGHBORS = 3

    def verify_unique_neighbor_count(pkt_stream, addr_attr, min_count, error_msg_prefix):
        neighbors = set()
        while True:
            pkt = pkt_stream.next()
            if pkt is None:
                break

            neighbors.add(getattr(pkt.wpan, addr_attr))
            if len(neighbors) >= min_count:
                break

        if len(neighbors) < min_count:
            raise Exception("%s only %d neighbors, expected at least %d" %
                            (error_msg_prefix, len(neighbors), min_count))

    # Step 1: All
    # - Description: Topology formation
    # - The REED device is added last
    # - If DUT=REED
    #   - the DUT may attach to any router
    # - If DUT=Router
    #   - the REED is not allowed to attach to the DUT
    #   - the REED is limited to 2 neighbors, including the DUT
    # - Pass Criteria: N/A
    print("Step 1: All")

    # Step 2: REED_1
    # - Description: Automatically joins the topology
    # - Pass Criteria:
    #   - For DUT = REED: The DUT MUST NOT attempt to become an active router by sending an Address Solicit Request
    print("Step 2: REED_1")
    pkts.filter_wpan_src64(REED_1).\
        filter_coap_request(consts.ADDR_SOL_URI).\
        must_not_next()

    # Step 3: REED_1
    # - Description: Automatically sends Link Request to neighboring Routers
    # - Pass Criteria:
    #   - For DUT = REED: The DUT MUST send a unicast Link Request to at least three neighbors
    #   - The following TLVs MUST be present in the Link Request:
    #     - Challenge TLV
    #     - Leader Data TLV
    #     - Source Address TLV
    #     - Version TLV
    print("Step 3: REED_1")
    link_requests = pkts.filter_wpan_src64(REED_1).\
        filter_mle_cmd(consts.MLE_LINK_REQUEST).\
        filter(lambda p: {
                          consts.CHALLENGE_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.SOURCE_ADDRESS_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type))

    verify_unique_neighbor_count(link_requests, 'dst64', K_MIN_NEIGHBORS, "REED_1 sent Link Requests to")

    # Step 4: Router_1
    # - Description: Automatically sends Link Accept to REED_1
    # - Pass Criteria:
    #   - For DUT = Router: The DUT MUST send Link Accept to the REED; the DUT MUST NOT send a Link Accept And Request
    #     message.
    #   - The following TLVs MUST be present in the Link Accept message:
    #     - Link-layer Frame Counter TLV
    #     - Source Address TLV
    #     - Response TLV
    #     - Version TLV
    #     - MLE Frame Counter TLV (optional)
    print("Step 4: Router_1")
    link_accepts = pkts.filter_wpan_dst64(REED_1).\
        filter_mle_cmd(consts.MLE_LINK_ACCEPT).\
        filter(lambda p: {
                          consts.LINK_LAYER_FRAME_COUNTER_TLV,
                          consts.SOURCE_ADDRESS_TLV,
                          consts.RESPONSE_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type))

    verify_unique_neighbor_count(link_accepts, 'src64', K_MIN_NEIGHBORS, "REED_1 received Link Accepts from")

    # Verify that no router sends a Link Accept and Request
    pkts.filter_wpan_dst64(REED_1).\
        filter_mle_cmd(consts.MLE_LINK_ACCEPT_AND_REQUEST).\
        must_not_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
