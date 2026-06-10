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

# Status SUCCESS
ST_MLR_SUCCESS = 0


def verify(pv):
    # 5.10.18 MATN-TC-22: Use of low MLR timeout defaults to MLR_TIMEOUT_MIN
    #
    # 5.10.18.1 Topology
    # - BR_1
    # - TD (DUT)
    #
    # 5.10.18.2 Purpose & Description
    # The purpose of this test case is to verify that a Primary BBR that is configured with a low value of MLR timeout
    #   (< MLR_TIMEOUT_MIN) is interpreted as using an MLR timeout of MLR_TIMEOUT_MIN by all the Thread Devices (DUT).
    #
    # Valid DUT Roles:
    # - DUT as TD (FED, REED or Router)
    #
    # Required Devices and Topology:
    # - BR_1: Test bed BR device operating as the Primary BBR.
    # - TD (DUT): Device operating as a Thread FTD (FED, REED or Router).
    #
    # Initial Conditions:
    # - P1: BR_1 MLR timeout value to configured to of BR_1 to be (MLR_TIMEOUT_MIN / 4) seconds.
    #
    # Spec Reference   | V1.2 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # Multicast        | 5.10.18      | 5.10.18

    pv.summary.show()

    MA1 = pv.vars['MA1']
    MLR_TIMEOUT_MIN = int(pv.vars['MLR_TIMEOUT_MIN'])
    BR_RLOC = pv.vars['BR_1_RLOC']

    duts = {
        'Router': pv.vars['DUT_Router_RLOC'],
        'FED': pv.vars['DUT_FED_RLOC'],
    }

    # Step 0
    # - Device: N/A
    # - Description: Topology formation – BR_1. Topology addition – TD (DUT). Boot the DUT. Configure the DUT to
    #   register multicast address, MA1, at its parent.
    # - Pass Criteria:
    #   - N/A
    print("Step 0: Topology formation – BR_1. Topology addition – TD (DUT). DUT registers MA1.")

    initial_regs = {}
    for role, rloc in duts.items():
        print(f"Checking initial registration for DUT as {role} ({rloc})")
        # Use range((0,0), cascade=False) to start search from the beginning for each DUT without affecting pv.pkts
        pkts = pv.pkts.range((0, 0), cascade=False)

        initial_reg = pkts.filter_coap_request('/n/mr').\
            filter(lambda p: p.ipv6.src == rloc).\
            filter(lambda p: p.ipv6.dst == BR_RLOC).\
            filter(lambda p: MA1 in p.coap.tlv.ipv6_address).\
            must_next()
        initial_regs[role] = initial_reg

        # Search for the ACK after the request and match the CoAP message ID.
        pv.pkts.range((initial_reg.number, 0), cascade=False).filter_coap_ack('/n/mr').\
            filter(lambda p: p.ipv6.src == BR_RLOC).\
            filter(lambda p: p.ipv6.dst == rloc).\
            filter(lambda p: p.coap.mid == initial_reg.coap.mid).\
            filter(lambda p: p.coap.tlv.status == ST_MLR_SUCCESS).\
            must_next()

    # Step 1
    # - Device: TD (DUT)
    # - Description: Within MLR_TIMEOUT_MIN seconds of initial registration, the device automatically re-registers for
    #   multicast address, MA1, at BR_1.
    # - Pass Criteria:
    #   - The DUT MUST re-register for multicast address, MA1, at BR_1.
    #   - The re-registration MUST occur within the time MLR_TIMEOUT_MIN seconds since initial registration.
    #   - No more than 2 re-registrations MUST occur within this time.
    print("Step 1: Within MLR_TIMEOUT_MIN seconds, the device automatically re-registers for MA1 at BR_1.")

    for role, rloc in duts.items():
        print(f"Checking re-registration for DUT as {role} ({rloc})")
        initial_reg = initial_regs[role]

        # Use range from initial_reg's position
        pkts = pv.pkts.range((initial_reg.number, 0), cascade=False)

        # Helper to filter for re-registration packets
        def find_re_reg_pkts(start_time):
            return pkts.filter_coap_request('/n/mr').\
                filter(lambda p: p.ipv6.src == rloc).\
                filter(lambda p: p.ipv6.dst == BR_RLOC).\
                filter(lambda p: MA1 in p.coap.tlv.ipv6_address).\
                filter(lambda p: p.sniff_timestamp > start_time).\
                filter(lambda p: p.sniff_timestamp <= initial_reg.sniff_timestamp + MLR_TIMEOUT_MIN)

        # The DUT MUST re-register.
        first_re_reg = find_re_reg_pkts(initial_reg.sniff_timestamp).must_next()

        # A second re-registration is allowed.
        second_re_reg = find_re_reg_pkts(first_re_reg.sniff_timestamp).next()

        # But no more than 2 re-registrations MUST occur.
        if second_re_reg:
            find_re_reg_pkts(second_re_reg.sniff_timestamp).must_not_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
