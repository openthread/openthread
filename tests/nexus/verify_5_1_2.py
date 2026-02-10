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


def verify(pv):
    """
    5.1.2 Child Address Timeout

    5.1.2.1 Topology
    - Leader
    - Router_1 (DUT)
    - MED_1
    - SED_1

    5.1.2.2 Purpose & Description
    The purpose of the test case is to verify that when the timer reaches the value of the Timeout TLV sent by the Child, the Parent stops responding to Address Query on the Child's behalf.

    Spec Reference                             | V1.1 Section    | V1.3.0 Section
    -------------------------------------------|-----------------|-----------------
    Timing Out Children                        | 4.7.5           | 4.6.3
    """

    pkts = pv.pkts
    pv.summary.show()

    LEADER = pv.vars['LEADER']
    ROUTER_1 = pv.vars['ROUTER_1']

    # Step 1: All
    # - Description: Verify topology is formed correctly
    # - Pass Criteria: N/A
    print("Step 1: Verify topology is formed correctly")
    pkts.filter_wpan_src64(LEADER).filter_mle_cmd(consts.MLE_ADVERTISEMENT).must_next()
    pkts.filter_wpan_src64(ROUTER_1).filter_mle_cmd(consts.MLE_ADVERTISEMENT).must_next()

    # Step 2: MED_1, SED_1
    # - Description: Harness silently powers-off both devices and waits for the keep-alive timeout to expire
    # - Pass Criteria: N/A
    print("Step 2: MED_1 and SED_1 power off")

    # Step 3: Leader
    # - Description: Harness instructs the Leader to send an ICMPv6 Echo Request to MED_1. As part of the process, the Leader automatically attempts to perform address resolution by sending an Address Query Request
    # - Pass Criteria: N/A
    print("Step 3: Leader sends Address Query for MED_1")
    pkts.filter_wpan_src64(LEADER).\
        filter_coap_request(consts.ADDR_QRY_URI).\
        filter(lambda p: p.coap.tlv.target_eid == pv.vars['MED_1_MLEID']).\
        must_next()

    # Step 4: Router_1 (DUT)
    # - Description: Does not respond to Address Query Request
    # - Pass Criteria: The DUT MUST NOT respond with an Address Notification Message
    print("Step 4: Router_1 (DUT) MUST NOT respond with an Address Notification Message")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_coap_request(consts.ADDR_NTF_URI).\
        must_not_next()

    # Step 6: Leader
    # - Description: Harness instructs the Leader to send an ICMPv6 Echo Request to SED_1. As part of the process, the Leader automatically attempts to perform address resolution by sending an Address Query Request
    # - Pass Criteria: N/A
    print("Step 6: Leader sends Address Query for SED_1")
    pkts.filter_wpan_src64(LEADER).\
        filter_coap_request(consts.ADDR_QRY_URI).\
        filter(lambda p: p.coap.tlv.target_eid == pv.vars['SED_1_MLEID']).\
        must_next()

    # Step 7: Router_1 (DUT)
    # - Description: Does not to Address Query Request
    # - Pass Criteria: The DUT MUST NOT respond with an Address Notification Message
    print("Step 7: Router_1 (DUT) MUST NOT respond with an Address Notification Message")
    pkts.filter_wpan_src64(ROUTER_1).\
        filter_coap_request(consts.ADDR_NTF_URI).\
        must_not_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
