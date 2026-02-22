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
from pktverify.addrs import Ipv6Addr

# MeshCop TLVs constants for Active Dataset
NM_ACTIVE_TIMESTAMP_TLV = 14
NM_CHANNEL_TLV = 0
NM_CHANNEL_MASK_TLV = 53
NM_EXTENDED_PAN_ID_TLV = 2
NM_NETWORK_MESH_LOCAL_PREFIX_TLV = 7
NM_NETWORK_KEY_TLV = 5
NM_NETWORK_NAME_TLV = 3
NM_PAN_ID_TLV = 1
NM_PSKC_TLV = 4
NM_SECURITY_POLICY_TLV = 12
NM_STATE_TLV = 16
NM_COMMISSIONER_SESSION_ID_TLV = 11
NM_STEERING_DATA_TLV = 8
NM_FUTURE_TLV = 130


# Monkey-patch CoapTlvParser to parse MeshCoP TLVs in CoAP payload
def meshcop_coap_tlv_parse(t, v, layer=None):
    kvs = []
    if t == NM_COMMISSIONER_SESSION_ID_TLV:
        kvs.append(('comm_sess_id', str(struct.unpack('>H', v)[0])))
    elif t == NM_STATE_TLV:
        kvs.append(('state', str(v[0])))
    elif t == NM_ACTIVE_TIMESTAMP_TLV:
        kvs.append(('active_timestamp', str(struct.unpack('>Q', v)[0] >> 16)))
    elif t == NM_CHANNEL_TLV:
        kvs.append(('channel', str(struct.unpack('>H', v[1:3])[0])))
    elif t == NM_CHANNEL_MASK_TLV:
        kvs.append(('channel_mask', v.hex()))
    elif t == NM_EXTENDED_PAN_ID_TLV:
        kvs.append(('ext_pan_id', v.hex()))
    elif t == NM_NETWORK_MESH_LOCAL_PREFIX_TLV:
        kvs.append(('mesh_local_prefix', v.hex()))
    elif t == NM_NETWORK_KEY_TLV:
        kvs.append(('network_key', v.hex()))
    elif t == NM_NETWORK_NAME_TLV:
        kvs.append(('network_name', v.decode('utf-8')))
    elif t == NM_PAN_ID_TLV:
        kvs.append(('pan_id', hex(struct.unpack('>H', v)[0])))
    elif t == NM_PSKC_TLV:
        kvs.append(('pskc', v.hex()))
    elif t == NM_SECURITY_POLICY_TLV:
        kvs.append(('security_policy', v.hex()))
    elif t == NM_STEERING_DATA_TLV:
        kvs.append(('steering_data', v.hex()))
    elif t == NM_FUTURE_TLV:
        kvs.append(('future_tlv', v.hex()))
    return kvs


def verify(pv):
    # 9.2.4 Updating the Active Operational Dataset via Commissioner
    #
    # 9.2.4.1 Topology
    # - Topology A: DUT as Leader, Commissioner (Non-DUT)
    # - Topology B: Leader (Non-DUT), DUT as Commissioner
    #
    # 9.2.4.2 Purpose & Description
    # - DUT as Leader (Topology A): The purpose of this test case is to verify the Leader’s behavior when receiving
    #   MGMT_ACTIVE_SET.req directly from the active Commissioner.
    # - DUT as Commissioner (Topology B): The purpose of this test case is to verify that the active Commissioner can
    #   set Active Operational Dataset parameters using the MGMT_ACTIVE_SET.req command.
    #
    # Spec Reference                          | V1.1 Section | V1.3.0 Section
    # ----------------------------------------|--------------|---------------
    # Updating the Active Operational Dataset | 8.7.4        | 8.7.4

    # Add MeshCoP TLVs to CoapTlvParser
    old_parse = verify_utils.CoapTlvParser.parse

    from pktverify import layer_fields
    layer_fields._LAYER_FIELDS['coap.tlv.comm_sess_id'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.state'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.active_timestamp'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.channel'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.channel_mask'] = layer_fields._bytes
    layer_fields._LAYER_FIELDS['coap.tlv.ext_pan_id'] = layer_fields._bytes
    layer_fields._LAYER_FIELDS['coap.tlv.mesh_local_prefix'] = layer_fields._bytes
    layer_fields._LAYER_FIELDS['coap.tlv.network_key'] = layer_fields._bytes
    layer_fields._LAYER_FIELDS['coap.tlv.network_name'] = layer_fields._str
    layer_fields._LAYER_FIELDS['coap.tlv.pan_id'] = layer_fields._auto
    layer_fields._LAYER_FIELDS['coap.tlv.pskc'] = layer_fields._bytes
    layer_fields._LAYER_FIELDS['coap.tlv.security_policy'] = layer_fields._bytes
    layer_fields._LAYER_FIELDS['coap.tlv.steering_data'] = layer_fields._bytes
    layer_fields._LAYER_FIELDS['coap.tlv.future_tlv'] = layer_fields._bytes

    def new_parse(t, v, layer=None):
        if t in (NM_COMMISSIONER_SESSION_ID_TLV, NM_STATE_TLV, NM_ACTIVE_TIMESTAMP_TLV, NM_CHANNEL_TLV,
                 NM_CHANNEL_MASK_TLV, NM_EXTENDED_PAN_ID_TLV, NM_NETWORK_MESH_LOCAL_PREFIX_TLV, NM_NETWORK_KEY_TLV,
                 NM_NETWORK_NAME_TLV, NM_PAN_ID_TLV, NM_PSKC_TLV, NM_SECURITY_POLICY_TLV, NM_STEERING_DATA_TLV,
                 NM_FUTURE_TLV):
            return meshcop_coap_tlv_parse(t, v, layer=layer)
        return old_parse(t, v, layer=layer)

    verify_utils.CoapTlvParser.parse = staticmethod(new_parse)

    pkts = pv.pkts
    pv.summary.show()

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A.
    print("Step 1: All")

    # Step 2: Topology B Commissioner DUT / Topology A Commissioner non-DUT
    # - Description:
    #   - Topology B: User instructs Commissioner DUT to send MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator.
    #   - Topology A: Harness instructs Commissioner to send MGMT_ACTIVE_SET.req to Leader.
    #   - Topology A and B: The MGMT_ACTIVE_SET.req will set a subset of the Active Operational Dataset: new, valid
    #     Active Timestamp TLV and new values for Active Operational Dataset TLVs.
    # - Pass Criteria: Commissioner sends MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator with the following
    #   format:
    #   - CoAP Request URI: coap://[<L>]:MM/c/as
    #   - CoAP Payload:
    #     - Commissioner Session ID TLV (valid)
    #     - Active Timestamp TLV: 101, 0
    #     - Channel Mask TLV: 00:04:00:1f:ff:e0
    #     - Extended PAN ID TLV: 00:0d:b7:00:00:00:00:00
    #     - Network Name TLV: "GRL"
    #     - PSKc TLV: 74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:00
    #     - Security Policy TLV: 0e:10:ef
    #   - Note: The Leader Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
    print("Step 2: Commissioner sends MGMT_ACTIVE_SET.req to Leader.")
    pkts.filter_coap_request(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: {
            NM_COMMISSIONER_SESSION_ID_TLV,
            NM_ACTIVE_TIMESTAMP_TLV,
            NM_CHANNEL_MASK_TLV,
            NM_EXTENDED_PAN_ID_TLV,
            NM_NETWORK_NAME_TLV,
            NM_PSKC_TLV,
            NM_SECURITY_POLICY_TLV
        } <= set(p.coap.tlv.type) and
               p.coap.tlv.active_timestamp == 101 and
               p.coap.tlv.channel_mask == '0004001fffe0' and
               p.coap.tlv.ext_pan_id == '000db70000000000' and
               p.coap.tlv.network_name == 'GRL' and
               p.coap.tlv.pskc == '7468726561646a70616b657465737400' and
               p.coap.tlv.security_policy == '0e10ef').\
        must_next()

    # Step 3: Leader
    # - Description: Automatically sends MGMT_ACTIVE_SET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_SET.rsp to the Commissioner with the following
    #   format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Accept (01))
    print("Step 3: Leader sends MGMT_ACTIVE_SET.rsp to Commissioner.")
    pkts.filter_coap_ack(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: p.coap.tlv.state == 1).\
        must_next()

    # Step 4: Topology B Commissioner DUT / Topology A Commissioner non-DUT
    # - Description:
    #   - Topology B: User instructs Commissioner DUT to send MGMT_ACTIVE_GET.req to Leader.
    #   - Topology A: Harness instructs Commissioner to send MGMT_ACTIVE_GET.req to Leader.
    # - Pass Criteria: Commissioner sends MGMT_ACTIVE_GET.req to Leader:
    #   - CoAP Request URI: coap://[<L>]:MM/c/ag
    #   - CoAP Payload: <empty> (get all Active Operational Dataset parameters)
    print("Step 4: Commissioner sends MGMT_ACTIVE_GET.req to Leader.")
    pkts.filter_coap_request(consts.MGMT_ACTIVE_GET_URI).\
        filter(lambda p: p.coap.payload is nullField).\
        must_next()

    # Step 5: Leader
    # - Description: Automatically sends MGMT_ACTIVE_GET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_GET.rsp to the Commissioner with the following
    #   format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: (entire Active Operational Dataset): Active Timestamp TLV, Channel TLV, Channel Mask TLV,
    #     Extended PAN ID TLV, Network Mesh-Local Prefix TLV, Network Master Key TLV, Network Name TLV, PAN ID TLV,
    #     PSKc TLV, Security Policy TLV.
    #   - The Active Operational Dataset values MUST be equivalent to the Active Operational Dataset values set in
    #     step 2.
    print("Step 5: Leader sends MGMT_ACTIVE_GET.rsp to Commissioner.")
    pkts.filter_coap_ack(consts.MGMT_ACTIVE_GET_URI).\
        filter(lambda p: {
            NM_ACTIVE_TIMESTAMP_TLV,
            NM_CHANNEL_TLV,
            NM_CHANNEL_MASK_TLV,
            NM_EXTENDED_PAN_ID_TLV,
            NM_NETWORK_MESH_LOCAL_PREFIX_TLV,
            NM_NETWORK_KEY_TLV,
            NM_NETWORK_NAME_TLV,
            NM_PAN_ID_TLV,
            NM_PSKC_TLV,
            NM_SECURITY_POLICY_TLV
        } <= set(p.coap.tlv.type) and
               p.coap.tlv.active_timestamp == 101 and
               p.coap.tlv.channel_mask == '0004001fffe0' and
               p.coap.tlv.ext_pan_id == '000db70000000000' and
               p.coap.tlv.network_name == 'GRL' and
               p.coap.tlv.pskc == '7468726561646a70616b657465737400' and
               p.coap.tlv.security_policy == '0e10ef').\
        must_next()

    # Step 6: Topology B Commissioner DUT / Topology A Commissioner non-DUT
    # - Description:
    #   - Topology B: User instructs Commissioner DUT to send MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator.
    #   - Topology A: Harness instructs Commissioner to send MGMT_ACTIVE_SET.req to Leader.
    #   - Topology A and B: The MGMT_ACTIVE_SET.req will set a subset of the Active Operational Dataset: new, valid
    #     Active Timestamp TLV, new values for specified Active Operational Dataset TLVs, and attempt to set Channel
    #     TLV.
    # - Pass Criteria: Commissioner sends MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator:
    #   - CoAP Request URI: coap://[<L>]:MM/c/as
    #   - CoAP Payload: Commissioner Session ID TLV (valid), Active Timestamp TLV: 102, 0, Channel TLV: ‘Secondary’
    #     <Attempt to set this>, Channel Mask TLV: 00:04:00:1f:ff:e0, Extended PAN ID TLV: 00:0d:b7:00:00:00:00:01 (new
    #     value), Network Name TLV: "threadcert" (new value), PSKc TLV: 74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:00,
    #     Security Policy TLV: 0e:10:ef.
    print("Step 6: Commissioner sends MGMT_ACTIVE_SET.req with Channel TLV (invalid for active set).")
    pkts.filter_coap_request(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: {
            NM_COMMISSIONER_SESSION_ID_TLV,
            NM_ACTIVE_TIMESTAMP_TLV,
            NM_CHANNEL_TLV,
            NM_EXTENDED_PAN_ID_TLV,
            NM_NETWORK_NAME_TLV
        } <= set(p.coap.tlv.type) and
               p.coap.tlv.active_timestamp == 102 and
               p.coap.tlv.channel == 12 and
               p.coap.tlv.ext_pan_id == '000db70000000001' and
               p.coap.tlv.network_name == 'threadcert').\
        must_next()

    # Step 7: Leader
    # - Description: Automatically sends MGMT_ACTIVE_SET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_SET.rsp to the Commissioner with the following
    #   format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Reject (ff))
    print("Step 7: Leader sends MGMT_ACTIVE_SET.rsp (Reject).")
    pkts.filter_coap_ack(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: p.coap.tlv.state == 255).\
        must_next()

    # Step 8: Topology B Commissioner DUT / Topology A Commissioner non-DUT
    # - Description:
    #   - Topology B: User instructs Commissioner DUT to send MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator.
    #   - Topology A: Harness instructs Commissioner to send MGMT_ACTIVE_SET.req to Leader.
    #   - Topology A and B: The MGMT_ACTIVE_SET.req will set a subset of the Active Operational Dataset: new, valid
    #     Active Timestamp TLV, new values for specified Active Operational Dataset TLVs, and attempt to set Network
    #     Mesh-Local Prefix TLV.
    # - Pass Criteria: Commissioner sends MGMT_ACTIVE_SET.req to Leader RLOC or Leader Anycast Locator:
    #   - CoAP Request URI: coap://[<L>]:MM/c/as
    #   - CoAP Payload: Commissioner Session ID TLV (valid), Active Timestamp TLV: 103, 0, Channel Mask TLV:
    #     00:04:00:1f:fe:e0 (new value), Extended PAN ID TLV: 00:0d:b7:00:00:00:00:00 (new value), Network Mesh-Local
    #     Prefix TLV: FD00:0DB7::" (Attempt to set this), Network Name TLV: "UL", PSKc TLV:
    #     74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:01 (new value), Security Policy TLV: 0e:10:ef.
    print("Step 8: Commissioner sends MGMT_ACTIVE_SET.req with Mesh-Local Prefix TLV (invalid for active set).")
    pkts.filter_coap_request(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: {
            NM_COMMISSIONER_SESSION_ID_TLV,
            NM_ACTIVE_TIMESTAMP_TLV,
            NM_CHANNEL_MASK_TLV,
            NM_NETWORK_MESH_LOCAL_PREFIX_TLV,
            NM_NETWORK_NAME_TLV,
            NM_PSKC_TLV
        } <= set(p.coap.tlv.type) and
               p.coap.tlv.active_timestamp == 103 and
               p.coap.tlv.channel_mask == '0004001ffee0' and
               p.coap.tlv.mesh_local_prefix == 'fd000db700000000' and
               p.coap.tlv.network_name == 'UL' and
               p.coap.tlv.pskc == '7468726561646a70616b657465737401').\
        must_next()

    # Step 9: Leader
    # - Description: Automatically sends MGMT_ACTIVE_SET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_SET.rsp to the Commissioner with the following
    #   format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Reject (ff))
    print("Step 9: Leader sends MGMT_ACTIVE_SET.rsp (Reject).")
    pkts.filter_coap_ack(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: p.coap.tlv.state == 255).\
        must_next()

    # Step 10: Topology B Commissioner DUT / Topology A Commissioner non-DUT
    # - Description:
    #   - Topology B: User instructs Commissioner DUT to send MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator.
    #   - Topology A: Harness instructs Commissioner to send MGMT_ACTIVE_SET.req to Leader.
    #   - Topology A and B: The MGMT_ACTIVE_SET.req will set a subset of the Active Operational Dataset: new, valid
    #     Active Timestamp TLV, new values for specified Active Operational Dataset TLVs, and attempt to set Network
    #     Master Key TLV and other TLVs.
    # - Pass Criteria: Commissioner sends MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator:
    #   - CoAP Request URI: coap://[<L>]:MM/c/as
    #   - CoAP Payload: Commissioner Session ID TLV (valid), Active Timestamp TLV: 104, 0, Channel Mask TLV:
    #     00:04:00:1f:ff:e0, Extended PAN ID TLV: 00:0d:b7:00:00:00:00:00, Network Master Key TLV: Set to different key
    #     value from the original, Network Name TLV: "GRL", PSKc TLV: 74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:00
    #     (new value), Security Policy TLV: 0e:10:ff.
    print("Step 10: Commissioner sends MGMT_ACTIVE_SET.req with Network Key TLV (invalid for active set).")
    pkts.filter_coap_request(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: {
            NM_COMMISSIONER_SESSION_ID_TLV,
            NM_ACTIVE_TIMESTAMP_TLV,
            NM_NETWORK_KEY_TLV,
            NM_SECURITY_POLICY_TLV
        } <= set(p.coap.tlv.type) and
               p.coap.tlv.active_timestamp == 104 and
               p.coap.tlv.network_key == '00112233445566778899aabbccddeeff' and
               p.coap.tlv.security_policy == '0e10ff').\
        must_next()

    # Step 11: Leader
    # - Description: Automatically sends MGMT_ACTIVE_SET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_SET.rsp to the Commissioner with the following
    #   format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Reject (ff))
    print("Step 11: Leader sends MGMT_ACTIVE_SET.rsp (Reject).")
    pkts.filter_coap_ack(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: p.coap.tlv.state == 255).\
        must_next()

    # Step 12: Topology B Commissioner DUT / Topology A Commissioner non-DUT
    # - Description:
    #   - Topology B: User instructs Commissioner DUT to send MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator.
    #   - Topology A: Harness instructs Commissioner to send MGMT_ACTIVE_SET.req to Leader.
    #   - Topology A and B: The MGMT_ACTIVE_SET.req will set a subset of the Active Operational Dataset: new, valid
    #     Active Timestamp TLV, and attempt to set PAN ID TLV.
    # - Pass Criteria: Commissioner sends MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator:
    #   - CoAP Request URI: coap://[<L>]:MM/c/as
    #   - CoAP Payload: Commissioner Session ID TLV (valid), Active Timestamp TLV: 105, 0, Channel Mask TLV:
    #     00:04:00:1f:ff:e0, Extended PAN ID TLV: 00:0d:b7:00:00:00:00:00, Network Name TLV: "GRL", PAN ID TLV: AFCE,
    #     PSKc TLV: 74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:00, Security Policy TLV: 0e:10:ff.
    print("Step 12: Commissioner sends MGMT_ACTIVE_SET.req with PAN ID TLV (invalid for active set).")
    pkts.filter_coap_request(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: {
            NM_COMMISSIONER_SESSION_ID_TLV,
            NM_ACTIVE_TIMESTAMP_TLV,
            NM_PAN_ID_TLV
        } <= set(p.coap.tlv.type) and
               p.coap.tlv.active_timestamp == 105 and
               p.coap.tlv.pan_id == 0xafce).\
        must_next()

    # Step 13: Leader
    # - Description: Automatically sends MGMT_ACTIVE_SET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_SET.rsp to the Commissioner with the following
    #   format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Reject (ff))
    print("Step 13: Leader sends MGMT_ACTIVE_SET.rsp (Reject).")
    pkts.filter_coap_ack(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: p.coap.tlv.state == 255).\
        must_next()

    # Step 14: Topology B Commissioner DUT / Topology A Commissioner non-DUT
    # - Description:
    #   - Topology B: User instructs Commissioner DUT to send MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator.
    #   - Topology A: Harness instructs Commissioner to send MGMT_ACTIVE_SET.req to Leader.
    #   - Topology A and B: The MGMT_ACTIVE_SET.req will set a subset of the Active Operational Dataset: New valid
    #     Active Timestamp TLV, and Invalid Commissioner Session ID.
    # - Pass Criteria: Commissioner sends MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator:
    #   - CoAP Request URI: coap://[<L>]:MM/c/as
    #   - CoAP Payload: Commissioner Session ID TLV (invalid), Active Timestamp TLV: 106, 0, Channel Mask TLV:
    #     00:04:00:1f:ff:e0, Extended PAN ID TLV: 00:0d:b7:00:00:00:00:00, Network Name TLV: "GRL", PSKc TLV:
    #     74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:00, Security Policy TLV: 0e:10:ff.
    print("Step 14: Commissioner sends MGMT_ACTIVE_SET.req with invalid Session ID.")
    pkts.filter_coap_request(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: {
            NM_COMMISSIONER_SESSION_ID_TLV,
            NM_ACTIVE_TIMESTAMP_TLV
        } <= set(p.coap.tlv.type) and
               p.coap.tlv.active_timestamp == 106 and
               p.coap.tlv.comm_sess_id == 65535).\
        must_next()

    # Step 15: Leader
    # - Description: Automatically sends MGMT_ACTIVE_SET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_SET.rsp to the Commissioner with the following
    #   format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Reject (ff))
    print("Step 15: Leader sends MGMT_ACTIVE_SET.rsp (Reject).")
    pkts.filter_coap_ack(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: p.coap.tlv.state == 255).\
        must_next()

    # Step 16: Topology B Commissioner DUT / Topology A Commissioner non-DUT
    # - Description:
    #   - Topology B: User instructs Commissioner DUT to send MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator.
    #   - Topology A: Harness instructs Commissioner to send MGMT_ACTIVE_SET.req to Leader.
    #   - Topology A and B: The MGMT_ACTIVE_SET.req will set a subset of the Active Operational Dataset: old, valid
    #     Active Timestamp TLV.
    # - Pass Criteria: Commissioner sends MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator:
    #   - CoAP Request URI: coap://[<L>]:MM/c/as
    #   - CoAP Payload: Commissioner Session ID TLV (valid), Active Timestamp TLV (old): 101, 0, Channel Mask TLV:
    #     00:04:00:1f:ff:e0, Extended PAN ID TLV: 00:0d:b7:00:00:00:00:00, Network Name TLV: "GRL", PSKc TLV:
    #     74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:00, Security Policy TLV: 0e:10:ff.
    print("Step 16: Commissioner sends MGMT_ACTIVE_SET.req with old Active Timestamp.")
    pkts.filter_coap_request(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: {
            NM_COMMISSIONER_SESSION_ID_TLV,
            NM_ACTIVE_TIMESTAMP_TLV
        } <= set(p.coap.tlv.type) and
               p.coap.tlv.active_timestamp == 101).\
        must_next()

    # Step 17: Leader
    # - Description: Automatically sends MGMT_ACTIVE_SET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_SET.rsp to the Commissioner with the following
    #   format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Reject (ff))
    print("Step 17: Leader sends MGMT_ACTIVE_SET.rsp (Reject).")
    pkts.filter_coap_ack(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: p.coap.tlv.state == 255).\
        must_next()

    # Step 18: Topology B Commissioner DUT / Topology A Commissioner non-DUT
    # - Description:
    #   - Topology B: User instructs Commissioner DUT to send MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator.
    #   - Topology A: Harness instructs Commissioner to send MGMT_ACTIVE_SET.req to Leader.
    #   - Topology A and B: The MGMT_ACTIVE_SET.req will set a subset of the Active Operational Dataset: new, valid
    #     Active Timestamp TLV, and unexpected Steering Data TLV.
    # - Pass Criteria: Commissioner sends MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator:
    #   - CoAP Request URI: coap://[<L>]:MM/c/as
    #   - CoAP Payload: Commissioner Session ID TLV (valid), Active Timestamp TLV: 107, 0, Channel Mask TLV:
    #     00:04:00:1f:ff:e0, Extended PAN ID TLV: 00:0d:b7:00:00:00:00:00, Network Name TLV: "GRL", PSKc TLV:
    #     74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:00, Security Policy TLV: 0e:10:ff, Steering Data TLV:
    #     11:33:20:44:00:00.
    print("Step 18: Commissioner sends MGMT_ACTIVE_SET.req with unexpected Steering Data TLV.")
    pkts.filter_coap_request(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: {
            NM_COMMISSIONER_SESSION_ID_TLV,
            NM_ACTIVE_TIMESTAMP_TLV,
            NM_STEERING_DATA_TLV
        } <= set(p.coap.tlv.type) and
               p.coap.tlv.active_timestamp == 107 and
               p.coap.tlv.steering_data == '113320440000').\
        must_next()

    # Step 19: Leader
    # - Description: Automatically responds to MGMT_ACTIVE_SET.req with a MGMT_ACTIVE_SET.rsp to Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_SET.rsp to the Commissioner with the following
    #   format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Accept (01))
    print("Step 19: Leader sends MGMT_ACTIVE_SET.rsp (Accept).")
    pkts.filter_coap_ack(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: p.coap.tlv.state == 1).\
        must_next()

    # Step 20: Topology B Commissioner DUT / Topology A Commissioner non-DUT
    # - Description:
    #   - Topology B: User instructs Commissioner DUT to send MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator.
    #   - Topology A: Harness instructs Commissioner to send MGMT_ACTIVE_SET.req to Leader.
    #   - Topology A and B: The MGMT_ACTIVE_SET.req will set a subset of the Active Operational Dataset: new, valid
    #     Active Timestamp TLV, and unspecified TLV (Future TLV).
    # - Pass Criteria: Commissioner sends MGMT_ACTIVE_SET.req to Leader RLOC or Anycast Locator:
    #   - CoAP Request URI: coap://[<L>]:MM/c/as
    #   - CoAP Payload: Commissioner Session ID TLV (valid), Active Timestamp TLV: 108, 0, Channel Mask TLV:
    #     00:04:00:1f:ff:e0, Extended PAN ID TLV: 00:0d:b7:00:00:00:00:00, Network Name TLV: "GRL", PSKc TLV:
    #     74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:00, Security Policy TLV: 0e:10:ff, Future TLV: Type 130, Length
    #     2, Value (aa 55).
    print("Step 20: Commissioner sends MGMT_ACTIVE_SET.req with Future TLV.")
    pkts.filter_coap_request(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: {
            NM_COMMISSIONER_SESSION_ID_TLV,
            NM_ACTIVE_TIMESTAMP_TLV,
            NM_FUTURE_TLV
        } <= set(p.coap.tlv.type) and
               p.coap.tlv.active_timestamp == 108 and
               p.coap.tlv.future_tlv == 'aa55').\
        must_next()

    # Step 21: Leader
    # - Description: Automatically responds to MGMT_ACTIVE_SET.req with a MGMT_ACTIVE_SET.rsp to Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_SET.rsp to the Commissioner with the following
    #   format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Accept (01))
    print("Step 21: Leader sends MGMT_ACTIVE_SET.rsp (Accept).")
    pkts.filter_coap_ack(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: p.coap.tlv.state == 1).\
        must_next()

    # Step 22: All
    # - Description: Verify connectivity by sending an ICMPv6 Echo Request to the DUT mesh local address.
    # - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
    print("Step 22: All")
    if pv.test_info.testcase == 'test_9_2_4_A':
        src_ext = pv.vars['COMMISSIONER']
        dst_addr = pv.vars['LEADER_MLEID']
        requester_addr = pv.vars['COMMISSIONER_MLEID']
    else:
        src_ext = pv.vars['LEADER']
        dst_addr = pv.vars['COMMISSIONER_MLEID']
        requester_addr = pv.vars['LEADER_MLEID']

    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(src_ext).\
        filter_ipv6_dst(dst_addr).\
        must_next()

    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_ipv6_src(dst_addr).\
        filter_ipv6_dst(requester_addr).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
