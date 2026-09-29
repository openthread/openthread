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
from pktverify.addrs import Ipv6Addr
from pktverify.bytes import Bytes

ST_MLR_INVALID = 2
ST_MLR_BBR_NOT_PRIMARY = 5
ST_MLR_GENERAL_FAILURE = 6


def verify(pv):
    # 5.10.17 MATN-TC-21: Incorrect Multicast registrations by Thread Device
    #
    # 5.10.17.1 Topology
    # - BR_1 (DUT)
    # - BR_2
    # - Router
    # - Host
    #
    # 5.10.17.2 Purpose & Description
    # The purpose of this test case is to verify that a BR_1 can correctly handle incorrect or invalid multicast
    #   registrations from a Thread device, whether that is due to invalid TLV values or other reasons enumerated
    #   below.
    #
    # Spec Reference   | V1.2 Section
    # -----------------|-------------
    # Multicast        | 5.10.17

    pkts = pv.pkts
    vars = pv.vars
    pv.summary.show()

    # MLDv2 and BLMR.ntf checks are intentionally skipped as per test requirements.

    # Step 0
    # - Device: N/A
    # - Description: Topology formation - BR_1, BR_2, Router
    # - Pass Criteria:
    #   - N/A
    print("Step 0: Topology formation - BR_1, BR_2, Router")

    # Step 1
    # - Device: Router
    # - Description: Harness instructs the device to register an invalid multicast address at BR_1. Automatically
    #   unicasts an MLR.req CoAP request to BR_1 as follows: coap://[<BR_1 RLOC>]:MM/n/mr Where the payload contains:
    #   IPv6 Addresses TLV: MAe1
    # - Pass Criteria:
    #   - N/A
    print("Step 1: Router registers an invalid multicast address MAe1 at BR_1.")
    pkts.filter_wpan_src64(vars['Router']).\
        filter_coap_request('/n/mr').\
        filter(lambda p: Ipv6Addr(vars['MAe1']) in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 2
    # - Device: BR_1
    # - Description: Does not update its multicast listener table.
    # - Pass Criteria:
    #   - None.
    print("Step 2: BR_1 does not update its multicast listener table.")

    # Step 3
    # - Device: BR_1
    # - Description: Automatically responds to the multicast registration with an error status
    # - Pass Criteria:
    #   - For DUT = BR_1:
    #   - The DUT MUST unicast an MLR.rsp CoAP response to TD as follows: 2.04 changed
    #   - Where the payload contains:
    #   - Status TLV: 2 [ST_MLR_INVALID]
    #   - IPv6 Addresses TLV: The invalid IPv6 address included in step 1
    print("Step 3: BR_1 responds with error status ST_MLR_INVALID and MAe1.")
    pkts.filter_wpan_src64(vars['BR_1']).\
        filter_coap_ack('/n/mr').\
        filter(lambda p: p.coap.tlv.status == ST_MLR_INVALID).\
        filter(lambda p: Ipv6Addr(vars['MAe1']) in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 4
    # - Device: Router
    # - Description: Harness instructs the device to register two invalid multicast addresses at BR_1. Automatically
    #   unicasts an MLR.req CoAP request to BR_1 as follows: coap://[<BR_1 RLOC>]:MM/n/mr Where the payload contains:
    #   IPv6 Addresses TLV: MAe2, MAe3
    # - Pass Criteria:
    #   - N/A
    print("Step 4: Router registers two invalid multicast addresses MAe2, MAe3 at BR_1.")
    pkts.filter_wpan_src64(vars['Router']).\
        filter_coap_request('/n/mr').\
        filter(lambda p: Ipv6Addr(vars['MAe2']) in p.coap.tlv.ipv6_address).\
        filter(lambda p: Ipv6Addr(vars['MAe3']) in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 5
    # - Device: BR_1
    # - Description: Does not update its multicast listener table.
    # - Pass Criteria:
    #   - None.
    print("Step 5: BR_1 does not update its multicast listener table.")

    # Step 6
    # - Device: BR_1
    # - Description: Automatically responds to the multicast registration with an error status
    # - Pass Criteria:
    #   - For DUT = BR_1:
    #   - The DUT MUST unicast an MLR.rsp CoAP response to TD as follows: 2.04 changed
    #   - Where the payload contains:
    #   - Status TLV: 2 [ST_MLR_INVALID]
    #   - IPv6 Addresses TLV: The two invalid IPv6 addresses included in step 4
    print("Step 6: BR_1 responds with error status ST_MLR_INVALID and addresses MAe2, MAe3.")
    pkts.filter_wpan_src64(vars['BR_1']).\
        filter_coap_ack('/n/mr').\
        filter(lambda p: p.coap.tlv.status == ST_MLR_INVALID).\
        filter(lambda p: Ipv6Addr(vars['MAe2']) in p.coap.tlv.ipv6_address).\
        filter(lambda p: Ipv6Addr(vars['MAe3']) in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 7
    # - Device: Router
    # - Description: Harness instructs the device to register a link local multicast address (MA6) at BR_1.
    #   Automatically unicasts an MLR.req CoAP request to BR_1 as follows: coap://[<BR_1 RLOC>]:MM/n/mr Where the
    #   payload contains: IPv6 Addresses TLV: MA6
    # - Pass Criteria:
    #   - N/A
    print("Step 7: Router registers a link local multicast address MA6 at BR_1.")
    pkts.filter_wpan_src64(vars['Router']).\
        filter_coap_request('/n/mr').\
        filter(lambda p: Ipv6Addr(vars['MA6']) in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 8
    # - Device: BR_1
    # - Description: Does not update its multicast listener table.
    # - Pass Criteria:
    #   - None.
    print("Step 8: BR_1 does not update its multicast listener table.")

    # Step 9
    # - Device: BR_1
    # - Description: Automatically responds to the multicast registration with an error.
    # - Pass Criteria:
    #   - For DUT = BR_1:
    #   - The DUT MUST unicast an MLR.rsp CoAP response to TD as follows: 2.04 changed
    #   - Where the payload contains:
    #   - Status TLV: 2 [ST_MLR_INVALID]
    #   - IPv6 Addresses TLV: MA6
    print("Step 9: BR_1 responds with error ST_MLR_INVALID and MA6.")
    pkts.filter_wpan_src64(vars['BR_1']).\
        filter_coap_ack('/n/mr').\
        filter(lambda p: p.coap.tlv.status == ST_MLR_INVALID).\
        filter(lambda p: Ipv6Addr(vars['MA6']) in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 10
    # - Device: Router
    # - Description: Harness instructs the device to register a mesh-local multicast address (MA5) at BR_1.
    #   Automatically unicasts an MLR.req CoAP request to BR_1 as follows: coap://[<BR_1 RLOC>]:MM/n/mr Where the
    #   payload contains: IPv6 Addresses TLV: MA5
    # - Pass Criteria:
    #   - N/A
    print("Step 10: Router registers a mesh-local multicast address MA5 at BR_1.")
    pkts.filter_wpan_src64(vars['Router']).\
        filter_coap_request('/n/mr').\
        filter(lambda p: Ipv6Addr(vars['MA5']) in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 11
    # - Device: BR_1
    # - Description: Does not update its multicast listener table.
    # - Pass Criteria:
    #   - None.
    print("Step 11: BR_1 does not update its multicast listener table.")

    # Step 12
    # - Device: BR_1
    # - Description: Automatically responds to the multicast registration with an error.
    # - Pass Criteria:
    #   - For DUT = BR_1:
    #   - The DUT MUST unicast an MLR.rsp CoAP response to TD as follows: 2.04 changed
    #   - Where the payload contains:
    #   - Status TLV: 2 [ST_MLR_INVALID]
    #   - IPv6 Addresses TLV: MA5
    print("Step 12: BR_1 responds with error ST_MLR_INVALID and MA5.")
    pkts.filter_wpan_src64(vars['BR_1']).\
        filter_coap_ack('/n/mr').\
        filter(lambda p: p.coap.tlv.status == ST_MLR_INVALID).\
        filter(lambda p: Ipv6Addr(vars['MA5']) in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 13
    # - Device: Router
    # - Description: Harness instructs the device to register one Link local (MA6) and one Admin-local (MA1)
    #   address at BR_1. Automatically unicasts an MLR.req CoAP request to BR_2 as follows:
    #   coap://[<BR_1 RLOC>]:MM/n/mr Where the payload contains: IPv6 Addresses TLV MA6, MA1
    # - Pass Criteria:
    #   - N/A
    print("Step 13: Router registers addresses MA6 and MA1 at BR_1.")
    pkts.filter_wpan_src64(vars['Router']).\
        filter_coap_request('/n/mr').\
        filter(lambda p: Ipv6Addr(vars['MA6']) in p.coap.tlv.ipv6_address).\
        filter(lambda p: Ipv6Addr(vars['MA1']) in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 14
    # - Device: BR_1
    # - Description: Automatically updates its multicast listener table with MA1 only.
    # - Pass Criteria:
    #   - None.
    print("Step 14: BR_1 updates its multicast listener table with MA1 only.")

    # Step 15
    # - Device: BR_1
    # - Description: Automatically responds to the multicast registration.
    # - Pass Criteria:
    #   - For DUT = BR_1:
    #   - The DUT MUST unicast an MLR.rsp CoAP response to TD as follows: 2.04 changed
    #   - Where the payload contains:
    #   - Status TLV: 2 [ST_MLR_INVALID]
    #   - IPv6 Addresses TLV: MA6 (only)
    print("Step 15: BR_1 responds with error ST_MLR_INVALID and MA6 only.")
    pkts.filter_wpan_src64(vars['BR_1']).\
        filter_coap_ack('/n/mr').\
        filter(lambda p: p.coap.tlv.status == ST_MLR_INVALID).\
        filter(lambda p: Ipv6Addr(vars['MA6']) in p.coap.tlv.ipv6_address).\
        filter(lambda p: len(p.coap.tlv.ipv6_address) == 1).\
        must_next()

    # Step 16
    # - Device: Host
    # - Description: Harness instructs the device to send a ICMPv6 Echo (ping) Request packet to the multicast
    #   address, MA1, on the backbone link.
    # - Pass Criteria:
    #   - N/A
    print("Step 16: Host sends ICMPv6 Echo Request to MA1 on the backbone.")
    pkts.filter_ping_request().\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['MA1'])).\
        must_next()

    # Step 17
    # - Device: BR_1
    # - Description: Automatically forwards the ping packet to its Thread Network.
    # - Pass Criteria:
    #   - For DUT = BR_1:
    #   - The DUT MUST forward the ping packet with multicast address, MA1, to its Thread Network encapsulated in
    #     an MPL packet, where:
    #   - MPL Option: If Source outer IP header == BR_1 RLOC Then S == 0 Else S == 1 and seed-id == BR_1 RLOC16
    print("Step 17: BR_1 forwards the ping packet to its Thread Network.")

    def check_mpl_option(p):
        if p.ipv6.src == Ipv6Addr(vars['BR_1_RLOC']):
            return p.ipv6.opt.mpl.flag.s == 0
        else:
            return p.ipv6.opt.mpl.flag.s == 1 and \
                   p.ipv6.opt.mpl.seed_id == Bytes(struct.pack('>H', vars['BR_1_RLOC16']))

    pkts.filter_wpan_src64(vars['BR_1']). \
        filter_ping_request(). \
        filter(lambda p: p.ipv6.dst == Ipv6Addr(vars['MA1']) or
                         (p.ipv6.dst == 'ff03::fc' and p.ipv6inner.dst == Ipv6Addr(vars['MA1']))). \
        filter(check_mpl_option). \
        must_next()

    # Step 18
    # - Device: Router
    # - Description: Harness instructs the device to register one valid multicast address at BR_1 and one
    #   invalid address (with a length that is too short). Due to the short length of the last address, the IPv6
    #   Addresses TLV is invalid i.e. message format error. Automatically unicasts an MLR.req CoAP request to
    #   BR_1 as follows: coap://[<BR_1 RLOC>]:MM/n/mr Where the payload contains: IPv6 Addresses TLV: MA3 (16
    #   bytes), MAe4 (14 bytes)
    print("Step 18: Router registers MA3 and malformed address (14 bytes).")
    pkts.filter_wpan_src64(vars['Router']).\
        filter_coap_request('/n/mr').\
        filter(lambda p: b'\x0e\x1e' in p.coap.payload).\
        must_next()

    # Step 19
    # - Device: BR_1
    # - Description: MAY automatically update its multicast listener table with MA3 only.
    # - Pass Criteria:
    #   - None.
    print("Step 19: BR_1 MAY update its multicast listener table with MA3.")

    # Step 20
    # - Device: BR_1
    # - Description: Automatically responds to the multicast registration with an error status
    # - Pass Criteria:
    #   - For DUT = BR_1: The DUT MUST respond in one of the following three ways:
    #   - 1) The DUT responds with CoAP error response 4.00 or 5.00 (in this case payload is not verified by Test
    #     Harness – it will Fail and inform the user of the situation.
    #   - 2) The DUT unicast an MLR.rsp CoAP response to TD as follows: 2.04 changed Where the payload contains:
    #     Status TLV: 2 [ST_MLR_INVALID], IPv6 Addresses TLV: MAe4 (stored in 14 or 16 bytes) or The entire TLV
    #     as it is in the request (Step 18)
    #   - 3) The DUT unicasts an MLR.rsp CoAP response to TD as follows: 2.04 Changed Where the payload contains:
    #     Status TLV: 6 [ST_MLR_GENERAL_FAILURE]
    print("Step 20: BR_1 responds with an error status.")

    def check_step20(p):
        code = str(p.coap.code)
        if code in ['4.00', '5.00', '128', '160']:
            return True

        if code in ['2.04', '68']:
            if p.coap.tlv.status == ST_MLR_GENERAL_FAILURE:
                return True
            if p.coap.tlv.status == ST_MLR_INVALID:
                # verify the payload for ST_MLR_INVALID (2) if possible
                # MAe4 was 14 bytes in Step 18.
                # In Step 18 filter: b'\x0e\x1e' (Type 14, Length 30)
                return b'\x0e\x1e' in p.coap.payload or \
                       b'\x0e\x0e' in p.coap.payload or \
                       b'\x0e\x10' in p.coap.payload
        return False

    pkts.filter_wpan_src64(vars['BR_1']). \
        filter_coap_ack('/n/mr'). \
        filter(check_step20). \
        must_next()

    # Step 21
    # - Device: Router
    # - Description: Harness instructs the device to register another multicast address at BR_2 (instead of
    #   BR_1). Automatically unicasts an MLR.req CoAP request to BR_2 as follows:
    #   coap://[<BR_2 RLOC>]:MM/n/mr Where the payload contains: IPv6 Addresses TLV: MA1
    # - Pass Criteria:
    #   - N/A
    print("Step 21: Router registers MA1 at BR_2.")
    pkts.filter_wpan_src64(vars['Router']).\
        filter_coap_request('/n/mr').\
        filter(lambda p: Ipv6Addr(vars['MA1']) in p.coap.tlv.ipv6_address).\
        must_next()

    # Step 22
    # - Device: BR_2
    # - Description: Automatically responds to the multicast registration with an error.
    # - Pass Criteria:
    #   - For DUT = BR_2:
    #   - The DUT MUST unicast a MLR.rsp CoAP response as below, to TD. 2.04 Changed
    #   - Where the payload contains:
    #   - Status TLV: 5 [ST_MLR_BBR_NOT_PRIMARY]
    print("Step 22: BR_2 responds with error ST_MLR_BBR_NOT_PRIMARY.")
    pkts.filter_wpan_src64(vars['BR_2']).\
        filter_coap_ack('/n/mr').\
        filter(lambda p: p.coap.tlv.status == ST_MLR_BBR_NOT_PRIMARY).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
