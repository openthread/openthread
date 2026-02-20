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
import struct

# Add the current directory to sys.path to find verify_utils
CUR_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(CUR_DIR)

import verify_utils
from pktverify import consts
from pktverify.null_field import nullField


# Monkey-patch CoapTlvParser to parse MeshCoP TLVs in CoAP payload
def meshcop_coap_tlv_parse(t, v):
    kvs = []
    if t == consts.NM_COMMISSIONER_SESSION_ID_TLV:
        kvs.append(('commissioner_session_id', str(struct.unpack('>H', v)[0])))
    elif t == consts.NM_STEERING_DATA_TLV:
        kvs.append(('steering_data', v.hex()))
    elif t == consts.NM_BORDER_AGENT_LOCATOR_TLV:
        kvs.append(('border_agent_locator', str(struct.unpack('>H', v)[0])))
    elif t == consts.TLV_REQUEST_TLV:
        kvs.append(('tlv_request', v.hex()))
    return kvs


def verify(pv):
    # 9.2.1 Commissioner – MGMT_COMMISSIONER_GET.req & rsp
    #
    # 9.2.1.1 Topology
    # - Topology A: DUT as Leader, Commissioner (Non-DUT)
    # - Topology B: Leader (Non-DUT), DUT as Commissioner
    #
    # 9.2.1.2 Purpose & Description
    # - DUT as Leader (Topology A): The purpose of this test case is to verify Leader’s behavior when receiving
    #   MGMT_COMMISSIONER_GET.req direct from the active Commissioner.
    # - DUT as Commissioner (Topology B): The purpose of this test case is to verify that the active Commissioner can
    #   read Commissioner Dataset parameters direct from the Leader using MGMT_COMMISSIONER_GET.req command.
    #
    # Spec Reference                    | V1.1 Section | V1.3.0 Section
    # ----------------------------------|--------------|---------------
    # Updating the Commissioner Dataset | 8.7.3        | 8.7.3

    # Add MeshCoP TLVs to CoapTlvParser
    old_parse = verify_utils.CoapTlvParser.parse

    from pktverify import layer_fields
    layer_fields._LAYER_FIELDS['coap.tlv.tlv_request'] = layer_fields._bytes

    def new_parse(t, v):
        if t in (consts.NM_COMMISSIONER_SESSION_ID_TLV, consts.NM_STEERING_DATA_TLV,
                 consts.NM_BORDER_AGENT_LOCATOR_TLV, consts.TLV_REQUEST_TLV):
            return meshcop_coap_tlv_parse(t, v)
        return old_parse(t, v)

    verify_utils.CoapTlvParser.parse = staticmethod(new_parse)

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    COMMISSIONER = pv.vars['COMMISSIONER']

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A.
    print("Step 1: Ensure topology is formed correctly.")

    # Step 2: Topology B Commissioner DUT / Topology A Leader DUT
    # - Description:
    #   - Topology B: User instructs DUT to send MGMT_COMMISSIONER_GET.req to Leader.
    #   - Topology A: Harness instructs Commissioner to send MGMT_COMMISSIONER_GET.req to DUT’s Anycast or Routing
    #     Locator.
    # - Pass Criteria:
    #   - Topology B: Verify MGMT_COMMISSIONER_GET.req frame has the following format:
    #     - CoAP Request URI: coap://[<L>]:MM/c/cg
    #     - CoAP Payload: <empty> - get all Commissioner Dataset parameters
    #     - Verify Destination Address of MGMT_COMMISSIONER_GET.req frame is DUT’s Anycast or Routing Locator (ALOC or
    #       RLOC):
    #       - ALOC: Mesh Local prefix with an IID of 0000:00FF:FE00:FC00.
    #       - RLOC: Mesh Local prefix with and IID of 0000:00FF:FE00:xxxx where xxxx is a 16-bit value that embeds the
    #         Router ID.
    #   - Topology A: CoAP Request URI: coap://[<L>]:MM/c/cg. CoAP Payload: <empty> - get all Commissioner Dataset
    #     parameters.
    print("Step 2: Commissioner sends MGMT_COMMISSIONER_GET.req with empty payload.")
    pkts.filter_coap_request(consts.MGMT_COMMISSIONER_GET_URI).\
        filter(lambda p: p.coap.payload is nullField).\
        must_next()

    # Step 3: Leader
    # - Description: Automatically sends MGMT_COMMISSIONER_GET.rsp to Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_COMMISSIONER_GET.rsp to the Commissioner with the
    #   following format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: (entire Commissioner Dataset)
    #     - Border Agent Locator TLV
    #     - Commissioner Session ID TLV
    #     - Steering Data TLV.
    print("Step 3: Leader sends MGMT_COMMISSIONER_GET.rsp with entire Commissioner Dataset.")
    pkts.filter_coap_ack(consts.MGMT_COMMISSIONER_GET_URI).\
        filter(lambda p: {
            consts.NM_BORDER_AGENT_LOCATOR_TLV,
            consts.NM_COMMISSIONER_SESSION_ID_TLV,
            consts.NM_STEERING_DATA_TLV
        } <= set(p.coap.tlv.type)).\
        must_next()

    # Step 4: Topology B Commissioner DUT / Topology A Leader DUT
    # - Description:
    #   - Topology B: User instructs DUT to send MGMT_COMMISSIONER_GET.req to Leader.
    #   - Topology A: Harness instructs Commissioner to send MGMT_COMMISSIONER_GET.req to DUT’s Anycast or Routing
    #     Locator.
    # - Pass Criteria:
    #   - Topology B: Verify MGMT_COMMISSIONER_GET.req frame has the following format:
    #     - CoAP Request URI: coap://[<L>]:MM/c/cg
    #     - CoAP Payload: Get TLV specifying: Commissioner Session ID TLV, Steering Data TLV
    #     - Verify Destination Address of MGMT_COMMISSIONER_GET.req frame is DUT’s Anycast or Routing Locator (ALOC or
    #       RLOC):
    #       - ALOC: Mesh Local prefix with an IID of 0000:00FF:FE00:FC00.
    #       - RLOC: Mesh Local prefix with and IID of 0000:00FF:FE00:xxxx where xxxx is a 16-bit value that embeds the
    #         Router ID.
    #   - Topology A: CoAP Request URI: coap://[<L>]:MM/c/cg. CoAP Payload: Get TLV specifying: Commissioner Session ID
    #     TLV, Steering Data TLV.
    print("Step 4: Commissioner sends MGMT_COMMISSIONER_GET.req with Get TLV (Session ID, Steering Data).")
    pkts.filter_coap_request(consts.MGMT_COMMISSIONER_GET_URI).\
        filter(lambda p: consts.TLV_REQUEST_TLV in p.coap.tlv.type and\
               p.coap.tlv.tlv_request == bytes([consts.NM_COMMISSIONER_SESSION_ID_TLV, consts.NM_STEERING_DATA_TLV]).hex()).\
        must_next()

    # Step 5: Leader
    # - Description: Automatically sends MGMT_COMMISSIONER_GET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_COMMISSIONER_GET.rsp to the Commissioner with the
    #   following format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: Encoded values for the requested Commissioner Dataset parameters:
    #     - Commissioner Session ID TLV
    #     - Steering Data TLV.
    print("Step 5: Leader sends MGMT_COMMISSIONER_GET.rsp with Session ID and Steering Data.")
    pkts.filter_coap_ack(consts.MGMT_COMMISSIONER_GET_URI).\
        filter(lambda p: {
            consts.NM_COMMISSIONER_SESSION_ID_TLV,
            consts.NM_STEERING_DATA_TLV
        } <= set(p.coap.tlv.type)).\
        must_next()

    # Step 6: Topology B Commissioner DUT / Topology A Leader DUT
    # - Description:
    #   - Topology B: User instructs Commissioner DUT to send MGMT_COMMISSIONER_GET.req to Leader.
    #   - Topology A: Harness instructs Commissioner to send MGMT_COMMISSIONER_GET.req to DUT’s Anycast or Routing
    #     Locator.
    # - Pass Criteria:
    #   - Topology B: Verify MGMT_COMMISSIONER_GET.req frame has the following format:
    #     - CoAP Request URI: coap://[<L>]:MM/c/cg
    #     - CoAP Payload: Get TLV specifying: Commissioner Session ID TLV (parameter in Commissioner Dataset), PAN ID
    #       TLV (parameter not in Commissioner Dataset)
    #     - Verify Destination Address of MGMT_COMMISSIONER_GET.req frame is DUT’s Anycast or Routing Locator (ALOC or
    #       RLOC).
    #   - Topology A: CoAP Request URI: coap://[<L>]:MM/c/cg. CoAP Payload: Get TLV specifying: Commissioner Session ID
    #     TLV (parameter in Commissioner Dataset), PAN ID TLV (parameter not in Commissioner Dataset).
    print("Step 6: Commissioner sends MGMT_COMMISSIONER_GET.req with Get TLV (Session ID, PAN ID).")
    pkts.filter_coap_request(consts.MGMT_COMMISSIONER_GET_URI).\
        filter(lambda p: consts.TLV_REQUEST_TLV in p.coap.tlv.type and\
               p.coap.tlv.tlv_request == bytes([consts.NM_COMMISSIONER_SESSION_ID_TLV, consts.NM_PAN_ID_TLV]).hex()).\
        must_next()

    # Step 7: Leader
    # - Description: Automatically sends MGMT_COMMISSIONER_GET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_COMMISSIONER_GET.rsp to the Commissioner with the
    #   following format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: Encoded value for the requested Commissioner Dataset parameters:
    #     - Commissioner Session ID TLV
    #     - (PAN ID TLV in Get TLV is ignored).
    print("Step 7: Leader sends MGMT_COMMISSIONER_GET.rsp with Session ID.")
    pkts.filter_coap_ack(consts.MGMT_COMMISSIONER_GET_URI).\
        filter(lambda p: set(p.coap.tlv.type) == {consts.NM_COMMISSIONER_SESSION_ID_TLV}).\
        must_next()

    # Step 8: Topology B Commissioner DUT / Topology A Leader DUT
    # - Description:
    #   - Topology B: User instructs Commissioner DUT to send MGMT_COMMISSIONER_GET.req to Leader.
    #   - Topology A: Harness instructs Commissioner to send MGMT_COMMISSIONER_GET.req to DUT’s Anycast or Routing
    #     Locator.
    # - Pass Criteria:
    #   - Topology B: Verify MGMT_COMMISSIONER_GET.req frame has the following format:
    #     - CoAP Request URI: coap://[<L>]:MM/c/cg
    #     - CoAP Payload: Get TLV specifying: Border Agent Locator TLV, Network Name TLV
    #     - Verify Destination Address of MGMT_COMMISSIONER_GET.req frame is DUT’s Anycast OR Routing Locator (ALOC or
    #       RLOC).
    #   - Topology A: CoAP Request URI: coap://[<L>]:MM/c/cg. CoAP Payload: Get TLV specifying: Border Agent Locator
    #     TLV, Network Name TLV.
    print("Step 8: Commissioner sends MGMT_COMMISSIONER_GET.req with Get TLV (BA Locator, Network Name).")
    pkts.filter_coap_request(consts.MGMT_COMMISSIONER_GET_URI).\
        filter(lambda p: consts.TLV_REQUEST_TLV in p.coap.tlv.type and\
               p.coap.tlv.tlv_request == bytes([consts.NM_BORDER_AGENT_LOCATOR_TLV, consts.NM_NETWORK_NAME_TLV]).hex()).\
        must_next()

    # Step 9: Leader
    # - Description: Automatically sends MGMT_COMMISSIONER_GET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_COMMISSIONER_GET.rsp to the Commissioner with the
    #   following format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: Encoded value for the requested Commissioner Dataset parameters:
    #     - Border Agent Locator TLV
    #     - (Network Name TLV is ignored).
    print("Step 9: Leader sends MGMT_COMMISSIONER_GET.rsp with BA Locator.")
    pkts.filter_coap_ack(consts.MGMT_COMMISSIONER_GET_URI).\
        filter(lambda p: set(p.coap.tlv.type) == {consts.NM_BORDER_AGENT_LOCATOR_TLV}).\
        must_next()

    # Step 10: All
    # - Description: Verify connectivity by sending an ICMPv6 Echo Request to the DUT mesh local address.
    # - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
    print("Step 10: Verify connectivity by sending an ICMPv6 Echo Request to the DUT mesh local address.")
    if pv.test_info.testcase == 'test_9_2_1_A':
        src_ext = COMMISSIONER
        dst_addr = pv.vars['LEADER_MLEID']
    else:
        src_ext = LEADER
        dst_addr = pv.vars['COMMISSIONER_MLEID']

    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(src_ext).\
        filter_ipv6_dst(dst_addr).\
        must_next()

    if pv.test_info.testcase == 'test_9_2_1_A':
        requester_addr = pv.vars['COMMISSIONER_MLEID']
    else:
        requester_addr = pv.vars['LEADER_MLEID']

    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_ipv6_src(dst_addr).\
        filter_ipv6_dst(requester_addr).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
