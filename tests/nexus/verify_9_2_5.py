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
    # 9.2.5 Updating the Active Operational Dataset via Thread node
    #
    # 9.2.5.1 Topology
    # DUT as Leader, Router_1
    #
    # 9.2.5.2 Purpose & Description
    # The purpose of this test case is to verify the DUT’s behavior when receiving MGMT_ACTIVE_SET.req from an active
    #   Thread node.
    #
    # Spec Reference                          | V1.1 Section | V1.3.0 Section
    # ----------------------------------------|--------------|---------------
    # Updating the Active Operational Dataset | 8.7.4        | 8.7.4

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A.
    print("Step 1: Ensure topology is formed correctly.")

    # Step 2: Router_1
    # - Description: Harness instructs Router_1 to send a MGMT_ACTIVE_SET.req to the Leader (DUT)’s Routing or Anycast
    #   Locator:
    #   - new, valid Timestamp TLV
    #   - all valid Active Operational Dataset parameters, with new values in the TLVs that don’t affect connectivity
    # - Pass Criteria:
    #   - CoAP Request URI: coap://[<L>]:MM/c/as
    #   - CoAP Payload:
    #     - Active Timestamp TLV (new valid value)
    #     - Channel Mask TLV (new value)
    #     - Extended PAN ID TLV (new value)
    #     - Mesh-Local Prefix (old value)
    #     - Network Name TLV (new value)
    #     - PSKc TLV (new value)
    #     - Security Policy TLV (new value)
    #     - Network Master Key (old value)
    #     - PAN ID (old value)
    #     - Channel (old value)
    #   - The DUT’s Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
    print("Step 2: Router_1 sends MGMT_ACTIVE_SET.req with new valid Active Timestamp.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_coap_request(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: {
            consts.NM_ACTIVE_TIMESTAMP_TLV,
            consts.NM_CHANNEL_MASK_TLV,
            consts.NM_EXTENDED_PAN_ID_TLV,
            consts.NM_NETWORK_MESH_LOCAL_PREFIX_TLV,
            consts.NM_NETWORK_NAME_TLV,
            consts.NM_PSKC_TLV,
            consts.NM_SECURITY_POLICY_TLV,
            consts.NM_NETWORK_KEY_TLV,
            consts.NM_PAN_ID_TLV,
            consts.NM_CHANNEL_TLV
        } <= set(p.coap.tlv.type)).\
        filter(lambda p: p.coap.tlv.active_timestamp == 20 and\
                         p.coap.tlv.network_name == 'nexus-test').\
        filter(lambda p: p.ipv6.dst.endswith(bytes.fromhex("000000fffe00fc00"))).\
        must_next()

    # Step 3: Leader (DUT)
    # - Description: Automatically sends MGMT_ACTIVE_SET.rsp to Router_1.
    # - Pass Criteria: The DUT MUST send MGMT_ACTIVE_SET.rsp to Router_1 with the following format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Accept (01))
    print("Step 3: Leader sends MGMT_ACTIVE_SET.rsp with State TLV = Accept.")
    pkts.filter_wpan_src64(LEADER).\
        filter_coap_ack(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: p.coap.tlv.state == 1).\
        must_next()

    # Step 4: Leader (DUT)
    # - Description: Automatically sends a Multicast MLE Data Response.
    # - Pass Criteria: The DUT MUST send a multicast MLE Data Response, including the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #     - Data version field [incremented]
    #     - Stable Version field [incremented]
    #   - Network Data TLV
    #   - Active Timestamp TLV [new value set in Step 2]
    print("Step 4: Leader sends a Multicast MLE Data Response.")
    pkts.filter_wpan_src64(LEADER).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
            consts.SOURCE_ADDRESS_TLV,
            consts.LEADER_DATA_TLV,
            consts.NETWORK_DATA_TLV,
            consts.ACTIVE_TIMESTAMP_TLV
        } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.mle.tlv.active_tstamp == 20).\
        must_next()

    # Step 5: Router_1
    # - Description: Automatically sends a unicast MLE Data Request to Leader, including the following TLVs:
    #   - TLV Request TLV:
    #     - Network Data TLV
    #   - Active Timestamp TLV
    # - Pass Criteria: N/A.
    print("Step 5: Router_1 sends a unicast MLE Data Request to Leader.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_DATA_REQUEST).\
        filter(lambda p: {
            consts.TLV_REQUEST_TLV,
            consts.ACTIVE_TIMESTAMP_TLV
        } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 6: Leader (DUT)
    # - Description: Automatically sends a unicast MLE Data Response to Router_1.
    # - Pass Criteria: The DUT MUST send a unicast MLE Data Response to Router_1, including the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #   - Network Data TLV
    #   - Active Operational Dataset TLV
    #     - Channel TLV
    #     - Channel Mask TLV [new value set in Step 2]
    #     - Extended PAN ID TLV [new value set in Step 2]
    #     - Network Mesh-Local Prefix TLV
    #     - Network Master Key TLV
    #     - Network Name TLV [new value set in Step 2]
    #     - PAN ID TLV
    #     - PSKc TLV [new value set in Step 2]
    #     - Security Policy TLV [new value set in Step 2]
    #   - Active Timestamp TLV [new value set in Step 2]
    print("Step 6: Leader sends a unicast MLE Data Response to Router_1.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
            consts.SOURCE_ADDRESS_TLV,
            consts.LEADER_DATA_TLV,
            consts.NETWORK_DATA_TLV,
            consts.ACTIVE_OPERATION_DATASET_TLV,
            consts.ACTIVE_TIMESTAMP_TLV
        } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.mle.tlv.active_tstamp == 20).\
        must_next()

    # Step 7: Router_1
    # - Description: Harness instructs Router_1 to send a MGMT_ACTIVE_SET.req to the Leader (DUT)’s Routing or Anycast
    #   Locator:
    #   - old, invalid Active Timestamp TLV
    #   - all valid Active Operational Dataset parameters, with new values in the TLVs that don’t affect connectivity
    # - Pass Criteria:
    #   - CoAP Request URI: coap://[<L>]:MM/c/as
    #   - CoAP Payload:
    #     - Active Timestamp TLV (old, invalid value)
    #     - Channel Mask TLV (new value)
    #     - Extended PAN ID TLV (new value)
    #     - Mesh-Local Prefix (old value)
    #     - Network Name TLV (new value)
    #     - PSKc TLV (new value)
    #     - Security Policy TLV (new value)
    #     - Network Master Key (old value)
    #     - PAN ID (old value)
    #     - Channel (old value)
    #   - The DUT’s Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
    print("Step 7: Router_1 sends MGMT_ACTIVE_SET.req with old invalid Active Timestamp.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_coap_request(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: p.coap.tlv.active_timestamp == 10).\
        filter(lambda p: p.ipv6.dst.endswith(bytes.fromhex("000000fffe00fc00"))).\
        must_next()

    # Step 8: Leader (DUT)
    # - Description: Automatically sends a MGMT_ACTIVE_SET.rsp to Router_1.
    # - Pass Criteria: The DUT MUST send MGMT_ACTIVE_SET.rsp to Router_1, with the following format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Reject (ff))
    print("Step 8: Leader sends MGMT_ACTIVE_SET.rsp with State TLV = Reject.")
    pkts.filter_wpan_src64(LEADER).\
        filter_coap_ack(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: p.coap.tlv.state == 255).\
        must_next()

    # Step 9: Router_1
    # - Description: Harness instructs Router_1 to send a MGMT_ACTIVE_SET.req to the Leader (DUT)’s Routing or Anycast
    #   Locator:
    #   - new, valid Active Timestamp TLV
    #   - all of valid Commissioner Dataset parameters plus one bogus TLV, and new values in the TLVs that don’t affect
    #     connectivity
    # - Pass Criteria:
    #   - CoAP Request URI: coap://[<L>]:MM/c/as
    #   - CoAP Payload:
    #     - Active Timestamp TLV (new, valid value)
    #     - Channel Mask TLV (new value, different from Step 2)
    #     - Extended PAN ID TLV (new value, different from Step 2)
    #     - Mesh-Local Prefix (old value)
    #     - Network Name TLV (new value, different from Step 2)
    #     - PSKc TLV (new value, different from Step 2)
    #     - Security Policy TLV (new value, different from Step 2)
    #     - Network Master Key (old value)
    #     - PAN ID (old value)
    #     - Channel (old value)
    #     - Future TLV:
    #       - Type 130
    #       - Length 2
    #       - Value (aa 55)
    #   - The DUT’s Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
    print("Step 9: Router_1 sends MGMT_ACTIVE_SET.req with new valid Active Timestamp and Future TLV.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_coap_request(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: {
            consts.NM_ACTIVE_TIMESTAMP_TLV,
            consts.NM_CHANNEL_MASK_TLV,
            consts.NM_EXTENDED_PAN_ID_TLV,
            consts.NM_NETWORK_NAME_TLV,
            consts.NM_PSKC_TLV,
            consts.NM_SECURITY_POLICY_TLV
        } <= set(p.coap.tlv.type)).\
        filter(lambda p: p.coap.tlv.active_timestamp == 30 and\
                         p.coap.tlv.network_name == 'nexus-925').\
        filter(lambda p: 130 in p.coap.tlv.type).\
        filter(lambda p: p.ipv6.dst.endswith(bytes.fromhex("000000fffe00fc00"))).\
        must_next()

    # Step 10: Leader (DUT)
    # - Description: Automatically sends a MGMT_ACTIVE_SET.rsp to Router_1.
    # - Pass Criteria: The DUT MUST send MGMT_ACTIVE_SET.rsp to Router_1 with the following format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Accept (01))
    print("Step 10: Leader sends MGMT_ACTIVE_SET.rsp with State TLV = Accept.")
    pkts.filter_wpan_src64(LEADER).\
        filter_coap_ack(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: p.coap.tlv.state == 1).\
        must_next()

    # Step 11: Leader (DUT)
    # - Description: Automatically sends a multicast MLE Data Response.
    # - Pass Criteria: The DUT MUST send a multicast MLE Data Response, including the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #     - Data version field [incremented]
    #     - Stable Version field [incremented]
    #   - Network Data TLV
    #   - Active Timestamp TLV [new value set in Step 9]
    print("Step 11: Leader sends a Multicast MLE Data Response.")
    pkts.filter_wpan_src64(LEADER).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: p.mle.tlv.active_tstamp == 30).\
        must_next()

    # Step 12: Router_1
    # - Description: Automatically sends a unicast MLE Data Request to the Leader (DUT), including the following TLVs:
    #   - TLV Request TLV:
    #     - Network Data TLV
    #   - Active Timestamp TLV
    # - Pass Criteria: N/A.
    print("Step 12: Router_1 sends a unicast MLE Data Request to Leader.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_DATA_REQUEST).\
        must_next()

    # Step 13: Leader (DUT)
    # - Description: Automatically sends a unicast MLE Data Response to Router_1.
    # - Pass Criteria: The following TLVs MUST be included in the Unicast MLE Data Response:
    #   - Source Address TLV
    #   - Leader Data TLV
    #   - Network Data TLV
    #   - Stable flag set to 0
    #   - Active Operational Dataset TLV
    #     - Channel TLV
    #     - Channel Mask TLV [new value set in Step 9]
    #     - Extended PAN ID TLV [new value set in Step 9]
    #     - Network Mesh-Local Prefix TLV
    #     - Network Master Key TLV
    #     - Network Name TLV [new value set in Step 9]
    #     - PAN ID TLV
    #     - PSKc TLV [new value set in Step 9]
    #     - Security Policy TLV [new value set in Step 9]
    #   - Active Timestamp TLV [new value set in Step 9]
    print("Step 13: Leader sends a unicast MLE Data Response to Router_1.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
            consts.SOURCE_ADDRESS_TLV,
            consts.LEADER_DATA_TLV,
            consts.NETWORK_DATA_TLV,
            consts.ACTIVE_OPERATION_DATASET_TLV,
            consts.ACTIVE_TIMESTAMP_TLV
        } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.mle.tlv.active_tstamp == 30).\
        must_next()

    # Step 14: Router_1
    # - Description: Harness instructs Router_1 to send a MGMT_ACTIVE_SET.req to the Leader (DUT)’s Routing or Anycast
    #   Locator:
    #   - new, valid Active Timestamp TLV
    #   - attempt to set Channel TLV to an unsupported channel + all of other TLVs
    # - Pass Criteria:
    #   - CoAP Request URI: coap://[<L>]:MM/c/as
    #   - CoAP Payload:
    #     - Active Timestamp TLV (new, valid value)
    #     - Channel TLV (unsupported value = 63)
    #     - Channel Mask TLV (old value set in Step 9)
    #     - Extended PAN ID TLV (old value set in Step 9)
    #     - Mesh-Local Prefix (old value)
    #     - Network Name TLV (old value set in Step 9)
    #     - PSKc TLV (old value set in Step 9)
    #     - Security Policy TLV (old value set in Step 9)
    #     - Network Master Key (old value)
    #     - PAN ID (old value)
    #   - The DUT Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
    print("Step 14: Router_1 sends MGMT_ACTIVE_SET.req with unsupported Channel.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_coap_request(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: p.coap.tlv.active_timestamp == 40 and\
                         p.coap.tlv.channel == 63).\
        filter(lambda p: p.ipv6.dst.endswith(bytes.fromhex("000000fffe00fc00"))).\
        must_next()

    # Step 15: Leader (DUT)
    # - Description: Automatically sends MGMT_ACTIVE_SET.rsp to Router_1.
    # - Pass Criteria: The DUT MUST send MGMT_ACTIVE_SET.rsp to Router_1 with the following format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Reject (ff))
    print("Step 15: Leader sends MGMT_ACTIVE_SET.rsp with State TLV = Reject.")
    pkts.filter_wpan_src64(LEADER).\
        filter_coap_ack(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: p.coap.tlv.state == 255).\
        must_next()

    # Step 16: All
    # - Description: Verify connectivity by sending an ICMPv6 Echo Request to the DUT mesh local address.
    # - Pass Criteria: The DUT must respond with an ICMPv6 Echo Reply.
    print("Step 16: Verify connectivity by sending an ICMPv6 Echo Request.")
    # Match any ping request and reply.
    _pkt = pkts.filter_ping_request().\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
