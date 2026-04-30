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
    # 6.1.1 Attaching to a Router
    #
    # 6.1.1.1 Topology
    #   - Topology A: DUT as End Device (ED_1)
    #   - Topology B: DUT as Sleepy End Device (SED_1)
    #   - Leader
    #
    # 6.1.1.2 Purpose & Description
    #   The purpose of this test case is to validate that the DUT is able to successfully attach to a network.
    #
    # Spec Reference        | V1.1 Section | V1.3.0 Section
    # ----------------------|--------------|---------------
    # Attaching to a Parent | 4.7.1        | 4.5.1

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']

    if 'ED_1' in pv.vars:
        dut = pv.vars['ED_1']
        name = 'ED_1'
    elif 'SED_1' in pv.vars:
        dut = pv.vars['SED_1']
        name = 'SED_1'
    else:
        raise ValueError("Neither ED_1 nor SED_1 found in topology vars")

    # Step 1: Leader
    #   - Description: Begin wireless sniffer and ensure the Leader is sending MLE Advertisements.
    #   - Pass Criteria: N/A
    print("Step 1: Begin wireless sniffer and ensure the Leader is sending MLE Advertisements.")
    pkts.filter_wpan_src64(LEADER).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        must_next()

    # Step 2: ED_1 / SED_1 (DUT)
    #   - Description: Automatically begins attach process by sending a multicast MLE Parent Request.
    #   - Pass Criteria:
    #     - The DUT MUST send a MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2)
    #       with an IP Hop Limit of 255.
    #     - The following TLVs MUST be present in the Parent Request:
    #       - Challenge TLV
    #       - Mode TLV
    #       - Scan Mask TLV = 0x80 (active Routers)
    #       - Version TLV
    #     - The Key Identifier Mode of the Security Control field of the MAC frame Auxiliary Security Header
    #       MUST be set to '0x02'.
    print(f"Step 2: {name} (DUT) sends a multicast MLE Parent Request.")

    pkts.filter_wpan_src64(dut).\
        filter_LLARMA().\
        filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        filter(lambda p: p.ipv6.hlim == 255).\
        filter(lambda p: consts.CHALLENGE_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.MODE_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.SCAN_MASK_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.VERSION_TLV in p.mle.tlv.type).\
        filter(lambda p: p.mle.tlv.scan_mask.r == 1).\
        filter(lambda p: p.mle.tlv.scan_mask.e == 0).\
        filter(lambda p: p.wpan.aux_sec.key_id_mode == 2).\
        must_next()

    # Step 3: Leader
    #   - Description: Automatically responds with a MLE Parent Response.
    #   - Pass Criteria: N/A
    print("Step 3: Leader responds with a MLE Parent Response.")

    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(dut).\
        filter_mle_cmd(consts.MLE_PARENT_RESPONSE).\
        must_next()

    # Step 4: ED_1 / SED_1 (DUT)
    #   - Description: Receives the MLE Parent Response and automatically sends a MLE Child ID Request.
    #   - Pass Criteria:
    #     - The DUT MUST send a MLE Child ID Request.
    #     - The following TLVs MUST be present in the Child ID Request:
    #       - Address Registration TLV
    #       - Link-layer Frame Counter TLV
    #       - Mode TLV
    #       - Response TLV
    #       - Timeout TLV
    #       - TLV Request TLV
    #       - Version TLV
    #       - MLE Frame Counter TLV (optional)
    #     - The Key Identifier Mode of the Security Control field of the MAC frame Auxiliary Security Header
    #       MUST be set to '0x02'.
    print(f"Step 4: {name} (DUT) sends a MLE Child ID Request.")

    pkts.filter_wpan_src64(dut).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter(lambda p: consts.ADDRESS_REGISTRATION_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.LINK_LAYER_FRAME_COUNTER_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.MODE_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.RESPONSE_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.TIMEOUT_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.TLV_REQUEST_TLV in p.mle.tlv.type).\
        filter(lambda p: consts.VERSION_TLV in p.mle.tlv.type).\
        filter(lambda p: p.wpan.aux_sec.key_id_mode == 2).\
        must_next()

    # Step 5: Leader
    #   - Description: Automatically responds with MLE Child ID Response.
    #   - Pass Criteria: N/A
    print("Step 5: Leader responds with MLE Child ID Response.")

    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(dut).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        must_next()

    # Step 6: Leader
    #   - Description: Harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request
    #     to the DUT link local address.
    #   - Pass Criteria:
    #     - The DUT MUST respond with an ICMPv6 Echo Reply.
    print("Step 6: Harness verifies connectivity using ICMPv6 Echo Request/Reply.")

    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(LEADER).\
        filter_wpan_dst64(dut).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(dut).\
        filter_wpan_dst64(LEADER).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
