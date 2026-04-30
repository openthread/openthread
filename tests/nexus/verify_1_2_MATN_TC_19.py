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
from pktverify.null_field import nullField

ST_MLR_SUCCESS = 0
PARENT_AGGREGATE_DELAY = 5


def verify(pv):
    # 5.10.15 MATN-TC-19: Multicast registration by MTD
    #
    # Purpose:
    # The purpose of this test case is to verify that an MTD can register a multicast address through a Parent Thread
    #   Router.
    #
    # Spec Reference | V1.2 Section
    # ---------------|-------------
    # MATN-TC-19     | 5.10.15

    pkts = pv.pkts
    pv.summary.show()

    BR_1 = pv.vars['BR_1']
    MTD = pv.vars['MTD']

    # Use the test-specific variables from the core
    v_MA2 = Ipv6Addr(pv.vars['TEST_MA2'])
    v_MA3 = Ipv6Addr(pv.vars['TEST_MA3'])
    v_MA5 = Ipv6Addr(pv.vars['TEST_MA5'])
    HOST_ULA = Ipv6Addr(pv.vars['HOST_BACKBONE_ULA'])

    # Step 0
    # - Device: N/A
    # - Description: Topology formation – BR_1, BR_2, Router, MTD
    # - Pass Criteria:
    #   - N/A
    print("Step 0: Topology formation – BR_1, BR_2, Router, MTD")

    # Step 1a
    # - Device: MTD
    # - Description: Registers addresses MA2 & MA5 with its parent. (MA5 is optional for MEDs)
    # - Pass Criteria:
    #   - For DUT = MTD:
    #   - The DUT MUST unicast an MLE Child Update Request command to Router, where the payload contains (amongst other
    #     fields):
    #   - Address Registration TLV: MA2
    #   - MA5 (Optional for MED)
    print("Step 1a: MTD Registers addresses MA2 & MA5 with its parent.")
    pkt = pkts.filter_wpan_src64(MTD).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        filter(lambda p: consts.ADDRESS_REGISTRATION_TLV in p.mle.tlv.type).\
        filter(lambda p: v_MA2 in p.mle.tlv.addr_reg).\
        must_next()
    assert v_MA5 in pkt.mle.tlv.addr_reg

    # Step 1b
    # - Device: Router
    # - Description: Automatically responds to the registration request.
    # - Pass Criteria:
    #   - For DUT = Router:
    #   - The DUT MUST unicast an MLE Child Update Response command to MTD, where the payload contains (amongst other
    #     fields):
    #   - Address Registration TLV: MA2
    #   - MA5
    print("Step 1b: Router Automatically responds to the registration request.")
    pkt = pkts.filter_wpan_dst64(MTD).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        filter(lambda p: consts.ADDRESS_REGISTRATION_TLV in p.mle.tlv.type).\
        filter(lambda p: v_MA2 in p.mle.tlv.addr_reg).\
        must_next()
    assert v_MA5 in pkt.mle.tlv.addr_reg

    # Step 1c
    # - Device: Router
    # - Description: Automatically registers multicast address, MA2 at BR_1
    # - Pass Criteria:
    #   - For DUT = Router:
    #   - The DUT MUST unicast an MLR.req CoAP request to BR_1 as follows: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr
    #   - Where the payload contains:
    #   - IPv6 Addresses TLV: MA2
    print("Step 1c: Router Automatically registers multicast address, MA2 at BR_1")
    target_pkt = pkts.filter_coap_request('/n/mr').\
        filter(lambda p: v_MA2 in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 2
    # - Device: MTD
    # - Description: Registers addresses MA2 and MA3 with its parent.
    # - Pass Criteria:
    #   - For DUT = MTD:
    #   - The DUT MUST unicast an MLE Child Update Request command to Router_1, where the payload contains (amongst
    #     other fields):
    #   - Address Registration TLV: MA2 and MA3.
    print("Step 2: MTD Registers addresses MA2 and MA3 with its parent.")
    pkt = pkts.filter_wpan_src64(MTD).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_REQUEST).\
        filter(lambda p: consts.ADDRESS_REGISTRATION_TLV in p.mle.tlv.type).\
        filter(lambda p: v_MA3 in p.mle.tlv.addr_reg).\
        must_next()
    assert v_MA2 in pkt.mle.tlv.addr_reg and v_MA5 not in pkt.mle.tlv.addr_reg

    # Step 3
    # - Device: Router
    # - Description: Automatically responds to the registration request.
    # - Pass Criteria:
    #   - For DUT = Router:
    #   - The DUT MUST unicast an MLE Child Update Response command to MTD, where the payload contains (amongst other
    #     fields):
    #   - Address Registration TLV: MA2 and MA3.
    print("Step 3: Router Automatically responds to the registration request.")
    step3_pkt = pkts.filter_wpan_dst64(MTD).\
        filter_mle_cmd(consts.MLE_CHILD_UPDATE_RESPONSE).\
        filter(lambda p: consts.ADDRESS_REGISTRATION_TLV in p.mle.tlv.type).\
        filter(lambda p: v_MA3 in p.mle.tlv.addr_reg).\
        must_next()
    assert v_MA2 in step3_pkt.mle.tlv.addr_reg

    # Step 4
    # - Device: Router
    # - Description: Wait at most PARENT_AGGREGATE_DELAY.
    # - Pass Criteria:
    #   - The next message (below) is sent not later than PARENT_AGGREGATE_DELAY seconds since step 3.
    print("Step 4: Router Wait at most PARENT_AGGREGATE_DELAY.")

    # Step 5
    # - Device: Router
    # - Description: Automatically registers multicast addresses, MA2 and MA3 at BR_1. Unicasts an MLR.req CoAP request
    #   to BR_1 as follows: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr Where the payload contains: IPv6 Addresses TLV:
    #   MA3, MA2 (optional).
    # - Pass Criteria:
    #   - N/A
    print("Step 5: Router Automatically registers multicast addresses, MA2 and MA3 at BR_1.")
    step5_pkt = pkts.filter_coap_request('/n/mr').\
        filter(lambda p: v_MA3 in p.coap.tlv.ipv6_address).\
        must_next()
    # MA2 is optional in Step 5 as per spec. OpenThread might only send new registrations.
    # assert v_MA2 in step5_pkt.coap.tlv.ipv6_address, f"MA2 ({v_MA2}) not in {step5_pkt.coap.tlv.ipv6_address}"

    # Verify Step 4 timing
    assert step5_pkt.sniff_timestamp - step3_pkt.sniff_timestamp <= PARENT_AGGREGATE_DELAY + 2.0, \
        f"Step 5 sent too late: {step5_pkt.sniff_timestamp - step3_pkt.sniff_timestamp}s > {PARENT_AGGREGATE_DELAY}s"

    # Step 6
    # - Device: BR_1
    # - Description: Automatically responds to the registration request. Unicasts an MLR.rsp CoAP response to Router_1
    #   as follows: 2.04 changed Where the payload contains: Status TLV: ST_MLR_SUCCESS.
    # - Pass Criteria:
    #   - N/A
    print("Step 6: BR_1 Automatically responds to the registration request.")
    pkts.filter_coap_ack('/n/mr').\
        filter(lambda p: p.coap.tlv.status == ST_MLR_SUCCESS).\
        must_next()

    # Step 7
    # - Device: Host
    # - Description: Harness instructs the device to send ICMPv6 Echo (ping) Request packet to multicast address, MA3,
    #   on the backbone link.
    # - Pass Criteria:
    #   - N/A
    print("Step 7: Host ICMPv6 Echo (ping) Request packet to multicast address, MA3, on the backbone link.")

    # Step 8
    # - Device: BR_1
    # - Description: Automatically forwards the ping request packet to its Thread Network encapsulated in an MPL packet.
    # - Pass Criteria:
    #   - N/A
    print("Step 8: BR_1 Automatically forwards the ping request packet to its Thread Network.")
    pkts.filter_wpan_src64(BR_1).\
        filter_ping_request().\
        filter(lambda p: p.ipv6.dst == 'ff03::fc').\
        must_next()

    # Step 8a
    # - Device: Router
    # - Description: Receives the multicast ping request packet and if MTD is a SED then it automatically pends it for
    #   delivery to MTD. If MTD is MED, it automatically forwards the packet as defined in step 10.
    # - Pass Criteria:
    #   - None.
    print("Step 8a: Router Receives the multicast ping request packet.")

    # Step 9
    # - Device: MTD
    # - Description: If MTD is a SED, it wakes up and/or polls for the pending ping request packet. If MTD is a MED,
    #   this test step is skipped.
    # - Pass Criteria:
    #   - For DUT = MTD (SED):
    #   - The DUT MUST send a MAC poll request to Router_1.
    print("Step 9: MTD sends a MAC poll request.")
    pkts.filter_wpan_src64(MTD).\
        filter(lambda p: p.wpan.frame_type == consts.MAC_FRAME_TYPE_MAC_CMD and p.wpan.cmd == consts.WPAN_DATA_REQUEST).\
        must_next()

    # Step 10
    # - Device: Router
    # - Description: Automatically forwards the ping request packet to MTD.
    # - Pass Criteria:
    #   - For DUT = Router:
    #   - The DUT MUST forward the ICMPv6 Echo (ping) Request packet with multicast address, MA3, to MTD using 802.15.4
    #     unicast.
    #   - The ping request packet MUST NOT be encapsulated in an MPL packet.
    print("Step 10: Router Automatically forwards the ping request packet to MTD.")
    pkts.filter_ping_request().\
        filter(lambda p: p.ipv6.dst == v_MA3).\
        filter(lambda p: p.wpan.dst64 == MTD or (p.wpan.dst16 is not nullField and p.wpan.dst16 != 0xffff)).\
        filter(lambda p: not p.ipv6inner).\
        must_next()

    # Step 11
    # - Device: MTD
    # - Description: Receives the multicast ping request packet and sends a unicast ICMPv6 Echo (ping) Response packet
    #   back to Host.
    # - Pass Criteria:
    #   - For DUT = MTD:
    #   - The DUT MUST unicast a ICMPv6 Echo (ping) Response packet back to Host.
    print("Step 11: MTD sends a unicast ICMPv6 Echo Response packet back to Host.")
    pkts.filter_wpan_src64(MTD).\
        filter_ping_reply().\
        filter(lambda p: p.ipv6.dst == HOST_ULA).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
