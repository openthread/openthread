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
from pktverify.addrs import Ipv6Addr
from pktverify.bytes import Bytes

# Status SUCCESS
ST_MLR_SUCCESS = 0


def verify(pv):
    # 5.10.19 MATN-TC-23: Automatic re-registration by Thread Device
    #
    # 5.10.19.1 Topology
    # - BR_1
    # - BR_2
    # - TD (DUT)
    #
    # 5.10.19.2 Purpose & Description
    # The purpose of this test case is to verify that a Thread Device DUT can re-register its multicast address
    #   before the MLR timeout expires. See also MATN-TC-05 which tests the BBR function in a similar situation.
    #
    # Spec Reference   | V1.2 Section | V1.3.0 Section
    # -----------------|--------------|---------------
    # Multicast        | 5.10.19      | 5.10.19

    pkts = pv.pkts
    pv.summary.show()

    BR_1_RLOC = pv.vars['BR_1_RLOC']
    MA1 = pv.vars['MA1']

    # Step 0
    # - Device: N/A
    # - Description: Topology formation - BR_1, BR_2, TD (DUT)
    # - Pass Criteria:
    #   - N/A
    print("Step 0: Topology formation - BR_1, BR_2, TD (DUT)")

    # Step 1
    # - Device: BR_1
    # - Description: Harness instructs the device to configure the value of the MLR timeout in the BBR Dataset of
    #   BR_1 to be (MLR_TIMEOUT_MIN + 30) seconds and distribute the BBR Dataset in network data. Wait until TD
    #   has received the new network data.
    # - Pass Criteria:
    #   - N/A
    print("Step 1: BR_1 configures MLR timeout and distributes BBR Dataset.")

    # Step 2
    # - Device: TD (DUT)
    # - Description: The DUT must be configured to register the multicast address, MA1. This causes the DUT to
    #   automatically send out MLR.req.
    # - Pass Criteria:
    #   - The DUT MUST unicast an MLR.req CoAP request to BR_1: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr
    #   - Where the payload contains one TLV only with at least one address MA1 inside (there may be more
    #     addresses): IPv6 Addresses TLV: the MA1
    print("Step 2: TD (DUT) registers multicast address, MA1.")
    # MLDv2 and BLMR.ntf checks are intentionally skipped.
    req2 = pkts.filter_coap_request('/n/mr').\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(BR_1_RLOC) or \
                         p.ipv6.dst == pv.vars['LEADER_ALOC']).\
        filter(lambda p: MA1 in p.coap.tlv.ipv6_address).\
        must_next()
    assert len(req2.coap.tlv.type) == 1, f"Expected 1 TLV, but got {len(req2.coap.tlv.type)}"

    # Step 3
    # - Device: BR_1
    # - Description: Automatically responds with MLR.rsp with status ST_MLR_SUCCESS.
    # - Pass Criteria:
    #   - N/A
    print("Step 3: BR_1 responds with MLR.rsp.")
    rsp3 = pkts.filter_coap_ack('/n/mr').\
        filter(lambda p: p.coap.tlv.status == ST_MLR_SUCCESS).\
        must_next()

    # Step 4
    # - Device: TD (DUT)
    # - Description: Within (MLR_TIMEOUT_MIN + 30) seconds of step 3, the DUT automatically re-registers for
    #   multicast address, MA1.
    # - Pass Criteria:
    #   - The DUT MUST unicast an MLR.req CoAP request to BR_1: coap://[<BR_1 RLOC or PBBR ALOC>]:MM/n/mr
    #   - Where the payload contains one TLV only with at least one address MA inside: IPv6 Addresses TLV: the
    #     multicast address MA1
    #   - The MLR.req CoAP request MUST be sent within (MLR_TIMEOUT_MIN + 30) seconds of step 3
    print("Step 4: TD (DUT) automatically re-registers for MA1.")
    # MLDv2 and BLMR.ntf checks are intentionally skipped.
    req4 = pkts.filter_coap_request('/n/mr').\
        filter(lambda p: p.ipv6.dst == Ipv6Addr(BR_1_RLOC) or \
                         p.ipv6.dst == pv.vars['LEADER_ALOC']).\
        filter(lambda p: MA1 in p.coap.tlv.ipv6_address).\
        must_next()
    assert len(req4.coap.tlv.type) == 1, f"Expected 1 TLV, but got {len(req4.coap.tlv.type)}"

    # The MLR.req CoAP request MUST be sent within (MLR_TIMEOUT_MIN + 30) seconds of step 3
    # Thread specification recommends sending the re-registration at 7/8 of the timeout.
    # We check that the request is sent in the latter half of the timeout window.
    MLR_TIMEOUT = float(pv.vars['MLR_TIMEOUT'])
    diff = req4.sniff_timestamp - rsp3.sniff_timestamp
    assert MLR_TIMEOUT / 4 < diff <= MLR_TIMEOUT, f"MLR re-registration time {diff}s is out of expected range ({MLR_TIMEOUT / 4}s, {MLR_TIMEOUT}s]"

    # Step 5
    # - Device: BR_1
    # - Description: Automatically responds with MLR.rsp and status ST_MLR_SUCCESS.
    # - Pass Criteria:
    #   - N/A
    print("Step 5: BR_1 responds with MLR.rsp.")
    pkts.filter_coap_ack('/n/mr').\
        filter(lambda p: p.coap.tlv.status == ST_MLR_SUCCESS).\
        must_next()


if __name__ == '__main__':
    verify_utils.run_main(verify)
