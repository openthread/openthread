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

# Add the current directory to sys.path to find verify_utils
CUR_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(CUR_DIR)

import verify_utils
from pktverify import consts

COAP_TYPE_CON = 0
COAP_TYPE_NON = 1


def verify(pv):
    # 5.3.11 Address Query Timeout Intervals
    #
    # 5.3.11.1 Topology
    # - DUT (Router)
    # - MED_1
    #
    # 5.3.11.2 Purpose & Description
    # The purpose of this test case is to validate the way AQ_TIMEOUT and AQ_RETRY_TIMEOUT intervals are used in the
    #   Address Query transmission algorithm.
    #
    # Spec Reference                         | V1.1 Section | V1.3.0 Section
    # ---------------------------------------|--------------|---------------
    # Transmission of Address Query Messages | 5.4.2.1      | 5.4.2.1

    pkts = pv.pkts
    pv.summary.show()

    DUT = pv.vars['DUT']
    MED_1 = pv.vars['MED_1']

    # Step 1: All
    # - Description: Build the topology as described and begin the wireless sniffer.
    # - Pass Criteria: N/A
    print("Step 1: All")

    # Step 2: MED_1
    # - Description: Harness instructs device to send an ICMPv6 Echo Request to a non-existent mesh-local address X.
    # - Pass Criteria:
    #   - The DUT MUST issue an Address Query Request on MED_1’s behalf.
    #   - The Address Query Request MUST be sent to the Realm-Local All-Routers multicast address (FF03::2).
    #   - CoAP URI-Path: NON POST coap://<FF03::2>
    #   - CoAP Payload:
    #     - Target EID TLV – non-existent mesh-local address X
    #   - An Address Query Notification MUST NOT be received within AQ_TIMEOUT interval.
    print("Step 2: MED_1")
    echo1 = pkts.filter_wpan_src64(MED_1).\
        filter_ping_request().\
        must_next()
    idx1 = pkts.index

    aq1 = pkts.filter_wpan_src64(DUT).\
        filter_RLARMA().\
        filter_coap_request(consts.ADDR_QRY_URI).\
        filter(lambda p: p.coap.type == COAP_TYPE_NON).\
        filter(lambda p: p.coap.tlv.target_eid == echo1.ipv6.dst).\
        must_next()

    # Step 3: MED_1
    # - Description: Harness instructs device to send an ICMPv6 Echo Request to a non-existent mesh-local address X
    #   before ADDRESS_QUERY_INITIAL_RETRY_DELAY timeout expires.
    # - Pass Criteria:
    #   - The DUT MUST NOT initiate a new Address Query frame.
    print("Step 3: MED_1")
    echo2 = pkts.filter_wpan_src64(MED_1).\
        filter_ping_request().\
        must_next()
    idx2 = pkts.index

    # Verify Step 2: An Address Query Notification MUST NOT be received within AQ_TIMEOUT interval.
    # We check between echo1 and echo2.
    pkts.range(idx1, idx2).\
        filter_wpan_dst64(DUT).\
        filter_coap_request(consts.ADDR_NTF_URI).\
        must_not_next()

    # Step 4: MED_1
    # - Description: Harness instructs device to send an ICMPv6 Echo Request to a non-existent mesh-local address X
    #   after ADDRESS_QUERY_INITIAL_RETRY_DELAY timeout expires.
    # - Pass Criteria:
    #   - The DUT MUST issue an Address Query Request on MED_1’s behalf.
    #   - The Address Query Request MUST be sent to the Realm-Local All-Routers multicast address (FF03::2).
    #   - CoAP URI-Path: NON POST coap://<FF03::2>
    #   - CoAP Payload:
    #     - Target EID TLV – non-existent mesh-local address X
    print("Step 4: MED_1")
    echo3 = pkts.filter_wpan_src64(MED_1).\
        filter_ping_request().\
        must_next()
    idx3 = pkts.index

    # Verify Step 3: The DUT MUST NOT initiate a new Address Query frame.
    # This means no new AQ between echo2 and echo3.
    pkts.range(idx2, idx3).\
        filter_wpan_src64(DUT).\
        filter_RLARMA().\
        filter_coap_request(consts.ADDR_QRY_URI).\
        filter(lambda p: p.coap.type == COAP_TYPE_NON).\
        filter(lambda p: p.coap.tlv.target_eid == echo2.ipv6.dst).\
        filter(lambda p: p.coap.mid != aq1.coap.mid).\
        must_not_next()

    # Verify Step 4: The DUT MUST issue an Address Query Request.
    # This means a new AQ must be present after echo3.
    pkts.range(idx3).\
        filter_wpan_src64(DUT).\
        filter_RLARMA().\
        filter_coap_request(consts.ADDR_QRY_URI).\
        filter(lambda p: p.coap.type == COAP_TYPE_NON).\
        filter(lambda p: p.coap.tlv.target_eid == echo3.ipv6.dst).\
        filter(lambda p: p.coap.mid != aq1.coap.mid).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
