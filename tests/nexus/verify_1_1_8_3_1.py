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
    # 8.3.1 On Mesh Commissioner - Commissioner Petitioning, Commissioner Keep-alive messaging, Steering Data Updating
    #   and Commissioner Resigning
    #
    # 8.3.1.1 Topology
    # - Topology A: DUT is the Leader, communicating with a Commissioner.
    # - Topology B: DUT is the Commissioner, communicating with a Leader.
    #
    # 8.3.1.2 Purpose & Description
    # - Commissioner as a DUT: The purpose of this test case is to verify that a Commissioner Candidate is able to
    #   register itself to the network as a Commissioner, send periodic Commissioner keep-alive messages, update
    #   steering data and unregister itself as a Commissioner.
    # - Leader as a DUT: The purpose of this test case is to verify that the Leader accepts the Commissioner Candidate
    #   as a Commissioner in the network, responds to periodic Commissioner keep-alive messages, propagates Thread
    #   Network Data correctly in the network and unregisters the Commissioner on its request.
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # Commissioning    | 8.7 / 8.8    | 8.7 / 8.8

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    COMMISSIONER = pv.vars['COMMISSIONER']

    # Step 1: All
    # - Description: Begin wireless sniffer and ensure topology is created and connectivity between nodes.
    # - Pass Criteria: N/A.
    print("Step 1: All")
    pkts.filter_wpan_src64(LEADER).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        must_next()
    pkts.filter_wpan_src64(COMMISSIONER).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        must_next()

    # Step 2: Commissioner
    # - Description: Commissioner sends a petition request to Leader.
    # - Pass Criteria:
    #   Verify that:
    #   - Commissioner sends a TMF Leader Petition Request (LEAD_PET.req) to Leader.
    #     - CoAP Request URI: CON POST coap://<L>:MM/c/lp
    #     - CoAP Payload: Commissioner ID TLV
    #
    #   Fail conditions:
    #   - Commissioner does not send a TMF Leader Petition Request to Leader with the correct TLV.
    print("Step 2: Commissioner sends a petition request to Leader.")
    pkts.filter_coap_request(consts.LEAD_PET_URI).\
        filter(lambda p: {
                          consts.NM_COMMISSIONER_ID_TLV
                          } <= set(p.coap.tlv.type)).\
        must_next()
    _step2_index = pkts.index

    # Step 3: Leader
    # - Description: Leader accepts Commissioner to the network.
    # - Pass Criteria:
    #   Verify that:
    #   - Leader sends a TMF Leader Petition Response (LEAD_PET.rsp) to Commissioner.
    #     - CoAP Response Code: 2.04 Changed
    #     - CoAP Payload: State TLV (value = Accept), Commissioner ID TLV (optional), Commissioner Session ID TLV.
    #   - Leader sends a MLE Data Response to the network with the following TLVs: Active Timestamp TLV, Leader Data
    #     TLV, Network Data TLV, Source Address TLV.
    #     - Leader Data TLV shows increased Data Version number.
    #     - Network Data TLV contains Commissioning Data sub-TLV with Stable flag not set.
    #     - Commissioning Data TLV contains Commissioner Session ID and Border Agent Locator sub-TLVs.
    #
    #   Fail conditions:
    #   - Leader does not send a TMF Leader Petition response to Commissioner with correct TLVs.
    #   - Leader does not send a MLE Data Response message with correct TLVs.
    print("Step 3: Leader accepts Commissioner to the network.")
    _pet_rsp = pkts.filter_coap_ack(consts.LEAD_PET_URI).\
        filter(lambda p: {
                          consts.NM_STATE_TLV,
                          consts.NM_COMMISSIONER_SESSION_ID_TLV
                          } <= set(p.coap.tlv.type)).\
        filter(lambda p: p.coap.tlv.state == consts.MESHCOP_ACCEPT).\
        must_next()

    _session_id = _pet_rsp.coap.tlv.comm_sess_id

    pkts.range(_step2_index).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
                          consts.ACTIVE_TIMESTAMP_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.thread_nwd.tlv.type is not nullField).\
        filter(lambda p: {
                          consts.NM_COMMISSIONER_SESSION_ID_TLV,
                          consts.NM_BORDER_AGENT_LOCATOR_TLV,
                          } <= set(p.thread_meshcop.tlv.type)).\
        filter(lambda p: p.thread_meshcop.tlv.commissioner_sess_id == _session_id).\
        filter(lambda p: p.thread_nwd.tlv.stable[p.thread_nwd.tlv.type.index(consts.NWD_COMMISSIONING_DATA_TLV)] == 0).\
        must_next()

    # Step 4: Commissioner
    # - Description: Within 50 seconds Commissioner sends Commissioner keep-alive message.
    # - Pass Criteria:
    #   Verify that:
    #   - Commissioner sends a TMF Leader Petition Keep Alive Request (LEAD_KA.req) to Leader.
    #     - CoAP Request URI: CON POST coap://<L>:MM/c/la
    #     - CoAP Payload: State TLV (value = Accept), Commissioner Session ID TLV.
    #
    #   Fail conditions:
    #   - Commissioner does not send a TMF Leader Petition Keep Alive Request to Leader with the correct TLVs.
    print("Step 4: Commissioner sends Commissioner keep-alive message.")
    pkts.filter_coap_request(consts.LEAD_KA_URI).\
        filter(lambda p: {
                          consts.NM_STATE_TLV,
                          consts.NM_COMMISSIONER_SESSION_ID_TLV
                          } <= set(p.coap.tlv.type)).\
        filter(lambda p: p.coap.tlv.state == consts.MESHCOP_ACCEPT).\
        filter(lambda p: p.coap.tlv.comm_sess_id == _session_id).\
        must_next()

    # Step 5: Leader
    # - Description: Leader responds with ‘Accept’.
    # - Pass Criteria:
    #   Verify that:
    #   - Leader sends a TMF Leader Petition Keep Alive Response (LEAD_KA.rsp) to Commissioner.
    #     - CoAP Response Code: 2.04 Changed
    #     - CoAP Payload: State TLV (value = Accept).
    #
    #   Fail conditions:
    #   - Leader does not send a TMF Leader Petition Keep Alive Response to Commissioner with correct TLV.
    print("Step 5: Leader responds with 'Accept'.")
    pkts.filter_coap_ack(consts.LEAD_KA_URI).\
        filter(lambda p: {
                          consts.NM_STATE_TLV
                          } <= set(p.coap.tlv.type)).\
        filter(lambda p: p.coap.tlv.state == consts.MESHCOP_ACCEPT).\
        must_next()

    # Step 6: Commissioner
    # - Description: Commissioner adds a new device to be commissioned. If Commissioner is a test bed device, it uses
    #   Leader Anycast address as destination address.
    # - Pass Criteria:
    #   Verify that:
    #   - Commissioner sends a TMF Set Commissioner Dataset Request (MGMT_COMMISSIONER_SET.req) to Leader ALOC 0xFC00
    #     or Leader RLOC.
    #     - CoAP Request URI: CON POST coap://<L>:MM/c/cs
    #     - CoAP Payload: Commissioner Session ID TLV, Steering Data TLV.
    #
    #   Fail conditions:
    #   - Commissioner does not send a TMF Set Commissioner Dataset Request to Leader with the correct TLVs.
    print("Step 6: Commissioner adds a new device to be commissioned.")
    _set_req = pkts.filter_coap_request(consts.MGMT_COMMISSIONER_SET_URI).\
        filter(lambda p: {
                          consts.NM_COMMISSIONER_SESSION_ID_TLV,
                          consts.NM_STEERING_DATA_TLV
                          } <= set(p.coap.tlv.type)).\
        filter(lambda p: p.coap.tlv.comm_sess_id == _session_id).\
        must_next()
    _step6_index = pkts.index

    # Step 7: Leader
    # - Description: Leader Responds with ‘Accept’.
    # - Pass Criteria:
    #   Verify that:
    #   - Leader sends a TMF Set Commissioner Dataset Response (MGMT_COMMISSIONER_SET.rsp) to the source address of
    #     the previous TMF Set Commissioner Dataset Request:
    #     - CoAP Response Code: 2.04 Changed
    #     - CoAP Payload: State TLV (value = Accept).
    #   - Leader sends a MLE Data Response to the network with the following TLVs: Active Timestamp TLV, Leader Data
    #     TLV, Network Data TLV, Source Address TLV.
    #     - Leader Data TLV shows increased Data Version number.
    #     - Network Data TLV contains Commissioning Data sub-TLV with Stable flag not set.
    #     - Commissioning Data TLV contains Commissioner Session ID, Border Agent Locator and Steering Data sub-TLVs.
    #
    #   Fail conditions:
    #   - Leader does not send a TMF Set Commissioner Dataset Response to Commissioner with correct TLV.
    #   - Leader does not send a MLE Data Response message with correct TLVs.
    print("Step 7: Leader Responds with 'Accept'.")
    pkts.filter_coap_ack(consts.MGMT_COMMISSIONER_SET_URI).\
        filter(lambda p: {
                          consts.NM_STATE_TLV
                          } <= set(p.coap.tlv.type)).\
        filter(lambda p: p.coap.tlv.state == consts.MESHCOP_ACCEPT).\
        must_next()

    pkts.range(_step6_index).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
                          consts.ACTIVE_TIMESTAMP_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.thread_nwd.tlv.type is not nullField).\
        filter(lambda p: {
                          consts.NM_COMMISSIONER_SESSION_ID_TLV,
                          consts.NM_BORDER_AGENT_LOCATOR_TLV,
                          consts.NM_STEERING_DATA_TLV
                          } <= set(p.thread_meshcop.tlv.type)).\
        filter(lambda p: p.thread_meshcop.tlv.commissioner_sess_id == _session_id).\
        filter(lambda p: p.thread_nwd.tlv.stable[p.thread_nwd.tlv.type.index(consts.NWD_COMMISSIONING_DATA_TLV)] == 0).\
        must_next()

    # Step 8: Commissioner
    # - Description: Commissioner sends a resign request to Leader.
    # - Pass Criteria:
    #   Verify that:
    #   - Commissioner sends a TMF Leader Petition Keep Alive Request (LEAD_KA.req) to Leader.
    #     - CoAP Request URI: CON POST coap://<L>:MM/c/la
    #     - CoAP Payload: State TLV (value = Reject), Commissioner Session ID TLV.
    #
    #   Fail conditions:
    #   - Commissioner does not send a TMF Leader Petition Keep Alive Request to Leader with the correct TLV.
    print("Step 8: Commissioner sends a resign request to Leader.")
    # Search from Step 6 to allow reordering with Step 7 Data Response
    _resign_req = pkts.range(_step6_index).\
        filter_coap_request(consts.LEAD_KA_URI).\
        filter(lambda p: {
                          consts.NM_STATE_TLV,
                          consts.NM_COMMISSIONER_SESSION_ID_TLV
                          } <= set(p.coap.tlv.type)).\
        filter(lambda p: p.coap.tlv.state in (consts.MESHCOP_REJECT, 255)).\
        filter(lambda p: p.coap.tlv.comm_sess_id == _session_id).\
        must_next()
    _resign_index = pkts.index

    # Step 9: Leader
    # - Description: Leader accepts the resignation by responding with ‘Reject’.
    # - Pass Criteria:
    #   Verify that:
    #   - Leader sends a TMF Leader Petition Keep Alive Response (LEAD_KA.rsp) to Commissioner.
    #     - CoAP Response Code: 2.04 Changed
    #     - CoAP Payload: State TLV (value = Reject).
    #   - Leader sends a MLE Data Response to the network with the following TLVs: Active Timestamp TLV, Leader Data
    #     TLV, Network Data TLV, Source Address TLV.
    #     - Leader Data TLV shows increased Data Version number.
    #     - Network Data TLV contains Commissioning Data sub-TLV with Stable flag not set and incremented value for
    #       Commissioner Session ID.
    #
    #   Fail conditions:
    #   - Leader does not send a TMF Leader Petition Keep Alive Response to Commissioner with correct TLV.
    #   - Leader does not send a MLE Data Response message with correct TLVs.
    print("Step 9: Leader accepts the resignation.")
    pkts.filter_coap_ack(consts.LEAD_KA_URI).\
        filter(lambda p: {
                          consts.NM_STATE_TLV
                          } <= set(p.coap.tlv.type)).\
        filter(lambda p: p.coap.tlv.state in (consts.MESHCOP_REJECT, 255)).\
        must_next()

    pkts.range(_resign_index).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
                          consts.ACTIVE_TIMESTAMP_TLV,
                          consts.LEADER_DATA_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.thread_nwd.tlv.type is not nullField).\
        filter(lambda p: {
                          consts.NM_COMMISSIONER_SESSION_ID_TLV
                          } <= set(p.thread_meshcop.tlv.type)).\
        filter(lambda p: p.thread_meshcop.tlv.commissioner_sess_id == _session_id + 1).\
        filter(lambda p: p.thread_nwd.tlv.stable[p.thread_nwd.tlv.type.index(consts.NWD_COMMISSIONING_DATA_TLV)] == 0).\
        must_next()

    # Step 10: Commissioner
    # - Description: Commissioner sends a petition request to Leader.
    # - Pass Criteria: Should there be something listed here for the Commissioner DUT?
    print("Step 10: Commissioner sends a petition request to Leader.")
    pkts.filter_coap_request(consts.LEAD_PET_URI).\
        filter(lambda p: {
                          consts.NM_COMMISSIONER_ID_TLV
                          } <= set(p.coap.tlv.type)).\
        must_next()

    # Step 11: Leader
    # - Description: Leader accepts Commissioner to the network.
    # - Pass Criteria:
    #   Verify that:
    #   - Leader sends a TMF Leader Petition Response (LEAD_PET.rsp) to Commissioner.
    #     - CoAP Response Code: 2.04 Changed
    #     - CoAP Payload: State TLV (value = Accept), Commissioner ID TLV (optional), Commissioner Session ID TLV.
    #     - Commissioner Session ID TLV contains higher Session ID number than in Step 9.
    print("Step 11: Leader accepts Commissioner to the network.")
    pkts.filter_coap_ack(consts.LEAD_PET_URI).\
        filter(lambda p: {
                          consts.NM_STATE_TLV,
                          consts.NM_COMMISSIONER_SESSION_ID_TLV
                          } <= set(p.coap.tlv.type)).\
        filter(lambda p: p.coap.tlv.state == consts.MESHCOP_ACCEPT).\
        filter(lambda p: p.coap.tlv.comm_sess_id > _session_id + 1).\
        must_next()


if __name__ == '__main__':
    # Add NM_COMMISSIONER_ID_TLV to parser
    def patch_more():
        old_parse = verify_utils.CoapTlvParser.parse

        def new_parse(t, v, layer=None):
            kvs = old_parse(t, v, layer)
            if t == consts.NM_COMMISSIONER_ID_TLV:
                kvs.append(('commissioner_id', v.decode('utf-8', errors='replace')))
            return kvs

        verify_utils.CoapTlvParser.parse = staticmethod(new_parse)

        from pktverify import layer_fields
        layer_fields._LAYER_FIELDS['coap.tlv.commissioner_id'] = layer_fields._str

    patch_more()
    verify_utils.run_main(verify)
