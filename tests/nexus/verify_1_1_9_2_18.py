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

# Timestamp values from specification
ACTIVE_TIMESTAMP_20000 = 20000
ACTIVE_TIMESTAMP_20 = 20
PENDING_TIMESTAMP_20 = 20
DELAY_TIMER_300S = 300000


def verify(pv):
    # 9.2.18 Rolling Back the Active Timestamp with Pending Operational Dataset
    #
    # 9.2.18.1 Topology
    # - Commissioner
    # - Leader
    # - Router 1
    # - MED 1
    # - SED 1
    #
    # 9.2.18.2 Purpose & Description
    # The purpose of this test case is to ensure that the DUT can roll back the Active Timestamp value by scheduling
    #   an update through the Pending Operational Dataset only with the inclusion of a new Network Master Key.
    #
    # Spec Reference          | V1.1 Section | V1.3.0 Section
    # ------------------------|--------------|---------------
    # Delay Timer Management  | 8.4.3.4      | 8.4.3.4

    pkts = pv.pkts
    pv.summary.show()

    COMMISSIONER = pv.vars['COMMISSIONER']
    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    SED_1 = pv.vars['SED_1']

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A
    print("Step 1: All - Ensure topology is formed correctly.")

    # Step 2: Commissioner
    # - Description: Harness instructs Commissioner to send MGMT_ACTIVE_SET.req to the Leader Routing or Anycast
    #   Locator:
    #   - CoAP Request URI: coap://[<L>]:MM/c/as
    #   - CoAP Payload:
    #     - Commissioner Session ID TLV (valid)
    #     - Active Timestamp TLV <20000>
    #     - Network Name TLV: "GRL"
    #     - PSKc TLV: 74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:04
    #   - The Leader Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00
    # - Pass Criteria: N/A
    print("Step 2: Commissioner sends MGMT_ACTIVE_SET.req to the Leader.")
    pkts.filter_wpan_src64(COMMISSIONER).\
        filter_coap_request(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: p.coap.tlv.comm_sess_id is not nullField).\
        filter(lambda p: p.coap.tlv.active_timestamp == ACTIVE_TIMESTAMP_20000).\
        filter(lambda p: p.coap.tlv.network_name == "GRL").\
        filter(lambda p: p.coap.tlv.pskc is not nullField).\
        must_next()

    # Step 3: Leader
    # - Description: Automatically responds with a MGMT_ACTIVE_SET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_SET.rsp to the Commissioner:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV <value = Accept (01)>
    print("Step 3: Leader responds with MGMT_ACTIVE_SET.rsp.")
    pkts.filter_wpan_src64(LEADER).\
        filter_coap_ack(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: p.coap.tlv.state == 1).\
        must_next()

    # Step 4: Commissioner
    # - Description: Harness instructs the Commissioner to send MGMT_PENDING_SET.req to the Leader Routing or Anycast
    #   Locator:
    #   - CoAP Request URI: coap://[<L>]:MM/c/ps
    #   - CoAP Payload:
    #     - Commissioner Session ID TLV (valid)
    #     - Pending Timestamp TLV <20s>
    #     - Active Timestamp TLV <20s>
    #     - Delay Timer TLV <20s>
    #     - Network Name TLV : "Should not be"
    # - Pass Criteria: N/A
    print("Step 4: Commissioner sends MGMT_PENDING_SET.req to the Leader.")
    pkts.filter_wpan_src64(COMMISSIONER).\
        filter_coap_request(consts.MGMT_PENDING_SET_URI).\
        filter(lambda p: p.coap.tlv.comm_sess_id is not nullField).\
        filter(lambda p: p.coap.tlv.pending_timestamp == PENDING_TIMESTAMP_20).\
        filter(lambda p: p.coap.tlv.active_timestamp == ACTIVE_TIMESTAMP_20).\
        filter(lambda p: p.coap.tlv.delay_timer == 20000).\
        filter(lambda p: p.coap.tlv.network_name == "Should not be").\
        must_next()

    # Step 5: Leader
    # - Description: Automatically sends MGMT_PENDING_SET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_PENDING_SET.rsp to the Commissioner:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV <value = Reject (-1)>
    print("Step 5: Leader sends MGMT_PENDING_SET.rsp with Reject.")
    pkts.filter_wpan_src64(LEADER).\
        filter_coap_ack(consts.MGMT_PENDING_SET_URI).\
        filter(lambda p: p.coap.tlv.state == 255).\
        must_next()

    # Step 6: Commissioner
    # - Description: Harness instructs Commissioner to send MGMT_PENDING_SET.req to the Leader Routing or Anycast
    #   Locator:
    #   - CoAP Request URI: coap://[<L>]:MM/c/ps
    #   - CoAP Payload:
    #     - Commissioner Session ID TLV (valid)
    #     - Pending Timestamp TLV <20s>
    #     - Active Timestamp TLV <20s>
    #     - Delay Timer TLV <300s>
    #     - Network Name TLV : "My House"
    #     - PSKc TLV: 74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:05
    #     - Network Master Key TLV: (00:11:22:33:44:55:66:77:88:99:aa:bb:cc:dd:ee:ee)
    #   - The Leader Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
    # - Pass Criteria: N/A
    print("Step 6: Commissioner sends MGMT_PENDING_SET.req to the Leader.")
    pkts.filter_wpan_src64(COMMISSIONER).\
        filter_coap_request(consts.MGMT_PENDING_SET_URI).\
        filter(lambda p: p.coap.tlv.comm_sess_id is not nullField).\
        filter(lambda p: p.coap.tlv.pending_timestamp == PENDING_TIMESTAMP_20).\
        filter(lambda p: p.coap.tlv.active_timestamp == ACTIVE_TIMESTAMP_20).\
        filter(lambda p: p.coap.tlv.delay_timer == DELAY_TIMER_300S).\
        filter(lambda p: p.coap.tlv.network_name == "My House").\
        filter(lambda p: p.coap.tlv.pskc is not nullField).\
        filter(lambda p: p.coap.tlv.network_key is not nullField).\
        must_next()

    # Step 7: Leader
    # - Description: Automatically sends MGMT_PENDING_SET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_PENDING_SET.rsp to Commissioner:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV <value = Accept (01)>
    print("Step 7: Leader sends MGMT_PENDING_SET.rsp with Accept.")
    pkts.filter_wpan_src64(LEADER).\
        filter_coap_ack(consts.MGMT_PENDING_SET_URI).\
        filter(lambda p: p.coap.tlv.state == 1).\
        must_next()

    # Step 8: Leader
    # - Description: Automatically sends new network data to neighbors and rx-on-when-idle Children.
    # - Pass Criteria: For DUT = Leader: The DUT MUST multicast a MLE Data Response to the Link-Local All Nodes
    #   multicast address (FF02::1), including the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #   - Network Data TLV: Commissioning Data TLV: (Stable flag <set to 0>, Border Agent Locator TLV, Commissioner
    #     Session ID TLV)
    #   - Active Timestamp TLV
    #   - Pending Timestamp TLV
    print("Step 8: Leader multicasts MLE Data Response.")
    pkts.filter_wpan_src64(LEADER).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
            consts.SOURCE_ADDRESS_TLV,
            consts.LEADER_DATA_TLV,
            consts.NETWORK_DATA_TLV,
            consts.ACTIVE_TIMESTAMP_TLV,
            consts.PENDING_TIMESTAMP_TLV
        } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.mle.tlv.active_tstamp == ACTIVE_TIMESTAMP_20000).\
        filter(lambda p: p.mle.tlv.pending_tstamp == PENDING_TIMESTAMP_20).\
        must_next()

    # Step 9: Router_1
    # - Description: Automatically sends a unicast MLE Data Request to the Leader.
    # - Pass Criteria: For DUT = Router: The DUT MUST send a unicast MLE Data Request to the Leader including the
    #   following TLVs:
    #   - TLV Request TLV: Network Data TLV
    #   - Active Timestamp TLV
    print("Step 9: Router_1 sends MLE Data Request to Leader.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_DATA_REQUEST).\
        filter(lambda p: {
            consts.TLV_REQUEST_TLV,
            consts.ACTIVE_TIMESTAMP_TLV
        } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 10: Leader
    # - Description: Automatically sends a unicast MLE Data Response to Router_1.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send a unicast MLE Data Response to Router_1, which includes the
    #   following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #   - Network Data TLV: Commissioning Data TLV: (Stable flag <set to 0>, Border Agent Locator TLV, Commissioner
    #     Session ID TLV)
    #   - Active Timestamp TLV
    #   - Pending Timestamp TLV
    #   - Pending Operational Dataset TLV: (Active Timestamp TLV, Network Master Key TLV, Network Name TLV)
    print("Step 10: Leader sends MLE Data Response to Router_1.")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
            consts.SOURCE_ADDRESS_TLV,
            consts.LEADER_DATA_TLV,
            consts.NETWORK_DATA_TLV,
            consts.ACTIVE_TIMESTAMP_TLV,
            consts.PENDING_TIMESTAMP_TLV,
            consts.PENDING_OPERATION_DATASET_TLV
        } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 11: Router_1
    # - Description: Automatically sends the new network data to neighbors and rx-on-while-idle Children (MED_1).
    # - Pass Criteria: For DUT = Router: The DUT MUST multicast a MLE Data Response with the new information, which the
    #   following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV (Data Version field MUST have the same value that the Leader set in Step 8, Stable Data
    #     Version field MUST have the same value that the Leader set in Step 8)
    #   - Network Data TLV: (Stable flag <set to 0>, Commissioning Data TLV: Border Agent Locator TLV, Commissioner
    #     Session ID TLV)
    #   - Active Timestamp TLV
    #   - Pending Timestamp TLV
    print("Step 11: Router_1 multicasts MLE Data Response.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
            consts.SOURCE_ADDRESS_TLV,
            consts.LEADER_DATA_TLV,
            consts.NETWORK_DATA_TLV,
            consts.ACTIVE_TIMESTAMP_TLV,
            consts.PENDING_TIMESTAMP_TLV
        } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 12A: Router_1
    # - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Child Update
    #   Request.
    # - Pass Criteria: For DUT = Router: The DUT MUST send MLE Child Update Request to SED_1, including the following
    #   TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV (Data version numbers MUST be the same as the ones sent in the multicast data response in
    #     step 8.)
    #   - Network Data TLV
    #   - Active Timestamp TLV <20000s>
    #   - Pending Timestamp TLV <20s>
    #   - Goto step 13
    print("Step 12A: Router_1 sends MLE Child Update Request to SED_1.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(SED_1).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        filter(lambda p: {
            consts.SOURCE_ADDRESS_TLV,
            consts.LEADER_DATA_TLV,
            consts.NETWORK_DATA_TLV,
            consts.ACTIVE_TIMESTAMP_TLV,
            consts.PENDING_TIMESTAMP_TLV
        } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.mle.tlv.active_tstamp == ACTIVE_TIMESTAMP_20000).\
        filter(lambda p: p.mle.tlv.pending_tstamp == PENDING_TIMESTAMP_20).\
        must_next()

    # Step 13: SED_1
    # - Description: Automatically requests the full network data from Router_1 via a unicast MLE Data Request.
    # - Pass Criteria: For DUT = SED: The DUT MUST send a unicast MLE Data Request to Router_1, including the following
    #   TLVs:
    #   - TLV Request TLV: Network Data TLV
    #   - Active Timestamp TLV
    print("Step 13: SED_1 sends MLE Data Request.")
    pkts.filter_wpan_src64(SED_1).\
        filter_wpan_dst64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_DATA_REQUEST).\
        filter(lambda p: {
            consts.TLV_REQUEST_TLV,
            consts.ACTIVE_TIMESTAMP_TLV
        } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 14: Router_1
    # - Description: Automatically sends the requested full network data to SED_1.
    # - Pass Criteria: For DUT = Router: The DUT MUST send a unicast MLE Data Response to SED_1, including the following
    #   TLVs:
    #   - Source Address TLV
    #   - Network Data TLV
    #   - Pending Operational Dataset TLV: (Channel TLV, Active Timestamp TLV, Channel Mask TLV, Extended PAN ID TLV,
    #     Network Mesh-Local Prefix TLV, Network Master Key TLV, Network Name TLV, PAN ID TLV, PSKc TLV, Security Policy
    #     TLV, Delay Timer TLV)
    #   - Active Timestamp TLV <20000s>
    #   - Pending Timestamp TLV <20s>
    print("Step 14: Router_1 sends MLE Data Response to SED_1.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(SED_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
            consts.SOURCE_ADDRESS_TLV,
            consts.NETWORK_DATA_TLV,
            consts.PENDING_OPERATION_DATASET_TLV,
            consts.ACTIVE_TIMESTAMP_TLV,
            consts.PENDING_TIMESTAMP_TLV
        } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 16: Harness
    # - Description: Wait for data to distribute and for Pending set Delay time to expire ~300s.
    # - Pass Criteria: N/A
    print("Step 16: Wait for Delay Timer to expire.")

    # Step 17: Commissioner
    # - Description: Harness verifies connectivity by instructing Commissioner to send an ICMPv6 Echo Request to the
    #   DUT mesh local address.
    # - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply. ** Note that Wireshark will not be able to
    #   decode the ICMPv6 Echo Request packet.
    print("Step 17: ICMPv6 Echo Request/Reply.")
    _pkt = pkts.filter_ping_request().\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
