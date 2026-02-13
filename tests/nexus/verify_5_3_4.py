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

# 5.3.4 MTD EID-to-RLOC Map Cache
#
# 5.3.4.1 Topology
# - Leader
# - Router_1 (DUT)
# - SED_1 (Attached to DUT)
# - MED_1 (Attached to Leader)
# - MED_2 (Attached to Leader)
# - MED_3 (Attached to Leader)
# - MED_4 (Attached to Leader)
#
# 5.3.4.2 Purpose & Description
# The purpose of this test case is to validate that the DUT is able to maintain an EID-to-RLOC Map Cache for a Sleepy
#   End Device child attached to it. Each EID-to-RLOC Set MUST support at least four non-link-local unicast IPv6
#   addresses.
#
# Spec Reference         | V1.1 Section | V1.3.0 Section
# -----------------------|--------------|---------------
# EID-to-RLOC Map Cache  | 5.5          | 5.5


def verify(pv):
    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    DUT = pv.vars['ROUTER_1']
    SED_1 = pv.vars['SED_1']
    SED_1_MLEID = pv.vars['SED_1_MLEID']

    MEDS_MLEID = verify_utils.get_vars(pv, 'MED', 4, '_MLEID')

    # Step 1: All
    # - Description: Build the topology as described and begin the wireless sniffer.
    # - Pass Criteria: N/A
    print("Step 1: All")

    # Step 2: SED_1
    # - Description: Harness instructs device to send ICMPv6 Echo Requests to MED_1, MED_2, MED_3, and MED_4.
    # - Pass Criteria:
    #   - The DUT MUST generate an Address Query Request on SED_1’s behalf to find each node’s RLOC.
    #   - The Address Query Requests MUST be sent to the Realm-Local All-Routers address (FF03::2).
    #   - CoAP URI-Path: NON POST coap://<FF03::2>
    #   - CoAP Payload:
    #     - Target EID TLV
    print("Step 2: SED_1")
    for mleid in MEDS_MLEID:
        # For each MED, verify a ping request exists. A fresh copy of `pkts` is used
        # to ensure the search is independent of the order of packets for other MEDs.
        pkts.copy().\
            filter_ping_request().\
            filter_wpan_src64(SED_1).\
            filter_ipv6_dst(mleid).\
            must_next()

        # Verify a corresponding address query is sent by the DUT.
        pkts.copy().\
            filter_wpan_src64(DUT).\
            filter_ipv6_dst(consts.REALM_LOCAL_ALL_ROUTERS_ADDRESS).\
            filter_coap_request(consts.ADDR_QRY_URI).\
            filter(lambda p: p.coap.tlv.target_eid == str(mleid)).\
            must_next()

    # Step 3: Leader
    # - Description: Automatically sends Address Notification Messages with RLOC of MED_1, MED_2, MED_3, MED_4.
    # - Pass Criteria: N/A
    print("Step 3: Leader")
    step3_pkts = pkts.copy()
    found_mleids = set()
    for _ in range(len(MEDS_MLEID)):
        p = step3_pkts.filter_wpan_src64(LEADER).\
            filter_coap_request(consts.ADDR_NTF_URI).\
            must_next()
        found_mleids.add(str(p.coap.tlv.target_eid))

    expected_mleids = {str(mleid) for mleid in MEDS_MLEID}
    if found_mleids != expected_mleids:
        raise ValueError(f"Expected ADDR_NTF for MLEIDs {expected_mleids}, but found for {found_mleids}")

    # Step 4: SED_1
    # - Description: Harness instructs the device to send ICMPv6 Echo Requests to MED_1, MED_2, MED_3 and MED_4.
    # - Pass Criteria:
    #   - The DUT MUST cache the addresses in its EID-to-RLOC set for its child SED_1.
    #   - The DUT MUST NOT send an Address Query during this step; If an address query message is sent, the test
    #     fails.
    #   - A ICMPv6 Echo Reply MUST be sent for each ICMPv6 Echo Request from SED_1.
    print("Step 4: SED_1")
    # Identify start of Step 4 by the first Echo Request (ID=5)
    _step4_start_finder = pkts.copy()
    _step4_start_finder.filter_ping_request(identifier=5).\
        filter_wpan_src64(SED_1).\
        must_next()

    # Create a checkpoint cursor pointing to the first packet of Step 4.
    # The `range` function expects a tuple of (wpan_index, eth_index).
    # We use (idx, idx) to ensure we start from this packet and include all subsequent ones.
    checkpoint_step4 = (_step4_start_finder.last_index, _step4_start_finder.last_index)

    # Verify Echo Requests/Replies in Step 4
    step4_pkts = pkts.range(checkpoint_step4)
    for mleid in MEDS_MLEID:
        _ping_req = step4_pkts.filter_ping_request().\
            filter_wpan_src64(SED_1).\
            filter_ipv6_dst(mleid).\
            must_next()

        step4_pkts.filter_ping_reply(identifier=_ping_req.icmpv6.echo.identifier).\
            filter_ipv6_dst(SED_1_MLEID).\
            must_next()

    # Verify NO Address Query was sent by DUT starting from the start of Step 4
    pkts.range(checkpoint_step4).\
        filter_wpan_src64(DUT).\
        filter_coap_request(consts.ADDR_QRY_URI).\
        must_not_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
