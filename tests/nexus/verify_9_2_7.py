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

# Active Timestamp for Leader.
ACTIVE_TIMESTAMP_LEADER = 10

# Active Timestamp for Step 5.
# Deviation from spec: Step 5 uses 15 instead of 20 to ensure it is older than the timestamp in Step 11.
ACTIVE_TIMESTAMP_STEP_5 = 15

# Active Timestamp for Router.
ACTIVE_TIMESTAMP_ROUTER = 20

# Pending Timestamp for Router.
PENDING_TIMESTAMP_ROUTER = 30

# Pending Timestamp for Commissioner.
PENDING_TIMESTAMP_COMMISSIONER = 40

# Active Timestamp for Commissioner.
ACTIVE_TIMESTAMP_COMMISSIONER = 80

# Delay timer value in seconds (60 minutes).
DELAY_TIMER_STEP_11 = 3600

# Delay timer value in seconds (1 minute).
DELAY_TIMER_STEP_17 = 60

# PAN ID for Step 17.
PAN_ID_STEP_17 = 0xafce

# Secondary Channel.
SECONDARY_CHANNEL = 12


def verify(pv):
    #  9.2.7 Commissioning – Delay Timer Management
    #
    #  9.2.7.1 Topology
    #  - NOTE: Two sniffers are required to run this test case!
    #  - Set on Leader: Active Timestamp = 10s
    #  - Set on Router: Active Timestamp = 20s, Pending Timestamp = 30s
    #  - At the start of the test, the Router has a current Pending Operational Dataset with a delay timer set to 60
    #    minutes.
    #  - Router Partition Weight is configured to a value of 47, to make it always lower than the Leader's weight.
    #  - Initially, there is no link between the Leader and the Router.
    #
    #  9.2.7.2 Purpose & Description
    #  The purpose of this test case is to verify that if the Leader receives a Pending Operational Dataset with a
    #    newer Pending Timestamp, it resets the running delay timer, installs the new Pending Operational Dataset, and
    #    disseminates the new Commissioning information in the network.
    #
    #  Spec Reference                          | V1.1 Section | V1.3.0 Section
    #  ----------------------------------------|--------------|---------------
    #  Updating the Active Operational Dataset | 8.7.4        | 8.7.4

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER = pv.vars['ROUTER']
    COMMISSIONER = pv.vars['COMMISSIONER']

    #  Step 1: All
    #  - Description: Ensure topology is formed correctly.
    #  - Pass Criteria: N/A
    print("Step 1: Ensure topology is formed correctly.")

    #  Step 2: Harness
    #  - Description: Enable link between the DUT and Router.
    #  - Pass Criteria: N/A
    print("Step 2: Enable link between the DUT and Router.")

    #  Step 3: Router
    #  - Description: Automatically attaches to the Leader (DUT). Within the MLE Child ID Request of the attach
    #    process, it includes the new active and pending timestamps.
    #  - Pass Criteria: N/A
    print("Step 3: Router attaches to the Leader.")

    #  Step 4: Leader (DUT)
    #  - Description: Automatically sends MLE Child ID Response to the Router.
    #  - Pass Criteria: The DUT MUST send a unicast MLE Child ID Response to the Router, including the following TLVs:
    #    - Active Operational Dataset TLV:
    #      - Channel TLV
    #      - Channel Mask TLV
    #      - Extended PAN ID TLV
    #      - Network Master Key TLV
    #      - Network Mesh-Local Prefix TLV
    #      - Network Name TLV
    #      - PAN ID TLV
    #      - PSKc TLV
    #      - Security Policy TLV
    #    - Active Timestamp TLV: 10s
    print("Step 4: Leader (DUT) sends MLE Child ID Response to the Router.")
    pkts.filter_mle_cmd(consts.MLE_CHILD_ID_RESPONSE).\
        filter(lambda p: p.mle.tlv.active_tstamp == ACTIVE_TIMESTAMP_LEADER).\
        must_next()

    #  Steps 5-8 can be interleaved
    with pkts.save_index():
        #  Step 5: Router
        #  - Description: Harness instructs the Router to send a MGMT_ACTIVE_SET.req to the DUT’s Anycast or Router
        #    Locator:
        #    - CoAP Request URI: coap://[<L>]:MM/c/as
        #    - CoAP Payload: < Commissioner Session ID TLV not present>, Active Timestamp TLV : 15s, Active Operational
        #      Dataset TLV: all parameters in Active Dataset
        #    - The Leader Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
        #  - Note: Deviation from spec: Step 5 uses 15 instead of 20 to ensure it is older than the timestamp in Step 11.
        #  - Pass Criteria: N/A
        print("Step 5: Router sends a MGMT_ACTIVE_SET.req to the DUT.")
        pkts.filter_coap_request(consts.MGMT_ACTIVE_SET_URI).\
            filter(lambda p: p.coap.tlv.active_timestamp == ACTIVE_TIMESTAMP_STEP_5).\
            must_next()

        #  Step 6: Leader (DUT)
        #  - Description: Automatically sends a MGMT_ACTIVE_SET.rsp to the Router.
        #  - Pass Criteria: The DUT MUST send MGMT_ACTIVE_SET.rsp to the Router with the following format:
        #    - CoAP Response Code: 2.04 Changed
        #    - CoAP Payload: State TLV (value = Accept)
        print("Step 6: Leader (DUT) sends a MGMT_ACTIVE_SET.rsp to the Router.")
        pkts.filter_coap_ack(consts.MGMT_ACTIVE_SET_URI).\
            filter(lambda p: p.coap.tlv.state == 1).\
            must_next()

    with pkts.save_index():
        #  Step 8: Leader (DUT)
        #  - Description: Automatically sends MGMT_DATASET_CHANGED.ntf to the Commissioner.
        #  - Pass Criteria: The DUT MUST send MGMT_DATASET_CHANGED.ntf to the Commissioner with the following format:
        #    - CoAP Request URI: coap://[Commissioner]:MM/c/dc
        #    - CoAP Payload: <empty>
        print("Step 8: Leader (DUT) sends MGMT_DATASET_CHANGED.ntf to the Commissioner.")
        pkts.filter_coap_request(consts.MGMT_DATASET_CHANGED_URI).\
            must_next()

    #  Step 7: Leader (DUT) multicasts a MLE Data Response with the new information.
    print("Step 7: Leader (DUT) multicasts a MLE Data Response with the new information.")
    pkts.filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        must_next()

    #  Step 9: Router
    #  - Description: Automatically sends a unicast MLE Data Request to the Leader (DUT) with its current Active
    #    Timestamp.
    #  - Pass Criteria: N/A
    print("Step 9: Router sends a unicast MLE Data Request to the Leader.")

    #  Step 10: Leader (DUT)
    #  - Description: Automatically sends a unicast MLE Data Response to the Router with the new active timestamp and
    #    active operational dataset.
    #  - Pass Criteria: The DUT MUST send a unicast MLE Data Response to the Router, including the following TLVs:
    #    - Active Operational Dataset TLV
    #      - Channel TLV
    #      - Channel Mask TLV
    #      - Extended PAN ID TLV
    #      - Network Master Key TLV
    #      - Network Mesh-Local Prefix TLV
    #      - Network Name TLV
    #      - PAN ID TLV
    #      - PSKc TLV
    #      - Security Policy TLV
    #    - Active Timestamp TLV: 15s
    print("Step 10: Leader (DUT) sends a unicast MLE Data Response to the Router.")
    pkts.filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: p.mle.tlv.active_tstamp == ACTIVE_TIMESTAMP_STEP_5).\
        must_next()

    #  Steps 11-14 can be interleaved
    with pkts.save_index():
        #  Step 11: Commissioner
        #  - Description: Harness instructs the Commissioner to send a MGMT_PENDING_SET.req to the DUT’s Anycast or Routing
        #    Locator:
        #    - CoAP Request URI: coap://[<L>]:MM/c/ps
        #    - CoAP Payload: < Commissioner Session ID TLV not present>, Pending Timestamp TLV: 30s, Active Timestamp TLV:
        #      20s, Delay Timer TLV
        #  - Pass Criteria: N/A
        print("Step 11: Commissioner sends a MGMT_PENDING_SET.req to the DUT.")
        pkts.filter_coap_request(consts.MGMT_PENDING_SET_URI).\
            must_next()

        #  Step 12: Leader (DUT)
        #  - Description: Automatically sends MGMT_PENDING_SET.rsp to the Commissioner and incorporates the new pending
        #    dataset values.
        #  - Pass Criteria: The DUT MUST send MGMT_PENDING_SET.rsp to the Commissioner with the following format:
        #    - CoAP Response Code: 2.04 Changed
        #    - CoAP Payload: State TLV (value = Accept)
        print("Step 12: Leader (DUT) sends MGMT_PENDING_SET.rsp to the Router.")
        pkts.filter_coap_ack(consts.MGMT_PENDING_SET_URI).\
            filter(lambda p: p.coap.tlv.state == 1).\
            must_next()

    with pkts.save_index():
        #  Step 14: Leader (DUT)
        #  - Description: Automatically sends a MGMT_DATASET_CHANGED.ntf to the Commissioner.
        #  - Pass Criteria: THE DUT MUST send MGMT_DATASET_CHANGED.ntf to the Commissioner with the following format:
        #    - CoAP Request URI: coap://[Commissioner]:MM/c/dc
        #    - CoAP Payload: <empty>
        print("Step 14: Leader (DUT) sends MGMT_DATASET_CHANGED.ntf to the Commissioner.")
        pkts.filter_coap_request(consts.MGMT_DATASET_CHANGED_URI).\
            must_next()

    #  Step 13: Leader (DUT) multicasts a MLE Data Response with the new information.
    print("Step 13: Leader (DUT) multicasts a MLE Data Response.")
    pkts.filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        must_next()

    #  Step 15: Router
    #  - Description: Automatically sends a unicast MLE Data Request to the Leader (DUT) with a new active timestamp.
    #  - Pass Criteria: N/A
    print("Step 15: Router sends a unicast MLE Data Request to the Leader.")

    #  Step 16: Leader (DUT)
    #  - Description: Automatically sends a unicast MLE Data Response to the Router with a new active timestamp, new
    #    pending timestamp, and a new pending operational dataset.
    #  - Pass Criteria: The DUT MUST send a unicast MLE Data Response to the Router, which includes the following
    #    TLVs:
    #    - Pending Operational Dataset TLV:
    #      - Active Timestamp TLV
    #      - Channel TLV
    #      - Channel Mask TLV
    #      - Delay Timer TLV
    #      - Extended PAN ID TLV
    #      - Network Master Key TLV
    #      - Network Mesh-Local Prefix TLV
    #      - Network Name TLV
    #      - PAN ID TLV
    #      - PSKc TLV
    #      - Security Policy TLV
    #    - Active Timestamp TLV: 20s
    #    - Pending Timestamp TLV: 100s
    print("Step 16: Leader (DUT) sends a unicast MLE Data Response to the Router.")
    pkts.filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: p.mle.tlv.active_tstamp == ACTIVE_TIMESTAMP_STEP_5 and\
                         p.mle.tlv.pending_tstamp == PENDING_TIMESTAMP_ROUTER and\
                         DELAY_TIMER_STEP_11 * 1000 - 10000 <= p.thread_meshcop.tlv.delay_timer <= DELAY_TIMER_STEP_11 * 1000).\
        must_next()

    #  Steps 17-21 can be interleaved
    with pkts.save_index():
        #  Step 17: Commissioner
        #  - Description: Harness instructs the Commissioner to send a MGMT_PENDING_SET.req to the Leader’s Anycast or
        #    Routing Locator:
        #    - CoAP Request URI: coap://[<L>]:MM/c/ps
        #    - CoAP Payload: Valid Commissioner Session ID TLV, Pending Timestamp TLV: 40s, Active Timestamp TLV: 80s,
        #      Delay Timer TLV: 1min, Channel TLV: ‘Secondary’, PAN ID TLV: 0xAFCE
        #    - The Leader Anycast Locator uses the Mesh local prefix with an IID of 0000:00FF:FE00:FC00.
        #  - Pass Criteria: N/A
        print("Step 17: Commissioner sends a MGMT_PENDING_SET.req to the Leader.")
        pkts.filter_coap_request(consts.MGMT_PENDING_SET_URI).\
            must_next()

        #  Step 18: Leader (DUT)
        #  - Description: Automatically sends a MGMT_PENDING_SET.rsp to the Commissioner with Status = Accept.
        #  - Pass Criteria: The DUT MUST send MGMT_PENDING_SET.rsp to the Commissioner with the following format:
        #    - CoAP Response Code: 2.04 Changed
        #    - CoAP Payload: State TLV (value = Accept (0x01))
        print("Step 18: Leader (DUT) sends a MGMT_PENDING_SET.rsp to the Commissioner.")
        pkts.filter_coap_ack(consts.MGMT_PENDING_SET_URI).\
            filter(lambda p: p.coap.tlv.state == 1).\
            must_next()

    #  Step 19: Leader (DUT) sends a multicast MLE Data Response.
    print("Step 19: Leader (DUT) sends a multicast MLE Data Response.")
    pkts.filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        must_next()

    #  Step 20: Router
    #  - Description: Automatically sends a unicast MLE Data Request to the DUT with the new active timestamp and
    #    pending timestamp:
    #    - Active Timestamp TLV: 20s
    #    - Pending Timestamp TLV: 100s
    #  - Pass Criteria: N/A
    print("Step 20: Router sends a unicast MLE Data Request to the DUT.")

    #  Step 21: Leader (DUT) sends a unicast MLE Data Response to the Router.
    print("Step 21: Leader (DUT) sends a unicast MLE Data Response to the Router.")
    pkts.filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: p.mle.tlv.active_tstamp == ACTIVE_TIMESTAMP_STEP_5 and\
                         p.mle.tlv.pending_tstamp == PENDING_TIMESTAMP_COMMISSIONER and\
                         DELAY_TIMER_STEP_17 * 1000 - 10000 <= p.thread_meshcop.tlv.delay_timer <= DELAY_TIMER_STEP_17 * 1000).\
        must_next()

    #  Step 22: All
    #  - Description: Verify that after 60 seconds, the Thread network moves to the Secondary channel, with PAN ID:
    #    0xAFCE.
    #  - Pass Criteria: N/A
    print("Step 22: Verify network moves to Secondary channel.")

    #  Step 23: All
    #  - Description: Verify connectivity by sending an ICMPv6 Echo Request to the DUT mesh local address.
    #  - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
    print("Step 23: Verify connectivity.")
    _pkt = pkts.filter_ping_request().\
        must_next()
    pkts.filter_ping_reply(identifier=_pkt.icmpv6.echo.identifier).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
