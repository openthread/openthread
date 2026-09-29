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

ACTIVE_TIMESTAMP_STEP3 = 70
PENDING_TIMESTAMP_STEP3 = 10
MESH_LOCAL_PREFIX_STEP3 = 'fd000db900000000'
DELAY_TIMER_STEP3 = 600000

ACTIVE_TIMESTAMP_STEP11 = 80
PENDING_TIMESTAMP_STEP11 = 20
MESH_LOCAL_PREFIX_STEP11 = 'fd000db700000000'
DELAY_TIMER_STEP11 = 200000
PAN_ID_STEP11 = 0xabcd


def verify(pv):
    # 9.2.15 Attaching with different Pending Operational Dataset
    #
    # 9.2.15.1 Topology
    # - Commissioner
    # - Leader
    # - Router_1
    # - Router_2 (DUT)
    #
    # 9.2.15.2 Purpose & Description
    # The purpose of this test case is to verify synchronization of a Pending Operational Dataset between an
    #   attaching Router and an existing Router.
    #
    # Spec Reference           | V1.1 Section | V1.3.0 Section
    # -------------------------|--------------|---------------
    # Dissemination of Datasets | 8.4.3        | 8.4.3

    pkts = pv.pkts
    pv.summary.show()

    COMMISSIONER = pv.vars['COMMISSIONER']
    LEADER = pv.vars['LEADER']
    ROUTER1 = pv.vars['ROUTER_1']
    DUT = pv.vars['DUT']

    # Step 1: Commissioner, Leader, Router_1
    # - Description: Setup the topology without the DUT. Ensure topology is formed correctly. Verify Commissioner,
    #   Leader and Router_1 are sending MLE advertisements.
    # - Pass Criteria: N/A
    print("Step 1: Setup the topology without the DUT.")

    # Step 2: Router_2 (DUT)
    # - Description: Configuration: Router_2 is configured out-of-band with Network Credentials of existing network.
    # - Pass Criteria: N/A
    print("Step 2: Configuration: Router_2 is configured out-of-band.")

    # Step 3: Commissioner
    # - Description: Harness instructs the Commissioner to send a MGMT_PENDING_SET.req to the Leader Routing or
    #   Anycast Locator, setting a subset of the Active Operational Dataset:
    #   - CoAP Request URI: coap://[<L>]:MM/c/ps
    #   - CoAP Payload:
    #     - valid Commissioner Session ID TLV
    #     - new, valid Active Timestamp TLV (70s)
    #     - new, valid Pending Timestamp TLV (10s)
    #     - new Network Mesh-Local Prefix TLV value (fd:00:0d:b9:00:00:00:00)
    #     - Delay Timer TLV: 600s
    # - Pass Criteria: N/A
    print("Step 3: Commissioner sends a MGMT_PENDING_SET.req to the Leader.")
    pkts.filter_coap_request(consts.MGMT_PENDING_SET_URI).\
        filter(lambda p: p.ipv6.src in pv.vars['COMMISSIONER_IPADDRS']).\
        filter(lambda p: {
                          consts.NM_COMMISSIONER_SESSION_ID_TLV,
                          consts.NM_ACTIVE_TIMESTAMP_TLV,
                          consts.NM_PENDING_TIMESTAMP_TLV,
                          consts.NM_NETWORK_MESH_LOCAL_PREFIX_TLV,
                          consts.NM_DELAY_TIMER_TLV
                          } <= set(p.coap.tlv.type) and\
               p.coap.tlv.active_timestamp == ACTIVE_TIMESTAMP_STEP3 and\
               p.coap.tlv.pending_timestamp == PENDING_TIMESTAMP_STEP3 and\
               p.coap.tlv.mesh_local_prefix == MESH_LOCAL_PREFIX_STEP3 and\
               p.coap.tlv.delay_timer == DELAY_TIMER_STEP3).\
        must_next()

    # Step 4: Leader
    # - Description: Automatically sends MGMT_PENDING_SET.rsp to the Commissioner:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Accept (0x01))
    # - Pass Criteria: N/A
    print("Step 4: Leader sends MGMT_PENDING_SET.rsp to the Commissioner.")
    pkts.filter_coap_ack(consts.MGMT_PENDING_SET_URI).\
        filter(lambda p: p.ipv6.dst in pv.vars['COMMISSIONER_IPADDRS']).\
        filter(lambda p: p.coap.tlv.state == consts.MESHCOP_ACCEPT).\
        must_next()

    # Step 5: Router_2 (DUT)
    # - Description: Begins attach process by sending a multicast MLE Parent Request.
    # - Pass Criteria:
    #   - The DUT MUST send a MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an IP
    #     Hop Limit of 255, including the following TLVs:
    #     - Challenge TLV
    #     - Mode TLV
    #     - Scan Mask TLV (Verify sent to routers only)
    #     - Version TLV
    #   - The first MLE Parent Request sent by the DUT MUST NOT be sent to all routers and REEDS.
    print("Step 5: Router_2 (DUT) sends a multicast MLE Parent Request.")
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

    # Step 6: Commissioner, Leader, Router_1
    # - Description: All devices automatically respond by sending MLE Parent Response to Router_2 (DUT).
    # - Pass Criteria: N/A
    print("Step 6: All devices respond by sending MLE Parent Response to Router_2 (DUT).")

    # Step 7: Router_2 (DUT)
    # - Description: Automatically sends Child ID Request to Router_1.
    # - Pass Criteria:
    #   - The DUT MUST send a MLE Child ID Request to Router_1, including the following TLVs:
    #     - Link-layer Frame Counter TLV
    #     - Mode TLV
    #     - Response TLV
    #     - Timeout TLV
    #     - TLV Request TLV
    #     - Version TLV
    #     - MLE Frame Counter TLV (optional)
    #   - The following TLVs MUST NOT be present in the Child ID Request:
    #     - Address Registration TLV
    print("Step 7: Router_2 (DUT) sends Child ID Request to Router_1.")
    _pkt = pkts.filter_wpan_src64(DUT).\
        filter_wpan_dst64(ROUTER1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter(lambda p: {
                          consts.LINK_LAYER_FRAME_COUNTER_TLV,
                          consts.MODE_TLV,
                          consts.RESPONSE_TLV,
                          consts.TIMEOUT_TLV,
                          consts.TLV_REQUEST_TLV,
                          consts.VERSION_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()
    _pkt.must_not_verify(lambda p: (consts.ADDRESS_REGISTRATION_TLV) in p.mle.tlv.type)

    # Step 8: Router_1
    # - Description: Automatically sends MLE Child ID Response to Router_2 (DUT), including the following TLVs:
    #   - Active Timestamp TLV
    #   - Address16 TLV
    #   - Leader Data TLV
    #   - Pending Operational Dataset TLV (corresponding to step 3)
    #   - Pending Timestamp TLV (corresponding to step 3)
    #   - Source Address TLV
    #   - Address Registration TLV (optional)
    #   - Network Data TLV (optional)
    #   - Route64 TLV (optional)
    # - Pass Criteria: N/A
    print("Step 8: Router_1 sends MLE Child ID Response to Router_2 (DUT).")
    pkts.filter_wpan_src64(ROUTER1).\
        filter_wpan_dst64(DUT).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter(lambda p: {
                          consts.ACTIVE_TIMESTAMP_TLV,
                          consts.ADDRESS16_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.PENDING_OPERATION_DATASET_TLV,
                          consts.PENDING_TIMESTAMP_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 9: Test Harness
    # - Description: Wait 120 seconds to allow the DUT to upgrade to a Router.
    # - Pass Criteria: N/A (implied)
    print("Step 9: Wait 120 seconds to allow the DUT to upgrade to a Router.")

    # Step 10: Router_2 (DUT)
    # - Description: Power down DUT for 200 seconds.
    # - Pass Criteria: N/A
    print("Step 10: Power down DUT for 200 seconds.")

    # Step 11: Commissioner
    # - Description: Harness instructs Commissioner to send a MGMT_PENDING_SET.req to the Leader Routing or Anycast
    #   Locator setting a subset of the Active Operational Dataset:
    #   - CoAP Request URI: coap://[<L>]:MM/c/ps
    #   - CoAP Payload:
    #     - valid Commissioner Session ID TLV
    #     - new, valid Active Timestamp TLV (80s)
    #     - new, valid Pending Timestamp TLV (20s)
    #     - new value for Network Mesh-Local Prefix TLV (fd:00:0d:b7:00:00:00:00)
    #     - Delay Timer TLV: 200s
    #     - new Pan ID TLV (abcd)
    #   - The Leader Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
    # - Pass Criteria: N/A
    print("Step 11: Commissioner sends a MGMT_PENDING_SET.req to the Leader.")
    pkts.filter_coap_request(consts.MGMT_PENDING_SET_URI).\
        filter(lambda p: p.ipv6.src in pv.vars['COMMISSIONER_IPADDRS']).\
        filter(lambda p: {
                          consts.NM_COMMISSIONER_SESSION_ID_TLV,
                          consts.NM_ACTIVE_TIMESTAMP_TLV,
                          consts.NM_PENDING_TIMESTAMP_TLV,
                          consts.NM_NETWORK_MESH_LOCAL_PREFIX_TLV,
                          consts.NM_DELAY_TIMER_TLV,
                          consts.NM_PAN_ID_TLV
                          } <= set(p.coap.tlv.type) and\
               p.coap.tlv.active_timestamp == ACTIVE_TIMESTAMP_STEP11 and\
               p.coap.tlv.pending_timestamp == PENDING_TIMESTAMP_STEP11 and\
               p.coap.tlv.mesh_local_prefix == MESH_LOCAL_PREFIX_STEP11 and\
               p.coap.tlv.delay_timer == DELAY_TIMER_STEP11 and\
               p.coap.tlv.pan_id == PAN_ID_STEP11).\
        must_next()

    # Step 12: Leader
    # - Description: Automatically sends MGMT_PENDING_SET.rsp to the Commissioner:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Accept (0x01))
    # - Pass Criteria: N/A
    print("Step 12: Leader sends MGMT_PENDING_SET.rsp to the Commissioner.")
    pkts.filter_coap_ack(consts.MGMT_PENDING_SET_URI).\
        filter(lambda p: p.ipv6.dst in pv.vars['COMMISSIONER_IPADDRS']).\
        filter(lambda p: p.coap.tlv.state == consts.MESHCOP_ACCEPT).\
        must_next()

    # Step 13: Router_2 (DUT)
    # - Description: Power up after 200 seconds.
    # - Pass Criteria: N/A (implied)
    print("Step 13: Power up after 200 seconds.")

    # Step 14: Router_2 (DUT)
    # - Description: Begins attach process by sending a multicast MLE Parent Request.
    # - Pass Criteria:
    #   - The DUT must send a MLE Parent Request to the Link-Local All-Routers multicast address (FF02::2) with an IP
    #     Hop Limit of 255, including the following TLVs:
    #     - Challenge TLV
    #     - Mode TLV
    #     - Scan Mask TLV (Verify sent to routers only)
    #     - Version TLV
    #   - The first MLE Parent Request sent by the DUT MUST NOT be sent to all routers and REEDS.
    print("Step 14: Router_2 (DUT) sends a multicast MLE Parent Request.")
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

    # Step 15: Commissioner, Leader, Router_1
    # - Description: All devices automatically send MLE Parent Response to Router_2 (DUT).
    # - Pass Criteria: N/A
    print("Step 15: All devices send MLE Parent Response to Router_2 (DUT).")

    # Step 16: Router_2 (DUT)
    # - Description: Automatically sends MLE Child ID Request to Router_1.
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
    print("Step 16: Router_2 (DUT) sends MLE Child ID Request to Router_1.")
    _pkt = pkts.filter_wpan_src64(DUT).\
        filter_wpan_dst64(ROUTER1).\
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
        must_next()
    _pkt.must_not_verify(lambda p: (consts.ADDRESS_REGISTRATION_TLV) in p.mle.tlv.type)

    # Step 17: Router_1
    # - Description: Automatically sends MLE Child ID Response to Router_2, including the following TLVs:
    #   - Active Timestamp TLV
    #   - Address16 TLV
    #   - Leader Data TLV
    #   - Pending Operational Dataset TLV (corresponding to step 11)
    #   - Pending Timestamp TLV (corresponding to step 11)
    #   - Source Address TLV
    #   - Address Registration TLV (optional)
    #   - Network Data TLV (optional)
    #   - Route64 TLV (optional)
    # - Pass Criteria: N/A
    print("Step 17: Router_1 sends MLE Child ID Response to Router_2.")
    pkts.filter_wpan_src64(ROUTER1).\
        filter_wpan_dst64(DUT).\
        filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter(lambda p: {
                          consts.ACTIVE_TIMESTAMP_TLV,
                          consts.ADDRESS16_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.PENDING_OPERATION_DATASET_TLV,
                          consts.PENDING_TIMESTAMP_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 18: Test Harness
    # - Description: Wait 200 seconds to allow the DUT to upgrade to a Router.
    # - Pass Criteria: N/A (implied)
    print("Step 18: Wait 200 seconds to allow the DUT to upgrade to a Router.")

    # Step 19: Leader
    # - Description: Harness instructs Leader to send a MGMT_ACTIVE_GET.req to Router_2 (DUT) to get the Active
    #   Operational Dataset. (Request entire Active Operational Dataset by not including the Get TLV):
    #   - CoAP Request URI: coap://[<L>]:MM/c/ag
    #   - CoAP Payload: <empty>
    # - Pass Criteria: N/A
    print("Step 19: Leader sends a MGMT_ACTIVE_GET.req to Router_2 (DUT).")
    pkts.filter_coap_request(consts.MGMT_ACTIVE_GET_URI).\
        filter(lambda p: p.ipv6.src in pv.vars['LEADER_IPADDRS'] and\
               p.ipv6.dst in pv.vars['DUT_IPADDRS']).\
        must_next()

    # Step 20: Router_2 (DUT)
    # - Description: Automatically sends MGMT_ACTIVE_GET.rsp to the Leader.
    # - Pass Criteria:
    #   - The DUT MUST send a MGMT_ACTIVE_GET.rsp to the Leader:
    #     - CoAP Response Code: 2.04 Changed
    #     - CoAP Payload: <entire active operational data set>
    #   - The PAN ID TLV MUST have a value of abcd.
    print("Step 20: Router_2 (DUT) sends MGMT_ACTIVE_GET.rsp to the Leader.")
    pkts.filter_coap_ack(consts.MGMT_ACTIVE_GET_URI).\
        filter(lambda p: p.ipv6.src in pv.vars['DUT_IPADDRS'] and\
               p.ipv6.dst in pv.vars['LEADER_IPADDRS']).\
        filter(lambda p: {
                          consts.NM_PAN_ID_TLV
                          } <= set(p.coap.tlv.type) and\
               p.coap.tlv.pan_id == PAN_ID_STEP11).\
        must_next()

    # Step 21: Commissioner
    # - Description: Harness verifies connectivity by instructing the Commissioner to send an ICMPv6 Echo Request to
    #   the DUT mesh local address.
    # - Pass Criteria: The DUT must respond with an ICMPv6 Echo Reply.
    print("Step 21: Commissioner sends an ICMPv6 Echo Request to the DUT.")
    _pkt = pkts.filter_ping_request().\
        filter(lambda p: p.ipv6.src in pv.vars['COMMISSIONER_IPADDRS'] and\
               p.ipv6.dst in pv.vars['DUT_IPADDRS']).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter(lambda p: p.ipv6.src in pv.vars['DUT_IPADDRS'] and\
               p.ipv6.dst in pv.vars['COMMISSIONER_IPADDRS']).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
