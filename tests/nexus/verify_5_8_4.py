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

# Define missing constants
NM_GET_TLV = 13
COAP_CODE_POST = 2
COAP_CODE_CHANGED = 68


def parse_tlvs(payload):
    tlvs = {}
    i = 0
    if payload == nullField:
        return tlvs
    while i < len(payload):
        if i + 2 > len(payload):
            break
        t = payload[i]
        l = payload[i + 1]
        if i + 2 + l > len(payload):
            break
        v = payload[i + 2:i + 2 + l]
        tlvs[t] = v
        i += 2 + l
    return tlvs


def verify_meshcop_tlvs(pkt, checks):
    """
    Parses CoAP payload and checks TLV values.
    checks: dict { type: lambda value: bool }
    """
    try:
        if not hasattr(pkt, 'coap') or pkt.coap.payload == nullField:
            return False

        payload = bytearray(pkt.coap.payload)
        tlvs = parse_tlvs(payload)

        for t, check in checks.items():
            if t not in tlvs:
                return False
            if not check(tlvs[t]):
                return False
        return True
    except Exception:
        return False


def verify(pv):
    # 5.8.4 Security Policy TLV
    #
    # 5.8.4.1 Topology
    # - Commissioner_1 is an On-mesh Commissioner.
    # - Commissioner_2 is not part of the original topology - it is introduced at step 11.
    # - Partition is formed with all Security Policy TLV bits set to 1.
    #
    # 5.8.4.2 Purpose & Description
    # The purpose of this test case is to verify network behavior when Security Policy TLV
    #   “O”, ”N”, ”R”, ”B” bits are disabled. “C” bit is not tested as it requires an
    #   External Commissioner which is currently not part of Thread Certification.
    #
    # Spec Reference           | V1.1 Section | V1.3.0 Section
    # -------------------------|--------------|---------------
    # Security Policy TLV (12) | 8.10.1.15    | 8.10.1.15

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    COMMISSIONER_1 = pv.vars['COMMISSIONER_1']
    COMMISSIONER_2 = pv.vars['COMMISSIONER_2']

    # Step 1: All
    # - Description: Build Topology. Ensure topology is formed correctly.
    # - Pass Criteria: N/A
    print("Step 1: Topology built")

    # Step 2: Commissioner_1
    # - Description: Harness instructs the device to send MGMT_ACTIVE_GET.req to the DUT.
    #   - CoAP Request URI: coap://[<L>]:MM/c/ag
    #   - CoAP Payload: <empty>
    # - Pass Criteria: N/A
    print("Step 2: Commissioner_1 sends MGMT_ACTIVE_GET.req")
    pkts.filter_wpan_src64(COMMISSIONER_1).\
        filter(lambda p: hasattr(p, 'coap') and\
                         p.coap.opt.uri_path_recon == consts.MGMT_ACTIVE_GET_URI and\
                         p.coap.code == COAP_CODE_POST).\
        must_next()

    # Step 3: Leader (DUT)
    # - Description: Automatically sends MGMT_ACTIVE_GET.rsp to Commissioner_1.
    # - Pass Criteria:
    #   - The DUT MUST send MGMT_ACTIVE_GET.rsp to Commissioner_1:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload:
    #     - Security Policy TLV Bits “O”, ”N”, ”R”, ”C” should be set to 1.
    print("Step 3: Leader responds with MGMT_ACTIVE_GET.rsp")
    pkts.filter_wpan_src64(LEADER).\
        filter(lambda p: hasattr(p, 'coap') and\
                         p.coap.code == COAP_CODE_CHANGED).\
        filter(lambda p: verify_meshcop_tlvs(p, {
            consts.NM_SECURITY_POLICY_TLV: lambda v: (v[2] & 0xF0) == 0xF0 # O, N, R, C bits are 1
        })).\
        must_next()

    # Step 4 & 5: Commissioner_1
    # - Description: Harness instructs the device to send MGMT_ACTIVE_SET.req to the DUT
    #   (disable “O” bit).
    #   - CoAP Request URI: coap://[<L>]:MM/c/as
    #   - CoAP Payload:
    #     - Commissioner Session ID TLV
    #     - Active Timestamp TLV = 15 (> step 3)
    #     - Security Policy TLV with “O” bit disabled.
    # - Pass Criteria: N/A
    print("Step 4 & 5: Commissioner_1 sends MGMT_ACTIVE_SET.req (Disable O)")
    pkts.filter_wpan_src64(COMMISSIONER_1).\
        filter(lambda p: hasattr(p, 'coap') and\
                         p.coap.opt.uri_path_recon == consts.MGMT_ACTIVE_SET_URI and\
                         p.coap.code == COAP_CODE_POST).\
        filter(lambda p: verify_meshcop_tlvs(p, {
            consts.NM_ACTIVE_TIMESTAMP_TLV: lambda v: v[5] == 15,
            consts.NM_SECURITY_POLICY_TLV: lambda v: (v[2] & 0x80) == 0 # O bit (bit 7) is 0
        })).\
        must_next()

    # Step 6: Leader (DUT)
    # - Description: Automatically sends MGMT_ACTIVE_SET.rsp to Commissioner_1.
    # - Pass Criteria:
    #   - The DUT MUST send MGMT_ACTIVE_SET.rsp to Commissioner_1:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload:
    #     - State TLV (value = Accept (0x01))
    print("Step 6: Leader responds with MGMT_ACTIVE_SET.rsp")
    pkts.filter_wpan_src64(LEADER).\
        filter(lambda p: hasattr(p, 'coap') and\
                         p.coap.code == COAP_CODE_CHANGED).\
        filter(lambda p: verify_meshcop_tlvs(p, {
            consts.NM_STATE_TLV: lambda v: v[0] == 1 # Accept
        })).\
        must_next()

    # Step 7: Commissioner_1
    # - Description: Harness instructs device to send MGMT_ACTIVE_GET.req to the DUT.
    #   - CoAP Request URI: coap://[<L>]:MM/c/ag
    #   - CoAP Payload:
    #     - Get TLV specifying: Network Master Key TLV
    # - Pass Criteria: N/A
    print("Step 7: Commissioner_1 sends MGMT_ACTIVE_GET.req (Master Key)")
    pkts.filter_wpan_src64(COMMISSIONER_1).\
        filter(lambda p: hasattr(p, 'coap') and\
                         p.coap.opt.uri_path_recon == consts.MGMT_ACTIVE_GET_URI and\
                         p.coap.code == COAP_CODE_POST).\
        filter(lambda p: verify_meshcop_tlvs(p, {
            NM_GET_TLV: lambda v: consts.NM_NETWORK_KEY_TLV in v
        })).\
        must_next()

    # Step 8: Leader (DUT)
    # - Description: Automatically sends MGMT_ACTIVE_GET.rsp to Commissioner_1.
    # - Pass Criteria:
    #   - The DUT MUST send MGMT_ACTIVE_GET.rsp to Commissioner_1:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload:
    #     - Network Master Key TLV MUST NOT be included.
    print("Step 8: Leader responds with MGMT_ACTIVE_GET.rsp (No Master Key)")
    pkts.filter_wpan_src64(LEADER).\
        filter(lambda p: hasattr(p, 'coap') and\
                         p.coap.code == COAP_CODE_CHANGED).\
        filter(lambda p: p.coap.payload == nullField or\
                         consts.NM_NETWORK_KEY_TLV not in parse_tlvs(p.coap.payload)).\
        must_next()

    # Step 9: Commissioner_1
    # - Description: Harness instructs device to send MGMT_ACTIVE_SET.req to the DUT
    #   (disable “N” bit).
    #   - CoAP Request URI: coap://[<L>]:MM/c/as
    #   - CoAP Payload:
    #     - Commissioner Session ID TLV
    #     - Active Timestamp TLV = 20 (> step 5)
    #     - Security Policy TLV with “N” bit disabled.
    # - Pass Criteria: N/A
    print("Step 9: Commissioner_1 sends MGMT_ACTIVE_SET.req (Disable N)")
    pkts.filter_wpan_src64(COMMISSIONER_1).\
        filter(lambda p: hasattr(p, 'coap') and\
                         p.coap.opt.uri_path_recon == consts.MGMT_ACTIVE_SET_URI and\
                         p.coap.code == COAP_CODE_POST).\
        filter(lambda p: verify_meshcop_tlvs(p, {
            consts.NM_ACTIVE_TIMESTAMP_TLV: lambda v: v[5] == 20,
            consts.NM_SECURITY_POLICY_TLV: lambda v: (v[2] & 0x40) == 0 # N bit (bit 6) is 0
        })).\
        must_next()

    # Step 10: Leader (DUT)
    # - Description: Automatically sends MGMT_ACTIVE_SET.rsp to Commissioner_1.
    # - Pass Criteria:
    #   - The DUT MUST send MGMT_ACTIVE_SET.rsp to Commissioner_1:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload:
    #     - State TLV (value = Accept (0x01))
    print("Step 10: Leader responds with MGMT_ACTIVE_SET.rsp")
    pkts.filter_wpan_src64(LEADER).\
        filter(lambda p: hasattr(p, 'coap') and\
                         p.coap.code == COAP_CODE_CHANGED).\
        filter(lambda p: verify_meshcop_tlvs(p, {
            consts.NM_STATE_TLV: lambda v: v[0] == 1 # Accept
        })).\
        must_next()

    # Step 11: Commissioner_2
    # - Description: Harness instructs device to try to join the network as a Native
    #   Commissioner.
    # - Pass Criteria: N/A
    print("Step 11: Commissioner_2 sends Discovery Request")
    pkts.filter_wpan_src64(COMMISSIONER_2).\
        filter_mle_cmd(consts.MLE_DISCOVERY_REQUEST).\
        must_next()

    # Step 12: Leader (DUT)
    # - Description: Automatically rejects Commissioner_2’s attempt to join.
    # - Pass Criteria:
    #   - The DUT MUST send a Discovery Response with Native Commissioning bit set to
    #     “Not Allowed”.
    print("Step 12: Leader sends Discovery Response")
    pkts.filter_wpan_src64(LEADER).\
        filter_wpan_dst64(COMMISSIONER_2).\
        filter_mle_cmd(consts.MLE_DISCOVERY_RESPONSE).\
        filter(lambda p: p.thread_meshcop.tlv.discovery_rsp_n == 0).\
        must_next()

    # Step 13: Commissioner_1
    # - Description: Harness instructs device to send MGMT_ACTIVE_SET.req to the DUT
    #   (“B” bit = 0).
    #   - CoAP Request URI: coap://[<L>]:MM/c/as
    #   - CoAP Payload:
    #     - Commissioner Session ID TLV
    #     - Active Timestamp TLV = 25 (> Step 9)
    #     - Security Policy TLV with “B” bit = 0 (default)
    #   - Note: This step is a legacy V1.1 behavior which has been deprecated in V1.2.1.
    #     For simplicity sake, this step has been left as-is because the B-bit is now
    #     reserved – and the value of zero is the new default behavior.
    # - Pass Criteria: N/A
    print("Step 13: Commissioner_1 sends MGMT_ACTIVE_SET.req (Disable B)")
    pkts.filter_wpan_src64(COMMISSIONER_1).\
        filter(lambda p: hasattr(p, 'coap') and\
                         p.coap.opt.uri_path_recon == consts.MGMT_ACTIVE_SET_URI and\
                         p.coap.code == COAP_CODE_POST).\
        filter(lambda p: verify_meshcop_tlvs(p, {
            consts.NM_ACTIVE_TIMESTAMP_TLV: lambda v: v[5] == 25,
            consts.NM_SECURITY_POLICY_TLV: lambda v: (v[2] & 0x08) == 0 # B bit (bit 3) is 0
        })).\
        must_next()

    # Step 14: Leader (DUT)
    # - Description: Automatically sends MGMT_ACTIVE_SET.rsp to Commissioner_1.
    # - Pass Criteria:
    #   - The DUT MUST send MGMT_ACTIVE_SET.rsp to Commissioner_1:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload:
    #     - State TLV (value = Accept (0x01))
    print("Step 14: Leader responds with MGMT_ACTIVE_SET.rsp")
    pkts.filter_wpan_src64(LEADER).\
        filter(lambda p: hasattr(p, 'coap') and\
                         p.coap.code == COAP_CODE_CHANGED).\
        filter(lambda p: verify_meshcop_tlvs(p, {
            consts.NM_STATE_TLV: lambda v: v[0] == 1 # Accept
        })).\
        must_next()

    # Step 15: Test Harness Device
    # - Description: Harness instructs device to discover network using beacons.
    # - Pass Criteria: N/A
    print("Step 15: Commissioner_1 sends Beacon Request")
    pkts.filter(lambda p: hasattr(p, 'wpan')).\
        filter(lambda p: (p.wpan.fcf & 0x7) == 3).\
        filter(lambda p: hasattr(p.wpan, 'cmd')).\
        filter(lambda p: p.wpan.cmd == 7).\
        must_next()

    # Step 16: Leader (DUT)
    # - Description: Automatically responds with beacon response frame.
    # - Pass Criteria:
    #   - The DUT MUST send beacon response frames.
    #   - The beacon payload MUST either be empty OR the payload format MUST be
    #     different from the Thread Beacon payload.
    #   - The Protocol ID and Version field values MUST be different from the values
    #     specified for the Thread beacon (Protocol ID= 3, Version = 2).
    print("Step 16: Leader responds with Beacon")
    pkts.filter_wpan_beacon().\
        filter(lambda p: p.wpan.src64 == LEADER).\
        filter(lambda p: not p.thread_bcn).\
        must_next()

    # Step 17: Commissioner_1
    # - Description: Harness instructs device to send MGMT_ACTIVE_SET.req to the DUT
    #   (disable “R” bit).
    #   - CoAP Request URI: coap://[<L>]:MM/c/as
    #   - CoAP Payload:
    #     - Commissioner Session ID TLV
    #     - Active Timestamp TLV = 30 (> step 13)
    #     - Security Policy TLV with “R” bit disabled.
    # - Pass Criteria: N/A
    print("Step 17: Commissioner_1 sends MGMT_ACTIVE_SET.req (Disable R)")
    pkts.filter_wpan_src64(COMMISSIONER_1).\
        filter(lambda p: hasattr(p, 'coap') and\
                         p.coap.opt.uri_path_recon == consts.MGMT_ACTIVE_SET_URI and\
                         p.coap.code == COAP_CODE_POST).\
        filter(lambda p: verify_meshcop_tlvs(p, {
            consts.NM_ACTIVE_TIMESTAMP_TLV: lambda v: v[5] == 30,
            consts.NM_SECURITY_POLICY_TLV: lambda v: (v[2] & 0x20) == 0 # R bit (bit 5) is 0
        })).\
        must_next()

    # Step 18: Leader (DUT)
    # - Description: Automatically sends MGMT_ACTIVE_SET.rsp to Commissioner_1.
    # - Pass Criteria:
    #   - The DUT MUST send MGMT_ACTIVE_SET.rsp to Commissioner_1:
    #   - CoAP Response Code: 2.04 Changed
    #   - CoAP Payload:
    #     - State TLV (value = Accept (0x01))
    print("Step 18: Leader responds with MGMT_ACTIVE_SET.rsp")
    pkts.filter_wpan_src64(LEADER).\
        filter(lambda p: hasattr(p, 'coap') and\
                         p.coap.code == COAP_CODE_CHANGED).\
        filter(lambda p: verify_meshcop_tlvs(p, {
            consts.NM_STATE_TLV: lambda v: v[0] == 1 # Accept
        })).\
        must_next()

    # Step 19: Leader (DUT)
    # - Description: Automatically sends multicast MLE Data Response. Commissioner_1
    #   responds with MLE Data Request.
    # - Pass Criteria:
    #   - The DUT MUST multicast MLE Data Response to the Link-Local All Nodes
    #     multicast address (FF02::1) with active timestamp value as set in Step 17.
    print("Step 19: Leader multicasts MLE Data Response")
    pkts.filter_wpan_src64(LEADER).\
        filter_LLANMA().\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: p.mle.tlv.active_tstamp == 30).\
        must_next()

    # Step 20: Leader (DUT)
    # - Description: Automatically sends unicast MLE Data Response to Commissioner_1.
    # - Pass Criteria:
    #   - The DUT MUST send a unicast MLE Data Response to Commissioner_1.
    #   - The Active Operational Set MUST contain a Security Policy TLV with R bit set
    #     to 0.
    print("Step 20: Leader unicasts MLE Data Response to Commissioner_1")
    pkts.filter_wpan_src64(LEADER).\
        filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: {
                          consts.ACTIVE_OPERATION_DATASET_TLV
                          } <= set(p.mle.tlv.type)).\
        filter(lambda p: p.thread_meshcop.tlv.sec_policy_r == 0).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
