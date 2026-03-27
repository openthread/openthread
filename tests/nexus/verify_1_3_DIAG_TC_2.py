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

# Diagnostic TLV Types
DG_EUI64_TLV = 23
DG_VERSION_TLV = 24
DG_VENDOR_NAME_TLV = 25
DG_VENDOR_MODEL_TLV = 26
DG_VENDOR_SW_VERSION_TLV = 27
DG_THREAD_STACK_VERSION_TLV = 28
DG_MLE_COUNTERS_TLV = 34

COAP_CODE_NOT_FOUND = 132


def read_uint16(b, offset):
    return (b[offset] << 8) | b[offset + 1]


def read_uint32(b, offset):
    return (b[offset] << 24) | (b[offset + 1] << 16) | (b[offset + 2] << 8) | b[offset + 3]


def read_uint64(b, offset):
    val = 0
    for i in range(8):
        val = (val << 8) | b[offset + i]
    return val


def get_diag_tlvs(p):
    """
    Parses Thread Diagnostic TLVs from CoAP payload.
    Returns a list of (type, value) tuples.
    """
    payload = p.coap.payload
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


def verify(pv):
    """
    4.2. [1.4] [CERT] Get Diagnostics – End Device

    4.2.2. Topology
    - Leader_1
    - Router_1
    - TD_1 (DUT)

    Spec Reference | Section
    ---------------|--------
    Thread 1.4     | 4.2
    """
    pkts = pv.pkts
    pv.summary.show()

    Leader_1_RLOC = pv.vars['Leader_1_RLOC']

    # Step 1
    # Device: Leader, Router_1
    # Description (DIAG-4.2): Enable the devices in order. The DUT is not yet activated. Leader
    #   configures its Network Data with an OMR prefix. Note: the OMR prefix can be added using
    #   an OT CLI command such as: "prefix add 2001:dead:beef:cafe::/64 paros med"
    # Pass Criteria:
    # - Single Thread Network is formed with Leader and Router_1.
    print("Step 1: Single Thread Network is formed with Leader and Router_1.")

    # Step 1b
    # Device: TD_1 (DUT)
    # Description (DIAG-4.2): Enable.
    # Pass Criteria:
    # - N/A
    print("Step 1b: TD_1 (DUT) Enable.")

    # Step 2
    # Device: Leader
    # Description (DIAG-4.2): Harness instructs device to send DIAG_GET.req to DUT's RLOC for the
    #   following Diagnostic TLV types: Type 19: Max Child Timeout TLV, Type 23: EUI-64 TLV, Type
    #   24: Version TLV, Type 25: Vendor Name TLV, Type 26: Vendor Model TLV, Type 27: Vendor SW
    #   Version TLV, Type 28: Thread Stack Version TLV
    # Pass Criteria:
    # - N/A
    print("Step 2: Leader sends DIAG_GET.req to DUT's RLOC.")

    # Step 3
    # Device: TD_1 (DUT)
    # Description (DIAG-4.2): Automatically responds either with 1. DIAG_GET.rsp containing the
    #   requested Diagnostic TLVs, or a particular subset of those TLVs; 2. CoAP response 4.04 Not
    #   Found in case no diagnostics are supported at all.
    # Pass Criteria:
    # - For case 1, the CoAP status code MUST be 2.04 Changed.
    # - Each TLV (as requested in step 2) that is present in the response MUST be validated. Any
    #   number of TLVs MAY be present, from 0 to 7, depending on DUT's diagnostics support level.
    # - The Max Child Timeout TLV (Type 19) is not expected to be present. However, if present it
    #   MUST be '0' (zero).
    # - Value of EUI-64 TLV (Type 23) MUST be non-zero.
    # - Value of Version TLV (Type 24) MUST be >= '5'.
    # - Value of Vendor Name TLV (Type 25) MUST have length >= 1.
    # - Value of Vendor Model TLV (Type 26) MUST have length >= 1.
    # - Value of Vendor SW Version TLV (Type 27) MUST have length >= 5.
    # - Value of Thread Stack Version TLV (Type 28) MUST have length >= 5.
    # - For case 2, the CoAP status code is 4.04 and TLVs MUST NOT be present.
    print("Step 3: TD_1 (DUT) responds either with 1. DIAG_GET.rsp or 2. CoAP response 4.04.")

    def check_step3(p):
        if p.coap.code == COAP_CODE_NOT_FOUND:
            return len(p.coap.payload) == 0

        if p.coap.code != consts.COAP_CODE_ACK:
            return False

        tlvs = get_diag_tlvs(p)
        types = {t for t, v in tlvs}

        # Case 1 validation
        # Each TLV that is present in the response MUST be validated.
        if consts.DG_MAX_CHILD_TIMEOUT_TLV in types:
            val = get_tlv_value(tlvs, consts.DG_MAX_CHILD_TIMEOUT_TLV)
            if read_uint32(val, 0) != 0:
                return False

        if DG_EUI64_TLV in types:
            val = get_tlv_value(tlvs, DG_EUI64_TLV)
            if read_uint64(val, 0) == 0:
                return False

        if DG_VERSION_TLV in types:
            val = get_tlv_value(tlvs, DG_VERSION_TLV)
            if read_uint16(val, 0) < 5:
                return False

        if DG_VENDOR_NAME_TLV in types:
            val = get_tlv_value(tlvs, DG_VENDOR_NAME_TLV)
            if len(val) < 1:
                return False

        if DG_VENDOR_MODEL_TLV in types:
            val = get_tlv_value(tlvs, DG_VENDOR_MODEL_TLV)
            if len(val) < 1:
                return False

        if DG_VENDOR_SW_VERSION_TLV in types:
            val = get_tlv_value(tlvs, DG_VENDOR_SW_VERSION_TLV)
            if len(val) < 5:
                return False

        if DG_THREAD_STACK_VERSION_TLV in types:
            val = get_tlv_value(tlvs, DG_THREAD_STACK_VERSION_TLV)
            if len(val) < 5:
                return False

        return True

    pkts.filter_ipv6_dst(Leader_1_RLOC).\
        filter(lambda p: (
            p.coap.code in (consts.COAP_CODE_ACK, COAP_CODE_NOT_FOUND) and
            p.coap.opt.uri_path_recon == consts.DIAG_GET_URI
        )).\
        filter(lambda p: (
            check_step3(p)
        )).\
        must_next()

    # Step 4
    # Device: N/A
    # Description (DIAG-4.2): Harness waits for 4 seconds. Note: this is done only to influence
    #   diagnostic Time fields that are verified in later steps.
    # Pass Criteria:
    # - N/A
    print("Step 4: Harness waits for 4 seconds.")

    # Step 4
    # Device: Leader
    # Description (DIAG-4.2): Harness instructs device to send DIAG_GET.req unicast to DUT's RLOC
    #   for the following Diagnostic TLV types: Type 34: MLE Counters TLV
    # Pass Criteria:
    # - N/A
    print("Step 4: Leader sends DIAG_GET.req unicast to DUT's RLOC for MLE Counters TLV.")

    # Step 5
    # Device: TD_1 (DUT)
    # Description (DIAG-4.2): Automatically responds with DIAG_GET.rsp containing the requested
    #   Diagnostic TLVs.
    # Pass Criteria:
    # - The DUT must send DIAG_GET.rsp
    # - The presence of each TLV (as requested in the previous step) MUST be validated.
    # - In the MLE Counters TLV (Type 34):
    #   - Radio Disabled Counter MUST be '0'
    #   - Detached Role Counter MUST be '1'
    #   - Child Role Counter MUST be '1'
    #   - Router Role Counter MUST be '0'
    #   - Leader Role Counter MUST be '0'
    #   - Attach Attempts Counter MUST be >= 1
    #   - Partition ID Changes Counter MUST be '1'
    #   - New Parent Counter MUST be '0'
    #   - Total Tracking Time MUST be > 4000
    #   - Child Role Time MUST be >= 1
    #   - Router Role Time MUST be '0'
    #   - Leader Role Time MUST be '0'
    print("Step 5: TD_1 (DUT) responds with DIAG_GET.rsp.")

    def check_step5(p):
        tlvs = get_diag_tlvs(p)
        types = {t for t, v in tlvs}

        if DG_MLE_COUNTERS_TLV not in types:
            return False

        val = get_tlv_value(tlvs, DG_MLE_COUNTERS_TLV)
        if len(val) < 66:
            return False

        # Radio Disabled Counter MUST be '0'
        if read_uint16(val, 0) != 0:
            return False
        # Detached Role Counter MUST be '1'
        if read_uint16(val, 2) != 1:
            return False
        # Child Role Counter MUST be '1'
        if read_uint16(val, 4) != 1:
            return False
        # Router Role Counter MUST be '0'
        if read_uint16(val, 6) != 0:
            return False
        # Leader Role Counter MUST be '0'
        if read_uint16(val, 8) != 0:
            return False
        # Attach Attempts Counter MUST be >= 1
        if read_uint16(val, 10) < 1:
            return False
        # Partition ID Changes Counter MUST be '1'
        if read_uint16(val, 12) != 1:
            return False
        # New Parent Counter MUST be '0'
        if read_uint16(val, 16) != 0:
            return False
        # Total Tracking Time MUST be > 4000
        if read_uint64(val, 18) <= 4000:
            return False
        # Child Role Time MUST be >= 1
        if read_uint64(val, 42) < 1:
            return False
        # Router Role Time MUST be '0'
        if read_uint64(val, 50) != 0:
            return False
        # Leader Role Time MUST be '0'
        if read_uint64(val, 58) != 0:
            return False

        return True

    pkts.filter_ipv6_dst(Leader_1_RLOC).\
        filter_coap_ack(consts.DIAG_GET_URI).\
        filter(lambda p: (
            check_step5(p)
        )).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
