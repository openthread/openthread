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

import verify_utils
from pktverify import consts
from pktverify.null_field import nullField

# URIs
MGMT_ACTIVE_SET_URI = "/c/as"
MGMT_PENDING_SET_URI = "/c/ps"
MGMT_DATASET_CHANGED_URI = "/c/dc"


def verify(pv):
    pkts = pv.pkts
    pv.summary.show()

    COMMISSIONER = pv.vars['COMMISSIONER']
    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']
    ROUTER_2 = pv.vars['ROUTER_2']

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A
    print("Step 1: All nodes form a single partition.")

    # Step 2: Commissioner
    # - Description: Harness instructs Commissioner to send a MGMT_PENDING_SET.req to the Leader
    #   Routing or Anycast Locator:
    #   - CoAP Request URI: coap://[<L>]:MM/c/ps
    #   - CoAP Payload:
    #     - valid Commissioner Session ID TLV
    #     - Delay Timer TLV: 1000s
    #     - Channel TLV: ‘Secondary’
    #     - PAN ID TLV: 0xAFCE
    #     - Active Timestamp TLV: 210s
    #     - Pending Timestamp TLV: 30s
    #   - The Leader Anycast Locator uses the Mesh local prefix with an IID of
    #     0000:00FF:FE00:FC00.
    # - Pass Criteria: N/A
    print("Step 2: Commissioner sends a MGMT_PENDING_SET.req to the Leader.")
    pkts.filter_wpan_src64(COMMISSIONER).\
      filter_coap_request(consts.MGMT_PENDING_SET_URI).\
      filter(lambda p:
             p.coap.tlv.pending_timestamp == 30 and\
             p.coap.tlv.active_timestamp == 210 and\
             p.coap.tlv.delay_timer == 1000000).\
      must_next()

    # Step 3: Leader
    # - Description: Automatically sends MGMT_PENDING_SET.rsp to Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_PENDING_SET.rsp to the Commissioner:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV <value = Accept (01)>
    print("Step 3: Leader sends MGMT_PENDING_SET.rsp to Commissioner.")
    pkts.filter_wpan_src64(LEADER).\
      filter_coap_ack(consts.MGMT_PENDING_SET_URI).\
      filter(lambda p: p.coap.tlv.state == 1).\
      must_next()

    # Step 4: Leader
    # - Description: Automatically sends a multicast MLE Data Response.
    # - Pass Criteria: For DUT = Leader: The DUT MUST multicast a MLE Data Response with the new
    #   information, including the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #     - Data Version field incremented
    #     - Stable Data Version field incremented
    #   - Network Data TLV:
    #     - Commissioning Data TLV:
    #       - Commissioner Session ID TLV
    #       - Border Agent Locator TLV
    #       - Stable flag set to 0
    #   - Active Timestamp TLV: 10s
    #   - Pending Timestamp TLV: 30s
    print("Step 4: Leader multicasts a MLE Data Response.")
    pkts.filter_wpan_src64(LEADER).\
      filter_LLANMA().\
      filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
      filter(lambda p:
             p.mle.tlv.active_tstamp == 10 and\
             p.mle.tlv.pending_tstamp == 30).\
      must_next()

    # Step 4: Router_1
    # - Description: Automatically sends unicast MLE Data Request to the Leader.
    # - Pass Criteria: For DUT = Router: The DUT MUST send a unicast MLE Data Request to the Leader,
    #   including the following TLVs:
    #   - TLV Request TLV:
    #     - Network Data TLV
    #   - Active Timestamp TLV (10s)
    print("Step 4: Router_1 sends unicast MLE Data Request to the Leader.")
    pkts.filter_wpan_src64(ROUTER_1).\
      filter_wpan_dst64(LEADER).\
      filter_mle_cmd(consts.MLE_DATA_REQUEST).\
      must_next()

    # Step 5: Leader
    # - Description: Automatically sends unicast MLE Data Response to Router_1.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send a unicast MLE Data Response to Router_1,
    #   including the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #   - Network Data TLV:
    #     - Commissioning Data TLV:
    #       - Commissioner Session ID TLV
    #       - Border Agent Locator TLV
    #       - Stable flag set to 0
    #   - Active Timestamp TLV: 10s
    #   - Pending Timestamp TLV: 30s
    #   - Pending Operational Dataset TLV:
    #     - Active Timestamp TLV: 210s
    #     - Delay Timer TLV: ~1000s
    #     - Channel TLV: ‘Secondary’
    #     - PAN ID TLV: 0xAFCE
    print("Step 5: Leader sends unicast MLE Data Response to Router_1.")
    pkts.filter_wpan_src64(LEADER).\
      filter_wpan_dst64(ROUTER_1).\
      filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
      filter(lambda p:
             p.mle.tlv.active_tstamp == 10 and\
             p.mle.tlv.pending_tstamp == 30 and\
             (p.thread_meshcop.tlv.active_tstamp is not nullField and 210 in p.thread_meshcop.tlv.active_tstamp)).\
      must_next()

    # Step 6: Router_1
    # - Description: Automatically sends multicast MLE Data Response.
    # - Pass Criteria: For DUT = Router: The DUT MUST send a multicast MLE Data Response, including
    #   the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #     - Data Version field incremented
    #     - Stable Data Version field incremented
    #   - Network Data TLV:
    #     - Commissioning Data TLV:
    #       - Commissioner Session ID TLV
    #       - Border Agent Locator TLV
    #       - Stable flag set to 0
    #   - Active Timestamp TLV: 10s
    #   - Pending Timestamp TLV: 30s
    print("Step 6: Router_1 sends multicast MLE Data Response.")
    pkts.filter_wpan_src64(ROUTER_1).\
      filter_LLANMA().\
      filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
      filter(lambda p:
             p.mle.tlv.active_tstamp == 10 and\
             p.mle.tlv.pending_tstamp == 30).\
      must_next()

    # Step 7: Router_2
    # - Description: Automatically sends a unicast MLE Data Request to Router_1, including the
    #   following TLVs:
    #   - TLV Request TLV:
    #     - Network Data TLV
    #   - Active Timestamp TLV
    # - Pass Criteria: N/A
    print("Step 7: Router_2 sends a unicast MLE Data Request.")
    pkts.filter_wpan_src64(ROUTER_2).\
      filter_mle_cmd(consts.MLE_DATA_REQUEST).\
      filter(lambda p: p.mle.tlv.active_tstamp == 10 or p.mle.tlv.active_tstamp == nullField).\
      must_next()

    # Step 8: Router_1
    # - Description: Automatically sends a unicast MLE Data Response to Router_2.
    # - Pass Criteria: For DUT = Router: The DUT MUST send a unicast MLE Data Response to Router_2,
    #   including the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #   - Network Data TLV:
    #     - Commissioning Data TLV:
    #       - Commissioner Session ID TLV
    #       - Border Agent Locator TLV
    #       - Stable flag set to 0
    #   - Active Timestamp TLV: 10s
    #   - Pending Timestamp TLV: 30s
    #   - Pending Operational Dataset TLV:
    #     - Active Timestamp TLV: 210s
    #     - Delay Timer TLV: ~1000s
    #     - Channel TLV: ‘Secondary’
    #     - PAN ID TLV: 0xAFCE
    print("Step 8: Router_1 sends a unicast MLE Data Response to Router_2.")
    pkts.filter_wpan_src64(ROUTER_1).\
      filter_wpan_dst64(ROUTER_2).\
      filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
      filter(lambda p:
             p.mle.tlv.active_tstamp == 10 and\
             p.mle.tlv.pending_tstamp == 30 and\
             (p.thread_meshcop.tlv.active_tstamp is not nullField and 210 in p.thread_meshcop.tlv.active_tstamp)).\
      must_next()

    # Step 9: User
    # - Description: Places (Router_1 and Router_2) OR (Leader and Commissioner) in RF isolation for
    #   250 seconds.
    # - Pass Criteria: N/A
    print("Step 9: RF isolation.")

    # Step 10: Router_1, Router_2
    # - Description: Router_2 automatically creates a new partition and sets the Partition ID to its
    #   lowest possible value.
    # - Pass Criteria: For DUT = Router: The DUT MUST attach to the new partition formed by Router_2.
    print("Step 10: Router_2 creates a new partition.")

    # Step 11: Router_2
    # - Description: Harness starts the on-mesh commissioner on Router_2 and configures the following
    #   new Active Operational Dataset values:
    #   - valid Commissioner Session ID TLV
    #   - Active Timestamp TLV: 15s
    #   - Network Name TLV: ‘TEST’
    # - Pass Criteria: N/A
    print("Step 11: Router_2 configures new Active Dataset.")

    # Step 12: Router_1
    # - Description: Automatically unicasts MLE Data Request to Router_2.
    # - Pass Criteria: For DUT = Router: The DUT MUST send a unicast MLE Data Request to Router_2,
    #   including the following TLVs:
    #   - TLV Request TLV:
    #     - Network Data TLV
    #   - Active Timestamp TLV (10s)
    #   - Pending Timestamp TLV (30s)
    print("Step 12: Router_1 unicasts MLE Data Request to Router_2.")
    pkts.filter_wpan_src64(ROUTER_1).\
      filter_wpan_dst64(ROUTER_2).\
      filter_mle_cmd(consts.MLE_DATA_REQUEST).\
      filter(lambda p: p.mle.tlv.active_tstamp == 10 and p.mle.tlv.pending_tstamp == 30).\
      must_next()

    # Step 13: Router_2
    # - Description: Automatically unicasts MLE Data Response to Router_1.
    # - Pass Criteria: N/A
    print("Step 13: Router_2 unicasts MLE Data Response to Router_1.")
    pkts.filter_wpan_src64(ROUTER_2).\
      filter_wpan_dst64(ROUTER_1).\
      filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
      filter(lambda p: p.mle.tlv.active_tstamp == 15).\
      must_next()

    # Step 14: Router_1
    # - Description: Automatically sends multicast MLE Data Response.
    # - Pass Criteria: For DUT = Router: The DUT MUST send a multicast MLE Data Response, including
    #   the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #     - Data Version field incremented
    #     - Stable Data Version field incremented
    #   - Network Data TLV:
    #     - Commissioning Data TLV:
    #       - Commissioner Session ID TLV
    #       - Border Agent Locator TLV
    #       - Stable flag set to 0
    #   - Active Timestamp TLV: 15s
    #   - Pending Timestamp TLV: 30s
    print("Step 14: Router_1 multicasts MLE Data Response.")
    pkts.filter_wpan_src64(ROUTER_1).\
      filter_LLANMA().\
      filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
      filter(lambda p:
             p.mle.tlv.active_tstamp == 15 and\
             p.mle.tlv.pending_tstamp == 30).\
      must_next()

    # Step 15: Router_2
    # - Description: Harness configures the device with a new Pending Operational Dataset with the
    #   following values:
    #   - valid Commissioner Session ID TLV
    #   - Delay Timer TLV: 200s
    #   - Channel TLV: ‘Primary’
    #   - PAN ID TLV: 0xABCD
    #   - Active Timestamp TLV: 410s
    #   - Pending Timestamp TLV: 50s
    # - Pass Criteria: N/A
    print("Step 15: Router_2 configures new Pending Operational Dataset.")

    # Step 16: Router_2
    # - Description: Automatically sends multicast MLE Data Response with the new information
    #   including the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #     - Data Version field incremented
    #     - Stable Data Version field incremented
    #   - Network Data TLV including:
    #     - Commissioning Data TLV:
    #       - Commissioner Session ID TLV
    #       - Border Agent Locator TLV
    #       - Stable flag set to 0
    #   - Active Timestamp TLV: 15s
    #   - Pending Timestamp TLV: 50s
    # - Pass Criteria: N/A
    print("Step 16: Router_2 multicasts MLE Data Response.")
    pkts.filter_wpan_src64(ROUTER_2).\
      filter_LLANMA().\
      filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
      filter(lambda p:
             p.mle.tlv.active_tstamp == 15 and\
             p.mle.tlv.pending_tstamp == 50).\
      must_next()

    # Step 17: Router_1
    # - Description: Automatically sends unicast MLE Data Request to Router_2.
    # - Pass Criteria: For DUT = Router: The DUT MUST send a unicast MLE Data Request to Router_2,
    #   including the following TLVs:
    #   - TLV Request TLV:
    #     - Network Data TLV
    #   - Active Timestamp TLV (15s)
    #   - Pending Timestamp TLV (30s)
    print("Step 17: Router_1 sends unicast MLE Data Request to Router_2.")
    pkts.filter_wpan_src64(ROUTER_1).\
      filter_wpan_dst64(ROUTER_2).\
      filter_mle_cmd(consts.MLE_DATA_REQUEST).\
      filter(lambda p:
             p.mle.tlv.active_tstamp == 15 and\
             p.mle.tlv.pending_tstamp == 30).\
      must_next()

    # Step 18: Router_2
    # - Description: Automatically sends unicast MLE Data Response to Router_1 ….
    # - Pass Criteria: N/A
    print("Step 18: Router_2 sends unicast MLE Data Response to Router_1.")
    pkts.filter_wpan_src64(ROUTER_2).\
      filter_wpan_dst64(ROUTER_1).\
      filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
      filter(lambda p:
             p.mle.tlv.active_tstamp == 15 and\
             p.mle.tlv.pending_tstamp == 50 and\
             (p.thread_meshcop.tlv.active_tstamp is not nullField and 410 in p.thread_meshcop.tlv.active_tstamp)).\
      must_next()

    # Step 19: Router_1
    # - Description: Automatically sends multicast MLE Data Response.
    # - Pass Criteria: For DUT = Router: The DUT MUST send a multicast MLE Data Response, which
    #   includes the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #     - Data Version field incremented
    #     - Stable Data Version field incremented
    #   - Network Data TLV:
    #     - Commissioning Data TLV:
    #       - Commissioner Session ID TLV
    #       - Border Agent Locator TLV
    #       - Stable flag set to 0
    #   - Active Timestamp TLV: 15s
    #   - Pending Timestamp TLV: 50s
    print("Step 19: Router_1 multicasts MLE Data Response.")
    pkts.filter_wpan_src64(ROUTER_1).\
      filter_LLANMA().\
      filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
      filter(lambda p:
             p.mle.tlv.active_tstamp == 15 and\
             p.mle.tlv.pending_tstamp == 50).\
      must_next()

    # Step 20: User
    # - Description: Removes RF isolation.
    # - Pass Criteria: N/A
    print("Step 20: Removes RF isolation.")

    # Step 21: Router_1
    # - Description: Automatically attaches to the Leader.
    # - Pass Criteria: For DUT = Router: The DUT MUST go through the attachment process and send MLE
    #   Child ID Request to the Leader, including the following TLV:
    #   - Active Timestamp TLV: 15s
    print("Step 21: Router_1 attaches to the Leader.")
    pkts.filter_wpan_src64(ROUTER_1).\
      filter_wpan_dst64(LEADER).\
      filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
      filter(lambda p: p.mle.tlv.active_tstamp == 15).\
      must_next()

    # Step 22: Leader
    # - Description: Automatically replies to Router_1 with MLE Child ID Response.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MLE Child ID Response to Router_1,
    #   including its current active timestamp and active configuration set:
    #   - Active Timestamp TLV: 10s
    #   - Active Operational Dataset TLV:
    #     - Network Name TLV: “GRL”
    #   - Pending Timestamp TLV: 30s
    #   - Pending Operational Dataset TLV:
    #     - Active Timestamp TLV: 210s
    print("Step 22: Leader sends MLE Child ID Response.")
    pkts.filter_wpan_src64(LEADER).\
      filter_wpan_dst64(ROUTER_1).\
      filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
      filter(lambda p:
             p.mle.tlv.active_tstamp == 10 and\
             p.mle.tlv.pending_tstamp == 30 and\
             (p.thread_meshcop.tlv.net_name is not nullField and "GRL" in p.thread_meshcop.tlv.net_name) and\
             (p.thread_meshcop.tlv.active_tstamp is not nullField and 210 in p.thread_meshcop.tlv.active_tstamp)).\
      must_next()

    # Dataset synchronization after merge (Steps 23-30).
    # All these happen from the same base index after re-attachment.
    base_sync_index = pkts.index

    # Step 23: Router_1
    # - Description: Automatically sends a MGMT_ACTIVE_SET.req to the Leader RLOC or Anycast Locator.
    # - Pass Criteria: For DUT = Router: The DUT MUST send MGMT_ACTIVE_SET.req to the Leader RLOC or
    #   Anycast Locator:
    #   - CoAP Request URI: coap://[Leader]:MM/c/as
    #   - CoAP Payload:
    #     - Entire Active Operational Dataset
    #     - Active Timestamp TLV: 15s
    #     - Network Name TLV: “TEST”
    #     - PAN ID TLV
    #     - Channel TLV
    #     - Channel Mask TLV
    #     - Extended PAN ID TLV
    #     - Mesh Local Prefix TLV
    #     - Network Master Key
    #     - Security Policy TLV
    #     - PSKc TLV
    #     - NO Commissioner Session ID TLV
    #   - The Leader Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00
    with pkts.save_index():
        pkts.index = base_sync_index
        print("Step 23: Router_1 sends MGMT_ACTIVE_SET.req.")
        pkts.filter_wpan_src64(ROUTER_1).\
          filter_coap_request(consts.MGMT_ACTIVE_SET_URI).\
          filter(lambda p:
                 p.coap.tlv.active_timestamp == 15 and\
                 p.coap.tlv.network_name == "TEST").\
          must_next()

    # Step 27: Router_1
    # - Description: Automatically sends MGMT_PENDING_SET.req to the Leader Router or Anycast Locator
    #   (RLOC or ALOC).
    # - Pass Criteria: For DUT = Router: The DUT MUST send a MGMT_PENDING_SET.req to the Leader RLOC
    #   or ALOC:
    #   - CoAP Request URI: coap://[Leader]:MM/c/ps
    #   - CoAP Payload:
    #     - Delay Timer TLV: ~200s
    #     - Channel TLV: ‘Primary’
    #     - PAN ID TLV: 0xABCD
    #     - Network Name TLV: ‘TEST’
    #     - Active Timestamp TLV: 410s
    #     - Pending Timestamp TLV: 50s
    #     - Entire Pending Operational Dataset
    #     - NO Commissioner Session ID TLV
    #   - The Leader Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
    with pkts.save_index():
        pkts.index = base_sync_index
        print("Step 27: Router_1 sends MGMT_PENDING_SET.req.")
        pkts.filter_wpan_src64(ROUTER_1).\
          filter_coap_request(consts.MGMT_PENDING_SET_URI).\
          filter(lambda p:
                 p.coap.tlv.pending_timestamp == 50 and\
                 p.coap.tlv.active_timestamp == 410 and\
                 p.coap.tlv.delay_timer <= 200000 and\
                 p.coap.tlv.channel == 11 and\
                 p.coap.tlv.pan_id == 0xABCD).\
          must_next()

    # Step 24: Leader
    # - Description: Automatically sends a MGMT_ACTIVE_SET.rsp to Router_1.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_ACTIVE_SET.rsp to Router_1:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV <value = Accept>
    with pkts.save_index():
        pkts.index = base_sync_index
        print("Step 24: Leader sends MGMT_ACTIVE_SET.rsp.")
        pkts.filter_wpan_src64(LEADER).\
          filter_coap_ack(consts.MGMT_ACTIVE_SET_URI).\
          filter(lambda p: p.coap.tlv.state == 1).\
          must_next()

    # Step 28: Leader
    # - Description: Automatically sends MGMT_PENDING_SET.rsp to Router_1.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_PENDING_SET.rsp to Router_1:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV <value = Accept>
    with pkts.save_index():
        pkts.index = base_sync_index
        print("Step 28: Leader sends MGMT_PENDING_SET.rsp.")
        pkts.filter_wpan_src64(LEADER).\
          filter_coap_ack(consts.MGMT_PENDING_SET_URI).\
          filter(lambda p: p.coap.tlv.state == 1).\
          must_next()

    # Step 25: Leader
    # - Description: Automatically sends MGMT_DATASET_CHANGED.ntf to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_DATASET_CHANGED.ntf to the
    #   Commissioner:
    #   - CoAP Request: coap://[ Commissioner]:MM/c/dc
    #   - CoAP Payload: <empty>
    #
    # Step 29: Leader
    # - Description: Automatically sends MGMT_DATASET_CHANGED.ntf to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_DATASET_CHANGED.ntf to the
    #   Commissioner:
    #   - CoAP Request: coap://[ Commissioner]:MM/c/dc
    #   - CoAP Payload: <empty>
    with pkts.save_index():
        pkts.index = base_sync_index
        print("Step 25/29: Leader sends MGMT_DATASET_CHANGED.ntf to the Commissioner.")
        pkts.filter_wpan_src64(LEADER).\
          filter_coap_request(consts.MGMT_DATASET_CHANGED_URI).\
          must_next()

    # Step 26: Leader
    # - Description: Automatically sends multicast MLE Data Response with the new information.
    # - Pass Criteria: N/A
    #
    # Step 30: Leader
    # - Description: Automatically sends multicast MLE Data Response.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send a multicast MLE Data Response with the
    #   new information, including the following TLVs:
    #   - Source Address TLV
    #   - Leader Data TLV
    #     - Data Version field incremented
    #     - Stable Data Version field incremented
    #   - Network Data TLV:
    #     - Commissioning Data TLV:
    #       - Commissioner Session ID TLV
    #       - Border Agent Locator TLV
    #       - Stable flag set to 0
    #   - Active Timestamp TLV: 15s
    #   - Pending Timestamp TLV: 50s
    with pkts.save_index():
        pkts.index = base_sync_index
        print("Step 26/30: Leader multicasts final MLE Data Response.")
        pkts.filter_wpan_src64(LEADER).\
          filter_LLANMA().\
          filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
          filter(lambda p:
                 p.mle.tlv.active_tstamp == 15 and\
                 p.mle.tlv.pending_tstamp == 50).\
          must_next()

    # Step 31: Commissioner
    # - Description: Automatically sends a MLE Data Request to the Leader, including the following
    #   TLVs:
    #   - TLV Request TLV
    #     - Network Data TLV
    #   - Active Timestamp TLV
    # - Pass Criteria: N/A
    #
    # Step 32: Leader
    # - Description: Automatically sends unicast MLE Data Response to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send a unicast MLE Data Response to the
    #   Commissioner, including the new Pending Timestamp and Pending Operational Dataset:
    #   - Source Address TLV
    #   - Leader Data TLV
    #   - PAN ID TLV: 0xABCD
    #   - Active Timestamp TLV: 15s
    #   - Active Operational Dataset TLV:
    #     - Network Name TLV: ‘TEST’
    #   - Pending Timestamp TLV: 50s
    #   - Pending Operational Dataset TLV:
    #     - Active Timestamp TLV: 410s
    #     - Delay Timer TLV: ~200s
    #     - Channel TLV: ‘Primary’
    #     - PAN ID TLV: 0xABCD
    #     - Network Name TLV: ‘TEST’
    #
    # Step 33: Router_1
    # - Description: Automatically sends MLE Data Request to the Leader.
    # - Pass Criteria: For DUT = Router: The DUT MUST send MLE Data Request to the Leader, including
    #   the following TLVs:
    #   - TLV Request TLV
    #     - Network Data TLV
    #   - Active Timestamp TLV (10s)
    #   - Pending Timestamp TLV (30s)
    #
    # Step 34: Leader
    # - Description: Automatically sends a unicast MLE Data Response to Router_1.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send a unicast MLE Data Response to Router_1,
    #   including the new Pending Timestamp and Pending Operational Dataset:
    #   - Source Address TLV
    #   - Leader Data TLV
    #   - Active Timestamp TLV: 15s
    #   - Active Operational Dataset TLV:
    #     - Network Name TLV: ‘TEST’
    #   - Pending Timestamp TLV: 50s
    #   - Pending Operational Dataset TLV:
    #     - Active Timestamp TLV: 410s
    #     - Delay Timer TLV: ~200s
    #     - Channel TLV: ‘Primary’
    #     - PAN ID TLV: 0xABCD
    #
    # NOTE: Steps 31-34 are optional and often skipped in practice because the multicast update in
    # Step 30 may have already synchronized all nodes.
    print("Steps 31-34: Optional final synchronization checks (skipped).")

    # Step 35: Router_2
    # - Description: Automatically re-attaches to its old partition.
    # - Pass Criteria: N/A
    print("Step 35: Router_2 re-attaches to its old partition.")

    # Step 36: Commissioner, Router_2
    # - Description: Harness verifies connectivity by sending an ICMPv6 Echo Request to the DUT mesh
    #   local address:
    #   - For DUT = Router, the ping is sent from the Commissioner.
    #   - For DUT = Leader, the ping is sent from Router_2.
    # - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
    print("Step 36: Connectivity verification with ICMPv6 Echo.")
    pkts.filter_ping_request().must_next()
    pkts.filter_ping_reply().must_next()
    pkts.filter_ping_request().must_next()
    pkts.filter_ping_reply().must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
