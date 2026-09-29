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
    # 9.2.2 On Mesh Commissioner – MGMT_COMMISSIONER_SET.req & rsp
    #
    # 9.2.2.1 Topology
    # - DUT as Leader, Commissioner (Non-DUT)
    #
    # 9.2.2.2 Purpose & Description
    # - DUT as Leader (Topology A): The purpose of this test case is to verify Leader’s behavior when receiving
    #   MGMT_COMMISSIONER_SET.req directly from the active Commissioner.
    #
    # Spec Reference                    | V1.1 Section | V1.3.0 Section
    # ----------------------------------|--------------|---------------
    # Updating the Commissioner Dataset | 8.7.3        | 8.7.3

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    COMMISSIONER = pv.vars['COMMISSIONER']

    # Step 1: All
    # - Description: Ensure topology is formed correctly.
    # - Pass Criteria: N/A.
    print("Step 1: All")

    # Step 2: Topology A Leader DUT
    # - This step should only be run when the DUT is the Leader. Skip this step if the DUT is the Commissioner.
    # - Description: Harness instructs Commissioner to send MGMT_COMMISSIONER_SET.req to DUT Anycast or Routing Locator:
    #   - CoAP Request URI: coap://[<L>]:MM/c/cs
    #   - CoAP Payload: (missing Commissioner Session ID TLV), Steering Data TLV (0xFF)
    # - Pass Criteria: N/A.
    print("Step 2: Topology A Leader DUT")
    pkts.filter_coap_request(consts.MGMT_COMMISSIONER_SET_URI).\
        filter(lambda p: {
                          consts.NM_STEERING_DATA_TLV
                          } <= set(p.coap.tlv.type) and\
               consts.NM_COMMISSIONER_SESSION_ID_TLV not in p.coap.tlv.type).\
        must_next()

    # Step 3: Leader
    # - Please note that step is only valid if step 2 is run.
    # - Description: DUT automatically responds to MGMT_COMMISSIONER_SET.req with a MGMT_COMMISSIONER_SET.rsp to
    #   Commissioner without user or harness intervention.
    # - Pass Criteria: Verify MGMT_COMMISSIONER_SET.rsp frame has the following format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Reject (0xFF))
    print("Step 3: Leader")
    pkts.filter(lambda p: p.coap.code == 68 and\
               p.coap.opt.uri_path_recon == consts.MGMT_COMMISSIONER_SET_URI and\
               p.coap.tlv.state == 255).\
        must_next()

    # Step 4: Topology B Commissioner (DUT) / Topology A Leader DUT
    # - Description:
    #   - Topology B: User instructs Commissioner DUT to send MGMT_COMMISSIONER_SET.req to Leader.
    #   - Topology A: Harness instructs Commissioner to send MGMT_COMMISSIONER_SET.req to DUT Anycast or
    #     Routing Locator.
    # - Pass Criteria:
    #   - Topology B: Verify MGMT_COMMISSIONER_SET.req frame has the following format:
    #     - CoAP Request URI: coap://[<L>]:MM/c/cs
    #     - CoAP Payload: Commissioner Session ID TLV, Steering Data TLV (0xFF)
    #   - Topology A:
    #     - CoAP Request URI: coap://[<L>]:MM/c/cs
    #     - CoAP Payload: Commissioner Session ID TLV, Steering Data TLV (0xFF)
    #   - Topology A & B: Verify Destination Address of MGMT_COMMISSIONER_SET.req frame is DUT’s Anycast or Routing
    #     Locator (ALOC or RLOC):
    #     - ALOC: Mesh Local prefix with an IID of 0000:00FF:FE00:FC00
    #     - RLOC: Mesh Local prefix with and IID of 0000:00FF:FE00:xxxx where xxxx is a 16-bit value that embeds the
    #       Router ID
    print("Step 4: Topology B Commissioner (DUT) / Topology A Leader DUT")
    pkts.filter_coap_request(consts.MGMT_COMMISSIONER_SET_URI).\
        filter(lambda p: {
                          consts.NM_COMMISSIONER_SESSION_ID_TLV,
                          consts.NM_STEERING_DATA_TLV
                          } <= set(p.coap.tlv.type)).\
        must_next()

    # Step 5: Leader
    # - Description: Automatically sends MGMT_COMMISSIONER_SET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_COMMISSIONER_SET.rsp to the Commissioner with the
    #   following format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Accept (0x01))
    print("Step 5: Leader")
    pkts.filter(lambda p: p.coap.code == 68 and\
               p.coap.opt.uri_path_recon == consts.MGMT_COMMISSIONER_SET_URI and\
               p.coap.tlv.state == 1).\
        must_next()

    # Step 6: Leader
    # - Description: Automatically sends a multicast MLE Data Response.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send a multicast MLE Data Response with the new information,
    #   including a Network Data TLV including:
    #   - Commissioning Data TLV
    #     - Stable flag set to 0;
    #     - Commissioner Session ID TLV, Border Agent Locator TLV, Steering Data TLV
    print("Step 6: Leader")
    pkts.filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
                          consts.NETWORK_DATA_TLV
                          } <= set(p.mle.tlv.type) and\
               {
                          consts.NWD_COMMISSIONING_DATA_TLV
                          } <= set(p.thread_nwd.tlv.type)).\
        must_next()

    # Step 7: Topology A Leader DUT
    # - Description: Harness instructs Commissioner to send MGMT_COMMISSIONER_SET.req to DUT Anycast or Routing Locator:
    #   - CoAP Request URI: coap://[<L>]:MM/c/cs
    #   - CoAP Payload: Commissioner Session ID TLV, Border Agent Locator TLV (0x0400) (not allowed TLV)
    # - Pass Criteria: N/A.
    print("Step 7: Topology A Leader DUT")
    pkts.filter_coap_request(consts.MGMT_COMMISSIONER_SET_URI).\
        filter(lambda p: {
                          consts.NM_COMMISSIONER_SESSION_ID_TLV,
                          consts.NM_BORDER_AGENT_LOCATOR_TLV
                          } <= set(p.coap.tlv.type)).\
        must_next()

    # Step 8: Leader
    # - Description: Automatically sends MGMT_COMMISSIONER_SET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_COMMISSIONER_SET.rsp to the Commissioner with the
    #   following format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Reject (0xFF))
    print("Step 8: Leader")
    pkts.filter(lambda p: p.coap.code == 68 and\
               p.coap.opt.uri_path_recon == consts.MGMT_COMMISSIONER_SET_URI and\
               p.coap.tlv.state == 255).\
        must_next()

    # Step 9: Topology A Leader DUT
    # - Description: Harness instructs Commissioner to send MGMT_COMMISSIONER_SET.req to DUT Anycast or Routing Locator:
    #   - CoAP Request URI: coap://[<L>]:MM/c/cs
    #   - CoAP Payload: Commissioner Session ID TLV, Steering Data TLV (0xFF), Border Agent Locator TLV (0x0400)
    #     (not allowed TLV)
    # - Pass Criteria: N/A.
    print("Step 9: Topology A Leader DUT")
    pkts.filter_coap_request(consts.MGMT_COMMISSIONER_SET_URI).\
        filter(lambda p: {
                          consts.NM_COMMISSIONER_SESSION_ID_TLV,
                          consts.NM_STEERING_DATA_TLV,
                          consts.NM_BORDER_AGENT_LOCATOR_TLV
                          } <= set(p.coap.tlv.type)).\
        must_next()

    # Step 10: Leader
    # - Description: Automatically sends MGMT_COMMISSIONER_SET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_COMMISSIONER_SET.rsp to the Commissioner with the
    #   following format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Reject (0xFF))
    print("Step 10: Leader")
    pkts.filter(lambda p: p.coap.code == 68 and\
               p.coap.opt.uri_path_recon == consts.MGMT_COMMISSIONER_SET_URI and\
               p.coap.tlv.state == 255).\
        must_next()

    # Step 11: Topology A Leader DUT
    # - Description: Harness instructs Commissioner to send MGMT_COMMISSIONER_SET.req to DUT’s Anycast or Routing
    #   Locator:
    #   - CoAP Request URI: coap://[<L>]:MM/c/cs
    #   - CoAP Payload: Commissioner Session ID TLV (0xFFFF) (invalid value), Steering Data TLV (0xFF)
    # - Pass Criteria: N/A.
    print("Step 11: Topology A Leader DUT")
    pkts.filter_coap_request(consts.MGMT_COMMISSIONER_SET_URI).\
        filter(lambda p: {
                          consts.NM_COMMISSIONER_SESSION_ID_TLV,
                          consts.NM_STEERING_DATA_TLV
                          } <= set(p.coap.tlv.type) and\
               p.coap.tlv.comm_sess_id == 65535).\
        must_next()

    # Step 12: Leader
    # - Description: Automatically sends MGMT_COMMISSIONER_SET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_COMMISSIONER_SET.rsp to the Commissioner with the
    #   following format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Reject (0xFF))
    print("Step 12: Leader")
    pkts.filter(lambda p: p.coap.code == 68 and\
               p.coap.opt.uri_path_recon == consts.MGMT_COMMISSIONER_SET_URI and\
               p.coap.tlv.state == 255).\
        must_next()

    # Step 13: Topology A Leader DUT
    # - Description: Harness instructs Commissioner to send MGMT_COMMISSIONER_SET.req to DUT’s Anycast or Routing
    #   Locator:
    #   - CoAP Request URI: coap://[<L>]:MM/c/cs
    #   - CoAP Payload: Commissioner Session ID TLV, Steering Data TLV (0xFF), Channel TLV (not allowed TLV)
    # - Pass Criteria: N/A.
    print("Step 13: Topology A Leader DUT")
    pkts.filter_coap_request(consts.MGMT_COMMISSIONER_SET_URI).\
        filter(lambda p: {
                          consts.NM_COMMISSIONER_SESSION_ID_TLV,
                          consts.NM_STEERING_DATA_TLV,
                          consts.NM_CHANNEL_TLV
                          } <= set(p.coap.tlv.type)).\
        must_next()

    # Step 14: Leader
    # - Description: Automatically sends MGMT_COMMISSIONER_SET.rsp to the Commissioner.
    # - Pass Criteria: For DUT = Leader: The DUT MUST send MGMT_COMMISSIONER_SET.rsp to the Commissioner with the
    #   following format:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload: State TLV (value = Accept (0x01))
    print("Step 14: Leader")
    pkts.filter(lambda p: p.coap.code == 68 and\
               p.coap.opt.uri_path_recon == consts.MGMT_COMMISSIONER_SET_URI and\
               p.coap.tlv.state == 1).\
        must_next()

    # Step 15: All
    # - Description: Verify connectivity by sending an ICMPv6 Echo Request to the DUT mesh local address.
    # - Pass Criteria: The DUT MUST respond with an ICMPv6 Echo Reply.
    print("Step 15: All")
    pkts.filter_ping_request().\
        must_next()
    pkts.filter_ping_reply().\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
