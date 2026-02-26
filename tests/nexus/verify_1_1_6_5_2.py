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


def verify(pv):
    # 6.5.2 Child Synchronization after Reset - No Parent Response
    #
    # 6.5.2.1 Topology
    #   - Topology A: DUT as End Device (ED_1)
    #   - Topology B: DUT as Sleepy End Device (SED_1)
    #   - Leader
    #   - Router_1
    #
    # 6.5.2.2 Purpose & Description
    # The purpose of this test case is to validate that after the DUT resets and receives no response from its parent,
    #   it will reattach to the network through a different parent.
    #
    # Spec Reference                   | V1.1 Section | V1.3.0 Section
    # ---------------------------------|--------------|---------------
    # Child Synchronization after Reset | 4.7.6        | 4.6.4

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    DUT = pv.vars['ED_1'] if 'ED_1' in pv.vars else pv.vars['SED_1']

    # Step 1: All
    # - Description: Ensure topology is formed correctly
    # - Pass Criteria: N/A
    print("Step 1: Ensure topology is formed correctly")

    # Step 2: Router_1
    # - Description: Harness silently removes Router_1 from the network
    # - Pass Criteria: N/A
    print("Step 2: Harness silently removes Router_1 from the network")

    # Step 3: ED_1 / SED_1 (DUT)
    # - Description: User is prompted to reset the DUT
    # - Pass Criteria: N/A
    print("Step 3: User is prompted to reset the DUT")

    # Step 4: ED_1 / SED_1 (DUT)
    # - Description: Automatically sends an MLE Child Update Request to Router_1
    # - Pass Criteria:
    #   - The following TLVs MUST be included in the Child Update Request:
    #     - Mode TLV
    #     - Challenge TLV (required for Thread version >= 4)
    #     - Address Registration TLV (optional)
    #   - If the DUT is a SED, it MUST resume polling after sending MLE Child Update Request.
    print("Step 4: DUT sends an MLE Child Update Request to Router_1")
    pkts.filter_wpan_src64(DUT).\
        filter_wpan_dst64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        filter(lambda p: consts.MODE_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.CHALLENGE_TLV in p.mle.tlv.type).\
        must_next()

    if 'SED_1' in pv.vars:
        pkts.filter_wpan_src64(DUT).\
            filter_wpan_cmd(consts.WPAN_DATA_REQUEST).\
            must_next()

    # Step 5: Router_1
    # - Description: No response
    # - Pass Criteria: N/A
    print("Step 5: Router_1 - No response")

    # Step 6: ED_1 / SED_1 (DUT)
    # - Description: Automatically attaches to the Leader
    # - Pass Criteria:
    #   - The DUT MUST attach to the Leader by following the procedure in 6.1.1 Attaching to a Router
    print("Step 6: DUT automatically attaches to the Leader")

    # Procedure in 6.1.1: Parent Request, Parent Response, Child ID Request, Child ID Response

    # Parent Request
    pkts.filter_wpan_src64(DUT).\
        filter_LLARMA().\
        filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        filter(lambda p: consts.CHALLENGE_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.MODE_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.SCAN_MASK_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.VERSION_TLV in p.mle.tlv.type).\
        must_next()

    # Parent Response from Leader
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(DUT).\
        filter_mle_cmd(consts.MLE_PARENT_RESPONSE).\
        filter(lambda p: consts.CHALLENGE_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.CONNECTIVITY_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.LEADER_DATA_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.LINK_LAYER_FRAME_COUNTER_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.LINK_MARGIN_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.RESPONSE_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.SOURCE_ADDRESS_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.VERSION_TLV in p.mle.tlv.type).\
        must_next()

    # Child ID Request to Leader
    pkts.filter_wpan_src64(DUT).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter(lambda p: consts.LINK_LAYER_FRAME_COUNTER_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.MODE_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.RESPONSE_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.TIMEOUT_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.TLV_REQUEST_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.VERSION_TLV in p.mle.tlv.type).\
        must_next()

    # Child ID Response from Leader
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(DUT).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter(lambda p: consts.ADDRESS16_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.LEADER_DATA_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.NETWORK_DATA_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.SOURCE_ADDRESS_TLV in p.mle.tlv.type).\
        must_next()

    # Step 7: Leader
    # - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the DUT
    #   link local address
    # - Pass Criteria:
    #   - The DUT MUST respond with ICMPv6 Echo Reply
    print("Step 7: Leader verifies connectivity via ICMPv6 Echo")
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst64(DUT).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(DUT).\
        filter_wpan_dst64(LEADER).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
