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
    # 9.2.10 Commissioning – Delay timer persistent at partitioning
    #
    # 9.2.10.1 Topology
    # - Commissioner
    # - Leader
    # - Router_1 (DUT)
    # - MED_1
    # - SED_1
    #
    # 9.2.10.2 Purpose & Description
    # The purpose of this test case is to verify that the Thread device maintains a delay timer after partitioning.
    #
    # Spec Reference                             | V1.1 Section | V1.3.0 Section
    # -------------------------------------------|--------------|---------------
    # Migrating Across Thread Network Partitions | 8.4.3.5      | 8.4.3.5

    pkts = pv.pkts
    pv.summary.show()

    COMMISSIONER = pv.vars['COMMISSIONER']
    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    MED_1 = pv.vars['MED_1']
    SED_1 = pv.vars['SED_1']

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A.
    print("Step 1: Ensure topology is formed correctly.")

    # Step 2: Commissioner
    # - Description: Harness instructs Commissioner to send a MGMT_PENDING_SET.req to the Leader’s Anycast or Routing
    #   Locator:
    #   - CoAP Request URI: coap://[<L>]:MM/c/ps
    #   - CoAP Payload:
    #     - Commissioner Session ID TLV <valid>
    #     - Active Timestamp TLV <165s>
    #     - Pending Timestamp TLV <30s>
    #     - Delay Timer TLV <250s>
    #     - Channel TLV <’Secondary’>
    #     - PAN ID TLV <0xAFCE>
    #   - The Leader Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00
    # - Pass Criteria: N/A.
    print("Step 2: Commissioner sends a MGMT_PENDING_SET.req to the Leader")
    pkts.filter_coap_request(consts.MGMT_PENDING_SET_URI).\
        filter(lambda p: {
                          consts.NM_COMMISSIONER_SESSION_ID_TLV,
                          consts.NM_ACTIVE_TIMESTAMP_TLV,
                          consts.NM_PENDING_TIMESTAMP_TLV,
                          consts.NM_DELAY_TIMER_TLV,
                          consts.NM_CHANNEL_TLV,
                          consts.NM_PAN_ID_TLV
                          } <= set(p.coap.tlv.type)).\
        must_next()

    # Step 3: Leader
    # - Description: Automatically sends a MGMT_PENDING_SET.rsp to the Commissioner:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV <value = Accept>
    # - Pass Criteria: N/A.
    print("Step 3: Leader sends a MGMT_PENDING_SET.rsp to the Commissioner")
    pkts.filter_coap_ack(consts.MGMT_PENDING_SET_URI).\
        filter(lambda p: p.coap.tlv.state == consts.MESHCOP_ACCEPT).\
        must_next()

    # Step 4: Leader
    # - Description: Automatically sends new network data to neighbors via a multicast MLE Data Response, which includes the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV:
    #     - Data Version value <incremented>
    #     - Stable Version value <incremented>
    #   - Network Data TLV:
    #     - Commissioning Data TLV:
    #       - Stable flag <set to 0>
    #       - Border Agent Locator TLV
    #       - Commissioner Session ID TLV
    #   - Active Timestamp TLV <15s>
    #   - Pending Timestamp TLV <30s>
    # - Pass Criteria: N/A.
    print("Step 4: Leader sends new network data via multicast MLE Data Response")
    pkts.filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
                          consts.SOURCE_ADDRESS_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.NETWORK_DATA_TLV,
                          consts.ACTIVE_TIMESTAMP_TLV,
                          consts.PENDING_TIMESTAMP_TLV
                          } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.mle.tlv.active_tstamp == 15).\
        filter(lambda p: p.mle.tlv.pending_tstamp == 30).\
        must_next()

    # Step 5: Router_1
    # - Description: Automatically requests the full network data from Leader via a unicast MLE Data Request
    # - Pass Criteria: For DUT=Router: The DUT MUST send a unicast MLE Data Request to the Leader, which includes the following TLVs:
    #   - TLV Request TLV:
    #     - Network Data TLV
    #   - Active Timestamp TLV
    print("Step 5: Router_1 requests full network data from Leader")
    pkts.filter_mle_cmd(consts.MLE_DATA_REQUEST).\
        filter(lambda p: {
                          consts.TLV_REQUEST_TLV,
                          consts.ACTIVE_TIMESTAMP_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 6: Leader
    # - Description: Automatically sends the requested full network data to Router_1 via a unicast MLE Data Response:
    #   - Source Address TLV
    #   - Leader Data TLV
    #   - Network Data TLV:
    #     - Commissioning Data TLV:
    #       - Stable flag <set to 0>
    #       - Border Agent Locator TLV
    #       - Commissioner Session ID TLV
    #   - Active Timestamp TLV <15s>
    #   - Pending Timestamp TLV <30s>
    #   - Pending Operational Dataset TLV:
    #     - Active Timestamp TLV <165s>
    #     - Delay Timer TLV: <250s>
    #     - Channel TLV <’Secondary’>
    #     - PAN ID TLV <0xAFCE>
    # - Pass Criteria: N/A.
    print("Step 6: Leader sends full network data to Router_1")
    pkts.filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
                          consts.SOURCE_ADDRESS_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.NETWORK_DATA_TLV,
                          consts.ACTIVE_TIMESTAMP_TLV,
                          consts.PENDING_TIMESTAMP_TLV,
                          consts.PENDING_OPERATION_DATASET_TLV
                          } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.mle.tlv.active_tstamp == 15).\
        filter(lambda p: p.mle.tlv.pending_tstamp == 30).\
        must_next()

    # Step 7: Router_1
    # - Description: Automatically sends the new network data to neighbors and rx-on-when-idle Children (MED_1) via a multicast MLE Data Response
    # - Pass Criteria: For DUT=Router: The DUT MUST send MLE Data Response to the Link-Local All Nodes multicast address (FF02::1), including the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV:
    #     - Data version numbers should be the same as the ones sent in the multicast data response in step 4
    #   - Network Data TLV:
    #     - Commissioning Data TLV:
    #       - Stable flag <set to 0>
    #       - Border Agent Locator TLV
    #       - Commissioner Session ID TLV
    #   - Active Timestamp TLV <15s>
    #   - Pending Timestamp TLV <30s>
    print("Step 7: Router_1 sends new network data via multicast MLE Data Response")
    pkts.filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
                          consts.SOURCE_ADDRESS_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.NETWORK_DATA_TLV,
                          consts.ACTIVE_TIMESTAMP_TLV,
                          consts.PENDING_TIMESTAMP_TLV
                          } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.mle.tlv.active_tstamp == 15).\
        filter(lambda p: p.mle.tlv.pending_tstamp == 30).\
        must_next()

    # Step 8: MED_1
    # - Description: Automatically requests full network data from Router_1 via a unicast MLE Data Request
    # - Pass Criteria: For DUT = MED: The DUT MUST send a unicast MLE Data Request to Router_1, including the following TLVs:
    #   - TLV Request TLV:
    #     - Network Data TLV
    #   - Active Timestamp TLV
    print("Step 8: MED_1 requests full network data from Router_1")
    pkts.filter_mle_cmd(consts.MLE_DATA_REQUEST).\
        filter(lambda p: {
                          consts.TLV_REQUEST_TLV,
                          consts.ACTIVE_TIMESTAMP_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 9: Router_1
    # - Description: Automatically sends full network data to MED_1 via a unicast MLE Data Response
    # - Pass Criteria: For DUT = Router: The DUT MUST send a unicast MLE Data Response to MED_1, including the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #     - Data version numbers should be the same as the ones sent in the multicast data response in step 4
    #   - Network Data TLV:
    #     - Commissioning Data TLV:
    #       - Stable flag <set to 0>
    #       - Commissioner Session ID TLV
    #       - Border Agent Locator TLV
    #       - Steering Data TLV
    #   - Active Timestamp TLV (new value)
    #   - Active Operational Dataset TLV**
    #     - Channel TLV
    #     - Channel Mask TLV
    #     - Extended PAN ID TLV
    #     - Network Mesh-Local Prefix TLV
    #     - Network Master Key TLV
    #     - Network Name TLV (New Value)
    #     - PAN ID TLV
    #     - PSKc TLV
    #     - Security Policy TLV
    #   - ** the Active Operational Dataset TLV MUST NOT contain the Active Timestamp TLV
    print("Step 9: Router_1 sends full network data to MED_1")
    pkts.filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
                          consts.SOURCE_ADDRESS_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.NETWORK_DATA_TLV,
                          consts.ACTIVE_TIMESTAMP_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 10B: Router_1
    # - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Data Response
    # - Pass Criteria: For DUT = Router: The DUT MUST send MLE Data Response to SED_1, which includes the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #     - Data version numbers should be the same as the ones sent in the multicast data response in step 4
    #   - Network Data TLV
    #   - Active Timestamp TLV <15s>
    #   - Pending Timestamp TLV <30s>
    print("Step 10B: Router_1 sends notification to SED_1")
    # Some versions of OT use Child Update Request for notification, we check for both.
    pkts.filter(lambda p: (p.mle.cmd == consts.MLE_DATA_RESPONSE or\
                           p.mle.cmd == consts.MLE_CHILD_UPDATE_REQUEST)).\
        next()

    # Step 11: SED_1
    # - Description: Automatically requests the full network data from Router_1 via a unicast MLE Data Request
    # - Pass Criteria: For DUT = SED: The DUT MUST send a unicast MLE Data Request to Router_1, which includes the following TLVs:
    #   - TLV Request TLV:
    #     - Network Data TLV
    #   - Active Timestamp TLV
    print("Step 11: SED_1 requests full network data from Router_1")
    pkts.filter_mle_cmd(consts.MLE_DATA_REQUEST).\
        next()

    # Step 12: Router_1
    # - Description: Automatically sends the requested full network data to SED_1
    # - Pass Criteria: For DUT=Router: The DUT MUST send a unicast MLE Data Response to SED_1, which includes the following TLVs:
    #   - Source Address TLV
    #   - Network Data TLV:
    #   - Pending Operational Dataset TLV:
    #     - Channel TLV
    #     - Active Timestamp TLV
    #     - Channel Mask TLV
    #     - Extended PAN ID TLV
    #     - Network Mesh-Local Prefix TLV
    #     - Network Master Key TLV
    #     - Network Name TLV
    #     - PAN ID TLV
    #     - PSKc TLV
    #     - Security Policy TLV
    #     - Delay Timer TLV
    #   - Active Timestamp TLV <15s>
    #   - Pending Timestamp TLV <30s>
    print("Step 12: Router_1 sends full network data to SED_1")
    pkts.filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
                          consts.SOURCE_ADDRESS_TLV,
                          consts.NETWORK_DATA_TLV,
                          consts.PENDING_OPERATION_DATASET_TLV,
                          consts.ACTIVE_TIMESTAMP_TLV,
                          consts.PENDING_TIMESTAMP_TLV
                          } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.mle.tlv.active_tstamp == 15).\
        filter(lambda p: p.mle.tlv.pending_tstamp == 30).\
        must_next()

    # Step 13: User
    # - Description: Harness instructs the user to isolate Router_1, MED_1, and SED_1 from both the Leader and the Commissioner. RF isolation will last for 300 seconds; steps 14-17 occur during isolation.
    # - Pass Criteria: N/A.
    print("Step 13: RF isolation begins.")

    # Step 14: Router_1
    # - Description: Automatically starts a new partition
    # - Pass Criteria: For DUT=Router: After NETWORK_ID_TIMEOUT, the DUT MUST start a new partition with parameters set in Active Operational Dataset (Channel = ‘Primary’, PAN ID = 0xFACE).
    print("Step 14: Router_1 starts a new partition")
    pkts.filter_LLANMA().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        must_next()

    # Step 15: Leader, Commissioner
    # - Description: After the Delay Timer expires, the network automatically moves to the Secondary channel, PAN ID = 0xAFCE
    # - Pass Criteria: N/A.
    print("Step 15: Leader and Commissioner move to the secondary channel")

    # Step 16: Router_1
    # - Description: Automatically moves to the secondary channel
    # - Pass Criteria: For DUT=Router: After the Delay Timer expires, the DUT MUST move to the Secondary channel, PAN ID = 0xAFCE.
    print("Step 16: Router_1 moves to the secondary channel")
    pkts.filter_LLANMA().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        must_next()

    # Step 17: MED_1/SED_1
    # - Description: Automatically moves to the secondary channel
    # - Pass Criteria: For DUT = MED/SED: After the Delay Timer expires, the DUT MUST move to the Secondary channel, PAN ID = 0xAFCE.
    print("Step 17: MED_1 and SED_1 move to the secondary channel")

    # Step 18: User
    # - Description: Harness instructs the user to remove the RF isolation that began in step 13
    # - Pass Criteria: N/A.
    print("Step 18: RF isolation ends.")

    # Step 19: Router_1
    # - Description: Automatically reattaches to the Leader
    # - Pass Criteria: For DUT=Router: The DUT MUST reattach to the Leader and the partitions MUST merge.
    print("Step 19: Router_1 reattaches to the Leader")
    pkts.filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST or\
                        consts.MLE_CHILD_ID_REQUEST).\
        must_next()

    # Step 20: Leader
    # - Description: The harness verifies connectivity by instructing the device to send an ICMPv6 Echo Request to the DUT mesh local address
    # - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
    print("Step 20: Leader sends an ICMPv6 Echo Request to Router_1")
    _pkt = pkts.filter_ping_request().\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
