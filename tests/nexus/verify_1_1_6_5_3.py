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
    # 6.5.3 Child Synchronization after Reset - MLE Child Update Request
    #
    # 6.5.3.1 Topology
    # - Topology A: DUT as End Device (ED_1) attached to Leader
    # - Topology B: DUT as Sleepy End Device (SED_1) attached to Leader
    #
    # 6.5.3.2 Purpose & Description
    # The purpose of this test case is to validate that after the DUT resets for a time period shorter than the Child
    # Timeout value, it sends an MLE Child Update Request to its parent and remains connected to its parent.
    #
    # Spec Reference                    | V1.1 Section | V1.3.0 Section
    # ----------------------------------|--------------|---------------
    # Child Synchronization after Reset | 4.7.6        | 4.6.4

    pkts = pv.pkts
    pv.summary.show()

    ROUTER_1 = pv.vars['ROUTER_1']

    if 'ED_1' in pv.vars:
        dut = pv.vars['ED_1']
        name = 'ED_1'
        topology = 'A'
    elif 'SED_1' in pv.vars:
        dut = pv.vars['SED_1']
        name = 'SED_1'
        topology = 'B'
    else:
        raise ValueError("Neither ED_1 nor SED_1 found in topology vars")

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A.
    print("Step 1: All")

    # Step 2: ED_1 / SED_1 (DUT)
    # - Description: Test Harness Prompt: Reset End Device for a time shorter than the Child Timeout Duration.
    # - Pass Criteria: N/A.
    print(f"Step 2: {name} (DUT)")

    # Step 3: ED_1 / SED_1 (DUT)
    # - Description: Automatically sends MLE Child Update Request to the Leader.
    # - Pass Criteria:
    #   - The following TLVs MUST be included in the Child Update Request:
    #     - Mode TLV
    #     - Challenge TLV (required for Thread version >= 4)
    #     - Address Registration TLV (optional)
    #   - If the DUT is a SED, it MUST resume polling after sending MLE Child Update Request.
    print(f"Step 3: {name} (DUT)")

    pkts.filter_wpan_src64(dut).\
      filter_wpan_dst64(ROUTER_1).\
      filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
      filter(lambda p: {
        consts.MODE_TLV,
        consts.CHALLENGE_TLV
      } <= set(p.mle.tlv.type)).\
      must_next()

    if topology == 'B':
        with pkts.save_index():
            pkts.filter(lambda p: p.wpan.src16 == pv.vars[name + '_RLOC16'] and
                                  p.wpan.dst16 == pv.vars['ROUTER_1_RLOC16']).\
              filter_wpan_cmd(consts.WPAN_DATA_REQUEST).\
              must_next()

    # Step 4: Leader
    # - Description: Automatically sends an MLE Child Update Response.
    # - Pass Criteria: N/A.
    print("Step 4: Leader")

    pkts.filter_wpan_src64(ROUTER_1).\
      filter_wpan_dst64(dut).\
      filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
      must_next()

    # Step 5: Leader
    # - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the
    #   DUT link local address.
    # - Pass Criteria:
    #   - The DUT MUST respond with ICMPv6 Echo Reply.
    print("Step 5: Leader")

    _pkt = pkts.filter_ping_request().\
      filter_wpan_src64(ROUTER_1).\
      filter_wpan_dst64(dut).\
      must_next()

    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
      filter_wpan_src64(dut).\
      filter_wpan_dst64(ROUTER_1).\
      must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
