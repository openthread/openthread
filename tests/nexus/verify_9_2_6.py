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

import struct
import verify_utils
from pktverify import consts


def verify(pv):
    # 9.2.6 Commissioning - Dissemination of Operational Datasets
    #
    # 9.2.6.1 Topology
    # - DUT as Leader (Topology A)
    # - DUT as Router (Topology B)
    # - DUT as MED/SED (Topologies C and D)
    #
    # Note: Two sniffers are required to run this test case!
    #
    # 9.2.6.2 Purpose & Description
    # - DUT as Leader (Topology A): The purpose of this test case is to verify that the Leader device properly collects
    #   and disseminates Operational Datasets through a Thread network.
    # - DUT as Router (Topology B): The purpose of this test case is to show that the Router device correctly sets the
    #   Commissioning information propagated by the Leader device and sends it properly to devices already attached to
    #   it.
    # - DUT as MED/SED (Topologies C and D):
    #   - MED - requires full network data
    #   - SED - requires only stable network data
    # - Set on Leader: Active TimeStamp = 10s
    #
    # Spec Reference                                     | V1.1 Section  | V1.3.0 Section
    # ---------------------------------------------------|---------------|---------------
    # Updating the Active / Pending Operational Dataset  | 8.7.4 / 8.7.5 | 8.7.4 / 8.7.5

    from pktverify import consts
    from pktverify.null_field import nullField

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    COMMISSIONER = pv.vars['COMMISSIONER']
    ROUTER_1 = pv.vars['ROUTER_1']
    MED_1 = pv.vars['MED_1']
    SED_1 = pv.vars['SED_1']

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A
    print("Step 1: Ensure topology is formed correctly.")

    # Step 2: Commissioner
    # - Description: Harness instructs Commissioner to send MGMT_COMMISSIONER_SET.req to the Leader Anycast or Routing
    #   Locator:
    #   - CoAP URI-Path: coap (S)://[<Leader>]:MM/c/cs
    #   - CoAP Payload: Commissioner Session ID TLV (valid value), Steering Data TLV (allowed TLV)
    # - Pass Criteria: N/A
    print("Step 2: Commissioner sends MGMT_COMMISSIONER_SET.req")
    pkts.filter_coap_request(consts.MGMT_COMMISSIONER_SET_URI).\
        filter(lambda p: p.coap.tlv.type is not nullField and\
               {
                consts.NM_COMMISSIONER_SESSION_ID_TLV,
                consts.NM_STEERING_DATA_TLV
                } <= set(p.coap.tlv.type)).\
        must_next()

    # Step 3: Leader
    # - Description: Automatically sends MGMT_COMMISSIONER_SET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_COMMISSIONER_SET.rsp with the following format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV <value = Accept (0x01)>
    print("Step 3: Leader sends MGMT_COMMISSIONER_SET.rsp")
    pkts.filter_coap_ack(consts.MGMT_COMMISSIONER_SET_URI).\
        filter(lambda p: p.coap.tlv.state == consts.MESHCOP_ACCEPT).\
        must_next()

    # Step 4: Leader
    # - Description: Automatically sends new network data to neighbors and rx-on-when-idle Children (MED_1) via a
    #   multicast MLE Data Response.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send a multicast MLE Data Response to the Link-Local All Nodes
    #   multicast address (FF02::1) with the new information, including the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV: Data Version field <incremented>, Stable Data Version field <NOT incremented>
    #   - Network Data TLV: Commissioning Data TLV:: Stable flag <set to 0>, Border Agent Locator TLV, Commissioner
    #     Session ID TLV, Steering Data TLV
    #   - Active Timestamp TLV
    print("Step 4: Leader multicasts MLE Data Response")
    index4 = pkts.index
    pkts.filter_wpan_src64(LEADER).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: p.mle.tlv.type is not nullField and\
               {
                consts.SOURCE_ADDRESS_TLV,
                consts.LEADER_DATA_TLV,
                consts.NETWORK_DATA_TLV,
                consts.ACTIVE_TIMESTAMP_TLV
                } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 5: Router_1
    # - Description: Automatically sends new network data to neighbors and rx-on-when-idle Children (MED_1) via a
    #   multicast MLE Data Response.
    # - Pass Criteria: For DUT = Router: The DUT MUST send a multicast MLE Data Response with the new information,
    #   including the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV: Data Version field <incremented>, Stable Data Version field <NOT incremented>
    #   - Network Data TLV: Commissioning Data TLV:: Stable flag <set to 0>, Border Agent Locator TLV, Commissioner
    #     Session ID TLV, Steering Data TLV
    #   - Active Timestamp TLV
    print("Step 5: Router_1 multicasts MLE Data Response")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: p.mle.tlv.type is not nullField and\
               {
                consts.SOURCE_ADDRESS_TLV,
                consts.LEADER_DATA_TLV,
                consts.NETWORK_DATA_TLV,
                consts.ACTIVE_TIMESTAMP_TLV
                } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 7: Commissioner
    # - Description: Harness instructs the Commissioner to send MGMT_ACTIVE_SET.req to the Leader Anycast or Routing
    #   Locator:
    #   - CoAP Request: coap://[<L>]:MM/c/as
    #   - CoAP Payload: valid Commissioner Session ID TLV, Active Timestamp TLV : 15s, Network Name TLV : Thread,
    #     PSKc TLV: 74:68:72:65:61:64:6a:70:61:6b:65:74:65:73:74:02 (new value)
    # - Pass Criteria: N/A
    print("Step 7: Commissioner sends MGMT_ACTIVE_SET.req")
    pkts.filter_coap_request(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: p.coap.tlv.type is not nullField and\
               {
                consts.NM_COMMISSIONER_SESSION_ID_TLV,
                consts.NM_ACTIVE_TIMESTAMP_TLV,
                consts.NM_NETWORK_NAME_TLV,
                consts.NM_PSKC_TLV
                } <= set(p.coap.tlv.type)).\
        must_next()

    # Step 6: Router_1
    # - Description: No update is sent to SED_1 because Stable Data Version is unchanged.
    # - Pass Criteria: For DUT = Router: The DUT MUST NOT send a unicast MLE Data Response or MLE Child Update Request
    #   to SED_1.
    print("Step 6: Router_1 does NOT send update to SED_1")
    pkts.range(index4, pkts.index).filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(SED_1).\
        filter(lambda p: p.mle.cmd in (consts.MLE_DATA_RESPONSE, consts.MLE_CHILD_UPDATE_REQUEST)).\
        must_not_next()

    # Step 8: Leader
    # - Description: Automatically sends MGMT_ACTIVE_SET.rsp to the Commissioner with Status = Accept.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send a MGMT_ACTIVE_SET.rsp frame with the following format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV <Accept>
    print("Step 8: Leader sends MGMT_ACTIVE_SET.rsp")
    pkts.filter_coap_ack(consts.MGMT_ACTIVE_SET_URI).\
        filter(lambda p: p.coap.tlv.state == consts.MESHCOP_ACCEPT).\
        must_next()

    # Step 9: Leader
    # - Description: Automatically sends new network data to neighbors via a multicast MLE Data Response.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send a multicast MLE Data Response to the Link-Local All Nodes
    #   multicast address (FF02::1) with the new information, including the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV: Data Version field <incremented>, Stable Data Version field <incremented>
    #   - Network Data TLV: Commissioning Data TLV:: Stable flag <set to 0>, Border Agent Locator TLV, Commissioner
    #     Session ID TLV, Steering Data TLV
    #   - Active Timestamp TLV: 15s
    print("Step 9: Leader multicasts MLE Data Response")
    pkts.filter_wpan_src64(LEADER).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: p.mle.tlv.type is not nullField and\
               {
                consts.SOURCE_ADDRESS_TLV,
                consts.LEADER_DATA_TLV,
                consts.NETWORK_DATA_TLV,
                consts.ACTIVE_TIMESTAMP_TLV
                } <= set(p.mle.tlv.type)).\
        must_next()
    index9 = pkts.index

    # Step 10: Router_1
    # - Description: Automatically requests the full network data from the Leader via a unicast MLE Data Request.
    # - Pass Criteria: For DUT = Router: The DUT MUST send a unicast MLE Data Request to the Leader, which includes the
    #   following TLVs:
    #   - TLV Request TLV: Network Data TLV
    #   - Active Timestamp TLV
    print("Step 10: Router_1 sends MLE Data Request to Leader")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_DATA_REQUEST).\
        filter(lambda p: p.mle.tlv.type is not nullField and\
               {
                consts.TLV_REQUEST_TLV,
                } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 11: Leader
    # - Description: Automatically sends the requested full network data to Router_1 via a unicast MLE Data Response.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send a unicast MLE Data Response to Router_1 including the
    #   following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV: Data version numbers should be the same as the ones sent in the multicast data response in
    #     step 9
    #   - Network Data TLV: Commissioning Data TLV:: Stable flag <set to 0>, Border Agent Locator TLV, Commissioner
    #     Session ID TLV, Steering Data TLV
    #   - Active Timestamp TLV <new value>
    #   - Active Operational Dataset TLV (MUST NOT contain the Active Timestamp TLV): Channel TLV, Channel Mask TLV,
    #     Extended PAN ID TLV, Network Mesh-Local Prefix TLV, Network Master Key TLV, Network Name TLV <new value>, PAN
    #     ID TLV, PSKc TLV, Security Policy TLV
    print("Step 11: Leader sends unicast MLE Data Response to Router_1")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: p.mle.tlv.type is not nullField and\
               {
                consts.SOURCE_ADDRESS_TLV,
                consts.LEADER_DATA_TLV,
                consts.NETWORK_DATA_TLV,
                consts.ACTIVE_TIMESTAMP_TLV,
                } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 12: Router_1
    # - Description: Automatically sends the full network data to neighbors and rx-on-while-idle Children (MED_1) via a
    #   multicast MLE Data Response.
    # - Pass Criteria: For DUT = Router: The DUT MUST send a multicast MLE Data Response with the new information,
    #   including the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV: Data version numbers should be the same as the ones sent in the multicast data response in
    #     step 9
    #   - Network Data TLV: Commissioning Data TLV:: Stable flag <set to 0>, Border Agent Locator TLV, Commissioner
    #     Session ID TLV, Steering Data TLV
    #   - Active Timestamp TLV <15s>
    print("Step 12: Router_1 multicasts MLE Data Response")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: p.mle.tlv.type is not nullField and\
               {
                consts.SOURCE_ADDRESS_TLV,
                consts.LEADER_DATA_TLV,
                consts.NETWORK_DATA_TLV,
                consts.ACTIVE_TIMESTAMP_TLV
                } <= set(p.mle.tlv.type)).\
        must_next()
    index12 = pkts.index

    # The MED and SED update groups (Steps 13-14 and 15-17) can happen in any order.
    # We search from index9 because proactive requests might happen early.
    med_pkts = pkts.range(index9, cascade=False)
    sed_pkts = pkts.range(index9, cascade=False)

    # Step 13: MED_1
    # - Description: Automatically requests full network data from Router_1 via a unicast MLE Data Request.
    # - Pass Criteria: For DUT = MED: The DUT MUST send a unicast MLE Data Request to Router_1, including the following
    #   TLVs:
    #   - TLV Request TLV: Network Data TLV
    #   - Active Timestamp TLV
    print("Step 13: MED_1 sends MLE Data Request to Router_1")
    # MED_1 is rx-on-when-idle and may have already updated from multicast Step 12.
    pkt13 = med_pkts.filter_wpan_src64(MED_1).\
        filter_wpan_dst64(ROUTER_1).\
        filter(lambda p: (p.mle.cmd == consts.MLE_DATA_REQUEST) and \
                p.mle.tlv.type is not nullField and\
               {
                consts.TLV_REQUEST_TLV,
                } <= set(p.mle.tlv.type)).\
        next()

    if pkt13:
        # Step 14: Router_1
        # - Description: Automatically sends full network data to MED_1 via a unicast MLE Data Response.
        # - Pass Criteria: For DUT = Router: The DUT MUST send a unicast MLE Data Response to MED_1, which includes the
        #   following TLVs:
        #   - Source Address TLV
        #   - Leader Data TLV: Data version numbers should be the same as the ones sent in the multicast data response in
        #     step 9.
        #   - Network Data TLV: Commissioning Data TLV:: Stable flag <set to 0>, Commissioner Session ID TLV, Border Agent
        #     Locator TLV, Steering Data TLV
        #   - Active Timestamp TLV (new value)
        #   - Active Operational Dataset TLV (MUST NOT contain the Active Timestamp TLV): Channel TLV, Channel Mask TLV,
        #     Extended PAN ID TLV, Network Mesh-Local Prefix TLV, Network Master Key TLV, Network Name TLV (New Value), PAN
        #     ID TLV, PSKc TLV, Security Policy TLV
        print("Step 14: Router_1 sends unicast MLE Data Response to MED_1")
        med_pkts.filter_wpan_src64(ROUTER_1).\
            filter_wpan_dst64(MED_1).\
            filter(lambda p: (p.mle.cmd == consts.MLE_DATA_RESPONSE or \
                               p.mle.cmd == consts.MLE_CHILD_UPDATE_RESPONSE) and \
                    p.mle.tlv.type is not nullField and\
                   {
                    consts.SOURCE_ADDRESS_TLV,
                    } <= set(p.mle.tlv.type)).\
            must_next()

    # Step 15A: Router_1
    # - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Child Update
    #   Request.
    # - Pass Criteria: For DUT = Router: The DUT MUST send MLE Child Update Request to SED_1, including the following
    #   TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV: Data version numbers should be the same as the ones sent in the multicast data response in
    #     step 9
    #   - Network Data TLV
    #   - Active Timestamp TLV <15s>
    #   - Goto step 16
    print("Step 15A: Router_1 sends Child Update Request or Data Response to SED_1")
    pkt15 = sed_pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(SED_1).\
        filter(lambda p: (p.mle.cmd == consts.MLE_CHILD_UPDATE_REQUEST or \
                           p.mle.cmd == consts.MLE_DATA_RESPONSE) and \
                p.mle.tlv.type is not nullField and\
               {
                consts.SOURCE_ADDRESS_TLV,
                consts.LEADER_DATA_TLV,
                } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 15B: Router_1
    # - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Data Response.
    # - Pass Criteria: For DUT = Router: The DUT MUST send MLE Data Response to SED_1, including the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV: Data version numbers should be the same as the ones sent in the multicast data response in
    #     step 9
    #   - Network Data TLV
    #   - Active Timestamp TLV <15s>
    print("Step 15B: Already verified in Step 15A")

    # If the notification didn't include the full Active Operational Dataset, SED_1 must request it.
    if consts.ACTIVE_OPERATION_DATASET_TLV not in set(pkt15.mle.tlv.type):
        # Step 16: SED_1
        # - Description: Automatically requests the full network data from Router_1 via a unicast MLE Data Request.
        # - Pass Criteria: For DUT = SED: The DUT MUST send a unicast MLE Data Request to Router_1, including the following
        #   TLVs:
        #   - TLV Request TLV: Network Data TLV
        #   - Active Timestamp TLV
        print("Step 16: SED_1 sends MLE Data Request to Router_1")
        # Search from index9 to allow request before notification
        pkts.range(index9).filter_wpan_src64(SED_1).\
            filter_wpan_dst64(ROUTER_1).\
            filter(lambda p: (p.mle.cmd == consts.MLE_DATA_REQUEST) and \
                    p.mle.tlv.type is not nullField and\
                   {
                    consts.TLV_REQUEST_TLV,
                    } <= set(p.mle.tlv.type)).\
            must_next()

        # Step 17: Router_1
        # - Description: Automatically sends the requested full network data to SED_1.
        # - Pass Criteria: For DUT = Router: The DUT MUST send a unicast MLE Data Response to SED_1, including the
        #   following TLVs:
        #   - Source Address TLV
        #   - Leader Data TLV: Data version numbers should be the same as the ones sent in the multicast data response in
        #     step 9
        #   - Network Data TLV
        #   - Active Timestamp TLV <15s>
        #   - Active Operational Dataset TLV (MUST NOT contain the Active Timestamp TLV): Channel TLV, Channel Mask TLV,
        #     Extended PAN ID TLV, Network Mesh-Local Prefix TLV, Network Master Key TLV, Network Name TLV <new value>, PAN
        #     ID TLV, PSKc TLV, Security Policy TLV.
        print("Step 17: Router_1 sends unicast MLE Data Response to SED_1")
        pkts.range(index9).filter_wpan_src64(ROUTER_1).\
            filter_wpan_dst64(SED_1).\
            filter(lambda p: (p.mle.cmd == consts.MLE_DATA_RESPONSE or \
                               p.mle.cmd == consts.MLE_CHILD_UPDATE_RESPONSE) and \
                    p.mle.tlv.type is not nullField and\
                   {
                    consts.SOURCE_ADDRESS_TLV,
                    } <= set(p.mle.tlv.type)).\
            must_next()

    pkts.index = max(med_pkts.index, sed_pkts.index)

    # Step 18: Commissioner
    # - Description: Harness instructs Commissioner to send MGMT_PENDING_SET.req to the Leader Anycast or Routing
    #   Locator:
    #   - CoAP Request: coap://[<L>]:MM/c/ps
    #   - CoAP Payload: Commissioner Session ID TLV <valid value>, Pending Timestamp TLV <30s>, Active Timestamp TLV
    #     <75s>, Delay Timer TLV <1 min>, Channel TLV <Secondary>
    # - Pass Criteria: N/A
    print("Step 18: Commissioner sends MGMT_PENDING_SET.req")
    # Commissioner sends MGMT_PENDING_SET.req after Step 12.
    # It might overlap with MED/SED updates (Steps 13-17).
    pkts_step18 = pv.pkts.range(index12)
    pkts_step18.filter_coap_request(consts.MGMT_PENDING_SET_URI).\
        filter(lambda p: p.coap.tlv.type is not nullField and\
               {
                consts.NM_COMMISSIONER_SESSION_ID_TLV,
                consts.NM_PENDING_TIMESTAMP_TLV,
                } <= set(p.coap.tlv.type)).\
        must_next()
    index18 = pkts_step18.index

    # Step 19: Leader
    # - Description: Automatically sends MGMT_PENDING_SET.rsp to the Commissioner with Status = Accept.
    # - Pass Criteria: For DUT = Leader: The Leader MUST send MGMT_PENDING_SET.rsp frame to the Commissioner with the
    #   following format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV <Accept>
    print("Step 19: Leader sends MGMT_PENDING_SET.rsp")
    pkts.range(index18).filter_coap_ack(consts.MGMT_PENDING_SET_URI).\
        filter(lambda p: p.coap.tlv.state == consts.MESHCOP_ACCEPT).\
        must_next()

    # Step 20: Leader
    # - Description: Automatically sends new network data to neighbors via a multicast MLE Data Response.
    # - Pass Criteria: For DUT = Leader: The DUT MUST multicast a MLE Data Response with the new information, including
    #   the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV: Data version field <incremented>, Stable Version field <incremented>
    #   - Network Data TLV: Commissioning Data TLV:: Stable flag <set to 0>, Border Agent Locator TLV, Commissioner
    #     Session ID TLV, Steering Data TLV
    #   - Active Timestamp TLV
    #   - Pending Timestamp TLV
    print("Step 20: Leader multicasts MLE Data Response")
    pkts.range(index18).filter_wpan_src64(LEADER).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: p.mle.tlv.type is not nullField and\
               {
                consts.SOURCE_ADDRESS_TLV,
                consts.LEADER_DATA_TLV,
                consts.NETWORK_DATA_TLV,
                consts.ACTIVE_TIMESTAMP_TLV,
                consts.PENDING_TIMESTAMP_TLV
                } <= set(p.mle.tlv.type)).\
        must_next()
    index20 = pkts.index

    # Step 21: Router_1
    # - Description: Automatically requests full network data from the Leader via a unicast MLE Data Request.
    # - Pass Criteria: For DUT = Router_1: The DUT MUST send a unicast MLE Data Request to the Leader, including the
    #   following TLVs:
    #   - Request TLV: Network Data TLV
    #   - Active Timestamp TLV
    print("Step 21: Router_1 sends MLE Data Request to Leader")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(LEADER).\
        filter_mle_cmd(consts.MLE_DATA_REQUEST).\
        filter(lambda p: p.mle.tlv.type is not nullField and\
               {
                consts.TLV_REQUEST_TLV,
                } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 22: Leader
    # - Description: Automatically sends full network data to Router_1 via a unicast MLE Data Response.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send a unicast MLE Data Response to Router_1, including the
    #   following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #   - Network Data TLV: Commissioning Data TLV:: Stable flag <set to 0>, Border Agent Locator TLV, Commissioner
    #     Session ID TLV, Steering Data TLV
    #   - Pending Operational Dataset TLV
    #   - Active Timestamp TLV
    #   - Pending Timestamp TLV
    print("Step 22: Leader sends unicast MLE Data Response to Router_1")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: p.mle.tlv.type is not nullField and\
               {
                consts.SOURCE_ADDRESS_TLV,
                consts.LEADER_DATA_TLV,
                consts.NETWORK_DATA_TLV,
                consts.ACTIVE_TIMESTAMP_TLV,
                } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 23: Router_1
    # - Description: Automatically sends new network data to neighbors and rx-on-when-idle Children via a multicast MLE
    #   Data Response.
    # - Pass Criteria: For DUT = Router: The DUT MUST multicast a MLE Data Response with the new information, including
    #   the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV: Data version numbers should be the same as the ones sent in the multicast data response in
    #     step 20
    #   - Network Data TLV: Commissioning Data TLV:: Stable flag <set to 0>, Border Agent Locator TLV, Commissioner
    #     Session ID TLV, Steering Data TLV
    #   - Active Timestamp TLV <15s>
    #   - Pending Timestamp TLV <30s>
    print("Step 23: Router_1 multicasts MLE Data Response")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: p.mle.tlv.type is not nullField and\
               {
                consts.SOURCE_ADDRESS_TLV,
                consts.LEADER_DATA_TLV,
                consts.NETWORK_DATA_TLV,
                consts.ACTIVE_TIMESTAMP_TLV,
                consts.PENDING_TIMESTAMP_TLV
                } <= set(p.mle.tlv.type)).\
        must_next()
    index23 = pkts.index

    # The MED and SED update groups (Steps 24-25 and 26-28) can happen in any order.
    # We search from index20 because proactive requests might happen early.
    med_pkts = pkts.range(index20, cascade=False)
    sed_pkts = pkts.range(index20, cascade=False)

    # Step 24: MED_1
    # - Description: Automatically requests full network data from Router_1 via a unicast MLE Data Request.
    # - Pass Criteria: For DUT = MED: The DUT MUST send a unicast MLE Data Request to Router_1 including the following
    #   TLVs:
    #   - TLV Request TLV: Network Data TLV
    #   - Active Timestamp TLV
    print("Step 24: MED_1 sends MLE Data Request to Router_1")
    # MED_1 is rx-on-when-idle and may have already updated from multicast Step 23.
    pkt24 = med_pkts.filter_wpan_src64(MED_1).\
        filter_wpan_dst64(ROUTER_1).\
        filter(lambda p: (p.mle.cmd == consts.MLE_DATA_REQUEST) and \
                p.mle.tlv.type is not nullField and\
               {
                consts.TLV_REQUEST_TLV,
                } <= set(p.mle.tlv.type)).\
        next()

    if pkt24:
        # Step 25: Router_1
        # - Description: Automatically sends full network data to MED_1 via a unicast MLE Data Response.
        # - Pass Criteria: For DUT = Router: The DUT MUST send a unicast MLE Data Response to MED_1, including the
        #   following TLVs:
        #   - Source Address TLV
        #   - Leader Data TLV: Data version numbers should be the same as the ones sent in the multicast data response in
        #     step 20
        #   - Network Data TLV: Commissioning Data TLV:: Stable flag <set to 0>, Border Agent Locator TLV, Commissioner
        #     Session ID TLV, Steering Data TLV
        #   - Pending Operational Dataset TLV: Channel TLV, Active Timestamp TLV, Channel Mask TLV, Extended PAN ID TLV,
        #     Network Mesh-Local Prefix TLV, Network Master Key TLV, Network Name TLV, PAN ID TLV, PSKc TLV, Security
        #     Policy TLV, Delay Timer TLV
        #   - Active Timestamp TLV
        #   - Pending Timestamp TLV
        print("Step 25: Router_1 sends unicast MLE Data Response to MED_1")
        med_pkts.filter_wpan_src64(ROUTER_1).\
            filter_wpan_dst64(MED_1).\
            filter(lambda p: (p.mle.cmd == consts.MLE_DATA_RESPONSE or \
                               p.mle.cmd == consts.MLE_CHILD_UPDATE_RESPONSE) and \
                    p.mle.tlv.type is not nullField and\
                   {
                    consts.SOURCE_ADDRESS_TLV,
                    consts.PENDING_OPERATION_DATASET_TLV
                    } <= set(p.mle.tlv.type)).\
            must_next()

    # Step 26A: Router_1
    # - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Child Update
    #   Request.
    # - Pass Criteria: For DUT = Router: The DUT MUST send MLE Child Update Request to SED_1, including the following
    #   TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV: Data version numbers should be the same as the ones sent in the multicast data response in
    #     step 20
    #   - Network Data TLV
    #   - Active Timestamp TLV <15s>
    #   - Pending Timestamp TLV <30s>
    #   - Goto step 27
    print("Step 26A: Router_1 sends Child Update Request or Data Response to SED_1")
    pkt26 = sed_pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(SED_1).\
        filter(lambda p: (p.mle.cmd == consts.MLE_CHILD_UPDATE_REQUEST or \
                           p.mle.cmd == consts.MLE_DATA_RESPONSE) and \
                p.mle.tlv.type is not nullField and\
               {
                consts.SOURCE_ADDRESS_TLV,
                consts.LEADER_DATA_TLV,
                } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 26B: Router_1
    # - Description: Automatically sends notification of new network data to SED_1 via a unicast MLE Data Response.
    # - Pass Criteria: For DUT = Router: The DUT MUST send MLE Data Response to SED_1, including the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV: Data version numbers should be the same as the ones sent in the multicast data response in
    #     step 20
    #   - Network Data TLV
    #   - Active Timestamp TLV <15s>
    #   - Pending Timestamp TLV <30s>
    print("Step 26B: Already verified in Step 26A")

    # If the notification didn't include the full Pending Operational Dataset, SED_1 must request it.
    if consts.PENDING_OPERATION_DATASET_TLV not in set(pkt26.mle.tlv.type):
        # Step 27: SED_1
        # - Description: Automatically requests the full network data from Router_1 via a unicast MLE Data Request.
        # - Pass Criteria: For DUT = SED: The DUT MUST send a unicast MLE Data Request to Router_1, including the following
        #   TLVs:
        #   - TLV Request TLV: Network Data TLV
        #   - Active Timestamp TLV
        print("Step 27: SED_1 sends MLE Data Request to Router_1")
        # Search from index20 to allow request before notification
        pkts.range(index20).filter_wpan_src64(SED_1).\
            filter_wpan_dst64(ROUTER_1).\
            filter(lambda p: (p.mle.cmd == consts.MLE_DATA_REQUEST) and \
                    p.mle.tlv.type is not nullField and\
                   {
                    consts.TLV_REQUEST_TLV,
                    } <= set(p.mle.tlv.type)).\
            must_next()

        # Step 28: Router_1
        # - Description: Automatically sends the requested full network data to SED_1.
        # - Pass Criteria: For DUT = Router: The DUT MUST send a unicast MLE Data Response to SED_1, including the
        #   following TLVs:
        #   - Source Address TLV
        #   - Network Data TLV
        #   - Pending Operational Dataset TLV: Channel TLV, Active Timestamp TLV, Channel Mask TLV, Extended PAN ID TLV,
        #     Network Mesh-Local Prefix TLV, Network Master Key TLV, Network Name TLV, PAN ID TLV, PSKc TLV, Security
        #     Policy TLV, Delay Timer TLV
        #   - Active Timestamp TLV <15s>
        #   - Pending Timestamp TLV <30s>
        print("Step 28: Router_1 sends unicast MLE Data Response to SED_1")
        pkts.range(index20).filter_wpan_src64(ROUTER_1).\
            filter_wpan_dst64(SED_1).\
            filter(lambda p: (p.mle.cmd == consts.MLE_DATA_RESPONSE or \
                               p.mle.cmd == consts.MLE_CHILD_UPDATE_RESPONSE) and \
                    p.mle.tlv.type is not nullField and\
                   {
                    consts.SOURCE_ADDRESS_TLV,
                    } <= set(p.mle.tlv.type)).\
            must_next()

    pkts.index = max(med_pkts.index, sed_pkts.index)

    # Step 29: Harness
    # - Description: Wait for delay timer to expire.
    # - Pass Criteria: N/A
    print("Step 29: Wait for delay timer to expire.")

    # Step 30: Harness
    # - Description: Harness verifies connectivity by sending an ICMPv6 Echo Request to the DUT mesh local address on
    #   the (new) Secondary channel.
    # - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
    print("Step 30: ICMPv6 Echo Request/Reply on Secondary channel")
    # LEADER
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(COMMISSIONER).\
        filter_ipv6_dst(pv.vars['LEADER_MLEID']).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(LEADER).\
        must_next()
    # ROUTER_1
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(COMMISSIONER).\
        filter_ipv6_dst(pv.vars['ROUTER_1_MLEID']).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(ROUTER_1).\
        must_next()
    # MED_1
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(COMMISSIONER).\
        filter_ipv6_dst(pv.vars['MED_1_MLEID']).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(MED_1).\
        must_next()
    # SED_1
    _pkt = pkts.filter_ping_request().\
        filter_wpan_src64(COMMISSIONER).\
        filter_ipv6_dst(pv.vars['SED_1_MLEID']).\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        filter_wpan_src64(SED_1).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
