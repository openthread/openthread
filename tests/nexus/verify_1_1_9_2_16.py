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
from pktverify.bytes import Bytes


def verify(pv):
    # 9.2.16 Attaching with different Active and Pending Operational Dataset
    #
    # 9.2.16.1 Topology
    # - Commissioner
    # - Leader
    # - Router_1
    # - Router_2 (DUT)
    #
    # 9.2.16.2 Purpose & Description
    # The purpose of this test case is to verify synchronization of Active and Pending Operational Datasets between an
    #   attaching Router and an existing Router.
    #
    # Spec Reference            | V1.1 Section | V1.3.0 Section
    # --------------------------|--------------|---------------
    # Dissemination of Datasets | 8.4.3        | 8.4.3

    pkts = pv.pkts

    COMMISSIONER = pv.vars['COMMISSIONER']
    LEADER = pv.vars['LEADER']
    LEADER_IPADDRS = pv.vars['LEADER_IPADDRS']
    ROUTER_1 = pv.vars['ROUTER_1']
    DUT = pv.vars['DUT']
    DUT_IPADDRS = pv.vars['DUT_IPADDRS']
    DUT_MLEID = pv.vars['DUT_MLEID']

    # Step 1: Commissioner, Leader, Router_1
    # - Description: Setup the topology without the DUT. Ensure topology is formed correctly. Verify Commissioner,
    #   Leader and Router_1 are sending MLE advertisements.
    # - Pass Criteria: N/A
    print("Step 1: Commissioner, Leader, Router_1")

    # Step 2: Router_2 (DUT)
    # - Description: Configuration: Router_2 is configured out-of-band with Network Credentials of existing network.
    # - Pass Criteria: N/A
    print("Step 2: Router_2 (DUT)")

    # Step 3: Commissioner
    # - Description: Harness instructs the Commissioner to send MGMT_PENDING_SET.req to the Leader RLOC or Anycast
    #   Locator setting a subset of the Active Operational Dataset:
    #   - CoAP Request URI: coap://[<L>]:MM/c/ps
    #   - CoAP Payload:
    #     - new, valid Active Timestamp TLV (10s)
    #     - new, valid Pending Timestamp TLV (10s)
    #     - new values for Network Mesh-Local Prefix TLV (fd:00:0d:b9:00:00:00:00)
    #     - Delay Timer TLV (600s)
    #   - The Leader Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
    # - Pass Criteria: N/A
    print("Step 3: Commissioner sends MGMT_PENDING_SET.req")
    pkts.filter_wpan_src64(COMMISSIONER).\
        filter_coap_request(consts.MGMT_PENDING_SET_URI).\
        filter(lambda p: {
                          consts.NM_COMMISSIONER_SESSION_ID_TLV,
                          consts.NM_ACTIVE_TIMESTAMP_TLV,
                          consts.NM_PENDING_TIMESTAMP_TLV,
                          consts.NM_NETWORK_MESH_LOCAL_PREFIX_TLV,
                          consts.NM_DELAY_TIMER_TLV
                          } <= set(p.coap.tlv.type)).\
        filter(lambda p: p.coap.tlv.active_timestamp == 10).\
        filter(lambda p: p.coap.tlv.pending_timestamp == 10).\
        filter(lambda p: p.coap.tlv.mesh_local_prefix == Bytes("fd000db900000000")).\
        filter(lambda p: p.coap.tlv.delay_timer == 600000).\
        must_next()

    # Step 4: Leader
    # - Description: Automatically sends MGMT_PENDING_SET.rsp to Commissioner:
    #   - Response code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Accept (0x01))
    # - Pass Criteria: N/A
    print("Step 4: Leader sends MGMT_PENDING_SET.rsp")
    pkts.filter_wpan_src64(LEADER).\
        filter_coap_ack(consts.MGMT_PENDING_SET_URI).\
        filter(lambda p: p.coap.tlv.state == consts.MESHCOP_ACCEPT).\
        must_next()

    # Step 5: Router_2 (DUT)
    # - Description: Begins attach process by sending a multicast MLE Parent Request.
    # - Pass Criteria:
    #   - The DUT MUST send a MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an IP
    #     Hop Limit of 255, including the following TLVs:
    #     - Mode TLV
    #     - Challenge TLV
    #     - Scan Mask TLV (Verify sent to routers only)
    #     - Version TLV
    #   - The first MLE Parent Request sent by the DUT MUST NOT be sent to all routers and REEDS.
    print("Step 5: Router_2 (DUT) sends MLE Parent Request")
    pkts.filter_wpan_src64(DUT).\
        filter_LLARMA().\
        filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        filter(lambda p: {
                          consts.MODE_TLV,
                          consts.CHALLENGE_TLV,
                          consts.SCAN_MASK_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type) and\
               p.ipv6.hlim == 255 and\
               p.mle.tlv.scan_mask.r == 1 and\
               p.mle.tlv.scan_mask.e == 0).\
        must_next()

    # Step 6: Router_1
    # - Description: Automatically responds with MLE Parent Response.
    # - Pass Criteria: N/A
    print("Step 6: Router_1 sends MLE Parent Response")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(DUT).\
        filter_mle_cmd(consts.MLE_PARENT_RESPONSE).\
        must_next()

    # Step 7: Router_2 (DUT)
    # - Description: Automatically sends MLE Child ID Request to Router_1.
    # - Pass Criteria:
    #   - The DUT MUST send an MLE Child ID Request, including the following TLVs:
    #     - Link-layer Frame Counter TLV
    #     - Mode TLV
    #     - Response TLV
    #     - Timeout TLV
    #     - TLV Request TLV
    #     - Version TLV
    #     - MLE Frame Counter TLV (optional)
    #   - The following TLV MUST NOT be present in the Child ID Request:
    #     - Address Registration TLV
    print("Step 7: Router_2 (DUT) sends MLE Child ID Request")
    pkts.filter_wpan_src64(DUT).\
        filter_wpan_dst64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter(lambda p: {
                          consts.LINK_LAYER_FRAME_COUNTER_TLV,
                          consts.MODE_TLV,
                          consts.RESPONSE_TLV,
                          consts.TIMEOUT_TLV,
                          consts.TLV_REQUEST_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next().\
        must_not_verify(lambda p: consts.ADDRESS_REGISTRATION_TLV in p.mle.tlv.type)

    # Step 8: Router_1
    # - Description: Automatically sends Child ID Response to the DUT, including the following TLVs:
    #   - Active Timestamp TLV
    #   - Address16 TLV
    #   - Leader Data TLV
    #   - Pending Timestamp TLV (corresponding to step 3)
    #   - Pending Operational Dataset TLV (corresponding to step 3)
    #   - Source Address TLV
    #   - Address Registration TLV (optional)
    #   - Network Data TLV (optional)
    #   - Route64 TLV (optional)
    # - Pass Criteria: N/A
    print("Step 8: Router_1 sends Child ID Response")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(DUT).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter(lambda p: {
                          consts.ACTIVE_TIMESTAMP_TLV,
                          consts.ADDRESS16_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.SOURCE_ADDRESS_TLV,
                          consts.PENDING_OPERATION_DATASET_TLV,
                          } <= set(p.mle.tlv.type) and\
               p.mle.tlv.pending_tstamp == 10).\
        must_next()

    # Step 9: Test Harness
    # - Description: Wait 120 seconds to allow the DUT to upgrade to a Router.
    # - Pass Criteria: N/A
    print("Step 9: Test Harness waits 120 seconds")

    # Step 10: Router_2 (DUT)
    # - Description: Power down for 200 seconds.
    # - Pass Criteria: N/A
    print("Step 10: Router_2 (DUT) powers down")

    # Step 11: Commissioner
    # - Description: Harness instructs the Commissioner to send a MGMT_PENDING_SET.req to the Leader RLOC or ALOC
    #   setting a subset of the Active Operational Dataset:
    #   - CoAP Request URI: coap://[<L>]:MM/c/ps
    #   - CoAP Payload:
    #     - valid Commissioner Session ID TLV
    #     - new, valid Active Timestamp TLV (20s)
    #     - new, valid Pending Timestamp TLV (20s)
    #     - new values for Network Mesh-Local Prefix TLV (fd00:0d:b7:00:00:00:00)
    #     - new value for PAN ID TLV (abcd)
    #     - Delay Timer TLV (200s)
    # - Pass Criteria: N/A
    print("Step 11: Commissioner sends MGMT_PENDING_SET.req")
    pkts.filter_wpan_src64(COMMISSIONER).\
        filter_coap_request(consts.MGMT_PENDING_SET_URI).\
        filter(lambda p: {
                          consts.NM_COMMISSIONER_SESSION_ID_TLV,
                          consts.NM_ACTIVE_TIMESTAMP_TLV,
                          consts.NM_PENDING_TIMESTAMP_TLV,
                          consts.NM_NETWORK_MESH_LOCAL_PREFIX_TLV,
                          consts.NM_NETWORK_NAME_TLV,
                          consts.NM_PAN_ID_TLV,
                          consts.NM_DELAY_TIMER_TLV
                          } <= set(p.coap.tlv.type)).\
        filter(lambda p: p.coap.tlv.active_timestamp == 20).\
        filter(lambda p: p.coap.tlv.pending_timestamp == 20).\
        filter(lambda p: p.coap.tlv.mesh_local_prefix == Bytes("fd000db700000000")).\
        filter(lambda p: p.coap.tlv.network_name == "threadCert").\
        filter(lambda p: p.coap.tlv.pan_id == 0xabcd).\
        filter(lambda p: p.coap.tlv.delay_timer == 230000).\
        must_next()

    # Step 12: Leader
    # - Description: Automatically sends MGMT_PENDING_SET.rsp to the Commissioner:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Accept (0x01))
    # - Pass Criteria: N/A
    print("Step 12: Leader sends MGMT_PENDING_SET.rsp")
    pkts.filter_wpan_src64(LEADER).\
        filter_coap_ack(consts.MGMT_PENDING_SET_URI).\
        filter(lambda p: p.coap.tlv.state == consts.MESHCOP_ACCEPT).\
        must_next()

    # Step 13: Commissioner
    # - Description: Harness instructs the Commissioner to send a MGMT_ACTIVE_SET.req to the Leader RLOC or Anycast
    #   Locator setting a subset of the Active Operational Dataset:
    #   - CoAP Request URI: coap://[<L>]:MM/c/as
    #   - CoAP Payload:
    #     - valid Commissioner Session ID TLV
    #     - new, valid Active Timestamp TLV (15s)
    #     - new value for Network Name TLV (“threadCert”)
    #     - new value for PSKc TLV: (74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:03)
    #   - The Leader Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
    # - Pass Criteria: N/A
    print("Step 13: Commissioner sends MGMT_ACTIVE_SET.req")
    pkts.filter_wpan_src64(COMMISSIONER).\
        filter_coap_request(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: {
                          consts.NM_COMMISSIONER_SESSION_ID_TLV,
                          consts.NM_ACTIVE_TIMESTAMP_TLV,
                          consts.NM_NETWORK_NAME_TLV,
                          consts.NM_PSKC_TLV
                          } <= set(p.coap.tlv.type)).\
        filter(lambda p: p.coap.tlv.active_timestamp == 15).\
        filter(lambda p: p.coap.tlv.network_name == "threadCert").\
        filter(lambda p: p.coap.tlv.pskc == Bytes("7468726561646a70616b657465737403")).\
        must_next()

    # Step 14: Leader
    # - Description: Automatically sends a MGMT_ACTIVE_SET.rsp to the Commissioner:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Accept (0x01))
    # - Pass Criteria: N/A
    print("Step 14: Leader sends MGMT_ACTIVE_SET.rsp")
    pkts.filter_wpan_src64(LEADER).\
        filter_coap_ack(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: p.coap.tlv.state == consts.MESHCOP_ACCEPT).\
        must_next()

    # Step 15: Router_2 (DUT)
    # - Description: Power up after 200 seconds.
    # - Pass Criteria: N/A
    print("Step 15: Router_2 (DUT) powers up")

    # Step 16: Router_2 (DUT)
    # - Description: Begins attach process by sending a multicast MLE Parent Request.
    # - Pass Criteria:
    #   - The DUT MUST send a MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an IP
    #     Hop Limit of 255, including the following TLVs:
    #     - Challenge TLV
    #     - Mode TLV
    #     - Scan Mask TLV (Verify sent to routers only)
    #     - Version TLV
    #   - The first MLE Parent Request sent by the DUT MUST NOT be sent to all routers and REEDS.
    print("Step 16: Router_2 (DUT) sends MLE Parent Request")
    pkts.filter_wpan_src64(DUT).\
        filter_LLARMA().\
        filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        filter(lambda p: {
                          consts.CHALLENGE_TLV,
                          consts.MODE_TLV,
                          consts.SCAN_MASK_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type) and\
               p.ipv6.hlim == 255 and\
               p.mle.tlv.scan_mask.r == 1 and\
               p.mle.tlv.scan_mask.e == 0).\
        must_next()

    # Step 17: Router_1
    # - Description: Automatically responds with MLE Parent Response.
    # - Pass Criteria: N/A
    print("Step 17: Router_1 sends MLE Parent Response")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(DUT).\
        filter_mle_cmd(consts.MLE_PARENT_RESPONSE).\
        must_next()

    # Step 18: Router_2 (DUT)
    # - Description: Automatically sends Child ID Request to Router_1.
    # - Pass Criteria:
    #   - The DUT MUST send a MLE Child ID Request to Router_1, including the following TLVs:
    #     - Active Timestamp TLV
    #     - Link-layer Frame Counter TLV
    #     - Mode TLV
    #     - Response TLV
    #     - Timeout TLV
    #     - TLV Request TLV
    #     - Version TLV
    #     - MLE Frame Counter TLV (optional)
    #   - The following TLV MUST NOT be present in the MLE Child ID Request:
    #     - Address Registration TLV
    print("Step 18: Router_2 (DUT) sends MLE Child ID Request")
    pkts.filter_wpan_src64(DUT).\
        filter_wpan_dst64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter(lambda p: {
                          consts.ACTIVE_TIMESTAMP_TLV,
                          consts.LINK_LAYER_FRAME_COUNTER_TLV,
                          consts.MODE_TLV,
                          consts.RESPONSE_TLV,
                          consts.TIMEOUT_TLV,
                          consts.TLV_REQUEST_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next().\
        must_not_verify(lambda p: consts.ADDRESS_REGISTRATION_TLV in p.mle.tlv.type)

    # Step 19: Router_1
    # - Description: Automatically sends Child ID Response to Router_2 (DUT), including the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #   - Address16 TLV
    #   - Network Data TLV (optional)
    #   - Route64 TLV (optional)
    #   - Address Registration TLV (optional)
    #   - Active Timestamp TLV (15s)
    #   - Active Operational Dataset TLV
    #     - includes Network Name sub-TLV (“threadCert”) corresponding to step 13
    #   - Pending Timestamp TLV (corresponding to step 10 / 11)
    #   - Pending Operational Dataset TLV (corresponding to step 11)
    # - Pass Criteria: N/A
    print("Step 19: Router_1 sends Child ID Response")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(DUT).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter(lambda p: {
                          consts.SOURCE_ADDRESS_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.ADDRESS16_TLV,
                          consts.ACTIVE_TIMESTAMP_TLV,
                          consts.ACTIVE_OPERATION_DATASET_TLV,
                          consts.PENDING_OPERATION_DATASET_TLV,
                          } <= set(p.mle.tlv.type) and\
               p.mle.tlv.active_tstamp == 15 and\
               p.mle.tlv.pending_tstamp == 20).\
        must_next()

    # Step 20: Test Harness
    # - Description: Wait 200 seconds to allow the DUT to upgrade to a Router.
    # - Pass Criteria: N/A
    print("Step 20: Test Harness waits 200 seconds")

    # Step 21: Leader
    # - Description: Harness instructs Leader to send MGMT_ACTIVE_GET.req to Router_2 (DUT) to get the Active
    #   Operational Dataset. (Request entire Active Operational Dataset by not including the Get TLV):
    #   - CoAP Request URI: coap://[<L>]:MM/c/ag
    #   - CoAP Payload: <empty>
    # - Pass Criteria: N/A
    print("Step 21: Leader sends MGMT_ACTIVE_GET.req")
    pkts.filter_wpan_src64(LEADER).\
        filter_coap_request(consts.MGMT_ACTIVE_GET_URI).\
        filter(lambda p: p.ipv6.dst in DUT_IPADDRS).\
        must_next()

    # Step 22: Router_2 (DUT)
    # - Description: Automatically responds to the Leader with a MGMT_ACTIVE_GET.rsp.
    # - Pass Criteria:
    #   - The DUT MUST send a MGMT_ACTIVE_GET.rsp to the Leader:
    #     - CoAP Response Code: 2.04 Changed
    #     - CoAP Payload: (complete active operational dataset)
    #   - The PAN ID TLV MUST have a value of abcd.
    #   - The Network Mesh-Local Prefix TLV MUST have a value of fd:00:0d:b7.
    print("Step 22: Router_2 (DUT) sends MGMT_ACTIVE_GET.rsp")
    pkts.filter_wpan_src64(DUT).\
        filter_coap_ack(consts.MGMT_ACTIVE_GET_URI).\
        filter(lambda p: p.ipv6.dst in LEADER_IPADDRS and\
               {
                consts.NM_PAN_ID_TLV,
                consts.NM_NETWORK_MESH_LOCAL_PREFIX_TLV,
                consts.NM_NETWORK_NAME_TLV,
                } <= set(p.coap.tlv.type) and\
               p.coap.tlv.pan_id == 0xabcd and\
               p.coap.tlv.mesh_local_prefix == Bytes("fd000db700000000") and\
               p.coap.tlv.network_name == "threadCert").\
        must_next()

    # Step 23: Commissioner
    # - Description: Harness verifies connectivity by instructing the Commissioner to send an ICMPv6 Echo Request to
    #   the DUT mesh local address.
    # - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
    print("Step 23: ICMPv6 Echo Request/Reply")
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(COMMISSIONER).\
        filter(lambda p: p.ipv6.dst == DUT_MLEID).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(DUT).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
