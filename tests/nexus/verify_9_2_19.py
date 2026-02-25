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
    # 9.2.19 Getting the Pending Operational Dataset
    #
    # 9.2.19.1 Topology
    # - Topology A: DUT as Leader, Commissioner (Non-DUT)
    # - Topology B: Leader (Non-DUT), DUT as Commissioner
    #
    # 9.2.19.2 Purpose & Description
    # - DUT as Leader (Topology A): The purpose of this test case is to verify the DUT’s behavior when receiving
    #   MGMT_PENDING_GET.req directly from the active Commissioner.
    # - DUT as Commissioner (Topology B): The purpose of this test case is to verify that the DUT can read Pending
    #   Operational Dataset parameters direct from the Leader using the MGMT_PENDING_GET.req command.
    #
    # Spec Reference                            | V1.1 Section | V1.3.0 Section
    # ------------------------------------------|--------------|---------------
    # Updating the Pending Operational Dataset | 8.7.5        | 8.7.5

    pkts = pv.pkts
    pv.summary.show()

    IS_DUT_COMMISSIONER = pv.test_info.testcase == 'test_9_2_19_B'

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A.
    print("Step 1: All")

    # Step 2: Topology B Commissioner DUT / Topology A Leader DUT
    # - Description:
    #   - Topology B: User instructs DUT to send MGMT_PENDING_GET.req to Leader.
    #   - Topology A: Harness instructs Commissioner to send MGMT_PENDING_GET.req to DUT Anycast or Routing Locator:
    #     - CoAP Request URI: coap://[<L>]:MM/c/pg
    #     - CoAP Payload: <empty>
    # - Pass Criteria:
    #   - Topology B: The MGMT_PENDING_GET.req frame MUST have the following format:
    #     - CoAP Request URI: coap://[<L>]:MM/c/pg
    #     - CoAP Payload: <empty> (get all Pending Operational Dataset parameters)
    #     - The Destination Address of MGMT_PENDING_GET.req frame MUST be Leader’s Anycast or Routing Locator (ALOC or
    #       RLOC):
    #       - ALOC: Mesh Local prefix with an IID of 0000:00FF:FE00:FC00
    #       - RLOC: Mesh Local prefix with and IID of 0000:00FF:FE00:xxxx where xxxx is a 16-bit value that embeds the
    #         Router ID
    #   - Topology A: N/A.
    print("Step 2: Topology B Commissioner DUT / Topology A Leader DUT")
    _pkt = pkts.filter_coap_request(consts.MGMT_PENDING_GET_URI).\
        filter(lambda p: p.coap.payload is nullField).\
        must_next()

    if IS_DUT_COMMISSIONER:
        _pkt.must_verify(lambda p: verify_utils.is_leader_aloc_or_rloc(p.ipv6.dst))

    # Step 3: Leader
    # - Description: Automatically responds to MGMT_PENDING_GET.req with a MGMT_PENDING_GET.rsp to Commissioner.
    # - Pass Criteria: For DUT = Leader: The MGMT_PENDING_GET.rsp frame MUST have the following format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: <empty> (no Pending Operational Dataset)
    print("Step 3: Leader")
    pkts.filter_coap_ack(consts.MGMT_PENDING_GET_URI).\
        filter(lambda p: p.coap.payload is nullField).\
        must_next()

    # Step 4: Topology B Commissioner DUT / Topology A Leader DUT
    # - Description:
    #   - Topology B: User instructs DUT to send MGMT_PENDING_SET.req to Leader.
    #   - Topology A: Harness instructs Commissioner to send MGMT_PENDING_SET.req to DUT’s Anycast or Routing Locator:
    #     - CoAP Request URI: coap://[<L>]:MM/c/ps
    #     - CoAP Payload: Active Timestamp TLV: 60s, Commissioner Session ID TLV (valid), Delay Timer TLV: 1 minute,
    #       PAN ID TLV: 0xAFCE (new value), Pending Timestamp TLV: 30s.
    # - Pass Criteria:
    #   - Topology B: The MGMT_PENDING_SET.req frame MUST have the following format:
    #     - CoAP Request URI: coap://[<L>]:MM/c/ps
    #     - CoAP Payload: Active Timestamp TLV: 60s, Commissioner Session ID TLV (valid), Delay Timer TLV: 1 minute,
    #       Pending Timestamp TLV: 30s, PAN ID TLV: 0xAFCE (new value).
    #     - The Destination Address of MGMT_PENDING_SET.req frame MUST be the Leader’s Anycast or Routing Locator (ALOC
    #       or RLOC):
    #       - ALOC: Mesh Local prefix with an IID of 0000:00FF:FE00:FC00
    #       - RLOC: Mesh Local prefix with and IID of 0000:00FF:FE00:xxxx where xxxx is a 16-bit value that embeds the
    #         Router ID.
    #   - Topology A: N/A.
    print("Step 4: Topology B Commissioner DUT / Topology A Leader DUT")
    _pkt = pkts.filter_coap_request(consts.MGMT_PENDING_SET_URI).\
        filter(lambda p: p.coap.tlv.active_timestamp == 60).\
        filter(lambda p: p.coap.tlv.delay_timer == 60000).\
        filter(lambda p: p.coap.tlv.pending_timestamp == 30).\
        filter(lambda p: p.coap.tlv.pan_id == 0xafce).\
        filter(lambda p: consts.NM_COMMISSIONER_SESSION_ID_TLV in p.coap.tlv.type).\
        must_next()

    if IS_DUT_COMMISSIONER:
        _pkt.must_verify(lambda p: verify_utils.is_leader_aloc_or_rloc(p.ipv6.dst))

    # Step 5: Leader
    # - Description: Automatically responds to MGMT_PENDING_SET.req with a MGMT_PENDING_SET.rsp to Commissioner.
    # - Pass Criteria: For DUT = Leader: The MGMT_PENDING_SET.rsp frame MUST have the following format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Accept (01))
    print("Step 5: Leader")
    pkts.filter_coap_ack(consts.MGMT_PENDING_SET_URI).\
        filter(lambda p: p.coap.tlv.state == 1).\
        must_next()

    # Step 6: Topology B Commissioner DUT / Topology A Leader DUT
    # - Description:
    #   - Topology B: User instructs DUT to send MGMT_PENDING_GET.req to Leader.
    #   - Topology A: Harness instructs Commissioner to send MGMT_PENDING_GET.req to DUT’s Anycast or Routing Locator:
    #     - CoAP Request URI: coap://[<L>]:MM/c/pg
    #     - CoAP Payload: <empty>
    # - Pass Criteria:
    #   - Topology B: The MGMT_PENDING_GET.req frame MUST have the following format:
    #     - CoAP Request URI: coap://[<L>]:MM/c/pg
    #     - CoAP Payload: <empty> (get all Pending Operational Dataset parameters)
    #     - The Destination Address of MGMT_PENDING_GET.req frame MUST be the Leader’s Anycast or Routing Locator (ALOC
    #       or RLOC):
    #       - ALOC: Mesh Local prefix with an IID of 0000:00FF:FE00:FC00
    #       - RLOC: Mesh Local prefix with and IID of 0000:00FF:FE00:xxxx where xxxx is a 16-bit value that embeds the
    #         Router ID
    #   - Topology A: N/A.
    print("Step 6: Topology B Commissioner DUT / Topology A Leader DUT")
    _pkt = pkts.filter_coap_request(consts.MGMT_PENDING_GET_URI).\
        filter(lambda p: p.coap.payload is nullField).\
        must_next()

    if IS_DUT_COMMISSIONER:
        _pkt.must_verify(lambda p: verify_utils.is_leader_aloc_or_rloc(p.ipv6.dst))

    # Step 7: Leader
    # - Description: Automatically responds to MGMT_PENDING_GET.req with a MGMT_PENDING_GET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The MGMT_PENDING_GET.rsp frame MUST have the following format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload:
    #     - Active Timestamp TLV
    #     - Channel TLV
    #     - Channel Mask TLV
    #     - Delay Timer TLV
    #     - Extended PAN ID TLV
    #     - Mesh-Local Prefix TLV
    #     - Network Master Key TLV
    #     - Network Name TLV
    #     - PAN ID TLV
    #     - Pending Timestamp TLV
    #     - PSKc TLV
    #     - Security Policy TLV
    print("Step 7: Leader")
    pkts.filter_coap_ack(consts.MGMT_PENDING_GET_URI).\
        filter(lambda p: {
            consts.NM_ACTIVE_TIMESTAMP_TLV,
            consts.NM_CHANNEL_TLV,
            consts.NM_CHANNEL_MASK_TLV,
            consts.NM_DELAY_TIMER_TLV,
            consts.NM_EXTENDED_PAN_ID_TLV,
            consts.NM_NETWORK_MESH_LOCAL_PREFIX_TLV,
            consts.NM_NETWORK_KEY_TLV,
            consts.NM_NETWORK_NAME_TLV,
            consts.NM_PAN_ID_TLV,
            consts.NM_PENDING_TIMESTAMP_TLV,
            consts.NM_PSKC_TLV,
            consts.NM_SECURITY_POLICY_TLV
        } <= set(p.coap.tlv.type)).\
        must_next()

    # Step 8: Topology B Commissioner DUT / Topology A Leader DUT
    # - Description:
    #   - Topology B: User instructs DUT to send MGMT_PENDING_GET.req to Leader.
    #   - Topology A: Harness instructs Commissioner to send MGMT_PENDING_GET.req to DUT’s Anycast or Routing Locator:
    #     - CoAP Request URI: coap://[<L>]:MM/c/pg
    #     - CoAP Payload: Get TLV specifying: PAN ID TLV
    # - Pass Criteria:
    #   - Topology B: The MGMT_PENDING_GET.req frame MUST have the following format:
    #     - CoAP Request URI: coap://[<L>]:MM/c/pg
    #     - CoAP Payload: Get TLV specifying: PAN ID TLV
    #     - The Destination Address of MGMT_PENDING_GET.req frame MUST be the Leader’s Anycast or Routing Locator (ALOC
    #       or RLOC):
    #       - ALOC: Mesh Local prefix with an IID of 0000:00FF:FE00:FC00
    #       - RLOC: Mesh Local prefix with and IID of 0000:00FF:FE00:xxxx where xxxx is a 16-bit value that embeds the
    #         Router ID
    #   - Topology A: N/A.
    print("Step 8: Topology B Commissioner DUT / Topology A Leader DUT")
    _pkt = pkts.filter_coap_request(consts.MGMT_PENDING_GET_URI).\
        filter(lambda p: consts.TLV_REQUEST_TLV in p.coap.tlv.type and\
               p.coap.tlv.tlv_request == bytes([consts.NM_PAN_ID_TLV]).hex()).\
        must_next()

    if IS_DUT_COMMISSIONER:
        _pkt.must_verify(lambda p: verify_utils.is_leader_aloc_or_rloc(p.ipv6.dst))

    # Step 9: Leader
    # - Description: Automatically responds to MGMT_PENDING_GET.req with a MGMT_PENDING_GET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The MGMT_PENDING_GET.rsp frame MUST have the following format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: Delay Timer TLV, PAN ID TLV
    print("Step 9: Leader")
    pkts.filter_coap_ack(consts.MGMT_PENDING_GET_URI).\
        filter(lambda p: {
            consts.NM_DELAY_TIMER_TLV,
            consts.NM_PAN_ID_TLV
        } <= set(p.coap.tlv.type)).\
        must_next()

    # Step 10: Harness
    # - Description: Wait for 92 seconds to allow pending data to become operational.
    # - Pass Criteria: N/A.
    print("Step 10: Harness")

    # Step 11: Topology B Commissioner DUT / Topology A Leader DUT
    # - Description:
    #   - Topology B: User instructs DUT to send MGMT_PENDING_GET.req to Leader.
    #   - Topology A: Harness instructs Commissioner to send MGMT_PENDING_GET.req to DUT’s Anycast or Routing Locator:
    #     - CoAP Request URI: coap://[<L>]:MM/c/pg
    #     - CoAP Payload: <empty>
    # - Pass Criteria:
    #   - Topology B: The MGMT_PENDING_GET.req frame MUST have the following format:
    #     - CoAP Request URI: coap://[<L>]:MM/c/pg
    #     - CoAP Payload: <empty> (get all Pending Operational Dataset parameters)
    #     - The Destination Address of MGMT_PENDING_GET.req frame MUST be the Leader’s Anycast or Routing Locator (ALOC
    #       or RLOC):
    #       - ALOC: Mesh Local prefix with an IID of 0000:00FF:FE00:FC00
    #       - RLOC: Mesh Local prefix with and IID of 0000:00FF:FE00:xxxx where xxxx is a 16-bit value that embeds the
    #         Router ID
    #   - Topology A: N/A.
    print("Step 11: Topology B Commissioner DUT / Topology A Leader DUT")
    _pkt = pkts.filter_coap_request(consts.MGMT_PENDING_GET_URI).\
        filter(lambda p: p.coap.payload is nullField).\
        must_next()

    if IS_DUT_COMMISSIONER:
        _pkt.must_verify(lambda p: verify_utils.is_leader_aloc_or_rloc(p.ipv6.dst))

    # Step 12: Leader
    # - Description: Automatically responds to MGMT_PENDING_GET.req with a MGMT_PENDING_GET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The MGMT_PENDING_GET.rsp frame MUST have the following format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: <empty> (no Pending Operational Dataset)
    print("Step 12: Leader")
    pkts.filter_coap_ack(consts.MGMT_PENDING_GET_URI).\
        filter(lambda p: p.coap.payload is nullField).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
