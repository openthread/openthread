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
import ipaddress

# Add the current directory to sys.path to find verify_utils
CUR_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(CUR_DIR)

import verify_utils

# Multicast Registration (MLR) URI
MLR_URI = '/n/mr'

# Status code for MLR success
ST_MLR_SUCCESS = 0


def verify(pv):
    # 5.10.11 MATN-TC-15: Change of Primary BBR triggers a re-registration
    #
    # 5.10.11.1 Topology
    # - BR_1
    # - BR_2
    # - Router
    # - TD (DUT)
    #
    # 5.10.11.2 Purpose & Description
    # The purpose of this test case is to verify that a Thread End Device detects a change of Primary BBR device and
    #   triggers a re-registration to its multicast groups.
    #
    # Spec Reference   | V1.2 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # Multicast        | 5.10.11      | 5.10.11

    pkts = pv.pkts
    vars = pv.vars
    pv.summary.show()

    MA1 = vars['MA1']

    def is_br_rloc_or_aloc(addr, br_name):
        """Checks if an IPv6 address is a BR RLOC or the PBBR ALOC."""
        try:
            addr_obj = ipaddress.ip_address(str(addr))
            iid = addr_obj.packed[8:]
        except (ValueError, AttributeError):
            return False

        # Check for RLOC or ALOC pattern 0000:00ff:fe00:xxxx
        if iid[:6] != b'\x00\x00\x00\xff\xfe\x00':
            return False

        locator = int.from_bytes(iid[6:], 'big')

        # Check for PBBR ALOC (fc38)
        if locator == 0xfc38:
            return True

        # Check for BR RLOC
        br_rloc16 = vars.get(f'{br_name}_RLOC16')
        if br_rloc16 is not None and locator == br_rloc16:
            return True

        return False

    # Step 0
    # - Device: N/A
    # - Description: Topology formation – BR_1, BR_2, Router. Topology formation - TD (DUT). Boot the DUT. Configure
    #   the DUT to register multicast address MA1.
    # - Pass Criteria:
    #   - N/A
    print("Step 0: Topology formation and initial registration.")
    # Verify initial registration at BR_1
    pkts.filter_wpan_src64(vars['TD']).\
        filter_coap_request(MLR_URI).\
        filter(lambda p: p.coap.tlv.ipv6_address == [MA1]).\
        filter(lambda p: is_br_rloc_or_aloc(p.ipv6.dst, 'BR_1')).\
        must_next()

    # Step 1
    # - Device: BR_1
    # - Description: Harness instructs the device to power down. Harness waits for BR_2 to become the Primary BBR.
    # - Pass Criteria:
    #   - N/A
    print("Step 1: BR_1 powers down, BR_2 becomes Primary.")

    # Step 2
    # - Device: TD (DUT)
    # - Description: Automatically detects the Primary BBR change and registers for multicast address, MA1, at BR_2.
    # - Pass Criteria:
    #   - The DUT MUST unicast an MLR.req CoAP request as follows: coap://[<BR_2 RLOC or PBBR ALOC>]:MM/n/mr
    #   - Where the payload contains: IPv6 Addresses TLV: MA1
    print("Step 2: TD registers for MA1 at BR_2.")
    pkts.filter_wpan_src64(vars['TD']).\
        filter_coap_request(MLR_URI).\
        filter(lambda p: p.coap.tlv.ipv6_address == [MA1]).\
        filter(lambda p: is_br_rloc_or_aloc(p.ipv6.dst, 'BR_2')).\
        must_next()

    # Step 3
    # - Device: Router
    # - Description: Automatically forwards the registration request to BR_2.
    # - Pass Criteria:
    #   - N/A
    print("Step 3: Router forwards the registration request.")
    pkts.filter_wpan_src64(vars['Router']).\
        filter(lambda p: p.wpan.dst16 == vars['BR_2_RLOC16']).\
        filter_coap_request(MLR_URI).\
        filter(lambda p: p.coap.tlv.ipv6_address == [MA1]).\
        filter(lambda p: is_br_rloc_or_aloc(p.ipv6.dst, 'BR_2')).\
        must_next()

    # Step 4
    # - Device: BR_2
    # - Description: Automatically unicasts an MLR.rsp CoAP response to TD as follows: 2.04 changed. Where the payload
    #   contains: Status TLV: 0 [ST_MLR_SUCCESS].
    # - Pass Criteria:
    #   - N/A
    print("Step 4: BR_2 sends an MLR.rsp.")
    pkts.filter_wpan_src64(vars['BR_2']).\
        filter(lambda p: p.wpan.dst16 == vars['Router_RLOC16']).\
        filter_coap_ack(MLR_URI).\
        filter(lambda p: p.coap.tlv.status == ST_MLR_SUCCESS).\
        must_next()

    # Step 5
    # - Device: Router
    # - Description: Automatically forwards the response to TD.
    # - Pass Criteria:
    #   - N/A
    print("Step 5: Router forwards the response.")
    pkts.filter_wpan_src64(vars['Router']).\
        filter_wpan_dst16(vars['TD_RLOC16']).\
        filter_coap_ack(MLR_URI).\
        filter(lambda p: p.coap.tlv.status == ST_MLR_SUCCESS).\
        must_next()

    # Step 6
    # - Device: TD (DUT)
    # - Description: Receives the CoAP registration response.
    # - Pass Criteria:
    #   - The DUT MUST receive the CoAP response.
    print("Step 6: TD receives the CoAP response.")


if __name__ == '__main__':
    verify_utils.run_main(verify)
