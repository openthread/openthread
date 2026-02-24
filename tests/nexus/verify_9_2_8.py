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
from pktverify.null_field import nullField


def verify(pv):
    # 9.2.8 Commissioning - Persistent Active/Pending Operational Datasets
    #
    # 9.2.8.1 Topology
    # - Commissioner
    # - Leader
    # - Router 1 (DUT)
    # - MED 1 (DUT)
    # - SED 1 (DUT)
    #
    # 9.2.8.2 Purpose & Description
    # The purpose of this test case is to verify that after a reset, the DUT reattaches to the test network using
    #   parameters set in Active/Pending Operational Datasets.
    #
    # Spec Reference                          | V1.1 Section | V1.3.0 Section
    # ----------------------------------------|--------------|---------------
    # Updating the Active Operational Dataset | 8.7.4        | 8.7.4

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    COMMISSIONER = pv.vars['COMMISSIONER']
    ROUTER_1 = pv.vars['ROUTER_1']
    MED_1 = pv.vars['MED_1']
    SED_1 = pv.vars['SED_1']

    DUT_NAMES = ['ROUTER_1', 'MED_1', 'SED_1']
    DUT_EXTADDRS = [ROUTER_1, MED_1, SED_1]

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A.
    print("Step 1: Ensure topology is formed correctly.")
    child_id_requests = pkts.filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST)
    last_index = pkts.index
    for extaddr in DUT_EXTADDRS:
        f = child_id_requests.copy().filter_wpan_src64(extaddr)
        f.must_next()
        last_index = max(last_index, f.index)
    pkts.index = last_index

    # Step 2: Commissioner
    # - Description: Harness instructs device to send MGMT_PENDING_SET.req to the Leader Anycast or Routing Locator:
    #   - CoAP Request URI: coap://[<L>]:MM/c/ps
    #   - CoAP Payload:
    #     - valid Commissioner Session ID TLV
    #     - Pending Timestamp TLV: 20s
    #     - Active Timestamp TLV: 70s
    #     - Delay Timer TLV: 60s
    #     - Channel TLV: ‘Secondary’
    #     - PAN ID TLV: 0xAFCE
    # - Pass Criteria: N/A.
    print("Step 2: Commissioner sends MGMT_PENDING_SET.req.")
    pkts.filter_wpan_src64(COMMISSIONER). \
        filter_ipv6_dst(pv.vars['LEADER_ALOC']). \
        filter_coap_request(consts.MGMT_PENDING_SET_URI). \
        filter(lambda p: p.coap.tlv.active_timestamp == 70). \
        filter(lambda p: p.coap.tlv.pending_timestamp == 20). \
        filter(lambda p: p.coap.tlv.delay_timer == 60000). \
        must_next()

    # Step 3: Leader
    # - Description: Automatically sends MGMT_PENDING_SET.rsq to the Commissioner.
    # - Pass Criteria:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Accept).
    print("Step 3: Leader sends MGMT_PENDING_SET.rsq.")
    pkts.filter(lambda p: p.coap is not nullField). \
        filter(lambda p: p.coap.code == consts.COAP_CODE_ACK). \
        filter(lambda p: p.coap.tlv is not nullField). \
        filter(lambda p: p.coap.tlv.state == consts.MESHCOP_ACCEPT). \
        must_next()

    # Step 4: Leader
    # - Description: Automatically sends a multicast MLE Data Response to the DUT with the new network data, including
    #   the following TLVs:
    #   - Leader Data TLV: Data Version field incremented, Stable Version field incremented
    #   - Network Data TLV: Commissioner Data TLV (Stable flag set to 0, Border Agent Locator TLV, Commissioner
    #     Session ID TLV)
    #   - Active Timestamp TLV: 70s
    #   - Pending Timestamp TLV: 20s
    # - Pass Criteria: N/A.
    print("Step 4: Leader sends multicast MLE Data Response.")
    pkts.filter_wpan_src64(LEADER). \
        filter_LLANMA(). \
        filter_mle_cmd(consts.MLE_DATA_RESPONSE). \
        filter(lambda p: p.mle.tlv.active_tstamp is not nullField). \
        filter(lambda p: p.mle.tlv.pending_tstamp == 20). \
        must_next()

    # Step 5 & 6: DUTs send MLE Data Request and Leader responds
    # - Description: Automatically sends a MLE Data Request to request the full new network data.
    # - Pass Criteria: The DUT MUST send a MLE Data Request to the Leader and include its current Active Timestamp.
    # - Description: Automatically sends a MLE Data Response including the following TLVs: Active Timestamp TLV,
    #   Pending Timestamp TLV, Active Operational Dataset TLV, Pending Operational Dataset TLV.
    print("Step 5 & 6: DUTs send MLE Data Request and Leader responds.")
    requests = pkts.filter(lambda p: p.wpan.src64 is not nullField). \
        filter(lambda p: p.wpan.src64 in DUT_EXTADDRS). \
        filter_mle_cmd(consts.MLE_DATA_REQUEST). \
        filter(lambda p: p.mle.tlv.active_tstamp is not nullField)

    responses = pkts.filter_wpan_src64(LEADER). \
        filter(lambda p: p.wpan.dst64 is not nullField). \
        filter(lambda p: p.wpan.dst64 in DUT_EXTADDRS). \
        filter_mle_cmd(consts.MLE_DATA_RESPONSE). \
        filter(lambda p: p.mle.tlv.active_tstamp is not nullField). \
        filter(lambda p: p.mle.tlv.pending_tstamp == 20)

    # Verify each DUT sends a request and receives a response, and advance the packet index.
    last_index = pkts.index
    for dut in DUT_EXTADDRS:
        f = requests.copy().filter_wpan_src64(dut)
        f.must_next()
        last_index = max(last_index, f.index)

    for dut in DUT_EXTADDRS:
        f = responses.copy().filter_wpan_dst64(dut)
        f.must_next()
        last_index = max(last_index, f.index)

    # Advance the main pkts index to the end of Step 6
    pkts.index = last_index

    # Step 7: User
    # - Description: Powers down the DUT for 60 seconds.
    # - Pass Criteria: N/A.
    print("Step 7: Powers down the DUT for 60 seconds.")

    # Step 8: Leader, Commissioner
    # - Description: After Delay Timer expires, the network moves to Channel = ‘Secondary’, PAN ID: 0xAFCE.
    # - Pass Criteria: N/A.
    print("Step 8: Network moves to Channel = 'Secondary', PAN ID: 0xAFCE.")
    pkts.filter_wpan_src64(LEADER). \
        filter_LLANMA(). \
        filter_mle_cmd(consts.MLE_DATA_RESPONSE). \
        filter(lambda p: p.mle.tlv.active_tstamp == 70). \
        filter(lambda p: p.mle.tlv.pending_tstamp is nullField). \
        must_next()

    # Step 9: User
    # - Description: Restarts the DUT.
    # - Pass Criteria:
    #   - The DUT MUST attempt to reattach by sending Parent Request using the parameters from Active Operational
    #     Dataset (Channel = ‘Primary’, PANID: 0xFACE).
    #   - The DUT MUST then attach using the parameters from the Pending Operational Dataset (Channel = ‘Secondary’,
    #     PANID: 0xAFCE).
    print("Step 9: DUTs restart and reattach.")
    # Search for reattach attempts for each DUT.
    for dut in DUT_EXTADDRS:
        # Per spec, DUT MUST attempt to reattach on old params first.
        # We start searching from the same point for each DUT to allow intermingled packets.
        pkts_dut = pkts.copy()
        pkts_dut.filter_wpan_src64(dut). \
            filter(lambda p: p.wpan_tap.ch_num == 11 and p.wpan.dst_pan == 0xFACE). \
            filter(lambda p: p.mle.cmd == consts.MLE_PARENT_REQUEST). \
            must_next()

        # Then, it MUST attach using the new params.
        pkts_dut.filter_wpan_src64(dut). \
            filter(lambda p: p.wpan_tap.ch_num == 12 and p.wpan.dst_pan == 0xAFCE). \
            filter(lambda p: p.mle.cmd == consts.MLE_PARENT_REQUEST). \
            must_next()

    # Step 10: Commissioner
    # - Description: Harness verifies connectivity by instructing the Commissioner to send an ICMPv6 Echo Request to
    #   the DUT mesh local address.
    # - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
    print("Step 10: Verify connectivity via ICMPv6 Echo.")
    for name, dut in zip(DUT_NAMES, DUT_EXTADDRS):
        pkts.filter_wpan_src64(COMMISSIONER). \
            filter(lambda p: p.icmpv6.type == consts.ICMPV6_TYPE_ECHO_REQUEST). \
            must_next()

        pkts.filter_wpan_src64(dut). \
            filter(lambda p: p.icmpv6.type == consts.ICMPV6_TYPE_ECHO_REPLY). \
            must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
