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
from pktverify.bytes import Bytes


def meshcop_coap_tlv_parse(t, v, layer=None):
    kvs = []
    if t == consts.TLV_REQUEST_TLV:
        kvs.append(('tlv_request', v.hex()))
    return kvs


def verify(pv):
    # 9.2.3 Getting the Active Operational Dataset
    #
    # 9.2.3.1 Topology
    # - Topology A: DUT as Leader, Commissioner (Non-DUT)
    # - Topology B: Leader (Non-DUT), DUT as Commissioner
    #
    # 9.2.3.2 Purpose & Description
    # - DUT as Leader (Topology A): The purpose of this test case is to verify the Leader’s behavior when receiving
    #   MGMT_ACTIVE_GET.req directly from the active Commissioner.
    # - DUT as Commissioner (Topology B): The purpose of this test case is to verify that the active Commissioner can
    #   read Active Operational Dataset parameters direct from the Leader using MGMT_ACTIVE_GET.req command.
    #
    # Spec Reference                          | V1.1 Section | V1.3.0 Section
    # ----------------------------------------|--------------|---------------
    # Updating the Active Operational Dataset | 8.7.4        | 8.7.4

    # Add MeshCoP TLVs to CoapTlvParser
    old_parse = verify_utils.CoapTlvParser.parse

    from pktverify import layer_fields
    layer_fields._LAYER_FIELDS['coap.tlv.tlv_request'] = layer_fields._bytes

    def new_parse(t, v, layer=None):
        if t == consts.TLV_REQUEST_TLV:
            return meshcop_coap_tlv_parse(t, v, layer=layer)
        return old_parse(t, v, layer=layer)

    verify_utils.CoapTlvParser.parse = staticmethod(new_parse)

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    COMMISSIONER = pv.vars['COMMISSIONER']

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A.
    print("Step 1: All - Ensure topology is formed correctly.")

    # Step 2: Topology B Commissioner DUT / Topology A Leader DUT
    # - Description:
    #   - Topology B: User instructs Commissioner DUT to send MGMT_ACTIVE_GET.req to Leader.
    #   - Topology A: Harness instructs Commissioner to send MGMT_ACTIVE_GET.req to DUT’s Anycast or Routing Locator:
    #     - CoAP Request URI: coap://[<L>]:MM/c/ag
    #     - CoAP Payload: <empty> (get all Active Operational Dataset parameters).
    # - Pass Criteria:
    #   - Topology B: Verify MGMT_ACTIVE_GET.req frame has the following format:
    #     - CoAP Request URI: coap://[<L>]:MM/c/ag
    #     - CoAP Payload: <empty> (get all Active Operational Dataset parameters).
    #     - Verify Destination Address of MGMT_ACTIVE_GET.req frame is DUT’s Anycast or Routing Locator (ALOC or RLOC):
    #       - ALOC: Mesh Local prefix with an IID of 0000:00FF:FE00:FC00.
    #       - RLOC: Mesh Local prefix with and IID of 0000:00FF:FE00:xxxx where xxxx is a 16-bit value that embeds the
    #         Router ID.
    #   - Topology A: N/A.
    print("Step 2: Commissioner sends MGMT_ACTIVE_GET.req (empty payload) to Leader.")
    pkts.filter_wpan_src64(COMMISSIONER).\
        filter_coap_request(consts.MGMT_ACTIVE_GET_URI).\
        filter(lambda p: p.coap.payload is nullField or len(p.coap.payload) == 0).\
        filter(lambda p: p.ipv6.dst[8:14] == Bytes("000000fffe00")).\
        must_next()

    # Step 3: Leader
    # - Description: Automatically sends MGMT_ACTIVE_GET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_GET.rsp to the Commissioner with the following
    #   format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: (entire Active Operational Dataset): Active Timestamp TLV, Channel TLV, Channel Mask TLV,
    #     Extended PAN ID TLV, Network Mesh-Local Prefix TLV, Network Master Key TLV, Network Name TLV, PAN ID TLV, PSKc
    #     TLV, Security Policy TLV.
    print("Step 3: Leader sends MGMT_ACTIVE_GET.rsp with entire Active Operational Dataset.")
    pkts.filter_wpan_src64(LEADER).\
        filter_coap_ack(consts.MGMT_ACTIVE_GET_URI).\
        filter(lambda p: {
            consts.NM_ACTIVE_TIMESTAMP_TLV,
            consts.NM_CHANNEL_TLV,
            consts.NM_CHANNEL_MASK_TLV,
            consts.NM_EXTENDED_PAN_ID_TLV,
            consts.NM_NETWORK_MESH_LOCAL_PREFIX_TLV,
            consts.NM_NETWORK_KEY_TLV,
            consts.NM_NETWORK_NAME_TLV,
            consts.NM_PAN_ID_TLV,
            consts.NM_PSKC_TLV,
            consts.NM_SECURITY_POLICY_TLV
        } <= set(p.coap.tlv.type)).\
        must_next()

    # Step 4: Topology B Commissioner DUT / Topology A Leader DUT
    # - Description:
    #   - Topology B: User instructs Commissioner DUT to send MGMT_ACTIVE_GET.req to Leader.
    #   - Topology A: Harness instructs Commissioner to send MGMT_ACTIVE_GET.req to DUT Anycast or Routing Locator:
    #     - CoAP Request URI: coap://[<L>]:MM/c/ag
    #     - CoAP Payload: Get TLV specifying: Channel Mask TLV, Network Mesh-Local Prefix TLV, Network Name TLV.
    # - Pass Criteria:
    #   - Topology B: Verify MGMT_ACTIVE_GET.req frame has the following format:
    #     - CoAP Request URI: coap://[<L>]:MM/c/ag
    #     - CoAP Payload: Get TLV specifying: Channel Mask TLV, Network Mesh-Local Prefix TLV, Network Name TLV.
    #     - Verify Destination Address of MGMT_ACTIVE_GET.req frame is DUT’s Anycast or Routing Locator (ALOC or RLOC):
    #       - ALOC: Mesh Local prefix with an IID of 0000:00FF:FE00:FC00.
    #       - RLOC: Mesh Local prefix with and IID of 0000:00FF:FE00:xxxx where xxxx is a 16-bit value that embeds the
    #         Router ID.
    #   - Topology A: N/A.
    print("Step 4: Commissioner sends MGMT_ACTIVE_GET.req (Get TLV: Channel Mask, Prefix, Name).")
    pkts.filter_wpan_src64(COMMISSIONER).\
        filter_coap_request(consts.MGMT_ACTIVE_GET_URI).\
        filter(lambda p: consts.TLV_REQUEST_TLV in p.coap.tlv.type).\
        filter(lambda p: set(Bytes(p.coap.tlv.tlv_request)) == {
            consts.NM_CHANNEL_MASK_TLV,
            consts.NM_NETWORK_MESH_LOCAL_PREFIX_TLV,
            consts.NM_NETWORK_NAME_TLV
        }).\
        filter(lambda p: p.ipv6.dst[8:14] == Bytes("000000fffe00")).\
        must_next()

    # Step 5: Leader
    # - Description: Automatically sends MGMT_ACTIVE_GET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_GET.rsp to the Commissioner with the following
    #   format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: Channel Mask TLV, Network Mesh-Local Prefix TLV, Network Name TLV.
    print("Step 5: Leader sends MGMT_ACTIVE_GET.rsp with requested TLVs.")
    pkts.filter_wpan_src64(LEADER).\
        filter_coap_ack(consts.MGMT_ACTIVE_GET_URI).\
        filter(lambda p: set(p.coap.tlv.type) == {
            consts.NM_CHANNEL_MASK_TLV,
            consts.NM_NETWORK_MESH_LOCAL_PREFIX_TLV,
            consts.NM_NETWORK_NAME_TLV
        }).\
        must_next()

    # Step 6: Topology B Commissioner DUT / Topology A Leader DUT
    # - Description:
    #   - Topology B: User instructs Commissioner DUT to send MGMT_ACTIVE_GET.req to Leader.
    #   - Topology A: Harness instructs Commissioner to send MGMT_ACTIVE_GET.req to DUT Anycast or Routing Locator:
    #     - CoAP Request URI: coap://[<L>]:MM/c/ag
    #     - CoAP Payload: Get TLV specifying: Channel TLV, Network Mesh-Local Prefix TLV, Network Name TLV, Scan Duration
    #       TLV (not allowed TLV), Energy List TLV (not allowed TLV).
    # - Pass Criteria:
    #   - Topology B: Verify MGMT_ACTIVE_GET.req frame has the following format:
    #     - CoAP Request URI: coap://[<L>]:MM/c/ag
    #     - CoAP Payload: Get TLV specifying: Channel TLV, Network Mesh-Local Prefix TLV, Network Name TLV, Scan Duration
    #       TLV (not allowed TLV), Energy List TLV (not allowed TLV).
    #     - Verify Destination Address of MGMT_ACTIVE_GET.req frame is DUT’s Anycast or Routing Locator (ALOC or RLOC):
    #       - ALOC: Mesh Local prefix with an IID of 0000:00FF:FE00:FC00.
    #       - RLOC: Mesh Local prefix with and IID of 0000:00FF:FE00:xxxx where xxxx is a 16-bit value that embeds the
    #         Router ID.
    #   - Topology A: N/A.
    print(
        "Step 6: Commissioner sends MGMT_ACTIVE_GET.req (Get TLV: Channel, Prefix, Name, Scan Duration, Energy List).")
    pkts.filter_wpan_src64(COMMISSIONER).\
        filter_coap_request(consts.MGMT_ACTIVE_GET_URI).\
        filter(lambda p: consts.TLV_REQUEST_TLV in p.coap.tlv.type).\
        filter(lambda p: set(Bytes(p.coap.tlv.tlv_request)) == {
            consts.NM_CHANNEL_TLV,
            consts.NM_NETWORK_MESH_LOCAL_PREFIX_TLV,
            consts.NM_NETWORK_NAME_TLV,
            consts.NM_SCAN_DURATION,
            consts.NM_ENERGY_LIST_TLV
        }).\
        filter(lambda p: p.ipv6.dst[8:14] == Bytes("000000fffe00")).\
        must_next()

    # Step 7: Leader
    # - Description: Automatically sends MGMT_ACTIVE_GET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_GET.rsp to the Commissioner with the following
    #   format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: Channel TLV, Network Mesh-Local Prefix TLV, Network Name TLV.
    print("Step 7: Leader sends MGMT_ACTIVE_GET.rsp with requested allowed TLVs.")
    pkts.filter_wpan_src64(LEADER).\
        filter_coap_ack(consts.MGMT_ACTIVE_GET_URI).\
        filter(lambda p: set(p.coap.tlv.type) == {
            consts.NM_CHANNEL_TLV,
            consts.NM_NETWORK_MESH_LOCAL_PREFIX_TLV,
            consts.NM_NETWORK_NAME_TLV
        }).\
        must_next()

    # Step 8: All
    # - Description: Verify connectivity by sending an ICMPv6 Echo Request to the DUT mesh local address.
    # - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
    print("Step 8: Verify connectivity by sending an ICMPv6 Echo Request/Reply.")
    _pkt = pkts.filter_ping_request().\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
