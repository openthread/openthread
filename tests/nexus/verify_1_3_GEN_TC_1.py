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
import struct

# Add the current directory to sys.path to find verify_utils
CUR_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(CUR_DIR)

import verify_utils
from pktverify import consts, coap
from pktverify.bytes import Bytes

# Diagnostic TLV Types from include/openthread/netdiag.h
DG_TYPE_LIST_TLV = 18
DG_VERSION_TLV = 24

# CoAP Codes
COAP_CODE_CHANGED = 68
COAP_CODE_NOT_FOUND = 132


def get_diag_tlvs(p):
    """
    Parses Thread Diagnostic TLVs from CoAP payload.
    Returns a list of (type, value) tuples.
    """
    try:
        payload = p.coap.payload
    except AttributeError:
        return []
    tlvs = []
    offset = 0
    while offset < len(payload):
        t = payload[offset]
        l = payload[offset + 1]
        v = Bytes(payload[offset + 2:offset + 2 + l])
        tlvs.append((t, v))
        offset += 2 + l
    return tlvs


def get_tlv_value(tlvs, tlv_type):
    """
    Returns the value of the first TLV of a given type.
    """
    for t, v in tlvs:
        if t == tlv_type:
            return v
    return None


def get_diag_tlv_type_list(p):
    """
    Parses Thread Diagnostic Type List TLV from CoAP payload.
    Returns a list of TLV types.
    """
    try:
        payload = p.coap.payload
    except AttributeError:
        return []
    offset = 0
    while offset < len(payload):
        t = payload[offset]
        l = payload[offset + 1]
        if t == DG_TYPE_LIST_TLV:
            return list(payload[offset + 2:offset + 2 + l])
        offset += 2 + l
    return []


def verify(pv):
    # 9.1. [1.3] [CERT] Thread Version is '4' or higher
    #
    # 9.1.1. Purpose
    # Verify that the Thread Version TLV uses the value '4' or higher. '4' is required for Thread 1.3.x devices. '5'
    #   is required for Thread 1.4.x devices.
    #
    # 9.1.2. Topology
    # - BR_1: Thread Border Router DUT
    # - Router_1: Thread Router DUT
    # - ED_1: Thread End Device DUT, including FEDs, MEDs and SEDs
    #
    # Spec Reference   | V1.1 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # Thread Version   | 5.18.2.1     | 5.18.2.1

    pkts = pv.pkts
    pv.summary.show()

    BR_1 = pv.vars['BR_1']
    ROUTER_1 = pv.vars['Router_1']
    ED_1 = pv.vars['ED_1']

    BR_1_RLOC16 = pv.vars['BR_1_RLOC16']
    ROUTER_1_RLOC16 = pv.vars['Router_1_RLOC16']
    ED_1_RLOC16 = pv.vars['ED_1_RLOC16']

    # Step 1: BR_1 Enable
    # - Description (GEN-9.1): Enable
    # - Pass Criteria (only applies if Device == DUT):
    #   - For DUT = BR_1: The DUT MUST become Leader of the Thread Network.
    print("Step 1: BR_1 becomes Leader of the Thread Network.")
    pkts.filter_wpan_src64(BR_1).\
        filter_mle_cmd(consts.MLE_ADVERTISEMENT).\
        filter(lambda p: {
                          consts.LEADER_DATA_TLV,
                          consts.SOURCE_ADDRESS_TLV
                          } <= set(p.mle.tlv.type)).\
        must_next()

    # Step 2: Router_1 Enable
    # - Description (GEN-9.1): Enable
    # - Pass Criteria (only applies if Device == DUT):
    #   - N/A
    print("Step 2: Router_1 is enabled.")

    # Step 3: Router_1 Automatically sends MLE Parent Request multicast.
    # - Description (GEN-9.1): Automatically sends MLE Parent Request multicast.
    # - Pass Criteria (only applies if Device == DUT):
    #   - For DUT = Router: The DUT MUST sends MLE Parent Request multicast, with MLE Version TLV with value >= 4.
    print("Step 3: Router_1 sends MLE Parent Request multicast with MLE Version TLV >= 4.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_LLARMA().\
        filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        filter(lambda p: p.mle.tlv.version >= 4).\
        must_next()

    # Step 4: BR_1 Automatically sends MLE Parent Response unicast.
    # - Description (GEN-9.1): Automatically sends MLE Parent Response unicast.
    # - Pass Criteria (only applies if Device == DUT):
    #   - For DUT = BR_1: The DUT MUST sends MLE Parent Response unicast, with MLE Version TLV with value >= 4.
    print("Step 4: BR_1 sends MLE Parent Response unicast with MLE Version TLV >= 4.")
    pkts.filter_wpan_src64(BR_1).\
        filter_wpan_dst64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_PARENT_RESPONSE).\
        filter(lambda p: p.mle.tlv.version >= 4).\
        must_next()

    # Step 5: Router_1 Automatically completes attachment procedure and joins the network.
    # - Description (GEN-9.1): Automatically completes attachment procedure and joins the network.
    # - Pass Criteria (only applies if Device == DUT):
    #   - For DUT = Router: The DUT MUST send MLE Child ID Request message with MLE Version TLV with value >= 4.
    print("Step 5: Router_1 sends MLE Child ID Request with MLE Version TLV >= 4.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter(lambda p: p.mle.tlv.version >= 4).\
        must_next()

    # Step 6: ED_1 Automatically sends MLE Parent Request multicast.
    # - Description (GEN-9.1): Automatically sends MLE Parent Request multicast.
    # - Pass Criteria (only applies if Device == DUT):
    #   - For DUT = ED_1: The DUT MUST sends MLE Parent Request multicast with MLE Version TLV with value >= 4.
    print("Step 6: ED_1 sends MLE Parent Request multicast with MLE Version TLV >= 4.")
    pkts.filter_wpan_src64(ED_1).\
        filter_LLARMA().\
        filter_mle_cmd(consts.MLE_PARENT_REQUEST).\
        filter(lambda p: p.mle.tlv.version >= 4).\
        must_next()

    # Step 7: Router_1 Automatically sends MLE Parent Response unicast.
    # - Description (GEN-9.1): Automatically sends MLE Parent Response unicast.
    # - Pass Criteria (only applies if Device == DUT):
    #   - For DUT = Router_1: The DUT MUST sends MLE Parent Response unicast with MLE Version TLV with value >= 4.
    print("Step 7: Router_1 sends MLE Parent Response unicast with MLE Version TLV >= 4.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(ED_1).\
        filter_mle_cmd(consts.MLE_PARENT_RESPONSE).\
        filter(lambda p: p.mle.tlv.version >= 4).\
        must_next()

    # Step 8: ED_1 Automatically completes attachment procedure and joins the network.
    # - Description (GEN-9.1): Automatically completes attachment procedure and joins the network.
    # - Pass Criteria (only applies if Device == DUT):
    #   - For DUT = ED_1: The DUT MUST send MLE Child ID Request message with MLE Version TLV with value >= 4.
    print("Step 8: ED_1 sends MLE Child ID Request with MLE Version TLV >= 4.")
    pkts.filter_wpan_src64(ED_1).\
        filter_mle_cmd(consts.MLE_CHILD_ID_REQUEST).\
        filter(lambda p: p.mle.tlv.version >= 4).\
        must_next()

    # Step 9: Router_1 Harness instructs device to send MLE Discovery Request multicast.
    # - Description (GEN-9.1): Note: only performed if BR_1 is DUT. Harness instructs device to send MLE Discovery
    #   Request multicast.
    # - Pass Criteria (only applies if Device == DUT):
    #   - N/A
    print("Step 9: Router_1 sends MLE Discovery Request multicast.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_LLARMA().\
        filter_mle_cmd(consts.MLE_DISCOVERY_REQUEST).\
        must_next()

    # Step 10: BR_1 Automatically responds with MLE Discovery Response unicast.
    # - Description (GEN-9.1): Note: only performed if BR_1 is DUT. Automatically responds with MLE Discovery
    #   Response unicast.
    # - Pass Criteria (only applies if Device == DUT):
    #   - For DUT = BR_1: The DUT MUST send MLE Discovery Response with MLE Discovery Response TLV with a value >= 4
    #     of the 'Ver' field bits.
    print("Step 10: BR_1 sends MLE Discovery Response with Discovery Version >= 4.")
    pkts.filter_wpan_src64(BR_1).\
        filter_wpan_dst64(ROUTER_1).\
        filter_mle_cmd(consts.MLE_DISCOVERY_RESPONSE).\
        filter(lambda p: p.thread_meshcop.tlv.discovery_rsp_ver >= 4).\
        must_next()

    # Step 11: ED_1 Harness instructs device to send MLE Discovery Request multicast.
    # - Description (GEN-9.1): Note: only performed if Router_1 is DUT. Harness instructs device to send MLE Discovery
    #   Request multicast.
    # - Pass Criteria (only applies if Device == DUT):
    #   - N/A
    print("Step 11: ED_1 sends MLE Discovery Request multicast.")
    pkts.filter_wpan_src64(ED_1).\
        filter_LLARMA().\
        filter_mle_cmd(consts.MLE_DISCOVERY_REQUEST).\
        must_next()

    # Step 12: Router_1 Automatically responds with MLE Discovery Response unicast.
    # - Description (GEN-9.1): Note: only performed if Router_1 is DUT. Automatically responds with MLE Discovery
    #   Response unicast.
    # - Pass Criteria (only applies if Device == DUT):
    #   - For DUT = Router_1: The DUT MUST send MLE Discovery Response with MLE Discovery Response TLV with a value >= 4
    #     of the 'Ver' field bits.
    print("Step 12: Router_1 sends MLE Discovery Response with Discovery Version >= 4.")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_wpan_dst64(ED_1).\
        filter_mle_cmd(consts.MLE_DISCOVERY_RESPONSE).\
        filter(lambda p: p.thread_meshcop.tlv.discovery_rsp_ver >= 4).\
        must_next()

    # Steps 13-18 are for v1.4 devices
    # Check if we have diagnostic packets to decide if we should verify them
    if pkts.filter_coap_request('/d/dg'):
        # Step 13: Router_1 sends TMF Get Diagnostic Request (DIAG_GET.req) unicast to the DUT.
        # - Description (GEN-9.1): Note: only performed if BR_1 is DUT and >= v1.4 Device. Harness instructs device to
        #   send TMF Get Diagnostic Request (DIAG_GET.req) unicast to the DUT, requesting Diagnostic Version TLV
        #   (Type 24)
        # - Pass Criteria (only applies if Device == DUT):
        #   - N/A
        print("Step 13: Router_1 sends DIAG_GET.req to BR_1.")
        pkts.filter_wpan_src16(ROUTER_1_RLOC16).\
            filter_wpan_dst16(BR_1_RLOC16).\
            filter_coap_request('/d/dg').\
            filter(lambda p: DG_VERSION_TLV in get_diag_tlv_type_list(p)).\
            must_next()

        # Step 14: BR_1 Automatically responds with Get Diagnostic Response unicast to Router_1.
        # - Description (GEN-9.1): Note: only performed if BR_1 is DUT and >= v1.4 Device. Automatically responds with
        #   Get Diagnostic Response (DIAG_GET.rsp) unicast to Router_1.
        # - Pass Criteria (only applies if Device == DUT):
        #   - For DUT = BR_1 and >= v1.4 Device: The DUT MUST send DIAG_GET.RSP with Version TLV (Type 24)
        #     with value >= 5.
        print("Step 14: BR_1 sends DIAG_GET.rsp with Version TLV >= 5.")
        pkts.filter_wpan_src16(BR_1_RLOC16).\
            filter_wpan_dst16(ROUTER_1_RLOC16).\
            filter_coap_ack('/d/dg').\
            filter(lambda p: struct.unpack('>H', get_tlv_value(get_diag_tlvs(p), DG_VERSION_TLV))[0] >= 5).\
            must_next()

        # Step 15: BR_1 Harness instructs device to send TMF Get Diagnostic Request unicast to the DUT.
        # - Description (GEN-9.1): Note: only performed if Router_1 is DUT and >= v1.4 Device. Harness instructs device
        #   to send TMF Get Diagnostic Request (DIAG_GET.req) unicast to the DUT, requesting Diagnostic Version TLV
        #   (Type 24)
        # - Pass Criteria (only applies if Device == DUT):
        #   - N/A
        print("Step 15: BR_1 sends DIAG_GET.req to Router_1.")
        pkts.filter_wpan_src16(BR_1_RLOC16).\
            filter_wpan_dst16(ROUTER_1_RLOC16).\
            filter_coap_request('/d/dg').\
            filter(lambda p: DG_VERSION_TLV in get_diag_tlv_type_list(p)).\
            must_next()

        # Step 16: Router_1 Automatically responds with Get Diagnostic Response unicast to the DUT.
        # - Description (GEN-9.1): Note: only performed if Router_1 is DUT and >= v1.4 Device. Automatically responds
        #   with Get Diagnostic Response (DIAG_GET.rsp) unicast to the DUT.
        # - Pass Criteria (only applies if Device == DUT):
        #   - For DUT = Router_1 and >= v1.4 device: The DUT MUST send DIAG_GET.RSP with Version TLV (Type 24)
        #     with value >= 5.
        print("Step 16: Router_1 sends DIAG_GET.rsp with Version TLV >= 5.")
        pkts.filter_wpan_src16(ROUTER_1_RLOC16).\
            filter_wpan_dst16(BR_1_RLOC16).\
            filter_coap_ack('/d/dg').\
            filter(lambda p: struct.unpack('>H', get_tlv_value(get_diag_tlvs(p), DG_VERSION_TLV))[0] >= 5).\
            must_next()

        # Step 17: Router_1 Harness instructs device to send TMF Get Diagnostic Request unicast to the DUT.
        # - Description (GEN-9.1): Note: only performed if ED_1 is DUT and >= v1.4 Device. Harness instructs device to
        #   send TMF Get Diagnostic Request (DIAG_GET.req) unicast to the DUT, requesting Diagnostic Version TLV
        #   (Type 24)
        # - Pass Criteria (only applies if Device == DUT):
        #   - N/A
        print("Step 17: Router_1 sends DIAG_GET.req to ED_1.")
        pkts.filter_wpan_src16(ROUTER_1_RLOC16).\
            filter_wpan_dst16(ED_1_RLOC16).\
            filter_coap_request('/d/dg').\
            filter(lambda p: DG_VERSION_TLV in get_diag_tlv_type_list(p)).\
            must_next()

        # Step 18: ED_1 Optionally responds with Get Diagnostic Response unicast to Router_1.
        # - Description (GEN-9.1): Note: only performed if ED_1 is DUT and >= v1.4 Device. Optionally responds with Get
        #   Diagnostic Response (DIAG_GET.rsp) unicast to Router_1.
        # - Pass Criteria (only applies if Device == DUT):
        #   - For DUT = ED_1 and >= v1.4 device: The DUT MUST respond in either of below two ways:
        #   - 1. DIAG_GET.rsp Response is successfully received, and MUST contain the Version TLV (Type 24)
        #     with value >= 5.
        #   - 2. CoAP error response is received with code 4.04 Not Found.
        #   - Note: in case 2, the DUT does not support the optional diagnostics request.
        print("Step 18: ED_1 optionally responds with Get Diagnostic Response or 4.04 Not Found.")
        pkts.filter_wpan_src16(ED_1_RLOC16).\
            filter_wpan_dst16(ROUTER_1_RLOC16).\
            filter_coap_ack('/d/dg').\
            filter(lambda p: (p.coap.code == COAP_CODE_CHANGED and\
                              struct.unpack('>H', get_tlv_value(get_diag_tlvs(p), DG_VERSION_TLV))[0] >= 5) or\
                             (p.coap.code == COAP_CODE_NOT_FOUND)).\
            must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
