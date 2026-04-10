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

ST_MLR_SUCCESS = 0


def verify(pv):
    # 5.10.16 MATN-TC-20: Automatic re-registration by Parent Router on behalf of MTD
    #
    # 5.10.16.1 Topology
    # - Router (DUT)
    # - MED
    # - BR_1
    # - BR_2
    #
    # 5.10.16.2 Purpose & Description
    # The purpose of this test case is to verify that a Parent Router handling a multicast registration on behalf of an
    #   MTD re-registers the multicast address on behalf of its child, before the MLR timeout expires.
    #
    # Spec Reference | V1.2 Section
    # ---------------|-------------
    # Multicast      | 5.10.16

    pkts = pv.pkts

    ROUTER = pv.vars['Router']
    ROUTER_16 = pv.vars['Router_RLOC16']
    MED = pv.vars['MED']
    MED_16 = pv.vars['MED_RLOC16']
    MED_MLEID = pv.vars['MED_MLEID']
    MED_ADDRS = [Ipv6Addr(a) for a in pv.vars.get('MED_IPADDRS', [MED_MLEID])]
    MA2 = Ipv6Addr(pv.vars['MA2'])
    BR1_RLOC = pv.vars['BR_1_RLOC']
    BR1_RLOC16 = pv.vars['BR_1_RLOC16']
    PBBR_ALOC = pv.vars.get('PBBR_ALOC', consts.PBBR_ALOC)

    kMlrTimeoutMin = 300
    kMlrTimeoutExtended = kMlrTimeoutMin + 30

    def has_addr(p, addr):
        iid_int = int(addr[8:].hex(), 16)
        return any(iid == iid_int for iid in p.mle.tlv.addr_reg_iid) or \
               any(a == addr for a in p.mle.tlv.addr_reg_ipv6)

    # Step 0
    # - Device: N/A
    # - Description: Topology formation - BR_1, BR_2. Topology formation - Router (DUT). The DUT is booted and added to
    #   the network. Topology formation - MED.
    # - Pass Criteria:
    #   - N/A
    print("Step 0: Topology formation - BR_1, BR_2. Topology formation - Router (DUT).")

    # Step 2
    # - Device: BR_1
    # - Description: Harness configures the value of the MLR timeout in the BBR Dataset of the device to be
    #   (MLR_TIMEOUT_MIN + 30) seconds and distribute the BBR Dataset.
    # - Pass Criteria:
    #   - N/A
    print("Step 2: Harness configures the value of the MLR timeout in the BBR Dataset.")

    # Step 3
    # - Device: Router (DUT)
    # - Description: Automatically distributes the new network data to MED.
    # - Pass Criteria:
    #   - None.
    print("Step 3: Automatically distributes the new network data to MED.")

    # Step 4
    # - Device: MED
    # - Description: Harness instructs the device to register multicast address MA2 with its parent. The device
    #   unicasts an MLE Child Update Request to the DUT, where the payload includes a single TLV with single address
    #   as below: Address Registration TLV: MA2
    # - Pass Criteria:
    #   - N/A
    print("Step 4: MED registers multicast address MA2 with its parent.")
    med_reg_pkt = pkts.filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        filter(lambda p: p.wpan.src64 == MED or p.wpan.src16 == MED_16).\
        filter(lambda p: {consts.ADDRESS_REGISTRATION_TLV} <= set(p.mle.tlv.type)).\
        filter(lambda p: has_addr(p, MA2)).\
        must_next()

    # Step 5
    # - Device: Router (DUT)
    # - Description: Automatically responds to the registration request.
    # - Pass Criteria:
    #   - The DUT MUST unicast an MLE Child Update Response command to MTD, where the payload contains (amongst other
    #     fields) this TLV with two addresses inside:
    #   - Address Registration TLV: ML-EID of MTD (M1g)
    #   - MA2
    print("Step 5: Router responds to the registration request.")
    pkts.filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        filter(lambda p: p.wpan.src64 == ROUTER or p.wpan.src16 == ROUTER_16).\
        filter(lambda p: p.wpan.dst64 == MED or p.wpan.dst16 == MED_16).\
        filter(lambda p: {consts.ADDRESS_REGISTRATION_TLV} <= set(p.mle.tlv.type)).\
        filter(lambda p: has_addr(p, MA2) and any(has_addr(p, addr) for addr in MED_ADDRS)).\
        must_next()

    # Step 6
    # - Device: Router (DUT)
    # - Description: Automatically requests to register multicast address MA2 at BR_1
    # - Pass Criteria:
    #   - The DUT MUST unicast an MLR.req CoAP request to BR_1 as follows: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr
    #   - Where the payload contains:
    #   - IPv6 Addresses TLV: MA2
    print("Step 6: Router requests to register multicast address MA2 at BR_1.")
    pkts.filter_coap_request('/n/mr').\
        filter(lambda p: p.wpan.src64 == ROUTER or p.wpan.src16 == ROUTER_16).\
        filter(lambda p: p.ipv6.dst in (BR1_RLOC, PBBR_ALOC)).\
        filter(lambda p: MA2 in p.coap.tlv.ipv6_address).\
        must_next()

    # Automatically responds to initial registration
    pkts.filter_coap_ack('/n/mr').\
        filter(lambda p: p.wpan.src64 == pv.vars['BR_1'] or p.wpan.src16 == BR1_RLOC16).\
        filter(lambda p: p.coap.tlv.status == ST_MLR_SUCCESS).\
        must_next()

    # Note: Checks for BLMR.ntf and MLDv2 are intentionally skipped.

    # Step 7
    # - Device: Router (DUT)
    # - Description: Before MLR timeout, automatically requests to re-register multicast address, MA2, at BR_1.
    # - Pass Criteria:
    #   - The DUT MUST unicast an MLR.req CoAP request to BR_1: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr
    #   - Where the payload contains one TLV with at least one address as follows:
    #   - IPv6 Addresses TLV: MA2
    #   - The CoAP request MUST be sent within (MLR_TIMEOUT_MIN + 30) seconds of step 4
    print("Step 7: Router re-registers multicast address MA2 at BR_1 before timeout.")
    re_reg_pkt_1 = pkts.filter_coap_request('/n/mr').\
        filter(lambda p: p.wpan.src64 == ROUTER or p.wpan.src16 == ROUTER_16).\
        filter(lambda p: p.ipv6.dst in (BR1_RLOC, PBBR_ALOC)).\
        filter(lambda p: MA2 in p.coap.tlv.ipv6_address).\
        must_next()

    # Timing check for Step 7: within (MLR_TIMEOUT_MIN + 30) seconds of step 4
    delta = re_reg_pkt_1.sniff_timestamp - med_reg_pkt.sniff_timestamp
    assert delta <= kMlrTimeoutExtended, f"Step 7 timing failure: {delta} > {kMlrTimeoutExtended}"

    # Step 8
    # - Device: BR_1
    # - Description: Automatically responds to the multicast registration with MLR.rsp with status ST_MLR_SUCCESS.
    # - Pass Criteria:
    #   - N/A
    print("Step 8: BR_1 responds to the multicast registration.")
    pkts.filter_coap_ack('/n/mr').\
        filter(lambda p: p.wpan.src64 == pv.vars['BR_1'] or p.wpan.src16 == BR1_RLOC16).\
        filter(lambda p: p.coap.tlv.status == ST_MLR_SUCCESS).\
        must_next()

    # Step 9
    # - Device: Router (DUT)
    # - Description: Before MLR timeout, automatically re-registers for multicast address MA2.
    # - Pass Criteria:
    #   - The DUT MUST unicast an MLR.req CoAP request to BR_1: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr
    #   - Where the payload contains one TLV with at least one address as follows:
    #   - IPv6 Addresses TLV: MA2
    #   - The request MUST be sent within (MLR_TIMEOUT_MIN + 30) seconds of step 6,
    print("Step 9: Router re-registers for multicast address MA2.")
    re_reg_pkt_2 = pkts.filter_coap_request('/n/mr').\
        filter(lambda p: p.wpan.src64 == ROUTER or p.wpan.src16 == ROUTER_16).\
        filter(lambda p: p.ipv6.dst in (BR1_RLOC, PBBR_ALOC)).\
        filter(lambda p: MA2 in p.coap.tlv.ipv6_address).\
        must_next()

    # Timing check for Step 9: within (MLR_TIMEOUT_MIN + 30) seconds of step 6
    delta = re_reg_pkt_2.sniff_timestamp - re_reg_pkt_1.sniff_timestamp
    assert delta <= kMlrTimeoutExtended, f"Step 9 timing failure: {delta} > {kMlrTimeoutExtended}"

    # Step 10
    # - Device: BR_1
    # - Description: Automatically responds to the multicast registration with MLR.rsp with status ST_MLR_SUCCESS.
    # - Pass Criteria:
    #   - N/A
    print("Step 10: BR_1 responds to the multicast registration.")
    pkts.filter_coap_ack('/n/mr').\
        filter(lambda p: p.wpan.src64 == pv.vars['BR_1'] or p.wpan.src16 == BR1_RLOC16).\
        filter(lambda p: p.coap.tlv.status == ST_MLR_SUCCESS).\
        must_next()

    # Step 11
    # - Device: BR_1
    # - Description: Harness configures the value of the device MLR timeout to be MLR_TIMEOUT_MIN seconds and
    #   distributes the BBR Dataset.
    # - Pass Criteria:
    #   - N/A
    print("Step 11: BR_1 configures the MLR timeout to be MLR_TIMEOUT_MIN seconds.")
    # Look for Network Data update from BR1
    step11_pkt = pkts.filter_mle_cmd(consts.MLE_DATA_RESPONSE).\
        filter(lambda p: p.wpan.src64 == pv.vars['BR_1'] or p.wpan.src16 == BR1_RLOC16).\
        must_next()

    # Step 12
    # - Device: Router (DUT)
    # - Description: Automatically recognizes the BBR Dataset update and re-registers MA2.
    # - Pass Criteria:
    #   - The DUT MUST unicast an MLR.req CoAP request to BR_1: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr
    #   - Where the payload contains at least MA2:
    #   - IPv6 Addresses TLV: MA2
    print("Step 12: Router recognizes BBR Dataset update and re-registers MA2.")
    re_reg_pkt_3 = pkts.filter_coap_request('/n/mr').\
        filter(lambda p: p.wpan.src64 == ROUTER or p.wpan.src16 == ROUTER_16).\
        filter(lambda p: p.ipv6.dst in (BR1_RLOC, PBBR_ALOC)).\
        filter(lambda p: MA2 in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 13
    # - Device: BR_1
    # - Description: Automatically responds to the multicast registration with MLR.rsp with status ST_MLR_SUCCESS.
    # - Pass Criteria:
    #   - N/A
    print("Step 13: BR_1 responds to the multicast registration.")
    pkts.filter_coap_ack('/n/mr').\
        filter(lambda p: p.wpan.src64 == pv.vars['BR_1'] or p.wpan.src16 == BR1_RLOC16).\
        filter(lambda p: p.coap.tlv.status == ST_MLR_SUCCESS).\
        must_next()

    # Step 14
    # - Device: Router (DUT)
    # - Description: Before MLR timeout, automatically re-registers for multicast address MA2.
    # - Pass Criteria:
    #   - The DUT MUST unicast an MLR.req CoAP request to BR_1: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr
    #   - Where the payload contains at least MA2:
    #   - IPv6 Addresses TLV: MA2
    #   - The request MUST be sent within MLR_TIMEOUT_MIN seconds of step 11,
    print("Step 14: Router re-registers for multicast address MA2.")
    re_reg_pkt_4 = pkts.filter_coap_request('/n/mr').\
        filter(lambda p: p.wpan.src64 == ROUTER or p.wpan.src16 == ROUTER_16).\
        filter(lambda p: p.ipv6.dst in (BR1_RLOC, PBBR_ALOC)).\
        filter(lambda p: MA2 in p.coap.tlv.ipv6_address).\
        must_next()

    # Timing check for Step 14: within MLR_TIMEOUT_MIN seconds of step 11
    delta = re_reg_pkt_4.sniff_timestamp - step11_pkt.sniff_timestamp
    assert delta <= kMlrTimeoutMin, f"Step 14 timing failure: {delta} > {kMlrTimeoutMin}"

    # Step 15
    # - Device: BR_1
    # - Description: Automatically responds to the multicast registration with MLR.rsp with status ST_MLR_SUCCESS.
    # - Pass Criteria:
    #   - N/A
    print("Step 15: BR_1 responds to the multicast registration.")
    pkts.filter_coap_ack('/n/mr').\
        filter(lambda p: p.wpan.src64 == pv.vars['BR_1'] or p.wpan.src16 == BR1_RLOC16).\
        filter(lambda p: p.coap.tlv.status == ST_MLR_SUCCESS).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
