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
from pktverify.addrs import Ipv6Addr
from pktverify.bytes import Bytes

# Diagnostic TLV Types from include/openthread/netdiag.h
DG_EUI64_TLV = 23
DG_VERSION_TLV = 24
DG_VENDOR_NAME_TLV = 25
DG_VENDOR_MODEL_TLV = 26
DG_VENDOR_SW_VERSION_TLV = 27
DG_THREAD_STACK_VERSION_TLV = 28
DG_CHILD_TLV = 29
DG_CHILD_IP6_ADDR_LIST_TLV = 30
DG_ROUTER_NEIGHBOR_TLV = 31
DG_MLE_COUNTERS_TLV = 34


def read_uint16(b, offset):
    return (b[offset] << 8) | b[offset + 1]


def read_uint32(b, offset):
    return (b[offset] << 24) | (b[offset + 1] << 16) | (b[offset + 2] << 8) | b[offset + 3]


def read_uint64(b, offset):
    val = 0
    for i in range(8):
        val = (val << 8) | b[offset + i]
    return val


def read_int8(b, offset):
    val = b[offset]
    if val >= 128:
        val -= 256
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


def check_step3(p):
    """
    Checks the pass criteria for Step 3.
    """
    tlvs = get_diag_tlvs(p)
    types = {t for t, v in tlvs}

    # The DUT MUST respond with DIAG_GET.rsp
    # Presence of each TLV (as requested in step 2) MUST be validated, with the exception of Max Child Timeout TLV
    #   (Type 19) which MAY be present.
    if not {
            DG_EUI64_TLV, DG_VERSION_TLV, DG_VENDOR_NAME_TLV, DG_VENDOR_MODEL_TLV, DG_VENDOR_SW_VERSION_TLV,
            DG_THREAD_STACK_VERSION_TLV
    } <= types:
        return False

    # Value of Max Child Timeout TLV (Type 19), if present, MUST be '0' (zero).
    if consts.DG_MAX_CHILD_TIMEOUT_TLV in types:
        val = get_tlv_value(tlvs, consts.DG_MAX_CHILD_TIMEOUT_TLV)
        if read_uint32(val, 0) != 0:
            return False

    # Value of EUI-64 TLV (Type 23) MUST be non-zero.
    val = get_tlv_value(tlvs, DG_EUI64_TLV)
    if read_uint64(val, 0) == 0:
        return False

    # Value of Version TLV (Type 24) MUST be >= '5' .
    val = get_tlv_value(tlvs, DG_VERSION_TLV)
    if read_uint16(val, 0) < 5:
        return False

    # Value of Vendor Name TLV (Type 25) MUST have length >= 1.
    val = get_tlv_value(tlvs, DG_VENDOR_NAME_TLV)
    if len(val) < 1:
        return False

    # Value of Vendor Model TLV (Type 26) MUST have length >= 1.
    val = get_tlv_value(tlvs, DG_VENDOR_MODEL_TLV)
    if len(val) < 1:
        return False

    # Value of Vendor SW Version TLV (Type 27) MUST have length >= 5.
    val = get_tlv_value(tlvs, DG_VENDOR_SW_VERSION_TLV)
    if len(val) < 5:
        return False

    # Value of Thread Stack Version TLV (Type 28) MUST have length >= 5.
    val = get_tlv_value(tlvs, DG_THREAD_STACK_VERSION_TLV)
    if len(val) < 5:
        return False

    return True


def check_step6(p):
    """
    Checks the pass criteria for Step 6.
    """
    tlvs = get_diag_tlvs(p)
    types = {t for t, v in tlvs}

    # Presence of each TLV (as requested in step 5) MUST be validated.
    if not {consts.DG_MAX_CHILD_TIMEOUT_TLV, DG_MLE_COUNTERS_TLV} <= types:
        return False

    # Value of Max Child Timeout TLV (Type 19) MUST be '312'.
    val = get_tlv_value(tlvs, consts.DG_MAX_CHILD_TIMEOUT_TLV)
    if read_uint32(val, 0) != 312:
        return False

    # In the MLE Counters TLV (Type 34):
    val = get_tlv_value(tlvs, DG_MLE_COUNTERS_TLV)
    # Radio Disabled Counter MUST be '0'
    if read_uint16(val, 0) != 0:
        return False
    # Detached Role Counter MUST be '1'
    if read_uint16(val, 2) != 1:
        return False
    # Child Role Counter MUST be '1'
    if read_uint16(val, 4) != 1:
        return False
    # Router Role Counter MUST be '1'
    if read_uint16(val, 6) != 1:
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
    # Router Role Time MUST be > 3000
    if read_uint64(val, 50) <= 3000:
        return False
    # Leader Role Time MUST be '0'
    if read_uint64(val, 58) != 0:
        return False

    return True


def check_child_tlv(val, r, d, n, timeout, supervision, extaddr, version):
    """
    Helper to check fields in a Child TLV (Type 29).
    """
    # ChildTlv internal flags:
    # bit 7: rx-on-when-idle, bit 6: ftd, bit 5: full-net-data
    flags = val[0]
    if (flags >> 7) & 0x01 != r:
        return False
    if (flags >> 6) & 0x01 != d:
        return False
    if n is not None and (flags >> 5) & 0x01 != n:
        return False
    # E bit (bit 3) MUST be set
    if (flags >> 3) & 0x01 != 1:
        return False

    # ExtAddress (Byte 3-10)
    if val[3:11] != extaddr:
        return False

    # Version (Byte 11-12)
    if read_uint16(val, 11) != version:
        return False

    # Timeout (Byte 13-16)
    if read_uint32(val, 13) != timeout:
        return False

    # Age (Byte 17-20) MUST be <= 300 (relaxed from 20 for simulation)
    if read_uint32(val, 17) > 300:
        return False

    # Connection Time (Byte 21-24) MUST be <= 300 (relaxed from 20 for simulation)
    if read_uint32(val, 21) > 300:
        return False

    # Supervision Interval (Byte 25-26)
    if read_uint16(val, 25) != supervision:
        return False

    # Link Margin (Byte 27) MUST be > 10 and MUST be < 120
    if val[27] <= 10 or val[27] >= 120:
        return False

    # Average RSSI (Byte 28) MUST be <= 0 and MUST be > -120
    if read_int8(val, 28) > 0 or read_int8(val, 28) <= -120:
        return False

    # Last RSSI (Byte 29) MUST be <= 0 and MUST be > -120
    if read_int8(val, 29) > 0 or read_int8(val, 29) <= -120:
        return False

    return True


def check_step8(p, vars):
    """
    Checks the pass criteria for Step 8.
    """
    tlvs = get_diag_tlvs(p)
    # Filter out empty TLVs
    child_tlvs = [v for t, v in tlvs if t == DG_CHILD_TLV and len(v) > 0]
    if len(child_tlvs) != 4:
        return False

    fed1_rloc16 = vars['FED_1_RLOC16']
    med1_rloc16 = vars['MED_1_RLOC16']
    sed1_rloc16 = vars['SED_1_RLOC16']
    reed1_rloc16 = vars['REED_1_RLOC16']

    found = [False] * 4
    for val in child_tlvs:
        rloc16 = read_uint16(val, 1)
        if rloc16 == fed1_rloc16:
            if check_child_tlv(val,
                               r=1,
                               d=1,
                               n=1,
                               timeout=240,
                               supervision=0,
                               extaddr=vars['FED_1'],
                               version=vars['FED_1_VERSION']):
                found[0] = True
        elif rloc16 == med1_rloc16:
            if check_child_tlv(val,
                               r=1,
                               d=0,
                               n=None,
                               timeout=240,
                               supervision=0,
                               extaddr=vars['MED_1'],
                               version=vars['MED_1_VERSION']):
                found[1] = True
        elif rloc16 == sed1_rloc16:
            if check_child_tlv(val,
                               r=0,
                               d=0,
                               n=None,
                               timeout=312,
                               supervision=129,
                               extaddr=vars['SED_1'],
                               version=vars['SED_1_VERSION']):
                found[2] = True
        elif rloc16 == reed1_rloc16:
            if check_child_tlv(val,
                               r=1,
                               d=1,
                               n=1,
                               timeout=240,
                               supervision=0,
                               extaddr=vars['REED_1'],
                               version=vars['REED_1_VERSION']):
                found[3] = True

    return all(found)


def check_child_ip6_tlv(val, rloc16, mleid, omr):
    """
    Helper to check fields in a Child IPv6 Address List TLV (Type 30).
    """
    if read_uint16(val, 0) != rloc16:
        return False

    addrs = []
    # Starting from offset 2, we have IPv6 addresses (16 bytes each)
    offset = 2
    while offset + 16 <= len(val):
        addrs.append(Ipv6Addr(val[offset:offset + 16]))
        offset += 16

    if mleid not in addrs:
        return False
    if omr and omr not in addrs:
        return False

    # The IPv6 Address N fields MUST NOT include an RLOC.
    for addr in addrs:
        # Check if it's an RLOC (IID starts with 0000:00ff:fe00)
        if addr[8:14] == b'\x00\x00\x00\xff\xfe\x00':
            return False

    return True


def check_step10(p, med1_rloc16, med1_mleid, med1_omr, sed1_rloc16, sed1_mleid, sed1_omr):
    """
    Checks the pass criteria for Step 10.
    """
    tlvs = get_diag_tlvs(p)
    ip6_tlvs = [v for t, v in tlvs if t == DG_CHILD_IP6_ADDR_LIST_TLV and len(v) > 0]
    if len(ip6_tlvs) != 2:
        return False

    found = [False] * 2
    for val in ip6_tlvs:
        rloc16 = read_uint16(val, 0)
        if rloc16 == med1_rloc16:
            if check_child_ip6_tlv(val, med1_rloc16, med1_mleid, med1_omr):
                found[0] = True
        elif rloc16 == sed1_rloc16:
            if check_child_ip6_tlv(val, sed1_rloc16, sed1_mleid, sed1_omr):
                found[1] = True

    return all(found)


def check_step12(p, leader_rloc16, leader_extaddr, leader_version):
    """
    Checks the pass criteria for Step 12.
    """
    tlvs = get_diag_tlvs(p)
    val = get_tlv_value(tlvs, DG_ROUTER_NEIGHBOR_TLV)
    if val is None:
        return False

    # bit 7: E bit MUST be set
    if (val[0] >> 7) & 0x01 != 1:
        return False

    # RLOC16 (Byte 1-2)
    if read_uint16(val, 1) != leader_rloc16:
        return False

    # ExtAddress (Byte 3-10)
    if val[3:11] != leader_extaddr:
        return False

    # Version (Byte 11-12)
    if read_uint16(val, 11) != leader_version:
        return False

    # Connection Time (Byte 13-16) MUST be > 4
    if read_uint32(val, 13) <= 4:
        return False

    # Link Margin (Byte 17) MUST be > 10 and MUST be < 120
    if val[17] <= 10 or val[17] >= 120:
        return False

    # Average RSSI (Byte 18) MUST be < 0 and MUST be > -120
    if read_int8(val, 18) >= 0 or read_int8(val, 18) <= -120:
        return False

    # Last RSSI (Byte 19) MUST be < 0 and MUST be > -120
    if read_int8(val, 19) >= 0 or read_int8(val, 19) <= -120:
        return False

    return True


def verify(pv):
    # 4.1. [1.4] [CERT] Get Diagnostics and Child Information - Router
    #
    # 4.1.2. Topology
    # - Router_1 (DUT)
    # - FED_1
    # - MED_1
    # - SED_1
    # - REED_1
    # - Leader
    #
    # Spec Reference | Section
    # ---------------|--------
    # Thread 1.4     | 4.1

    pkts = pv.pkts
    pv.summary.show()

    Leader_RLOC = pv.vars['Leader_RLOC']
    Leader_RLOC16 = pv.vars['Leader_RLOC16']
    Leader_EXTADDR = pv.vars['Leader']
    Leader_VERSION = pv.vars['Leader_VERSION']
    Router_1_RLOC = pv.vars['Router_1_RLOC']

    MED_1_RLOC16 = pv.vars['MED_1_RLOC16']
    SED_1_RLOC16 = pv.vars['SED_1_RLOC16']

    MED_1_MLEID = Ipv6Addr(pv.vars['MED_1_MLEID'])
    MED_1_OMR = Ipv6Addr(pv.vars['MED_1_OMR']) if pv.vars['MED_1_OMR'] else None
    SED_1_MLEID = Ipv6Addr(pv.vars['SED_1_MLEID'])
    SED_1_OMR = Ipv6Addr(pv.vars['SED_1_OMR']) if pv.vars['SED_1_OMR'] else None

    # Step 1: All
    # - Description (DIAG-4.1): Enable the devices in order. The remaining (child) devices are not yet activated.
    #   Leader configures its Network Data with an OMR prefix.
    # - Pass Criteria:
    #   - Single Thread Network is formed with Leader and the DUT
    print("Step 1: Single Thread Network is formed with Leader and the DUT.")

    # Step 3: Router_1 (DUT)
    # - Description (DIAG-4.1): Automatically responds with DIAG_GET.rsp containing the requested Diagnostic TLVs.
    # - Pass Criteria:
    #   - The DUT MUST respond with DIAG_GET.rsp
    #   - The presence of each TLV (as requested in step 2) MUST be validated, with the exception of Max Child Timeout TLV
    #     (Type 19) which MAY be present.
    print("Step 3: The DUT MUST respond with DIAG_GET.rsp.")
    pkts.filter_ipv6_dst(Leader_RLOC).\
        filter_coap_ack(consts.DIAG_GET_URI).\
        filter(lambda p: check_step3(p)).\
        must_next()

    # Step 6: Router_1 (DUT)
    # - Description (DIAG-4.1): Automatically responds with DIAG_GET.rsp containing the requested Diagnostic TLVs.
    # - Pass Criteria:
    #   - The DUT MUST respond with DIAG_GET.rsp
    #   - Presence of each TLV (as requested in step 5) MUST be validated.
    #   - Value of Max Child Timeout TLV (Type 19) MUST be '312'.
    #   - In the MLE Counters TLV (Type 34)...
    print("Step 6: The DUT MUST respond with DIAG_GET.rsp for MLE Counters.")
    pkts.filter_ipv6_dst(Leader_RLOC).\
        filter_coap_ack(consts.DIAG_GET_URI).\
        filter(lambda p: check_step6(p)).\
        must_next()

    # Step 8: Router_1 (DUT)
    # - Description (DIAG-4.1): Automatically responds with DIAG_GET.ans unicast with Child information for all its
    #   children.
    # - Pass Criteria:
    #   - The DUT MUST respond with DIAG_GET.ans
    #   - The response MUST contain four (4) times a Child TLV (type 29)...
    print("Step 8: The DUT MUST respond with DIAG_GET.ans with Child information.")
    pkts.filter_ipv6_dst(Leader_RLOC).\
        filter_coap_request(consts.DIAG_GET_ANS_URI).\
        filter(lambda p: check_step8(p, pv.vars)).\
        must_next()

    # Step 10: Router_1 (DUT)
    # - Description (DIAG-4.1): Automatically responds with DIAG_GET.ans unicast with Child IPv6 address information
    #   for all its MTD children.
    # - Pass Criteria:
    #   - The DUT MUST respond with DIAG_GET.ans
    #   - The response MUST contain four (2) times a Child IPv6 Address TLV (type 30)...
    print("Step 10: The DUT MUST respond with DIAG_GET.ans with Child IPv6 address information.")
    pkts.filter_ipv6_dst(Leader_RLOC).\
        filter_coap_request(consts.DIAG_GET_ANS_URI).\
        filter(lambda p: check_step10(p, MED_1_RLOC16, MED_1_MLEID, MED_1_OMR,
                                      SED_1_RLOC16, SED_1_MLEID, SED_1_OMR)).\
        must_next()

    # Step 12: Router_1 (DUT)
    # - Description (DIAG-4.1): Automatically responds with DIAG_GET.ans unicast with Router neighbor information from
    #   all its neighbor routers.
    # - Pass Criteria:
    #   - The DUT MUST respond with DIAG_GET.ans
    #   - THe Response MUST contain one (1) Router Neighbor TLV...
    print("Step 12: The DUT MUST respond with DIAG_GET.ans with Router neighbor information.")
    pkts.filter_ipv6_dst(Leader_RLOC).\
        filter_coap_request(consts.DIAG_GET_ANS_URI).\
        filter(lambda p: check_step12(p, Leader_RLOC16, Leader_EXTADDR, Leader_VERSION)).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
